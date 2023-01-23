#include "model.h"

#include <iostream>

#include "symmetri/types.h"
namespace symmetri {

auto getThreadId() {
  return static_cast<size_t>(
      std::hash<std::thread::id>()(std::this_thread::get_id()));
}

Reducer processTransition(size_t T_i, const std::string &case_id,
                          TransitionState result, size_t thread_id,
                          clock_s::time_point start_time,
                          clock_s::time_point end_time) {
  return [=](Model &&model) -> Model & {
    if (result == TransitionState::Completed) {
      const auto &place_list = model.net.output_n;
      model.tokens_n.insert(model.tokens_n.begin(), place_list[T_i].begin(),
                            place_list[T_i].end());
    }

    model.event_log.push_back({case_id, model.net.transition[T_i],
                               TransitionState::Started, start_time,
                               thread_id});
    model.event_log.push_back({case_id, model.net.transition[T_i],
                               result == TransitionState::Completed
                                   ? TransitionState::Completed
                                   : TransitionState::Error,
                               end_time, thread_id});
    // we know for sure this transition is active because otherwise it wouldn't
    // produce a reducer.
    model.active_transitions_n.erase(
        std::find(model.active_transitions_n.begin(),
                  model.active_transitions_n.end(), T_i));
    return model;
  };
}

Reducer processTransition(size_t T_i, const Eventlog &new_events,
                          TransitionState result) {
  return [=](Model &&model) -> Model & {
    if (result == TransitionState::Completed) {
      const auto &place_list = model.net.output_n;
      model.tokens_n.insert(model.tokens_n.begin(), place_list[T_i].begin(),
                            place_list[T_i].end());
    }

    for (const auto &e : new_events) {
      model.event_log.push_back(e);
    }

    // we know for sure this transition is active because otherwise it wouldn't
    // produce a reducer.
    model.active_transitions_n.erase(
        std::find(model.active_transitions_n.begin(),
                  model.active_transitions_n.end(), T_i));

    return model;
  };
}

Reducer createReducerForTransition(size_t T_i, const PolyAction &task,
                                   const std::string &case_id) {
  const auto start_time = clock_s::now();
  const auto &[ev, res] = runTransition(task);
  const auto end_time = clock_s::now();
  const auto thread_id = getThreadId();
  return ev.empty() ? processTransition(T_i, case_id, res, thread_id,
                                        start_time, end_time)
                    : processTransition(T_i, ev, res);
}

bool canFire(const SmallVector &pre, const std::vector<size_t> &tokens) {
  return std::all_of(pre.begin(), pre.end(), [&](const auto &m_p) {
    return std::count(tokens.begin(), tokens.end(), m_p) >=
           std::count(pre.begin(), pre.end(), m_p);
  });
};

gch::small_vector<uint8_t, 32> possibleTransitions(
    const std::vector<size_t> tokens, std::vector<SmallVector> &p_to_ts_n,
    std::vector<int8_t> &priorities) {
  gch::small_vector<uint8_t, 32> possible_transition_list_n;
  for (const size_t place : tokens) {
    for (size_t t : p_to_ts_n[place]) {
      if (std::find(possible_transition_list_n.begin(),
                    possible_transition_list_n.end(),
                    t) == possible_transition_list_n.end()) {
        possible_transition_list_n.push_back(t);
      }
    }
  }

  // sort transition list according to priority
  std::sort(possible_transition_list_n.begin(),
            possible_transition_list_n.end(),
            [&](size_t a, size_t b) { return priorities[a] > priorities[b]; });

  return possible_transition_list_n;
}

Model &runTransitions(Model &model,
                      moodycamel::BlockingConcurrentQueue<Reducer> &reducers,
                      const StoppablePool &polymorphic_actions, bool run_all,
                      const std::string &case_id) {
  model.timestamp = clock_s::now();

  // todo; rangify this.

  // find possible transitions
  auto possible_transition_list_n = possibleTransitions(
      model.tokens_n, model.net.p_to_ts_n, model.net.priority);
  // fire possible transitions
  for (size_t i = 0;
       i < possible_transition_list_n.size() && model.tokens_n.size() > 0;
       ++i) {
    size_t T_i = possible_transition_list_n[i];
    const auto &pre = model.net.input_n[T_i];
    if (canFire(pre, model.tokens_n)) {
      // deduct the marking
      for (const size_t place : pre) {
        // erase one by one. using std::remove_if would remove all tokens at a
        // particular place.
        model.tokens_n.erase(
            std::find(model.tokens_n.begin(), model.tokens_n.end(), place));
      }

      auto task = model.net.store[T_i];

      // if the transition is direct, we short-circuit the
      // marking mutation and do it immediately.
      if (isDirectTransition(task)) {
        model.tokens_n.insert(model.tokens_n.begin(),
                              model.net.output_n[T_i].begin(),
                              model.net.output_n[T_i].end());
        model.event_log.push_back({case_id, model.net.transition[T_i],
                                   TransitionState::Started, model.timestamp,
                                   0});
        model.event_log.push_back({case_id, model.net.transition[T_i],
                                   TransitionState::Completed, model.timestamp,
                                   0});
      } else {
        model.active_transitions_n.push_back(T_i);
        polymorphic_actions.enqueue([=, &reducers] {
          reducers.enqueue(createReducerForTransition(T_i, task, case_id));
        });
      }
      if (run_all) {
        // reset counter & update possible fire-list
        i = -1;  // minus 1 because it gets incremented by the for loop
        possible_transition_list_n = possibleTransitions(
            model.tokens_n, model.net.p_to_ts_n, model.net.priority);
      } else {
        break;
      }
    }
  }
  return model;
}

std::vector<Place> Model::getMarking() const {
  std::vector<Place> marking;
  marking.reserve(tokens_n.size());
  std::transform(tokens_n.cbegin(), tokens_n.cend(),
                 std::back_inserter(marking),
                 [&](auto place_index) { return net.place[place_index]; });
  return marking;
}

std::vector<Transition> Model::getActiveTransitions() const {
  std::vector<Transition> transition_list;
  transition_list.reserve(active_transitions_n.size());
  std::transform(active_transitions_n.cbegin(), active_transitions_n.cend(),
                 std::back_inserter(transition_list),
                 [&](auto place_index) { return net.transition[place_index]; });
  return transition_list;
}

}  // namespace symmetri
