#pragma once
#include <blockingconcurrentqueue.h>

#include <thread>
#include <vector>

#include "model.h"

namespace symmetri {
struct StoppablePool {
  StoppablePool(unsigned int thread_count,
                const std::shared_ptr<std::atomic<bool>> &_stop,
                const std::function<void()> &worker)
      : pool(thread_count), stop_flag(_stop) {
    std::generate(std::begin(pool), std::end(pool),
                  [worker]() { return std::thread(worker); });
  }
  void stop() {
    stop_flag->store(true);
    for (auto &&t : pool) {
      t.join();
    }
  }
 private:
  std::vector<std::thread> pool;
  std::shared_ptr<std::atomic<bool>> stop_flag;
};

StoppablePool executeTransition(
    const TransitionActionMap &local_store, const Conversions &marking_mapper,
    moodycamel::BlockingConcurrentQueue<Reducer> &reducers,
    moodycamel::BlockingConcurrentQueue<Transition> &actions, int state_size,
    unsigned int thread_count, const std::string &case_id);

}  // namespace symmetri
