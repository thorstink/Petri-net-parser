#pragma once

#include "rpp/rpp.hpp"

namespace rximgui {
static inline rpp::subjects::publish_subject<int> framebus;
static inline auto frameout = framebus.get_observer();
static inline auto sendframe = []() { frameout.on_next(1); };
static inline auto frames = framebus.get_observable();
static inline rpp::schedulers::run_loop rl{};

}  // namespace rximgui
