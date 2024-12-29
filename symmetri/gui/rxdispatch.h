#pragma once

#include <functional>
#include <iostream>
#include <variant>

#include "model.h"
#include "rpp/rpp.hpp"
#include "rximgui.h"

namespace rxdispatch {

using Update =
    std::variant<model::Reducer, std::function<model::Reducer(void)>>;

void unsubscribe();
moodycamel::BlockingConcurrentQueue<Update>& getQueue();
void push(Update&& r);

struct VisitPackage {
  auto operator()(model::Reducer r) {
    return rpp::source::just(rximgui::rl, std::move(r)).as_dynamic();
  }
  auto operator()(std::function<model::Reducer(void)> f) {
    return (rpp::source::just(scheduler, std::move(f)) |
            rpp::operators::map([](auto f) { return f(); }))
        .as_dynamic();
  }

 private:
  inline static rpp::schedulers::thread_pool scheduler{4};
};

inline auto get_events_observable() {
  return rpp::source::create<Update>([](auto&& observer) {
           Update f;
           auto mainthreadid = std::this_thread::get_id();
           while (not observer.is_disposed()) {
             std::cout << "dispatch " << mainthreadid << std::endl;
             getQueue().wait_dequeue(f);
             observer.on_next(std::forward<Update>(f));
           }
         }) |
         rpp::operators::subscribe_on(rpp::schedulers::new_thread{}) |
         rpp::operators::flat_map(
             [](auto value) { return std::visit(VisitPackage(), value); }) |
         rpp::operators::tap([](auto&&) {
           std::cout << "tap " << std::this_thread::get_id() << std::endl;
         });
}

}  // namespace rxdispatch
