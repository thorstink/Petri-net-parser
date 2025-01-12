#pragma once

#include "model.h"

namespace farbart {
model::Computer writeGraphToDisk(const model::ViewModel& vm);
void draw_confirmation_popup(const model::ViewModel& vm);

}  // namespace farbart
