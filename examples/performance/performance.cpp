#include <symmetri/symmetri.h>

#include <iostream>
#include <symmetri/utilities.hpp>
struct Simple {};
int i = 0;
bool isSynchronous(const Simple &) { return true; }
symmetri::Token fire(const Simple &) {
  i++;
  return symmetri::Success;
}

void printLog(const symmetri::Eventlog &eventlog) {
  for (const auto &[caseid, t, s, c] : eventlog) {
    std::cout << "Eventlog: " << caseid << ", " << t << ", " << s.toString()
              << ", " << c.time_since_epoch().count() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  using namespace symmetri;
  auto pool = std::make_shared<TaskSystem>(1);
  PetriNet petri = [=] {
    if (argc > 1) {
      const auto pnml = std::string(argv[1]);
      return PetriNet({pnml}, "benchmark", pool);
    } else {
      Net net = {
          {"t0", {{{"Pa", Success}, {"Pb", Success}}, {{"Pc", Success}}}},
          {"t1",
           {{{"Pc", Success}, {"Pc", Success}},
            {{"Pb", Success}, {"Pb", Success}}}}};
      Marking initial_marking = {{"Pa", Success}, {"Pa", Success},
                                 {"Pa", Success}, {"Pa", Success},
                                 {"Pb", Success}, {"Pb", Success}};
      Marking goal_marking = {
          {"Pb", Success}, {"Pb", Success}, {"Pc", Success}, {"Pc", Success}};

      PetriNet petri(net, "benchmark", pool, initial_marking, goal_marking);
      petri.registerCallbackInPlace<Simple>("t0");
      petri.registerCallback("t1", Simple{});
      return petri;
    }
  }();
  std::cout << "start" << std::endl;
  const auto begin = Clock::now();
  const auto result = fire(petri);
  const auto end = Clock::now();
  std::cout << i << " transitions in "
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     begin)
                   .count()
            << " [us], execution trace: " << calculateTrace(getLog(petri))
            << std::endl;

  printLog(getLog(petri));

  return result == Success ? 0 : -1;
}
