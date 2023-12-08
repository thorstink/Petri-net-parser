#include "drawable.h"
#include "graph.hpp"
#include "imgui.h"
#include "redux.hpp"
// Creating a node graph editor for Dear ImGui
// Quick sample, not production code!
// This is quick demo I crafted in a few hours in 2015 showcasing how to use
// Dear ImGui to create custom stuff, which ended up feeding a thread full of
// better experiments. See https://github.com/ocornut/imgui/issues/306 for
// details

// Fast forward to 2023, see e.g.
// https://github.com/ocornut/imgui/wiki/Useful-Extensions#node-editors

// Changelog
// - v0.05 (2023-03): fixed for renamed api: AddBezierCurve()->AddBezierCubic().
// - v0.04 (2020-03): minor tweaks
// - v0.03 (2018-03): fixed grid offset issue, inverted sign of 'scrolling'
// - v0.01 (2015-08): initial version

#include <math.h>  // fmodf

#include "symmetri/colors.hpp"

inline ImU32 getColor(symmetri::Token token) {
  using namespace symmetri;
  switch (token) {
    case Color::Scheduled:
    case Color::Started:
    case Color::Deadlocked:
    case Color::Canceled:
    case Color::Paused:
    case Color::Failed:
      return IM_COL32(255, 0, 0, 255);
      break;
    case Color::Success:
      return IM_COL32(0, 255, 0, 255);
      break;
    default: {
      // create new color and lookup if it already exists.
      return IM_COL32(255, 200, 0, 255);
    }
  }
};

// NB: You can use math functions/operators on ImVec2 if you #define
// IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h" Here we only
// declare simple +/- operators so others don't leak into the demo code.
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
  return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
  return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

// Dummy data structure provided for the example.
// Note that we storing links as indices (not ID) to make example code shorter.
template <>
void draw(Graph& g) {
  // State
  static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
  static bool show_grid = true;
  static Symbol node_selected = -1;

  // Initialization
  ImGuiIO& io = ImGui::GetIO();
  // Draw a list of nodes on the left side
  bool open_context_menu = false;
  Symbol node_hovered_in_list = -1;
  int node_hovered_in_scene = -1;
  ImGui::BeginChild("node_list", ImVec2(150, 0));
  ImGui::Text("Places");
  ImGui::Separator();
  for (const auto& node : g.nodes) {
    if (node.id.chr() == 'P') {
      ImGui::PushID(node.id);
      if (ImGui::Selectable(node.name.c_str(), node.id == node_selected)) {
        node_selected = node.id;
      }
      if (ImGui::IsItemHovered()) {
        node_hovered_in_list = node.id;
        // node_hovered_in_list = node.id;
        // open_context_menu |= ImGui::IsMouseClicked(1);
      }
      ImGui::PopID();
    }
  }

  ImGui::Dummy(ImVec2(0.0f, 20.0f));
  ImGui::Text("Transitions");
  ImGui::Separator();
  for (const auto& node : g.nodes) {
    if (node.id.chr() == 'T') {
      ImGui::PushID(node.id);
      if (ImGui::Selectable(node.name.c_str(), node.id == node_selected)) {
        node_selected = node.id;
      }
      if (ImGui::IsItemHovered()) {
        node_hovered_in_list = node.id;
        // node_hovered_in_list = node.id;
        // open_context_menu |= ImGui::IsMouseClicked(1);
      }
      ImGui::PopID();
    }
  }
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginGroup();

  const float NODE_SLOT_RADIUS = 4.0f;
  const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

  // Create our child canvas
  ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x,
              scrolling.y);
  ImGui::SameLine(ImGui::GetWindowWidth() - 100);
  ImGui::Checkbox("Show grid", &show_grid);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
  ImGui::PopStyleVar();  // WindowPadding
  ImGui::PushItemWidth(120.0f);

  const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // Display grid
  if (show_grid) {
    ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    float GRID_SZ = 64.0f;
    ImVec2 win_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetWindowSize();
    for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
      draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                         ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
    for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
      draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                         ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
  }

  // Display links
  draw_list->ChannelsSplit(2);
  draw_list->ChannelsSetCurrent(0);  // Background
  for (const auto& [edge, color, from_to] : g.arcs) {
    ImVec2 p1 = offset + from_to[0]->GetCenterPos();
    ImVec2 p2 = offset + from_to[1]->GetCenterPos();
    ImVec2 d = p1 - p2;
    d.x *= 0.33;
    d.y *= 0.33;
    ImU32 imcolor = getColor(color);
    draw_list->AddLine(p1, p2, imcolor, 2.0f);
  }

  // Display nodes
  for (auto& node : g.nodes) {
    ImGui::PushID(node.id);
    ImVec2 node_rect_min = offset + node.Pos;

    // Display node contents first
    draw_list->ChannelsSetCurrent(1);  // Foreground
    bool old_any_active = ImGui::IsAnyItemActive();
    auto textWidth = ImGui::CalcTextSize(node.name.c_str()).x;
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING +
                              ImVec2(8.0f - textWidth * 0.5f, -20.0f));
    ImGui::BeginGroup();  // Lock horizontal position
    ImGui::Text("%s", node.name.c_str());
    ImGui::EndGroup();

    // Save the size of what we have emitted and whether any of the widgets are
    // being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    node.Size =
        ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    node.Size.x = node.Size.y;
    ImVec2 node_rect_max = node_rect_min + node.Size;

    // Display node box
    draw_list->ChannelsSetCurrent(0);  // Background
    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", node.Size);
    if (ImGui::IsItemHovered()) {
      node_hovered_in_scene = node.id;
      open_context_menu |= ImGui::IsMouseClicked(1);
    }
    bool node_moving_active = ImGui::IsItemActive();
    if (node_widgets_active || node_moving_active) node_selected = node.id;
    if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      node.Pos = node.Pos + io.MouseDelta;

    const int opacity = 255;
    auto select_color = node.id == node_hovered_in_list
                            ? IM_COL32(255, 255, 0, opacity)
                            : IM_COL32(100, 100, 100, opacity);
    if (node.id.chr() == 'P') {
      draw_list->AddCircleFilled(offset + node.GetCenterPos(),
                                 0.5f * node.Size.x,
                                 IM_COL32(135, 135, 135, opacity), -5);
      draw_list->AddCircle(offset + node.GetCenterPos(), 0.5f * node.Size.x,
                           select_color, -5, 3.0f);

    } else {
      draw_list->AddRectFilled(node_rect_min, node_rect_max,
                               IM_COL32(200, 200, 200, opacity), 4.0f);
      draw_list->AddRect(node_rect_min, node_rect_max, select_color, 4.0f, 0,
                         3.0f);
    }

    ImGui::PopID();
  }
  draw_list->ChannelsMerge();

  // Open context menu
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) ||
        !ImGui::IsAnyItemHovered()) {
      node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
      open_context_menu = true;
    }
  if (open_context_menu) {
    ImGui::OpenPopup("context_menu");
    // if (node_hovered_in_list != -1) node_selected = node_hovered_in_list;
    // if (node_hovered_in_scene != -1) node_selected = node_hovered_in_scene;
  }

  // Draw context menu
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  if (ImGui::BeginPopup("context_menu")) {
    // Node* node = node_selected != Symbol('x') ? &nodes[node_selected] : NULL;
    ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
    if (false) {
      // if (node) {
      // ImGui::Text("Node '%s'", node->Name);
      ImGui::Separator();
      if (ImGui::MenuItem("Rename..", NULL, false, false)) {
      }
      if (ImGui::MenuItem("Delete", NULL, false, false)) {
      }
      if (ImGui::MenuItem("Copy", NULL, false, false)) {
      }
    } else {
      if (ImGui::MenuItem("Add")) {
        MVC::push([=](Model&& m) {
          std::cout << "add node" << std::endl;
          return m;
        });
        // nodes.push_back(Node(nodes.Size, "New node", scene_pos, 2, 2, 1, 1));
      }
      if (ImGui::MenuItem("Paste", NULL, false, false)) {
      }
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();

  // Scrolling
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    scrolling = scrolling + io.MouseDelta;

  ImGui::PopItemWidth();
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  ImGui::EndGroup();
}
