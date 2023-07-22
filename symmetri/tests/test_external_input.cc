#include <catch2/catch_test_macros.hpp>
#include <condition_variable>

#include "symmetri/symmetri.h"

using namespace symmetri;

std::atomic<bool> i_ran(false);
void t() { i_ran.store(true); }

TEST_CASE("Test external input.") {
  Net net = {
      {"t0", {{}, {"Pa"}}}, {"t1", {{"Pa"}, {"Pb"}}}, {"t2", {{"Pc"}, {"Pb"}}}};
  Store store = {{"t0", nullptr},
                 {"t1", &t},
                 {"t2", []() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                  }}};
  Marking m0 = {{"Pa", 0}, {"Pb", 0}, {"Pc", 1}};
  Marking final_m = {{"Pb", 2}};
  auto stp = std::make_shared<TaskSystem>(3);

  symmetri::PetriNet app(net, m0, final_m, store, {}, "test_net_ext_input",
                         stp);

  // enqueue a trigger;
  stp->push([trigger = app.registerTransitionCallback("t0")]() {
    // sleep a bit so it gets triggered _after_ the net started.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    trigger();
  });

  // run the net
  auto [ev, res] = symmetri::fire(app);

  REQUIRE(res == State::Completed);
  REQUIRE(i_ran);
}
