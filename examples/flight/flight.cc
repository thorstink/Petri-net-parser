#include <spdlog/spdlog.h>

#include <iostream>

#include "symmetri/symmetri.h"
#include "transition.hpp"

/**
 * @brief We want to use the Foo class with Symmetri; Foo has nice functionality
 * such as Pause and Resume, and it can also get preempted/cancelled. We need to
 * define functions to let Symmetri use these functionalities. It is as simple
 * by creating specialized version of the fire/cancel/isDirect/pause/resume
 * functions. One does not need to implement all - if nothing is defined, a
 * default version is used.
 *
 */
namespace symmetri {
template <>
Result fire(const Foo &f) {
  return f.fire() ? Result{{}, symmetri::State::Error}
                  : Result{{}, symmetri::State::Completed};
}
template <>
Result cancel(const Foo &f) {
  f.cancel();
  return {{}, symmetri::State::UserExit};
}
template <>
bool isDirect(const Foo &) {
  return false;
}
template <>
void pause(const Foo &f) {
  f.pause();
}
template <>
void resume(const Foo &f) {
  f.resume();
}
}  // namespace symmetri

/**
 * @brief A simple printer for the eventlog
 *
 * @param eventlog
 */
void printLog(const symmetri::Eventlog &eventlog) {
  for (const auto &[caseid, t, s, c] : eventlog) {
    spdlog::info("EventLog: {0}, {1}, {2}, {3}", caseid, t, printState(s),
                 c.time_since_epoch().count());
  }
}

int main(int, char *argv[]) {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [thread %t] %v");

  // We get some paths to PNML-files
  const auto pnml1 = std::string(argv[1]);
  const auto pnml2 = std::string(argv[2]);
  const auto pnml3 = std::string(argv[3]);

  // We create a threadpool to which transitions can be dispatched. In this case
  // 4 is too many; in theory you can deduce the maximum amount of parallel
  // transitions from your petri net.
  auto pool = std::make_shared<symmetri::TaskSystem>(4);

  // Here we create the first PetriNet based on composing pnml1 and pnml2 using
  // flat composition. The associated transitions are two instance of the
  // Foo-class.
  symmetri::PetriNet subnet({pnml1, pnml2}, {{"P2", 1}},
                            {{"T0", Foo("SubFoo")}, {"T1", Foo("SubBar")}}, {},
                            "SubNet", pool);

  // We create another PetriNet by flatly composing all three petri nets. Again
  // we have 2 Foo-transitions, and the third transition is the subnet. This
  // show how you can also nest PetriNets.
  symmetri::PetriNet bignet(
      {pnml1, pnml2, pnml3}, {{"P3", 5}},
      {{"T0", subnet}, {"T1", Foo("Bar")}, {"T2", Foo("Foo")}}, {}, "RootNet",
      pool);

  // Parallel to the PetriNet execution, we run a thread through which we can
  // get some keyboard input for interaction
  auto t = std::thread([bignet] {
    bool is_running = true;
    while (is_running) {
      std::cout << "input options: \n [p] - pause\n [r] - resume\n [x] - "
                   "exit\n [l] - print log\n";
      char input;
      std::cin >> input;
      switch (input) {
        case 'p':
          pause(bignet);
          break;
        case 'r':
          resume(bignet);
          break;
        case 'l': {
          printLog(bignet.getEventLog());
          break;
        }
        case 'x': {
          cancel(bignet);
          is_running = false;
          break;
        }
        default:
          break;
      }
      std::cin.get();
    };
  });

  // this is where we call the blocking fire-function that executes the petri
  // net
  auto [el, result] = fire(bignet);

  // print the results and eventlog
  spdlog::info("Result of this net: {0}", printState(result));
  printLog(el);

  t.join();  // clean up
  return result == symmetri::State::Completed ? 0 : -1;
}
