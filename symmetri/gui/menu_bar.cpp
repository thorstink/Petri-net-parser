#include "menu_bar.h"

#include <mutex>
// clang-format off
#include "imgui.h"
#include "imfilebrowser.h"
// clang-format on
#include "draw_about.h"
#include "load_file.h"
#include "reducers.h"
#include "rxdispatch.h"
#include "symmetri/parsers.h"
#include "write_graph_to_disk.h"
ImGui::FileBrowser file_dialog = ImGui::FileBrowser();

void draw_menu_bar(const model::ViewModel &vm) {
  static std::once_flag flag;
  std::call_once(flag, [&] {
    file_dialog.SetTitle("Open a Symmetri-net");
    file_dialog.SetTypeFilters({".pnml", ".grml"});
  });
  file_dialog.Display();
  if (file_dialog.HasSelected()) {
    loadPetriNet(file_dialog.GetSelected());
    file_dialog.ClearSelected();
  }
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New")) {
      }
      if (ImGui::MenuItem("Open")) {
        file_dialog.Open();
      }
      if (ImGui::MenuItem("Save")) {
        rxdispatch::push(farbart::writeGraphToDisk(vm));
      }
      // Exit...
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      //...
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      //...
      if (ImGui::MenuItem("Reset view")) {
        resetNetView();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      //...
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        addView(&draw_about);
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
  ImGui::SetNextWindowSize(
      ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
  ImGui::SetNextWindowPos(ImVec2(0, 20));  // fix this
}
