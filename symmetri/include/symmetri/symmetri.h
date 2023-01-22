#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "symmetri/actions.h"
#include "symmetri/polyaction.h"
#include "symmetri/types.h"

namespace symmetri {

/**
 * @brief Calculates a hash given an event log. This hash is only influenced by
 * the order of the completions of transitions and if the output of those
 * transitions is Completed, or something else.
 *
 * @param event_log An eventlog, can be both from a terminated or a still active
 * net.
 * @return size_t The hashed result.
 */
size_t calculateTrace(const Eventlog &event_log) noexcept;

/**
 * @brief A convenience function to get a string representation of the
 * state-enum.
 *
 * @param s The TransitionState
 * @return std::string The TransitionState as a human readable string.
 */
std::string printState(symmetri::TransitionState s) noexcept;

/**
 * @brief A Store is a mapping from Transitions, represented by a string that is
 * also used for their identification in the petri-net, to a PolyTask. A
 * PolyTask may contain side-effects.
 *
 */
using Store = std::map<std::string, PolyAction>;

/**
 * @brief Forward decleration for the implementation of the Application class.
 * This is used to hide implementation from the end-user and speed up
 * compilation times.
 *
 */
struct Impl;

/**
 * @brief The Application class executes the petri net, by using the
 *
 */
class Application {
 private:
  mutable std::shared_ptr<Impl> impl;
  std::function<void(const std::string &t)> p;
  std::function<void(const clock_s::time_point &t)> publish;
  void createApplication(
      const symmetri::StateNet &net, const symmetri::NetMarking &m0,
      const symmetri::NetMarking &final_marking, const Store &store,
      const std::vector<std::pair<symmetri::Transition, int8_t>> &priority,
      const std::string &case_id, const symmetri::StoppablePool &stp);

 public:
  Application(
      const std::set<std::string> &path_to_petri,
      const symmetri::NetMarking &final_marking, const Store &store,
      const std::vector<std::pair<symmetri::Transition, int8_t>> &priority,
      const std::string &case_id, const symmetri::StoppablePool &stp);

  Application(
      const symmetri::StateNet &net, const symmetri::NetMarking &m0,
      const symmetri::NetMarking &final_marking, const Store &store,
      const std::vector<std::pair<symmetri::Transition, int8_t>> &priority,
      const std::string &case_id, const symmetri::StoppablePool &stp);

  TransitionResult operator()() const noexcept;
  symmetri::Eventlog getEvenLog() const noexcept;
  std::vector<symmetri::Place> getMarking() const noexcept;
  std::vector<std::string> getActiveTransitions() const noexcept;
  std::function<void()> registerTransitionCallback(
      const std::string &transition) const noexcept;
  void togglePause() const noexcept;
  void exitEarly() const noexcept;
};

}  // namespace symmetri
