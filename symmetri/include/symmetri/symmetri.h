#pragma once

#include <functional>
#include <set>
#include <unordered_map>

#include "symmetri/polytransition.h"
#include "symmetri/tasks.h"
#include "symmetri/types.h"

namespace symmetri {
/**
 * @brief A Store is a mapping from Transitions, represented by a string that is
 * also used for their identification in the petri-net, to a PolyTask. A
 * PolyTask may contain side-effects.
 *
 */
using Store = std::unordered_map<Transition, PolyTransition>;

/**
 * @brief Forward declaration for the implementation of the PetriNet class.
 * This is used to hide implementation from the end-user and speed up
 * compilation times.
 *
 */
struct Petri;

/**
 * @brief The PetriNet class is a class that can create, configure and
 * run a Petri net.
 *
 */
class PetriNet final {
 public:
  /**
   * @brief Construct a new PetriNet object from a set of paths to pnml-files
   *
   * @param path_to_pnml
   * @param final_marking
   * @param store
   * @param priority
   * @param case_id
   * @param stp
   */
  PetriNet(const std::set<std::string> &path_to_pnml,
           const Marking &final_marking, const Store &store,
           const PriorityTable &priority, const std::string &case_id,
           std::shared_ptr<TaskSystem> stp);

  /**
   * @brief Construct a new PetriNet object from a set of paths to
   * grml-files. Grml fils already have priority, so they are not needed.
   *
   * @param path_to_grml
   * @param final_marking
   * @param store
   * @param case_id
   * @param stp
   */
  PetriNet(const std::set<std::string> &path_to_grml,
           const Marking &final_marking, const Store &store,
           const std::string &case_id, std::shared_ptr<TaskSystem> stp);

  /**
   * @brief Construct a new PetriNet object from a net and initial marking
   *
   * @param net
   * @param m0
   * @param final_marking
   * @param store
   * @param priority
   * @param case_id
   * @param stp
   */
  PetriNet(const Net &net, const Marking &m0, const Marking &final_marking,
           const Store &store, const PriorityTable &priority,
           const std::string &case_id, std::shared_ptr<TaskSystem> stp);

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
   * @brief Get the Event Log object. If the Petri net is running, this call is
   * blocking as it is executed on the Petri net execution loop. Otherwise it
   * directly returns the log.
   *
   *
   * @return Eventlog
   */
  Eventlog getEventLog() const noexcept;

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
   * @brief reuseApplication resets the application such that the same net can
   * be used again after an cancel call. You do need to supply a new case_id
   * which must be different.
   *
   */
  bool reuseApplication(const std::string &case_id);

  /**
   * @brief Fire for a PetriNet means that it executes the Petri net until it
   * reaches a final marking, deadlocks or is preempted by a user.
   *
   * @return Result
   */
  friend Result fire(const PetriNet &app);

  /**
   * @brief cancel interrupts and stops the Petri net execution and
   * calls cancel on all child transitions that are active. If transitions do
   * not have a cancel functionality implemented, they will not be cancelled.
   * Their reducers however will not be processed.
   *
   * @return Result
   */
  friend Result cancel(const PetriNet &app);

  /**
   * @brief pause interrupts and pauses the Petri net execution and
   * calls pause on all child transitions that are active. The Petri net will
   * still consume reducers produced by finished transitions but it will not
   * queue new transitions for execution. This mostly happens when active
   * transitions do not have a pause-functionality implemented.
   *
   * @param app
   */
  friend void pause(const PetriNet &app);

  /**
   * @brief resume breaks the pause and immediately will try to fire all
   * possible transitions. It will also call resume on all active transitions.
   *
   */
  friend void resume(const PetriNet &app);

  friend Eventlog getLog(const PetriNet &app);

 private:
  std::shared_ptr<Petri> impl;  ///< Pointer to the implementation, all
                                ///< information is stored in Petri
  std::function<void(const std::string &)>
      register_functor;  ///< At PetriNet construction this function is
                         ///< created. It can be used to assign a trigger to
                         ///< transitions - allowing the user to invoke a
                         ///< transition without meeting the pre-conditions.
};

Result fire(const PetriNet &);
Result cancel(const PetriNet &);
void pause(const PetriNet &);
void resume(const PetriNet &);
bool isDirect(const PetriNet &);
Eventlog getLog(const PetriNet &);

}  // namespace symmetri
