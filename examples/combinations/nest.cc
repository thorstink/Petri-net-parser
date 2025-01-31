#include "foo.hpp"
#include "symmetri/parsers.h"
#include "symmetri/symmetri.h"
#include "symmetri/utilities.hpp"

using namespace std::chrono_literals;

using namespace symmetri;

int main(int, char *argv[]) {
  const auto tasks = std::string(argv[1]);
  const auto single_step_processor = std::string(argv[2]);
  const auto dual_step_processor = std::string(argv[3]);

  auto pool = std::make_shared<TaskSystem>(2);
  // the child net
  PetriNet child_net(
      std::get<Net>(readPnml({dual_step_processor})), "child_net", pool,
      {{"TaskBucket", Success}, {"ResourceDualProcessor", Success}},
      {{"SuccessfulTasks", Success}, {"ResourceDualProcessor", Success}});

  child_net.registerCallback("StepOne", Foo{0.75, 1ms});
  child_net.registerCallback("StepTwo", Foo{0.75, 2ms});

  // for the parent net:
  const auto read = readPnml({tasks, single_step_processor});
  const Net net = std::get<Net>(read);
  const Marking initial_marking = std::get<Marking>(read);
  const auto goal_marking = getGoal(initial_marking);
  PetriNet parent_net(net, "parent_net", pool, initial_marking, goal_marking);
  parent_net.registerCallback("SingleStepProcessor", child_net);

  // run!
  auto now = Clock::now();
  auto result = fire(parent_net);
  auto dt =
      std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - now);

  printLog(getLog(parent_net));

  std::cout << "Token of this net: " << result.toString() << ". It took "
            << dt.count()
            << " ms, token count: " << parent_net.getMarking().size()
            << std::endl;

  return 0;
}
