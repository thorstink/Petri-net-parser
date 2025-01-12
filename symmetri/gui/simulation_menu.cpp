#include "simulation_menu.h"

#include <ranges>

#include "petri.h"
#include "reducers.h"
void draw_simulation_menu(const model::ViewModel& vm) {
  ImGui::Text("Simulation");
  ImGui::Separator();

  for (auto transition_idx : vm.t_fireable) {
    if (ImGui::Selectable(vm.net.transition[transition_idx].c_str())) {
      tryFire(transition_idx);
    }
  }
  ImGui::Dummy(ImVec2(0.0f, 20.0f));
}
