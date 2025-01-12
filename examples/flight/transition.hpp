#pragma once
#include <atomic>
#include <chrono>
#include <iostream>

/**
 * @brief Foo is an example of a class that has success/fail options and
 * pause/resume functionalities. The custom copy constructor is because
 * atomic<bool> is not default copyable. The action is simply a counter with a
 * sleep that counts until it reaches the max count, which means success, or it
 * gets preempted, forcing a fail. This class, a can be seen, is completely
 * independent of Symmetri. In order to let Symmetri use the functionality,
 * custom functions need to be implemented. This is done in flight.cc.
 */
class Foo {
 private:
  const std::string name;                ///< identifier for the class
  const unsigned count;                  ///< the max count
  const std::chrono::seconds interval;   ///< sleep in between increments
  mutable std::atomic<bool> cancel_;     ///< did someone call cancel?
  mutable std::atomic<bool> is_paused_;  ///< did some call pause?

 public:
  Foo(std::string m, unsigned count = 5, int s = 1)
      : name(m), count(count), interval(s), cancel_(false), is_paused_(false) {
    printf("ctor %s\n", m.c_str());
  }
  Foo(Foo&& f) = delete;
  Foo(const Foo& o) = delete;
  Foo& operator=(Foo&& other) = delete;
  Foo& operator=(const Foo& other) = delete;

  bool fire() const {
    cancel_.store(false);
    is_paused_.store(false);
    std::cout << "Running " << name << std::endl;
    unsigned counter = 0;
    do {
      if (!is_paused_.load()) {
        std::cout << "Running " << name << " " << counter++ << std::endl;

      } else {
        std::cout << "Paused " << name << " " << counter << std::endl;
      }
      std::this_thread::sleep_for(interval);
    } while (counter < count && !cancel_.load());
    return cancel_.load();
  }

  void cancel() const {
    std::cout << "cancel " << name << std::endl;
    cancel_.store(true);
  }

  void pause() const {
    std::cout << "pause " << name << std::endl;
    is_paused_.store(true);
  }

  void resume() const {
    std::cout << "resume " << name << std::endl;
    is_paused_.store(false);
  }
};
