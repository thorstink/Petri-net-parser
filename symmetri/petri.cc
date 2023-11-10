#include "petri.h"

bool isSynchronous(const DirectMutation &) { return true; }

symmetri::Token fire(const DirectMutation &) {
  return symmetri::Color::Success;
}

namespace symmetri {
std::tuple<std::vector<std::string>, std::vector<std::string>,
           std::vector<std::string>, std::vector<Callback>>
convert(const Net &_net) {
  const auto transition_count = _net.size();
  std::vector<std::string> transitions;
  std::vector<std::string> places;
  std::vector<std::string> colors;
  for (const auto &[t, c] : Color::getColors()) {
    colors.push_back(c);
  }
  std::vector<Callback> store;
  transitions.reserve(transition_count);
  store.reserve(transition_count);
  for (const auto &[t, io] : _net) {
    transitions.push_back(t);
    store.push_back(DirectMutation{});
    for (const auto &p : io.first) {
      places.push_back(p.first);
    }
    for (const auto &p : io.second) {
      places.push_back(p.first);
    }
    // sort and remove duplicates.
    std::sort(places.begin(), places.end());
    auto last = std::unique(places.begin(), places.end());
    places.erase(last, places.end());
  }
  return {transitions, places, colors, store};
}

std::tuple<std::vector<SmallVectorInput>, std::vector<SmallVectorInput>>
populateIoLookups(const Net &_net, const std::vector<Place> &ordered_places,
                  const std::vector<Place> &) {
  std::vector<SmallVectorInput> input_n, output_n;
  for (const auto &[t, io] : _net) {
    SmallVectorInput q_in, q_out;
    for (const auto &p : io.first) {
      q_in.push_back({toIndex(ordered_places, p.first), p.second});
    }
    input_n.push_back(q_in);
    for (const auto &p : io.second) {
      q_out.push_back({toIndex(ordered_places, p.first), p.second});
    }
    output_n.push_back(q_out);
  }
  return {input_n, output_n};
}

std::vector<SmallVector> createReversePlaceToTransitionLookup(
    size_t place_count, size_t transition_count,
    const std::vector<SmallVectorInput> &input_transitions) {
  std::vector<SmallVector> p_to_ts_n;
  for (size_t p = 0; p < place_count; p++) {
    SmallVector q;
    for (size_t c = 0; c < transition_count; c++) {
      for (const auto &[input_place, input_color] : input_transitions[c]) {
        if (p == input_place && std::find(q.begin(), q.end(), c) == q.end()) {
          q.push_back(c);
        }
      }
    }
    p_to_ts_n.push_back(q);
  }
  return p_to_ts_n;
}

std::vector<int8_t> createPriorityLookup(
    const std::vector<Transition> transition, const PriorityTable &_priority) {
  std::vector<int8_t> priority;
  for (const auto &t : transition) {
    auto ptr = std::find_if(_priority.begin(), _priority.end(),
                            [t](const auto &a) { return a.first == t; });
    priority.push_back(ptr != _priority.end() ? ptr->second : 0);
  }
  return priority;
}

Petri::Petri(const Net &_net, const PriorityTable &_priority,
             const Marking &_initial_tokens, const Marking &_final_marking,
             const std::string &_case_id, std::shared_ptr<TaskSystem> stp)
    : event_log({}),
      state(Color::Scheduled),
      case_id(_case_id),
      thread_id_(std::nullopt),
      reducer_queue(
          std::make_shared<moodycamel::BlockingConcurrentQueue<Reducer>>(128)),
      pool(stp) {
  event_log.reserve(1000);
  std::tie(net.transition, net.place, net.color, net.store) = convert(_net);
  std::tie(net.input_n, net.output_n) =
      populateIoLookups(_net, net.place, net.color);
  net.p_to_ts_n = createReversePlaceToTransitionLookup(
      net.place.size(), net.transition.size(), net.input_n);
  net.priority = createPriorityLookup(net.transition, _priority);
  initial_tokens = toTokens(_initial_tokens);
  tokens = initial_tokens;
  final_marking = toTokens(_final_marking);
}

std::vector<AugmentedToken> Petri::toTokens(
    const Marking &marking) const noexcept {
  std::vector<AugmentedToken> tokens;
  for (const auto &[p, c] : marking) {
    tokens.push_back({toIndex(net.place, p), c});
  }
  return tokens;
}

void Petri::fireSynchronous(const size_t t) {
  const auto &task = net.store[t];
  const auto &transition = net.transition[t];
  const auto &lookup_t = net.output_n[t];
  event_log.push_back({case_id, transition, Color::Started, Clock::now()});
  auto result = ::fire(task);
  event_log.push_back({case_id, transition, result, Clock::now()});
  switch (result) {
    case Color::Scheduled:
    case Color::Started:
    case Color::Deadlock:
    case Color::UserExit:
    case Color::Paused:
    case Color::Error:
      break;
    default: {
      tokens.reserve(tokens.size() + lookup_t.size());
      for (const auto &[p, c] : lookup_t) {
        tokens.push_back({p, result});
      }
    }
  };
}

void Petri::fireAsynchronous(const size_t t) {
  const auto &task = net.store[t];
  const auto &transition = net.transition[t];
  scheduled_callbacks.push_back(t);
  event_log.push_back({case_id, transition, Color::Scheduled, Clock::now()});
  pool->push([=] {
    reducer_queue->enqueue(scheduleCallback(t, task, reducer_queue));
  });
}

void deductMarking(std::vector<AugmentedToken> &tokens,
                   const SmallVectorInput &inputs) {
  for (const auto place : inputs) {
    // erase one by one. using std::remove_if would remove all tokens at
    // a particular place.
    tokens.erase(std::find(tokens.begin(), tokens.end(), place));
  }
}

void Petri::tryFire(const Transition &t) {
  auto it = std::find(net.transition.begin(), net.transition.end(), t);
  const auto t_idx = std::distance(net.transition.begin(), it);
  if (canFire(net.input_n[t_idx], tokens)) {
    deductMarking(tokens, net.input_n[t_idx]);
    isSynchronous(net.store[t_idx]) ? fireSynchronous(t_idx)
                                    : fireAsynchronous(t_idx);
  }
}

gch::small_vector<size_t, 32> &&updatePossibleTransitions(
    gch::small_vector<size_t, 32> &&possible_transitions,
    const std::vector<SmallVectorInput> &input_n,
    const std::vector<int8_t> &priority,
    const std::vector<AugmentedToken> &tokens) {
  possible_transitions.erase(
      std::remove_if(
          possible_transitions.begin(), possible_transitions.end(),
          [&](auto t_idx) { return !canFire(input_n[t_idx], tokens); }),
      possible_transitions.end());

  // sort transition list according to priority
  std::sort(possible_transitions.begin(), possible_transitions.end(),
            [&](size_t a, size_t b) { return priority[a] > priority[b]; });

  return std::move(possible_transitions);
}

void Petri::fireTransitions() {
  // find possibly active transitions && remove inactive transitions
  auto possible_transition_list_n = updatePossibleTransitions(
      possibleTransitions(tokens, net.input_n, net.p_to_ts_n), net.input_n,
      net.priority, tokens);

  // loop
  while (!possible_transition_list_n.empty()) {
    const auto t_idx = possible_transition_list_n.front();
    deductMarking(tokens, net.input_n[t_idx]);
    if (isSynchronous(net.store[t_idx])) {
      fireSynchronous(t_idx);
      // add the output places-connected transitions as new possible
      // transitions if they are not already in the list.
      for (const auto &[p, c] : net.output_n[t_idx]) {
        for (const auto &t : net.p_to_ts_n[p]) {
          if (std::find(possible_transition_list_n.begin(),
                        possible_transition_list_n.end(),
                        t) == possible_transition_list_n.end()) {
            possible_transition_list_n.push_back(t);
          }
        }
      }
    } else {
      fireAsynchronous(t_idx);
    }
    possible_transition_list_n =
        updatePossibleTransitions(std::move(possible_transition_list_n),
                                  net.input_n, net.priority, tokens);
  }

  return;
}

Marking Petri::getMarking() const {
  Marking marking;
  marking.reserve(tokens.size());
  std::transform(tokens.cbegin(), tokens.cend(), std::back_inserter(marking),
                 [&](auto place_index) -> std::pair<std::string, Token> {
                   return {net.place[place_index.place], place_index.color};
                 });
  return marking;
}

Eventlog Petri::getLog() const {
  Eventlog eventlog = event_log;
  // get event log from parent nets:
  for (size_t pt_idx : scheduled_callbacks) {
    auto sub_el = ::getLog(net.store[pt_idx]);
    if (!sub_el.empty()) {
      eventlog.insert(eventlog.end(), sub_el.begin(), sub_el.end());
    }
  }
  return eventlog;
}

}  // namespace symmetri
