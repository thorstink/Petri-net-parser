#define IMGUI_DEFINE_MATH_OPERATORS

#include "draw_menu.h"
#include "imgui_internal.h"
#include "reducers.h"
#include "shared.h"
#include "simulation_menu.h"

void draw_context_menu(const model::ViewModel& vm) {
  ImGui::OpenPopup("context_menu");
  // Draw context menu
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  if (ImGui::BeginPopup("context_menu")) {
    if (vm.selected_node_idx.has_value()) {
      const bool is_place = std::get<0>(vm.selected_node_idx.value());
      const size_t selected_idx = std::get<1>(vm.selected_node_idx.value());
      ImGui::Text(
          "%s '%s'", (is_place ? "Place" : "Transition"),
          (is_place ? vm.net.place : vm.net.transition)[selected_idx].c_str());
      ImGui::Separator();
      if (ImGui::BeginMenu("Add arc to...")) {
        for (const auto& node_idx : is_place ? vm.t_view : vm.p_view) {
          if (is_place) {
            drawColorDropdownMenu(vm.net.transition[node_idx].c_str(),
                                  vm.colors, [&](const char* c) {
                                    addArc(is_place, selected_idx, node_idx,
                                           symmetri::Token(c));
                                  });

          } else if (ImGui::MenuItem((vm.net.place[node_idx].c_str()))) {
            addArc(is_place, selected_idx, node_idx, symmetri::Success);
          }
          if (ImGui::IsItemHovered()) {
            bool is_target_place = not is_place;
            if (not vm.selected_target_node_idx.has_value() ||
                vm.selected_target_node_idx.value() !=
                    std::tuple<bool, size_t>{is_target_place, node_idx}) {
              setSelectedTargetNode(is_target_place, node_idx);
            }
          }
        }
        ImGui::EndMenu();
      }
      if (is_place) {
        drawColorDropdownMenu("Add marking", vm.colors, [=](const char* c) {
          addTokenToPlace(
              symmetri::AugmentedToken{selected_idx, symmetri::Token(c)});
        });
      }

      if (ImGui::MenuItem("Delete")) {
        is_place ? removePlace(selected_idx) : removeTransition(selected_idx);
      }
    } else if (vm.selected_arc_idxs.has_value()) {
      ImGui::Text("Arc");
      ImGui::Separator();
      if (ImGui::MenuItem("Delete")) {
        const auto& [is_input, idx, sub_idx] = vm.selected_arc_idxs.value();
        removeArc(is_input, idx, sub_idx);
      }
    } else {
      // @todo make this offset relative to the menubars
      ImVec2 scene_pos = ImGui::GetIO().MousePos + ImVec2(-275, -40);
      if (ImGui::MenuItem("Add place")) {
        addNode(true, scene_pos);
      }
      if (ImGui::MenuItem("Add transition")) {
        addNode(false, scene_pos);
      }
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}
