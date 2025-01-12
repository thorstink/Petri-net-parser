#pragma once

/** @file petri.h */

#include <functional>
#include <optional>
#include <tuple>

#include "externals/blockingconcurrentqueue.h"
#include "externals/small_vector.hpp"
#include "symmetri/callback.h"
#include "symmetri/tasks.h"
#include "symmetri/types.h"

namespace symmetri {

/**
 * @brief AugmentedToken describes a token with a color in a
 * particular place.
 *
 */
using AugmentedToken = std::tuple<size_t, Token>;

/**
 * @brief a minimal Event representation.
 *
 */
struct SmallEvent {
  size_t transition;        ///< The transition that generated the event
  Token state;              ///< The result of the event
  Clock::time_point stamp;  ///< The timestamp of the event
};

/**
 * @brief a list of events
 *
 */
using SmallLog = std::vector<SmallEvent>;

/**
 * @brief General purpose stack-allocated mini vector for indices
 *
 */
using SmallVector = gch::small_vector<size_t, 4>;

/**
 * @brief General purpose stack-allocated mini vector for colored markings
 *
 */
using SmallVectorInput = gch::small_vector<AugmentedToken, 4>;

/**
 * @brief a small helper function to get the index representation of a place or
 * transition.
 *
 * @param m
 * @param s
 * @return size_t
 */
size_t toIndex(const std::vector<std::string> &m, const std::string &s);

/**
 * @brief calculates a list of possible transitions given the current
 * token-distribution. It returns a list of transitions sorted by priority. The
 * first element being the high priority.
 *
 * @param tokens
 * @param p_to_ts_n
 * @return gch::small_vector<size_t, 32>
 */
gch::small_vector<size_t, 32> possibleTransitions(
    const std::vector<AugmentedToken> &tokens,
    const std::vector<SmallVectorInput> &input_n,
    const std::vector<SmallVector> &p_to_ts_n);

/**
 * @brief Takes a vector of input places (pre-conditions) and the current token
 * distribution to determine whether the pre-conditions are met, e.g. the
 * transition is active.
 *
 * @param pre vector of preconditions
 * @param tokens the current token distribution
 * @return true if the pre-conditions are met
 * @return false otherwise
 */
bool canFire(const SmallVectorInput &pre,
             const std::vector<AugmentedToken> &tokens);

/**
 * @brief Forward declaration of the Petri-class
 *
 */
struct Petri;

/**
 * @brief A Reducer updates the Petri-object. Reducers are used to process the
 * post-callback marking mutations.
 */
using Reducer = std::function<void(Petri &)>;

/**
 * @brief Schedules the callback associated with transition t_idx/transition. It
 * returns a reducer which is to be executed _after_ Callback task completed.
 *
 * @param t_idx the index of the transition as used in the Petri-class
 * @param task the Callback to be scheduled
 * @return Reducer
 */
Reducer scheduleCallback(
    size_t t_idx, const Callback &task,
    const std::shared_ptr<moodycamel::BlockingConcurrentQueue<Reducer>>
        &reducer_queue);

/**
 * @brief deducts the set input from the current token distribution
 *
 * @param inputs a vector representing the tokens to be removed
 */
void deductMarking(std::vector<AugmentedToken> &tokens,
                   const SmallVectorInput &inputs);

/**
 * @brief Petri is a data structure that encodes the Petri net and holds
 * pointers to the thread-pool and the reducer-queue. It is optimized for
 * calculating the active transition set and dispatching a Callback to the
 * TaskSystem.
 *
 */
struct Petri {
  /**
   * @brief Construct a new Petri from a multiset description of a Petri
   * net, a lookup table for the transitions, optional priorities and an initial
   * marking. A lot of conversion work is done in the constructor, so you should
   * avoid creating Models during the run-time of a Petri application.
   *
   * @param _net
   * @param _priority
   * @param _initial_tokens
   * @param _final_marking
   * @param _case_id
   * @param threadpool
   */
  explicit Petri(const Net &_net, const PriorityTable &_priority,
                 const Marking &_initial_tokens, const Marking &_final_marking,
                 const std::string &_case_id,
                 std::shared_ptr<TaskSystem> threadpool);
  ~Petri() noexcept = default;
  Petri(Petri const &) = delete;
  Petri(Petri &&) noexcept = delete;
  Petri &operator=(Petri const &) = delete;
  Petri &operator=(Petri &&) noexcept = delete;

  /**
   * @brief outputs the marking as a vector of tokens; e.g. [1 1 1 0 5] means 3
   * tokens in place 1, 1 token in place 0 and 1 token in place 5.
   *
   * @param marking
   * @return std::vector<size_t>
   */
  std::vector<AugmentedToken> toTokens(const Marking &marking) const noexcept;

  /**
   * @brief Get the current marking. It is represented by a vector of places:
   * every occurance of a place in this vectors implies a token in that place.
   * This representation does not show the empty places explicitly; if a place
   * is not in this vector you can assume it has no token.
   *
   * @return std::vector<Place>
   */
  Marking getMarking() const;

  /**
   * @brief get the current eventlog, also copies in all child eventlogs of
   * active petri nets.
   *
   * @return Eventlog
   */
  Eventlog getLogInternal() const;

  /**
   * @brief Try to fire a single transition. Does nothing if t is not active.
   *
   * @param t string representation of the transition that is tried to fire.
   */
  void tryFire(const Transition &t);

  /**
   * @brief Fires all active transitions until it there are none left.
   * Associated asynchronous Callbacks are scheduled and synchronous Callback
   * are executed immediately.
   *
   */
  void fireTransitions();

  struct PTNet {
    /**
     * @brief (ordered) list of string representation of transitions
     *
     */
    std::vector<std::string> transition;

    /**
     * @brief (ordered) list of string representation of places
     *
     */
    std::vector<std::string> place;

    /**
     * @brief list of list of inputs to transitions. This vector is indexed like
     * `transition`.
     *
     */
    std::vector<SmallVectorInput> input_n;

    /**
     * @brief list of list of outputs of transitions. This vector is indexed
     * like `transition`.
     *
     */
    std::vector<SmallVectorInput> output_n;

    /**
     * @brief list of list of transitions that have places as inputs. This
     * vector is index like `place`
     *
     */
    std::vector<SmallVector> p_to_ts_n;

    /**
     * @brief This vector holds priorities for all transitions. This vector is
     * index like `transition`.
     *
     */
    std::vector<int8_t> priority;

    /**
     * @brief This is the same 'lookup table', only index using  `transition` so
     * it is compatible with index lookup.
     *
     */
    std::vector<Callback> store;

    void registerCallback(const std::string &t, Callback &&callback) noexcept {
      if (std::find(transition.begin(), transition.end(), t) !=
          transition.end()) {
        store[toIndex(transition, t)] = std::move(callback);
      }
    }
  } net;  ///< Is a data-oriented design of a Petri net

  std::vector<AugmentedToken> initial_tokens;  ///< The initial marking
  std::vector<AugmentedToken> tokens;          ///< The current marking
  std::vector<AugmentedToken> final_marking;   ///< The final marking
  std::vector<size_t> scheduled_callbacks;     ///< List of active transitions
  SmallLog log;         ///< The most up to date event_log
  Token state;          ///< The current state of the Petri
  std::string case_id;  ///< The unique identifier for this Petri-run
  std::atomic<std::optional<unsigned int>>
      thread_id_;  ///< The id of the thread from which the Petri is fired.

  std::shared_ptr<moodycamel::BlockingConcurrentQueue<Reducer>>
      reducer_queue;  ///< A pointer to the reducer queue. It is a shared
                      ///< pointer because it needs to be captured by Reducers
                      ///< which are executed later, guaranteeing the queue is
                      ///< not destroyed while in use.
  std::shared_ptr<TaskSystem>
      pool;  ///< A pointer to the threadpool used to defer Callbacks.

 private:
  /**
   * @brief Runs the Callback associated with t immediately.
   *
   * @param t transition as index in transition vector
   */
  void fireSynchronous(const size_t t);

  /**
   * @brief Schedules the Callback associated with t on the threadpool
   *
   * @param t transition as index in transition vector
   */
  void fireAsynchronous(const size_t t);
};

std::tuple<std::vector<std::string>, std::vector<std::string>,
           std::vector<Callback>>
convert(const Net &_net);
std::tuple<std::vector<SmallVectorInput>, std::vector<SmallVectorInput>>
populateIoLookups(const Net &_net, const std::vector<Place> &ordered_places);
std::vector<SmallVector> createReversePlaceToTransitionLookup(
    size_t place_count, size_t transition_count,
    const std::vector<SmallVectorInput> &input_transitions);

std::vector<int8_t> createPriorityLookup(
    const std::vector<Transition> transition, const PriorityTable &_priority);
}  // namespace symmetri
