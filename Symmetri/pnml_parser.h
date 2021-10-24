#pragma once
#include "Symmetri/types.h"
#include "json.hpp"
#include <tuple>

std::tuple<symmetri::TransitionMutation, symmetri::TransitionMutation,
           symmetri::Marking, nlohmann::json, symmetri::Conversions,
           symmetri::Conversions>
constructTransitionMutationMatrices(std::string file);

nlohmann::json
webMarking(const symmetri::Marking &M,
           const std::map<Eigen::Index, std::string> &index_marking_map);
