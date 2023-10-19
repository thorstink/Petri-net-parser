#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <map>

#include "petri.h"
#include "symmetri/utilities.hpp"

using namespace symmetri;
using namespace moodycamel;

// global counters to keep track of how often the transitions are called.
std::atomic<unsigned int> T0_COUNTER, T1_COUNTER;
// two transitions
void petri0() {
  auto inc = T0_COUNTER.load() + 1;
  T0_COUNTER.store(inc);
}
auto petri1() {
  auto inc = T1_COUNTER.load() + 1;
  T1_COUNTER.store(inc);
  return symmetri::Color::Success;
}

std::tuple<Net, Store, PriorityTable, Marking> PetriTestNet() {
  T0_COUNTER.store(0);
  T1_COUNTER.store(0);

  Net net = {{"t0",
              {{{"Pa", Color::toString(Color::Success)},
                {"Pb", Color::toString(Color::Success)}},
               {{"Pc", Color::toString(Color::Success)}}}},
             {"t1",
              {{{"Pc", Color::toString(Color::Success)},
                {"Pc", Color::toString(Color::Success)}},
               {{"Pb", Color::toString(Color::Success)},
                {"Pb", Color::toString(Color::Success)},
                {"Pd", Color::toString(Color::Success)}}}}};

  Store store = {{"t0", &petri0}, {"t1", &petri1}};
  PriorityTable priority;

  Marking m0 = {{"Pa", Color::toString(Color::Success)},
                {"Pa", Color::toString(Color::Success)},
                {"Pa", Color::toString(Color::Success)},
                {"Pa", Color::toString(Color::Success)},
                {"Pb", Color::toString(Color::Success)},
                {"Pb", Color::toString(Color::Success)}};
  return {net, store, priority, m0};
}

TEST_CASE("Test equaliy of nets") {
  auto [net, store, priority, m0] = PetriTestNet();
  auto net2 = net;
  auto net3 = net;
  CHECK(stateNetEquality(net, net2));
  // change net
  net2.insert({"tx", {{}, {}}});
  CHECK(!stateNetEquality(net, net2));

  // same transitions but different places.
  net3.at("t0").second.push_back({"px", Color::toString(Color::Success)});
  CHECK(!stateNetEquality(net, net3));
}

TEST_CASE("Run one transition iteration in a petri net") {
  auto [net, store, priority, m0] = PetriTestNet();

  auto stp = std::make_shared<TaskSystem>(1);
  Petri m(net, store, priority, m0, {}, "s", stp);

  // t0 is enabled.
  m.fireTransitions();
  // t0 is dispatched but it's reducer has not yet run, so pre-conditions are
  // processed but post are not:
  {
    Marking expected = {{"Pa", Color::toString(Color::Success)},
                        {"Pa", Color::toString(Color::Success)}};
    CHECK(MarkingEquality(m.getMarking(), expected));
  }
  // now there should be two reducers;
  Reducer r1, r2;
  CHECK(m.reducer_queue->wait_dequeue_timed(r1, std::chrono::seconds(1)));
  CHECK(m.reducer_queue->wait_dequeue_timed(r1, std::chrono::seconds(1)));
  CHECK(m.reducer_queue->wait_dequeue_timed(r2, std::chrono::seconds(1)));
  CHECK(m.reducer_queue->wait_dequeue_timed(r2, std::chrono::seconds(1)));
  // verify that t0 has actually ran twice.
  CHECK(T0_COUNTER.load() == 2);
  // the marking should still be the same.
  {
    Marking expected = {{"Pa", Color::toString(Color::Success)},
                        {"Pa", Color::toString(Color::Success)}};
    CHECK(MarkingEquality(m.getMarking(), expected));
  }

  // process the reducers
  r1(m);
  r2(m);
  // and now the post-conditions are processed:
  CHECK(m.scheduled_callbacks.empty());
  {
    Marking expected = {{"Pa", Color::toString(Color::Success)},
                        {"Pa", Color::toString(Color::Success)},
                        {"Pc", Color::toString(Color::Success)},
                        {"Pc", Color::toString(Color::Success)}};
    CHECK(MarkingEquality(m.getMarking(), expected));
  }
}

TEST_CASE("Run until net dies") {
  using namespace moodycamel;

  auto [net, store, priority, m0] = PetriTestNet();
  auto stp = std::make_shared<TaskSystem>(1);
  Petri m(net, store, priority, m0, {}, "s", stp);

  Reducer r;
  Callback a([] {});
  // we need to enqueue one 'no-operation' to start the live net.
  m.reducer_queue->enqueue([](Petri&) {});
  do {
    if (m.reducer_queue->try_dequeue(r)) {
      r(m);
      m.fireTransitions();
    }
  } while (m.scheduled_callbacks.size() > 0);

  // For this specific net we expect:
  Marking expected = {{"Pb", Color::toString(Color::Success)},
                      {"Pb", Color::toString(Color::Success)},
                      {"Pd", Color::toString(Color::Success)},
                      {"Pd", Color::toString(Color::Success)}};
  CHECK(MarkingEquality(m.getMarking(), expected));

  CHECK(T0_COUNTER.load() == 4);
  CHECK(T1_COUNTER.load() == 2);
}

TEST_CASE("Run until net dies with DirectMutations") {
  using namespace moodycamel;

  auto [net, store, priority, m0] = PetriTestNet();
  // we can pass an empty story, and DirectMutation will be used for the
  // undefined transitions
  auto stp = std::make_shared<TaskSystem>(1);
  Petri m(net, {}, priority, m0, {}, "s", stp);

  Reducer r;
  // we need to enqueue one 'no-operation' to start the live net.
  m.reducer_queue->enqueue([](Petri&) {});
  do {
    if (m.reducer_queue->try_dequeue(r)) {
      r(m);
      m.fireTransitions();
    }
  } while (m.scheduled_callbacks.size() > 0);
  // For this specific net we expect:
  Marking expected = {{"Pb", Color::toString(Color::Success)},
                      {"Pb", Color::toString(Color::Success)},
                      {"Pd", Color::toString(Color::Success)},
                      {"Pd", Color::toString(Color::Success)}};

  CHECK(MarkingEquality(m.getMarking(), expected));
}

// TEST_CASE(
//     "getFireableTransitions return a vector of unique of unique fireable "
//     "transitions") {
//   Net net = {{"a", {{"Pa"}, {}}},
//              {"b", {{"Pa"}, {}}},
//              {"c", {{"Pa"}, {}}},
//              {"d", {{"Pa"}, {}}},
//              {"e", {{"Pb"}, {}}}};

//   Store store;
//   for (auto [t, dm] : net) {
//     store.insert({t, DirectMutation{}});
//   }
//   // with this initial marking, all but transition e are possible.
//   Marking m0 = {{"Pa", 1}};
//   auto stp = std::make_shared<TaskSystem>(1);

//   Petri m(net, store, {}, m0, {}, "s", stp);
//   auto fireable_transitions = m.getFireableTransitions();
//   auto find = [&](auto a) {
//     return std::find(fireable_transitions.begin(),
//     fireable_transitions.end(),
//                      a);
//   };
//   CHECK(find("a") != fireable_transitions.end());
//   CHECK(find("b") != fireable_transitions.end());
//   CHECK(find("c") != fireable_transitions.end());
//   CHECK(find("d") != fireable_transitions.end());
//   CHECK(find("e") == fireable_transitions.end());
// }

TEST_CASE("Step through transitions") {
  std::map<std::string, size_t> hitmap;
  {
    Net net = {{"a", {{{"Pa", Color::toString(Color::Success)}}, {}}},
               {"b", {{{"Pa", Color::toString(Color::Success)}}, {}}},
               {"c", {{{"Pa", Color::toString(Color::Success)}}, {}}},
               {"d", {{{"Pa", Color::toString(Color::Success)}}, {}}},
               {"e", {{{"Pb", Color::toString(Color::Success)}}, {}}}};
    Store store;
    for (const auto& [t, dm] : net) {
      hitmap.insert({t, 0});
      store.insert({t, [&, t = t] { hitmap[t] += 1; }});
    }
    auto stp = std::make_shared<TaskSystem>(1);
    // with this initial marking, all but transition e are possible.
    Marking m0 = {{"Pa", Color::toString(Color::Success)},
                  {"Pa", Color::toString(Color::Success)},
                  {"Pa", Color::toString(Color::Success)},
                  {"Pa", Color::toString(Color::Success)}};
    Petri m(net, store, {}, m0, {}, "s", stp);

    // auto scheduled_callbacks = m.getActiveTransitions();
    // CHECK(scheduled_callbacks.size() == 4);  // abcd
    m.tryFire("e");
    m.tryFire("b");
    m.tryFire("b");
    m.tryFire("c");
    m.tryFire("b");
    // there are no reducers ran, so this doesn't update.
    CHECK(m.scheduled_callbacks.size() == 4);
    // there should be no markers left.
    CHECK(m.getMarking().size() == 0);
    // there should be nothing left to fire
    // scheduled_callbacks = m.getActiveTransitions();
    // CHECK(scheduled_callbacks.size() == 0);
    int j = 0;
    Reducer r;
    while (j < 2 * 4 &&
           m.reducer_queue->wait_dequeue_timed(r, std::chrono::seconds(1))) {
      j++;
      r(m);
    }
    // reducers update, there should be active transitions left.
    CHECK(m.scheduled_callbacks.size() == 0);
  }

  // validate we only ran transition 3 times b, 1 time c and none of the others.
  CHECK(hitmap.at("a") == 0);
  CHECK(hitmap.at("b") == 3);
  CHECK(hitmap.at("c") == 1);
  CHECK(hitmap.at("d") == 0);
  CHECK(hitmap.at("e") == 0);
}

TEST_CASE("create fireable transitions shortlist") {
  auto [net, store, priority, m0] = PetriTestNet();
  auto stp = std::make_shared<TaskSystem>(1);
  Petri m(net, store, priority, m0, {}, "s", stp);
}
