#include "shared.h"

#include "color.hpp"
#include "imgui.h"
#include "symmetri/types.h"

void drawColorDropdownMenu(
    const char* menu_name, const std::vector<std::string>& colors,
    const std::function<void(const std::string&)>& func) {
  if (ImGui::BeginMenu(menu_name)) {
    for (const auto& color : colors) {
      ImGui::PushStyleColor(ImGuiCol_Text,
                            getColor(symmetri::Token(color.c_str())));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                            getColor(symmetri::Token(color.c_str())));
      if (ImGui::MenuItem(color.c_str())) {
        func(color);
      }
      ImGui::PopStyleColor();
      ImGui::PopStyleColor();
    }
    ImGui::EndMenu();
  };
}

void drawColorDropdownMenu(const char* menu_name,
                           const std::vector<const char*>& colors,
                           const std::function<void(const char*)>& func) {
  if (ImGui::BeginMenu(menu_name)) {
    for (const auto& color : colors) {
      ImGui::PushStyleColor(ImGuiCol_Text, getColor(symmetri::Token(color)));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                            getColor(symmetri::Token(color)));
      if (ImGui::MenuItem(color)) {
        func(color);
      }
      ImGui::PopStyleColor();
      ImGui::PopStyleColor();
    }
    ImGui::EndMenu();
  };
}

ImU32 getColor(symmetri::Token token) {
  using namespace symmetri;
  static std::unordered_map<Token, ImU32> color_table;
  const auto ptr = color_table.find(token);
  if (ptr != std::end(color_table)) {
    return ptr->second;
  } else {
    const auto rgb = hsv_to_rgb(ratio(), 1., 1.);
    const auto color = IM_COL32(rgb[0], rgb[1], rgb[2], 128);
    color_table.insert({token, color});
    return color;
  }
};
