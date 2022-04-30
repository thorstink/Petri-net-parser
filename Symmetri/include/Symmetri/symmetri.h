#pragma once

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
namespace symmetri {

using clock_t = std::chrono::system_clock;

enum class TransitionState { Started, Completed, Error };

struct Event {
  std::string case_id, transition;
  TransitionState state;
  clock_t::time_point stamp;
};

using Eventlog = std::vector<Event>;
using TransitionResult = std::pair<Eventlog, TransitionState>;

size_t calculateTrace(std::vector<Event> event_log);
std::string printState(symmetri::TransitionState s);

template <typename T>
constexpr TransitionResult run(const T &x) {
  if constexpr (std::is_same_v<void, decltype(x())>) {
    x();
    return {{}, TransitionState::Completed};
  }
  if constexpr (std::is_same_v<TransitionState, decltype(x())>) {
    return {{}, x()};
  }
  if constexpr (std::is_same_v<TransitionResult, decltype(x())>) {
    return x();
  }
}

class PolyAction {
 public:
  PolyAction() {}
  template <typename T>
  PolyAction(T x) : self_(std::make_shared<model<T>>(std::move(x))) {}

  friend TransitionResult run(const PolyAction &x) { return x.self_->run_(); }

 private:
  struct concept_t {
    virtual ~concept_t() = default;
    virtual TransitionResult run_() const = 0;
  };
  template <typename T>
  struct model final : concept_t {
    model(T x) : data_(std::move(x)) {}
    TransitionResult run_() const override { return run(data_); }
    T data_;
  };

  std::shared_ptr<const concept_t> self_;
};

using TransitionActionMap = std::unordered_map<std::string, PolyAction>;

struct Application {
 private:
  std::function<void(const std::string &t)> p;
  std::function<TransitionResult()> runApp;

 public:
  Application(const std::set<std::string> &path_to_petri,
              const TransitionActionMap &store, unsigned int thread_count,
              const std::string &case_id = "NOCASE", bool use_webserver = true);

  template <typename T>
  inline std::function<void(T)> push(const std::string &transition) const {
    return [transition, this](T) { p(transition); };
  }

  TransitionResult operator()() const;
};

}  // namespace symmetri
