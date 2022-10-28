#include "Symmetri/symmetri.h"

#include <blockingconcurrentqueue.h>
#include <signal.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <condition_variable>
#include <fstream>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <tuple>

#include "actions.h"
#include "model.h"
#include "pnml_parser.h"
namespace symmetri {

std::condition_variable cv;
std::mutex cv_m;  // This mutex is used for two purposes:
                  // 1) to synchronize accesses to PAUSE
                  // 2) for the condition variable cv
std::atomic<bool> PAUSE(false);

void blockIfPaused(const std::string &case_id) {
  std::unique_lock<std::mutex> lk(cv_m);
  if (PAUSE.load()) {
    spdlog::get(case_id)->info("Execution is paused");
    cv.wait(lk, [] { return !PAUSE.load(); });
    spdlog::get(case_id)->info("Execution is resumed");
  }
}

// Define the function to be called when ctrl-c (SIGINT) is sent to process
std::atomic<bool> EARLY_EXIT(false);
inline void signal_handler(int signal) noexcept { EARLY_EXIT.store(true); }

using namespace moodycamel;

size_t calculateTrace(Eventlog event_log) noexcept {
  // calculate a trace-id, in a simple way.
  return std::hash<std::string>{}(std::accumulate(
      event_log.begin(), event_log.end(), std::string(""),
      [](const auto &acc, const Event &n) {
        return n.state == TransitionState::Completed ? acc + n.transition : acc;
      }));
}

std::string printState(symmetri::TransitionState s) noexcept {
  std::string ret;
  switch (s) {
    case TransitionState::Started:
      ret = "Started";
      break;
    case TransitionState::Completed:
      ret = "Completed";
      break;
    case TransitionState::Deadlock:
      ret = "Deadlock";
      break;
    case TransitionState::UserExit:
      ret = "UserExit";
      break;
    case TransitionState::Error:
      ret = "Error";
      break;
  }

  return ret;
}

bool check(const Store &store, const symmetri::StateNet &net) noexcept {
  return std::all_of(net.cbegin(), net.cend(), [&store](const auto &p) {
    const auto &t = std::get<0>(p);
    bool store_has_transition =
        std::find_if(store.begin(), store.end(), [&](const auto &e) {
          return e.first == t;
        }) != store.end();
    if (!store_has_transition) {
      spdlog::error("Transition {0} is not in store", t);
    }

    return store_has_transition;
  });
}

struct Impl {
  Model m;
  const symmetri::NetMarking m0_;
  BlockingConcurrentQueue<Reducer> reducers;
  BlockingConcurrentQueue<PolyAction> polymorphic_actions;
  StoppablePool stp;
  const std::string case_id;
  std::optional<symmetri::NetMarking> final_marking;
  ~Impl() { stp.stop(); }
  Impl(const symmetri::StateNet &net, const symmetri::NetMarking &m0,
       const std::optional<symmetri::NetMarking> &final_marking,
       const Store &store, unsigned int thread_count,
       const std::string &case_id)
      : m(net, store, m0),
        m0_(m0),
        reducers(256),
        polymorphic_actions(256),
        stp(thread_count, polymorphic_actions),
        case_id(case_id),
        final_marking(final_marking) {}
  const Model &getModel() const { return m; }
  symmetri::Eventlog getEventLog() const { return m.event_log; }
  TransitionResult run() {
    // todo.. not have to assign it manually to reset.
    m.M = m0_;
    m.event_log = {};

    // enqueue a noop to start, not
    // doing this would require
    // external input to 'start' (even if the petri net is alive)
    reducers.enqueue([](Model &&m) -> Model & { return m; });

    // this is the check whether we should break the loop. It's either early
    // exit, otherwise there must be event_logs.
    auto stop_condition = [&] {
      return EARLY_EXIT.load(std::memory_order_relaxed) ||
             (m.pending_transitions.empty() && !m.event_log.empty());
    };
    Reducer f;
    do {
      // get a reducer. Immediately, or wait a bit
      while (reducers.try_dequeue(f) ||
             reducers.wait_dequeue_timed(f, std::chrono::seconds(1))) {
        do {
          m = f(std::move(m));
        } while (reducers.try_dequeue(f));
        blockIfPaused(case_id);
        if (final_marking.has_value() &&
            MarkingReached(m.M, final_marking.value())) {
          break;
        }
        try {
          m = runAll(m, reducers, polymorphic_actions, case_id);
        } catch (const std::exception &e) {
          spdlog::get(case_id)->error(e.what());
        }
      }
    } while (!stop_condition());

    // determine what was the reason we terminated.
    TransitionState result;
    if (EARLY_EXIT.load()) {
      result = TransitionState::UserExit;
    } else if (final_marking.has_value()
                   ? MarkingReached(m.M, final_marking.value())
                   : false) {
      result = TransitionState::Completed;
    } else if (m.pending_transitions.empty()) {
      result = TransitionState::Deadlock;
    } else {
      result = TransitionState::Error;
    }

    // publish a log
    // spdlog::get(case_id)->info(
    //     printState(result) + " of {0}-net. Trace-hash is {1}", case_id,
    //     calculateTrace(m.event_log));
    // spdlog::get(case_id)->info(
    //     printState(result) + " of {0}-net. Final markin-hash is {1}",
    //     case_id, hashNM(m.M));

    return {m.event_log, result};
  }
};

Application::Application(
    const std::set<std::string> &files,
    const std::optional<symmetri::NetMarking> &final_marking,
    const Store &store, unsigned int thread_count, const std::string &case_id) {
  const auto &[net, m0] = readPetriNets(files);
  createApplication(net, m0, final_marking, store, thread_count, case_id);
}

Application::Application(
    const symmetri::StateNet &net, const symmetri::NetMarking &m0,
    const std::optional<symmetri::NetMarking> &final_marking,
    const Store &store, unsigned int thread_count, const std::string &case_id) {
  createApplication(net, m0, final_marking, store, thread_count, case_id);
}

void Application::createApplication(
    const symmetri::StateNet &net, const symmetri::NetMarking &m0,
    const std::optional<symmetri::NetMarking> &final_marking,
    const Store &store, unsigned int thread_count, const std::string &case_id) {
  signal(SIGINT, signal_handler);
  std::stringstream s;
  s << "[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [thread %t] [" << case_id << "] %v";
  auto console = spdlog::stdout_color_mt(case_id);

  console->set_pattern(s.str());
  if (check(store, net)) {
    // init
    impl = std::make_shared<Impl>(net, m0, final_marking, store, thread_count,
                                  case_id);
    // register a function that "forces" transitions into the queue.
    p = [this](const std::string &t) {
      impl->polymorphic_actions.enqueue(
          [t, this, task = getTransition(impl->m.store, t)] {
            impl->reducers.enqueue(runTransition(t, task, impl->case_id));
          });
    };
  }
}

TransitionResult Application::operator()() const noexcept {
  if (impl == nullptr) {
    spdlog::error("Error not all transitions are in the store");
    return {{}, TransitionState::Error};
  } else {
    auto res = impl->run();
    return res;
  }
}

std::tuple<clock_s::time_point, symmetri::Eventlog, symmetri::StateNet,
           symmetri::NetMarking, std::set<std::string>>
Application::get() const noexcept {
  auto &m = impl->getModel();

  return std::make_tuple(m.timestamp, impl->getEventLog(), m.net, m.M,
                         m.pending_transitions);
}

const symmetri::StateNet &Application::getNet() {
  const auto &m = impl->getModel();
  return m.net;
}

void Application::doMeData(std::function<void()> f) const {
  impl->reducers.enqueue([=](Model &&m) -> Model & {
    f();
    return m;
  });
}

// symmetri::NetMarking Application::getMarking() {
//   const auto m = impl->getModel().M;
//   return m;
// }

// inline std::function<symmetri::NetMarking()>
// Application::registerTransitionCallback() const {
//   return [this]() { return impl->getModel().M; };
// }

void Application::togglePause() {
  std::lock_guard<std::mutex> lk(cv_m);
  PAUSE.store(!PAUSE.load());
  cv.notify_all();
}

}  // namespace symmetri
