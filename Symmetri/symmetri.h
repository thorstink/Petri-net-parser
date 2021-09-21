#pragma once
#include "model.h"
#include <functional>
#include <string>

namespace symmetri {
std::function<void()> start(const std::string &,
                            const model::TransitionActionMap &);
} // namespace symmetri
