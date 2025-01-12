#include <symmetri/parsers.h>
#include <symmetri/symmetri.h>

#include <fstream>
#include <iostream>
#include <symmetri/utilities.hpp>

#include "transition.hpp"

/**
 * @brief We want to use the Foo class with Symmetri; Foo has nice
 * functionalities such as Pause and Resume and it can also get
 * preempted/cancelled. We need to define functions to let Symmetri use these
 * functionalities. It is as simple by creating specialized version of the
 * fire/cancel/pause/resume functions. One does not need to implement
 * all - if nothing is defined, a default version is used.
 *
 */
const static symmetri::Token FooFail("FooFail");

symmetri::Token fire(const Foo &f) {
  return f.fire() ? FooFail : symmetri::Success;
}

void cancel(const Foo &f) { f.cancel(); }

void pause(const Foo &f) { f.pause(); }

void resume(const Foo &f) { f.resume(); }

/**
 * @brief A simple printer for the eventlog
 *
 * @param eventlog
 */
void printLog(const symmetri::Eventlog &eventlog) {
  for (const auto &[caseid, t, s, c] : eventlog) {
    std::cout << "Eventlog: " << caseid << ", " << t << ", " << s.toString()
              << ", " << c.time_since_epoch().count() << std::endl;
  }
}

int main(int, char *argv[]) {
  using namespace symmetri;

  // We get some paths to PNML-files
  const auto pnml1 = std::string(argv[1]);
  const auto pnml2 = std::string(argv[2]);
  const auto pnml3 = std::string(argv[3]);

  // We create a threadpool to which transitions can be dispatched. In this
  // case 4 is too many; in theory you can deduce the maximum amount of
  // parallel transitions from your petri net.
  auto pool = std::make_shared<TaskSystem>(4);

  // Here we create the first PetriNet based on composing pnml1 and pnml2
  // using flat composition. The associated transitions are two instance of
  // the Foo-class.
  Marking sub_goal_marking = {{"P2", Success}};
  std::set<std::string> pnmls = {pnml1, pnml2};
  PetriNet subnet(pnmls, "SubNet", pool, sub_goal_marking);
  subnet.registerCallbackInPlace<Foo>("T0", "SubFoo");
  subnet.registerCallbackInPlace<Foo>("T1", "SubBar");

  // We create another PetriNet by flatly composing all three petri nets.
  // Again we have 2 Foo-transitions, and the first transition (T0) is the
  // subnet. This show how you can also nest PetriNets.
  Marking big_goal_marking = {{"P3", Success},
                              {"P3", Success},
                              {"P3", Success},
                              {"P3", Success},
                              {"P3", Success}};
  PetriNet bignet({pnml1, pnml2, pnml3}, "RootNet", pool, big_goal_marking);
  bignet.registerCallback("T0", subnet);
  bignet.registerCallbackInPlace<Foo>("T1", "Bar");
  bignet.registerCallbackInPlace<Foo>("T2", "Foo");
  // a flag to check if we are running
  std::atomic<bool> running(true);

  // Parallel to the PetriNet execution, we run a thread through which we
  // can get some keyboard input for interaction
  auto t = std::thread([&] {
    while (running) {
      std::cout << "input options: \n [p] - pause\n [r] - resume\n [x] - "
                   "exit\n";
      char input;
      std::cin >> input;
      switch (input) {
        case 'p':
          pause(bignet);
          break;
        case 'r':
          resume(bignet);
          break;
        case 'x': {
          cancel(bignet);
          running = false;
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
  auto result = fire(bignet);
  running = false;
  // print the results and eventlog

  printLog(getLog(bignet));
  std::cout << "Token of this net: " << result.toString()
            << ", token count: " << bignet.getMarking().size() << std::endl;
  t.join();  // clean up
  return 0;
}
