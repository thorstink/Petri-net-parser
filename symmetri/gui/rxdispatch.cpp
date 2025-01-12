#include "rxdispatch.h"

#include "blockingconcurrentqueue.h"
namespace rxdispatch {

static moodycamel::BlockingConcurrentQueue<Update> reducer_queue{10};

moodycamel::BlockingConcurrentQueue<Update>& getQueue() {
  return reducer_queue;
}

void unsubscribe() {
  push(model::Reducer([](auto&& m) { return m; }));
}

void push(Update&& r) { reducer_queue.enqueue(std::forward<Update>(r)); }

}  // namespace rxdispatch
