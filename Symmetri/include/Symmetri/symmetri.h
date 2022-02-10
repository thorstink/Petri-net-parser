#pragma once
#include <set>
#include <string>

#include "Symmetri/types.h"
namespace symmetri {

std::function<symmetri::OptionalError()> start(
    const std::set<std::string> &path_to_petri, const TransitionActionMap &);
}  // namespace symmetri
