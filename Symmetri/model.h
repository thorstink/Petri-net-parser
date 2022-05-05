#pragma once

#include <blockingconcurrentqueue.h>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <tuple>

#include "Symmetri/symmetri.h"
#include "types.h"

namespace symmetri {

struct Model;
using Reducer = std::function<Model &(Model &&)>;

Reducer runTransition(const std::string &T_i, const PolyAction &task,
                      const std::string &case_id);

struct Model {
  Model(const StateNet &net,
        const std::unordered_map<std::string, symmetri::PolyAction> &store,
        const NetMarking &M0)
      : net(net), store(store), timestamp(clock_t::now()), M(M0), cache({}) {}

  Model &operator=(const Model &x) { return *this; }
  Model(const Model &) = delete;

  const StateNet net;
  const std::unordered_map<std::string, symmetri::PolyAction> store;

  clock_t::time_point timestamp;
  NetMarking M;
  std::set<Transition> pending_transitions;
  std::vector<Event> event_log;
  std::unordered_map<size_t, std::tuple<NetMarking, std::vector<PolyAction>,
                                        std::set<std::string>>>
      cache;
};

Model &runAll(
    Model &model, moodycamel::BlockingConcurrentQueue<Reducer> &reducers,
    moodycamel::BlockingConcurrentQueue<PolyAction> &polymorphic_actions,
    const std::string &case_id);

}  // namespace symmetri
