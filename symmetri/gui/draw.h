#pragma once

#include "model.h"

void drawUi(const model::ViewModel& vm) {
  for (auto drawable : vm.drawables) {
    drawable(vm);
  }
}
