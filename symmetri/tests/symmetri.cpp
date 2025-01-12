#include "symmetri/symmetri.h"

#include <filesystem>
#include <iostream>

#include "doctest/doctest.h"

using namespace symmetri;

void t0() {}
auto t1() {}

std::tuple<Net, PriorityTable, Marking> SymmetriTestNet() {
  Net net = {{"t0", {{{"Pa", Success}, {"Pb", Success}}, {{"Pc", Success}}}},
             {"t1",
              {{{"Pc", Success}, {"Pc", Success}},
               {{"Pb", Success}, {"Pb", Success}, {"Pd", Success}}}}};

  PriorityTable priority;

  Marking m0 = {{"Pa", Success}, {"Pa", Success}, {"Pa", Success},
                {"Pa", Success}, {"Pb", Success}, {"Pb", Success}};
  return {net, priority, m0};
}

TEST_CASE("Create a using the net constructor without end condition.") {
  auto [net, priority, initial_marking] = SymmetriTestNet();
  auto threadpool = std::make_shared<TaskSystem>(1);
  Marking goal_marking = {};
  PetriNet app(net, "test_net_without_end", threadpool, initial_marking,
               goal_marking, priority);
  app.registerCallback("t0", &t0);
  app.registerCallback("t1", &t1);
  // we can run the net
  auto res = fire(app);
  auto ev = getLog(app);
  // because there's no final marking, but the net is finite, it deadlocks.
  CHECK(res == Deadlocked);
  CHECK(!ev.empty());
}

TEST_CASE("Create a using the net constructor with end condition.") {
  auto threadpool = std::make_shared<TaskSystem>(1);
  auto [net, priority, initial_marking] = SymmetriTestNet();
  Marking goal_marking(
      {{"Pb", Success}, {"Pb", Success}, {"Pd", Success}, {"Pd", Success}});
  PetriNet app(net, "test_net_with_end", threadpool, initial_marking,
               goal_marking, priority);
  app.registerCallback("t0", &t0);
  app.registerCallback("t1", &t1);
  // we can run the net
  auto res = fire(app);
  auto ev = getLog(app);

  // now there is an end condition.
  CHECK(res == Success);
  CHECK(!ev.empty());
}

TEST_CASE("Create a using pnml constructor.") {
  const std::string pnml_file = std::filesystem::current_path().append(
      "../../../symmetri/tests/assets/PT1.pnml");

  auto threadpool = std::make_shared<TaskSystem>(1);
  PriorityTable priority;
  Marking final_marking({{"P1", Success}});
  const auto case_id = "success";
  PetriNet app({pnml_file}, case_id, threadpool, final_marking, priority);
  app.registerCallback("T0", &t0);
  // so we can run it,
  auto res = fire(app);
  auto ev = getLog(app);
  // and the result is properly completed.
  CHECK(res == Success);
  CHECK(!ev.empty());
}

TEST_CASE("Reuse an application with a new case_id.") {
  auto [net, priority, initial_marking] = SymmetriTestNet();
  Marking goal_marking = {};
  const auto initial_id = "initial0";
  const auto new_id = "something_different0";
  auto threadpool = std::make_shared<TaskSystem>(1);
  PetriNet app(net, initial_id, threadpool, initial_marking, goal_marking,
               priority);
  app.registerCallback("t0", &t0);
  app.registerCallback("t1", &t1);

  CHECK(!app.reuseApplication(initial_id));
  CHECK(app.reuseApplication(new_id));
  // fire a transition so that there's an entry in the eventlog
  fire(app);
  auto eventlog = getLog(app);
  // double check that the eventlog is not empty
  CHECK(!eventlog.empty());
  // the eventlog should have a different case id.
  for (const auto& event : eventlog) {
    CHECK(event.case_id == new_id);
  }
}

TEST_CASE("Can not reuse an active application with a new case_id.") {
  auto [net, priority, initial_marking] = SymmetriTestNet();
  Marking goal_marking = {};
  const auto initial_id = "initial1";
  const auto new_id = "something_different1";
  auto threadpool = std::make_shared<TaskSystem>(1);
  PetriNet app(net, initial_id, threadpool, initial_marking, goal_marking,
               priority);
  app.registerCallback(
      "t0", [] { std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
  app.registerCallback("t1", &t1);
  threadpool->push([&]() mutable {
    // this should fail because we can not do this while everything is active.
    CHECK(!app.reuseApplication(new_id));
  });
  fire(app);
}

TEST_CASE("Deadlocked transition shows up in marking") {
  auto [net, priority, initial_marking] = SymmetriTestNet();
  Marking goal_marking = {};
  const auto initial_id = "deadlock transition";
  auto threadpool = std::make_shared<TaskSystem>(1);
  PetriNet app(net, initial_id, threadpool, initial_marking, goal_marking,
               priority);
  app.registerCallback("t0", [] { return Deadlocked; });
  fire(app);
  const auto marking = app.getMarking();
  for (auto [p, t] : marking) {
    std::cout << t.toString() << ", " << p << ", " << t.toIndex() << ", "
              << (Deadlocked == t) << (t == Deadlocked) << std::endl;
  }
  const bool has_deadlock_token =
      std::find_if(marking.cbegin(), marking.cend(), [=](const auto& pc) {
        return Deadlocked == pc.second && pc.first == "Pc";
      }) != marking.end();
  CHECK(has_deadlock_token);
}

TEST_CASE("Error'd transition shows up in marking") {
  auto [net, priority, initial_marking] = SymmetriTestNet();
  Marking goal_marking = {};
  const auto initial_id = "deadlock transition";
  auto threadpool = std::make_shared<TaskSystem>(1);
  PetriNet app(net, initial_id, threadpool, initial_marking, goal_marking,
               priority);
  app.registerCallback("t0", [] { return Failed; });
  fire(app);
  const auto marking = app.getMarking();
  const bool has_failed_token =
      std::find_if(marking.cbegin(), marking.cend(), [=](const auto& pc) {
        return Failed == pc.second && pc.first == "Pc";
      }) != marking.end();
  CHECK(has_failed_token);
}

TEST_CASE("Test pause and resume") {
  std::atomic<int> i = 0;
  Net net = {{"t0", {{{"Pa", Success}}, {{"Pa", Success}}}},
             {"t1", {{}, {{"Pb", Success}}}}};
  auto threadpool = std::make_shared<TaskSystem>(2);
  Marking initial_marking = {{"Pa", Success}};
  Marking goal_marking = {{"Pb", Success}};
  PetriNet app(net, "random_id", threadpool, initial_marking, goal_marking);
  app.registerCallback("t0", [&] { i++; });
  int check1, check2;
  threadpool->push([&, app, t1 = app.getInputTransitionHandle("t1")]() {
    const auto dt = std::chrono::milliseconds(5);
    std::this_thread::sleep_for(dt);
    pause(app);
    std::this_thread::sleep_for(dt);
    check1 = i.load();
    std::this_thread::sleep_for(dt);
    check2 = i.load();
    resume(app);
    std::this_thread::sleep_for(dt);
    t1();
  });
  fire(app);
  CHECK(check1 == check2);
  CHECK(i.load() > check2);
  CHECK(i.load() > check2 + 1);
}
const static Token ExternalState("ExternalState");
CREATE_CUSTOM_TOKEN(CustomState);

TEST_CASE("Types") {
  using namespace symmetri;
  CHECK(not bool(Scheduled == ExternalState));
  CHECK(not bool(Started == ExternalState));
  CHECK(not bool(Success == ExternalState));
  CHECK(not bool(Deadlocked == ExternalState));
  CHECK(not bool(Paused == ExternalState));
  CHECK(not bool(Canceled == ExternalState));
  CHECK(not bool(Failed == ExternalState));
  CHECK(bool(ExternalState == ExternalState));
  CHECK(bool(ExternalState.toString() != NULL));
}

TEST_CASE("Test color names") {
  CHECK_EQ(std::string(ExternalState.toString()), "ExternalState");
  CHECK_EQ(std::string(Canceled.toString()), "Canceled");
  CHECK_EQ(std::string(Started.toString()), "Started");
  CHECK_EQ(std::string(Failed.toString()), "Failed");
}

TEST_CASE("Print some Types") {
  using namespace symmetri;
  std::cout << Scheduled.toString() << ", " << Scheduled.toIndex() << std::endl;
  std::cout << Started.toString() << ", " << Started.toIndex() << std::endl;
  std::cout << Success.toString() << ", " << Success.toIndex() << std::endl;
  std::cout << Deadlocked.toString() << ", " << Deadlocked.toIndex()
            << std::endl;
  std::cout << Paused.toString() << ", " << Paused.toIndex() << std::endl;
  std::cout << Canceled.toString() << ", " << Canceled.toIndex() << std::endl;
  std::cout << Failed.toString() << ", " << Failed.toIndex() << std::endl;
  std::cout << ExternalState.toString() << ", " << ExternalState.toIndex()
            << std::endl;
  std::cout << CustomState.toString() << ", " << CustomState.toIndex()
            << std::endl;
}

TEST_CASE("Print all Types") {
  for (auto color : symmetri::Token::getColors()) {
    std::cout << color << std::endl;
  }
}
