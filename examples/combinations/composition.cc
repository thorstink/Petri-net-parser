#include "foo.hpp"
#include "symmetri/parsers.h"
#include "symmetri/symmetri.h"
#include "symmetri/utilities.hpp"
using namespace std::chrono_literals;

using namespace symmetri;

int main(int, char *argv[]) {
  // We get some paths to PNML-files
  const auto tasks = std::string(argv[1]);
  const auto single_step_processor = std::string(argv[2]);
  const auto dual_step_processor = std::string(argv[3]);

  auto pool = std::make_shared<TaskSystem>(2);

  const auto read =
      readPnml({tasks, single_step_processor, dual_step_processor});

  const Net net = std::get<Net>(read);
  const Marking initial_marking = std::get<Marking>(read);
  const auto goal_marking = getGoal(initial_marking);

  PetriNet petri(net, "composed_net", pool, initial_marking, goal_marking);

  petri.registerCallback("SingleStepProcessor", Foo{0.75, 3ms});
  petri.registerCallback("StepOne", Foo{0.75, 1ms});
  petri.registerCallback("StepTwo", Foo{0.75, 2ms});

  auto now = Clock::now();
  auto result = fire(petri);
  auto dt =
      std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - now);

  printLog(getLog(petri));
  std::cout << "Token of this net: " << result.toString() << ". It took "
            << dt.count() << " ms, token count: " << petri.getMarking().size()
            << std::endl;

  return 0;
}
