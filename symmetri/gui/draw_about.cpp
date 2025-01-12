#include "draw_about.h"

#include "reducers.h"
#define QUOTE(x) #x
#define STR(x) QUOTE(x)

const auto no_move_draw_resize =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

void AlignForWidth(float width, float alignment = 0.5f) {
  ImGuiStyle& style = ImGui::GetStyle();
  float avail = ImGui::GetContentRegionAvail().x;
  float off = (avail - width) * alignment;
  if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void draw_about(const model::ViewModel&) {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::Begin("About", NULL, no_move_draw_resize);
  ImGui::Text(
      "Farbart version %s\nA GUI for creating and simulating "
      "Symmetri-nets.",
      STR(VERSION));

  ImGuiStyle& style = ImGui::GetStyle();
  float width = 0.0f;
  width += ImGui::CalcTextSize("Ok").x;
  width += style.ItemSpacing.x;
  width += 100.0f;
  width += style.ItemSpacing.x;
  width += ImGui::CalcTextSize("Copy!").x;
  AlignForWidth(width);

  if (ImGui::Button("Ok")) {
    removeView(&draw_about);
  }
  ImGui::SameLine();
  ImGui::InvisibleButton("Fixed", ImVec2(100.0f, 1.0f));  // Fixed size
  ImGui::SameLine();
  if (ImGui::Button("Copy to clipboard")) {
    ImGui::LogToClipboard();
    ImGui::LogText(
        "Farbart version %s\nA GUI for creating and simulating "
        "Symmetri-nets.",
        STR(VERSION));
    ImGui::LogFinish();
  }

  ImGui::End();
}
