#include "actions.h"
namespace symmetri {
using namespace moodycamel;
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
auto getThreadId() {
  return static_cast<size_t>(
      std::hash<std::thread::id>()(std::this_thread::get_id()));
}

StoppablePool executeTransition(const TransitionActionMap &local_store,
                                BlockingConcurrentQueue<Reducer> &reducers,
                                BlockingConcurrentQueue<Transition> &actions,
                                unsigned int thread_count,
                                const std::string &case_id) {
  auto stop_token = std::make_shared<std::atomic<bool>>(false);
  auto worker = [&, stop_token] {
    Transition transition;
    while (stop_token->load() == false) {
      if (actions.wait_dequeue_timed(transition,
                                     std::chrono::milliseconds(250))) {
        if (stop_token->load() == true) {
          break;
        }
        const auto start_time = clock_t::now();
        const auto thread_id = getThreadId();

        auto logstart = [&](const Event &event) {
          reducers.enqueue(Reducer([=](Model &&model) -> Model & {
            model.event_log.push_back(event);
            return model;
          }));
        };

        const auto &[event_log, end_time] = std::visit(
            overloaded{
                [&](const nonLoggedFunction &action)
                    -> std::pair<std::vector<Event>, clock_t::time_point> {
                  logstart({case_id, transition, TransitionState::Started,
                            start_time});
                  action();
                  auto end_time = clock_t::now();
                  return {{{case_id, transition, TransitionState::Completed,
                            end_time}},
                          end_time};
                },
                [&](const loggedFunction &action)
                    -> std::pair<std::vector<Event>, clock_t::time_point> {
                  return {action(), clock_t::now()};
                },
                [](const Application &action)
                    -> std::pair<std::vector<Event>, clock_t::time_point> {
                  return {action(), clock_t::now()};
                },
            },
            local_store.at(transition));

        reducers.enqueue(Reducer([=](Model &&model) -> Model & {
          auto it = std::find_if(
              event_log.begin(), event_log.end(), [](const auto &e) {
                return std::get<TransitionState>(e) == TransitionState::Error;
              });
          if (it == event_log.end()) {
            for (const auto &m_p : model.net.at(transition).second) {
              model.M[m_p] += 1;
            }
          }
          std::move(event_log.begin(), event_log.end(),
                    std::back_inserter(model.event_log));

          model.pending_transitions.erase(transition);
          model.transition_end_times[transition] = end_time;
          model.log.emplace(transition,
                            TaskInstance{start_time, end_time, thread_id});
          return model;
        }));
      }
    };
  };

  return StoppablePool(thread_count, stop_token, worker, actions);
}
}  // namespace symmetri
