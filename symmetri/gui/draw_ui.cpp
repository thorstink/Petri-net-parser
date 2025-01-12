#define IMGUI_DEFINE_MATH_OPERATORS

#include "draw_ui.h"

#include "draw_context_menu.h"
#include "draw_graph.h"
#include "draw_menu.h"
#include "imgui.h"
#include "reducers.h"

constexpr float menu_width = 350;

const auto no_move_draw_resize = ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove;

void draw_interface(const model::ViewModel& vm) {
  ImGui::SetNextWindowSize(
      ImVec2(menu_width, ImGui::GetIO().DisplaySize.y - 20));
  ImGui::SetNextWindowPos(ImVec2(0, 20));

  ImGui::Begin("tools", NULL, no_move_draw_resize);
  draw_menu(vm);
  ImGui::End();
  ImGui::SameLine();
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - menu_width,
                                  ImGui::GetIO().DisplaySize.y));
  ImGui::SetNextWindowPos(ImVec2(menu_width, 20));
  ImGui::Begin("graph", NULL,
               no_move_draw_resize | ImGuiWindowFlags_NoBringToFrontOnFocus);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
      ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
    setContextMenuActive();
  } else if (std::find(vm.drawables.begin(), vm.drawables.end(),
                       &draw_context_menu) != vm.drawables.end() &&
             ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
             !ImGui::IsAnyItemHovered()) {
    setContextMenuInactive();
  }
  draw_graph(vm);
  ImGui::End();
}
