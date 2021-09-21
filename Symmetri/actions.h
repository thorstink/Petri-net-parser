#pragma once
#include "model.h"
#include <blockingconcurrentqueue.h>
#include <thread>
#include <vector>

namespace actions {

std::vector<std::thread> executeTransition(
    const model::TransitionActionMap &local_store,
    moodycamel::BlockingConcurrentQueue<model::Reducer> &reducers,
    moodycamel::BlockingConcurrentQueue<types::Transition> &actions);

} // namespace actions
