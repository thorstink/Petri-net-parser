#pragma once

#include <Eigen/Dense>
#include <optional>
#include <string>
#include <unordered_map>

namespace types {
using Transition = std::string;
using Marking = Eigen::VectorXi;
using TransitionMutation = std::unordered_map<Transition, Marking>;
using Transitions = std::vector<Transition>;


} // namespace types
