#pragma once
#include <iostream>
#include <random>

#include "symmetri/types.h"

void printLog(const symmetri::Eventlog &eventlog) {
  for (const auto &[caseid, t, s, c] : eventlog) {
    std::cout << "Eventlog: " << caseid << ", " << t << ", " << s.toString()
              << ", " << c.time_since_epoch().count() << std::endl;
  }
}

symmetri::Marking getGoal(const symmetri::Marking &initial_marking) {
  auto goal = initial_marking;
  for (auto &[p, c] : goal) {
    p = (p == "TaskBucket") ? "SuccessfulTasks" : p;
    c = symmetri::Success;
  }
  return goal;
}

CREATE_CUSTOM_TOKEN(Red)

struct Foo {
  Foo(double success_rate, std::chrono::milliseconds sleep_time)
      : sleep_time(sleep_time),
        generator(std::random_device{}()),
        distribution(success_rate) {}
  const std::chrono::milliseconds sleep_time;
  mutable std::default_random_engine generator;
  mutable std::bernoulli_distribution distribution;
};

symmetri::Token fire(const Foo &f) {
  auto now = symmetri::Clock::now();
  while (symmetri::Clock::now() - now < f.sleep_time) {
  }
  if (f.distribution(f.generator)) {
    return symmetri::Success;
  } else {
    return symmetri::Red;
  }
}

bool isSynchronous(const Foo &) { return false; }
