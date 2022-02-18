#include "actions.h"

#include <spdlog/spdlog.h>

#include "expected.hpp"
namespace symmetri {
using namespace moodycamel;

auto now() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             clock_t::now().time_since_epoch())
      .count();
}

auto getThreadId() {
  return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

tl::expected<std::function<OptionalError()>, std::string> getAction(
    const TransitionActionMap &local_store, const Transition &t) {
  auto fun_ptr = local_store.find(t);
  if (fun_ptr != local_store.end()) {
    return fun_ptr->second;
  } else {
    return tl::make_unexpected("noop");
  }
}

OptionalError executeAction(std::function<OptionalError()> action) {
  return action();
}

std::vector<std::thread> executeTransition(
    const TransitionActionMap &local_store, const Conversions &marking_mapper,
    BlockingConcurrentQueue<Reducer> &reducers,
    BlockingConcurrentQueue<Transition> &actions, int state_size,
    unsigned int thread_count, const std::string &case_id) {
  std::vector<std::thread> pool(thread_count);

  auto worker = [&, state_size] {
    Transition transition;
    while (true) {
      actions.wait_dequeue(transition);
      const auto start_time = now();

      auto t = getAction(local_store, transition);

      if (t.has_value()) {
        spdlog::get(case_id)->info("Transition {0} started.", transition);
        auto optional_error = t.value()();
        spdlog::get(case_id)->info("Transition {0} ended.", transition);

        const auto end_time = now();
        const auto thread_id = getThreadId();
        if (optional_error.has_value()) {
          Marking error_mutation = mutationVectorFromMap(
              marking_mapper, state_size, optional_error.value());
          reducers.enqueue(Reducer([=](Model model) {
            model.data->M += error_mutation;
            model.data->active_transitions.erase(transition);
            model.data->log.emplace_back(
                std::make_tuple(thread_id, start_time, end_time, transition));
            return model;
          }));
        } else {
          reducers.enqueue(Reducer([=](Model model) {
            model.data->M += model.data->Dp.at(transition);
            model.data->active_transitions.erase(transition);
            model.data->log.emplace_back(
                std::make_tuple(thread_id, start_time, end_time, transition));
            return model;
          }));
        }
      } else {
        spdlog::get(case_id)->error(
            "No function assigned to transition label {0}.", transition);
      }
    };
  };
  // populate the pool
  std::generate(std::begin(pool), std::end(pool),
                [worker]() { return std::thread(worker); });

  return pool;
}
}  // namespace symmetri
