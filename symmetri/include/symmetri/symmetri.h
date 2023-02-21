#pragma once

#include <functional>
#include <set>

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
 * @param s The State
 * @return std::string The State as a human readable string.
 */
std::string printState(symmetri::State s) noexcept;

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
struct Petri;

/**
 * @brief The Application class is a class that can create, configure and
 * execute a Petri net.
 *
 */
class Application final {
 private:
  std::shared_ptr<Petri> impl;  ///< Pointer to the implementation, all
                                ///< information is stored in Petri
  std::function<void(const std::string &)>
      register_functor;  ///< At Application construction this function is
                         ///< created. It can be used to assign a trigger to
                         ///< transitions - allowing the user to invoke a
                         ///< transition without meeting the pre-conditions.

 public:
  /**
   * @brief Construct a new Application object from a set of paths to pnml-files
   *
   * @param path_to_pnml
   * @param final_marking
   * @param store
   * @param priority
   * @param case_id
   * @param stp
   */
  Application(
      const std::set<std::string> &path_to_pnml,
      const symmetri::Marking &final_marking, const Store &store,
      const std::vector<std::pair<symmetri::Transition, int8_t>> &priority,
      const std::string &case_id,
      std::shared_ptr<const symmetri::StoppablePool> stp);

  /**
   * @brief Construct a new Application object from a net and initial marking
   *
   * @param net
   * @param m0
   * @param final_marking
   * @param store
   * @param priority
   * @param case_id
   * @param stp
   */
  Application(
      const symmetri::Net &net, const symmetri::Marking &m0,
      const symmetri::Marking &final_marking, const Store &store,
      const std::vector<std::pair<symmetri::Transition, int8_t>> &priority,
      const std::string &case_id,
      std::shared_ptr<const symmetri::StoppablePool> stp);

  /**
   * @brief This executes the net, like a transition, it returns a result.
   * This is equal to calling `fireTransition(app)`.
   *
   * @return Result
   */
  Result execute() const noexcept;

  /**
   * @brief register transition gives a handle to manually force a transition to
   * fire. This is usefull if you want to trigger a transition that has no input
   * places. It is not recommended to use this for transitions with input
   * places! This violates the mathematics of petri nets.
   *
   * @param transition the name of transition. This transition has to be
   * available in the net.
   * @return std::function<void()>
   */
  std::function<void()> registerTransitionCallback(
      const std::string &transition) const noexcept;

  /**
   * @brief Tries to fire the transition.
   *
   * @param transition
   * @return true if it fired the transition
   * @return false if the preconditions were not met, the transition was not
   * fired
   */
  bool tryFireTransition(const std::string &transition) const noexcept;

  /**
   * @brief Get the Event Log object. If the Petri net is running, this call is
   * blocking as it is executed on the Petri net execution loop. Otherwise it
   * directly returns the log.
   *
   *
   * @return symmetri::Eventlog
   */
  symmetri::Eventlog getEventLog() const noexcept;

  /**
   * @brief Get the State, represented by a vector of *active* transitions (who
   * can still produce reducers and hence marking mutations) and the *current
   * marking*. If the Petri net is running, this call is blocking as it is
   * executed on the Petri net execution loop. Otherwise it directly returns the
   * state.
   *
   * @return std::pair<std::vector<Transition>, std::vector<Place>>
   */
  std::pair<std::vector<Transition>, std::vector<Place>> getState()
      const noexcept;

  /**
   * @brief Get a vector of Fireable Transitions. If the Petri net is running,
   * this call is blocking as it is executed on the Petri net execution loop.
   * Otherwise it directly returns the vector of fireable transitions.
   *
   * @return std::vector<Transition>
   */
  std::vector<Transition> getFireableTransitions() const noexcept;

  /**
   * @brief Execution of the net will be paused. Meaning no reducers will be
   * processed and no new transitions will be queued. Already active transitions
   * can still be executed though.
   *
   */
  void togglePause() const noexcept;

  /**
   * @brief exitEarly breaks the Petri net loop as soon as possible.
   *
   */
  void exitEarly() const noexcept;
};

/**
 * @brief by defining a fireTransition for an Application type, we can also nest
 * Applications as transitions in other nets.
 *
 * @param app
 * @return Result
 */
Result fireTransition(const Application &app);

}  // namespace symmetri
