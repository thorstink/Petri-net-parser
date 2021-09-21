#include "actions.h"

namespace actions {
using namespace moodycamel;

auto now() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             model::clock_t::now().time_since_epoch())
      .count();
}

auto getThreadId() {
  return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

std::optional<model::Reducer> noTransitionInStore(const types::Transition &t) {
  return model::Reducer([=](model::Model model) {
    std::stringstream s;
    s << "Transition " << t << " is not in the store.";
    throw std::runtime_error(s.str());
    return model;
  });
}

std::vector<std::thread>
executeTransition(const model::TransitionActionMap &local_store,
                  BlockingConcurrentQueue<model::Reducer> &reducers,
                  BlockingConcurrentQueue<types::Transition> &actions) {

  unsigned int n = std::thread::hardware_concurrency();
  unsigned int thread_count = std::max<unsigned int>(1, n / 2);
  thread_count = 3;
  std::cout << thread_count << " concurrent threads are supported.\n";
  std::vector<std::thread> pool(thread_count);

  auto worker = [&] {
    types::Transition transition;
    while (true) {
      actions.wait_dequeue(transition);

      const auto start_time = now();
      auto fun_ptr = local_store.find(transition);
      auto model_update = (fun_ptr != local_store.end())
                              ? fun_ptr->second()
                              : noTransitionInStore(transition);

      const auto end_time = now();
      const auto thread_id = getThreadId();

      reducers.enqueue(
          model_update.has_value()
              ? model::Reducer([=, update = std::move(model_update)](
                                   model::Model model) mutable {
                  model.data->M += model.data->Dp.at(transition);
                  model.data->log.emplace_back(std::make_tuple(
                      thread_id, start_time, end_time, transition));
                  return update.value()(model);
                })
              : model::Reducer([=](model::Model model) {
                  model.data->M += model.data->Dp.at(transition);
                  model.data->log.emplace_back(std::make_tuple(
                      thread_id, start_time, end_time, transition));
                  return model;
                }));
    }
  };
  // populate the pool
  std::generate(std::begin(pool), std::end(pool),
                [worker]() { return std::thread(worker); });

  return pool;
}
} // namespace actions