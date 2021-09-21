#include "transitions.h"
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace model;

void sleep(std::chrono::milliseconds ms) {
  // auto start = std::chrono::high_resolution_clock::now();
  // while ((std::chrono::high_resolution_clock::now() - start) < ms) {
  //   std::this_thread::yield();
  // }
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  return;
}

OptionalReducer action0() {
  std::cout << "Executing Transition 0 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(190));
  return std::nullopt;
}
OptionalReducer action1() {
  std::cout << "Executing Transition 1 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(700));

  return Reducer([](model::Model model) {
    // if you'd really want, here you could put stuff in the model.
    return model;
  });
}
OptionalReducer action2() {
  std::cout << "Executing Transition 2 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(200));
  return std::nullopt;
}
OptionalReducer action3() {
  std::cout << "Executing Transition 3 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(600));
  return std::nullopt;
}
OptionalReducer action4() {
  std::cout << "Executing Transition 4 on thread " << std::this_thread::get_id()
            << '\n';
  auto dur = 200 + std::rand() / ((RAND_MAX + 1500u) / 1500);
  sleep(std::chrono::milliseconds(dur));
  return std::nullopt;
}

OptionalReducer action5() {
  std::cout << "Executing Transition 5 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(450));
  return std::nullopt;
}

OptionalReducer action6() {
  std::cout << "Executing Transition 6 on thread " << std::this_thread::get_id()
            << '\n';
  sleep(std::chrono::milliseconds(250));
  return std::nullopt;
}
