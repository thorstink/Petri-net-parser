#define IMGUI_DEFINE_MATH_OPERATORS

#include "draw_graph.h"

#include <math.h>  // fmodf

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string_view>

#include "color.hpp"
#include "draw_context_menu.h"
#include "draw_graph.h"
#include "draw_marking.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "reducers.h"
#include "rxdispatch.h"
#include "shared.h"
#include "symmetri/colors.hpp"

// State
static ImVec2 size;
static ImVec2 offset;

static const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

static ImVec2 GetCenterPos(const model::Coordinate& pos, const ImVec2& size) {
  return ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
}

void draw_grid(const model::ViewModel& vm) {
  auto draw_list = ImGui::GetWindowDrawList();
  ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
  float GRID_SZ = 64.0f;
  ImVec2 win_pos = ImGui::GetCursorScreenPos();
  ImVec2 canvas_sz = ImGui::GetWindowSize();
  for (float x = fmodf(vm.scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
    draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                       ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
  for (float y = fmodf(vm.scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
    draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                       ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
};

void draw_arc(size_t t_idx, const model::ViewModel& vm) {
  const auto draw = [&](const symmetri::AugmentedToken& t, bool is_input,
                        size_t sub_idx) {
    if (std::find(vm.p_view.begin(), vm.p_view.end(), std::get<size_t>(t)) ==
        vm.p_view.end()) {
      return;
    }

    const ImVec2 i =
        offset + GetCenterPos(!is_input ? vm.t_positions[t_idx]
                                        : vm.p_positions[std::get<size_t>(t)],
                              size);
    const ImVec2 o =
        offset + GetCenterPos(is_input ? vm.t_positions[t_idx]
                                       : vm.p_positions[std::get<size_t>(t)],
                              size);

    const float max_distance = 2.f;
    const auto mouse_pos = ImGui::GetIO().MousePos;
    ImVec2 mouse_pos_projected_on_segment = ImLineClosestPoint(i, o, mouse_pos);
    ImVec2 mouse_pos_delta_to_segment =
        mouse_pos_projected_on_segment - mouse_pos;
    const bool is_segment_hovered = (ImLengthSqr(mouse_pos_delta_to_segment) <=
                                     max_distance * max_distance);
    if (is_segment_hovered && ImGui::IsMouseClicked(0)) {
      setSelectedArc(is_input, t_idx, sub_idx);
    }

    const auto is_selected_arc = [&]() {
      return vm.selected_arc_idxs.has_value() &&
             std::get<0>(*vm.selected_arc_idxs) == is_input &&
             std::get<1>(*vm.selected_arc_idxs) == t_idx &&
             std::get<2>(*vm.selected_arc_idxs) == sub_idx;
    };
    ImU32 imcolor =
        getColor(std::get<symmetri::Token>(t)) |
        ((ImU32)IM_F32_TO_INT8_SAT(is_selected_arc() ? 1.0f : 0.65f))
            << IM_COL32_A_SHIFT;

    auto draw_list = ImGui::GetWindowDrawList();

    const auto d = o - i;
    const auto theta = std::atan2(d.y, d.x) - M_PI_2;
    const float h = draw_list->_Data->FontSize * 1.00f;
    const float r = h * 0.40f * 1;
    const ImVec2 center = i + d * 0.6f;
    const auto a_sin = std::sin(theta);
    const auto a_cos = std::cos(theta);
    const auto a = ImRotate(ImVec2(+0.000f, +1.f) * r, a_cos, a_sin);
    const auto b = ImRotate(ImVec2(-1.f, -1.f) * r, a_cos, a_sin);
    const auto c = ImRotate(ImVec2(+1.f, -1.f) * r, a_cos, a_sin);
    draw_list->AddTriangleFilled(center + a, center + b, center + c, imcolor);
    draw_list->AddLine(i, o, imcolor, is_segment_hovered ? 3.0f : 2.0f);
  };

  for (size_t sub_idx = 0; sub_idx < vm.net.input_n[t_idx].size(); sub_idx++) {
    draw(vm.net.input_n[t_idx][sub_idx], true, sub_idx);
  }
  for (size_t sub_idx = 0; sub_idx < vm.net.output_n[t_idx].size(); sub_idx++) {
    draw(vm.net.output_n[t_idx][sub_idx], false, sub_idx);
  }
};

void draw_nodes(bool is_place, size_t idx, const std::string& name,
                const model::Coordinate& position, bool highlight,
                const std::vector<symmetri::AugmentedToken>& tokens) {
  ImGui::PushID(is_place ? idx + 10000 : idx);
  ImVec2 node_rect_min = offset + ImVec2(position.x, position.y);

  // Display node contents first
  auto textWidth = ImGui::CalcTextSize(name.c_str()).x;
  ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING +
                            ImVec2(8.0f - textWidth * 0.5f, -20.0f));
  ImGui::BeginGroup();              // Lock horizontal position
  ImGui::Text("%s", name.c_str());  // this crashed once..
  ImGui::EndGroup();

  // Save the size of what we have emitted and whether any of the widgets are
  // being used
  size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
  size.x = size.y;
  ImVec2 node_rect_max = node_rect_min + size;

  // Display node box
  ImGui::SetCursorScreenPos(node_rect_min);
  ImGui::InvisibleButton("node", size);

  const bool node_moving_active = ImGui::IsItemActive();
  const bool is_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  if (node_moving_active && is_clicked) {
    setSelectedNode(is_place, idx);
  } else if (node_moving_active &&
             ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    moveNode(is_place, idx, ImGui::GetIO().MouseDelta);
  }

  const int opacity = 255;
  auto draw_list = ImGui::GetWindowDrawList();
  auto select_color = highlight ? IM_COL32(255, 255, 0, opacity)
                                : IM_COL32(100, 100, 100, opacity);

  if (is_place) {
    draw_list->AddCircleFilled(offset + GetCenterPos(position, size),
                               0.5f * size.x, IM_COL32(200, 200, 200, opacity),
                               -5);
    draw_list->AddCircle(offset + GetCenterPos(position, size), 0.5f * size.x,
                         select_color, -5, 3.0f);
    const auto is_token_in_place = [idx](const auto token) {
      return std::get<size_t>(token) == idx;
    };
    const auto coordinates =
        getTokenCoordinates(std::ranges::count_if(tokens, is_token_in_place));
    int i = 0;
    for (auto color : tokens | std::views::filter(is_token_in_place) |
                          std::views::transform([](const auto& t) {
                            return std::get<symmetri::Token>(t);
                          })) {
      draw_list->AddCircle(
          coordinates[i++] + offset + GetCenterPos(position, size),
          0.05f * size.x, getColor(color), -5, 3.0f);
    }
  } else {
    draw_list->AddRectFilled(node_rect_min, node_rect_max,
                             IM_COL32(135, 135, 135, opacity), 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max, select_color, 4.0f, 0,
                       3.0f);
  }

  ImGui::PopID();
};

void draw_graph(const model::ViewModel& vm) {
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
      ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
      !ImGui::IsAnyItemHovered() &&
      (vm.selected_node_idx.has_value() || vm.selected_arc_idxs.has_value())) {
    resetSelection();
  }

  offset = ImVec2(vm.scrolling.x, vm.scrolling.y);

  static char view_name[256] = "";
  strcpy(view_name, vm.active_file.c_str());
  ImGui::PushItemWidth(-1);

  // currently we render the text green if the file does not yet exist.
  const bool file_exists =
      std::filesystem::is_regular_file(std::filesystem::path(vm.active_file));
  if (not file_exists) {
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
  }
  ImGui::InputText("##filepath", view_name, 256,
                   ImGuiInputTextFlags_CallbackEdit, &updateActiveFile);
  if (not file_exists) {
    ImGui::PopStyleColor();
  }

  ImGui::PopItemWidth();

  const auto io = ImGui::GetIO();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / io.Framerate, io.Framerate);

  ImGui::SameLine();
  bool show_grid = vm.show_grid;
  if (ImGui::Checkbox("Show grid", &show_grid)) {
    showGrid(show_grid);
  }

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

  // Display grid
  if (vm.show_grid) {
    draw_grid(vm);
  }

  // is now also true if there's nothing selected.
  const bool is_selected_node = vm.selected_node_idx.has_value();
  const bool is_target_node = vm.selected_target_node_idx.has_value();
  // draw places & transitions
  for (auto&& idx : vm.t_view) {
    draw_arc(idx, vm);
    const bool should_hightlight =
        (is_selected_node &&
         (!std::get<0>(vm.selected_node_idx.value()) &&
          idx == std::get<1>(vm.selected_node_idx.value()))) ||
        (is_target_node &&
         (!std::get<0>(vm.selected_target_node_idx.value()) &&
          idx == std::get<1>(vm.selected_target_node_idx.value())));

    draw_nodes(false, idx, vm.net.transition[idx], vm.t_positions[idx],
               should_hightlight, vm.tokens);
  }
  for (auto idx : vm.p_view) {
    const bool should_hightlight =
        (is_selected_node &&
         (std::get<0>(vm.selected_node_idx.value()) &&
          idx == std::get<1>(vm.selected_node_idx.value()))) ||
        (is_target_node &&
         (std::get<0>(vm.selected_target_node_idx.value()) &&
          idx == std::get<1>(vm.selected_target_node_idx.value())));
    draw_nodes(true, idx, vm.net.place[idx], vm.p_positions[idx],
               should_hightlight, vm.tokens);
  }

  // Scrolling
  if (ImGui::IsWindowHovered() &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
    moveView(ImGui::GetIO().MouseDelta);
    if (std::find(vm.drawables.begin(), vm.drawables.end(),
                  &draw_context_menu) != vm.drawables.end()) {
      setContextMenuInactive();
    }
  }

  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}
