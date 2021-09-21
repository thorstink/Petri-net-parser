#pragma once
#include "types.h"
#include <tuple>

std::tuple<types::TransitionMutation, types::TransitionMutation,
           types::Marking>
constructTransitionMutationMatrices(std::string file);