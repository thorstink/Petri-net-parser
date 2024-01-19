#include "menu_bar.h"

#include "model.h"
#include "position_parsers.h"
#include "rxdispatch.h"
model::Reducer updateActiveFile(const std::filesystem::path &file) {
  return [=](model::Model &m_ptr) {
    auto &m = *m_ptr.data;
    m.active_file = file;
    symmetri::Net net;
    symmetri::Marking marking;
    symmetri::PriorityTable pt;
    std::map<std::string, std::pair<float, float>> positions;

    const std::filesystem::path pn_file = m.active_file.value();
    if (pn_file.extension() == std::string(".pnml")) {
      std::tie(net, marking) = symmetri::readPnml({pn_file});
      positions = farbart::readPnmlPositions({pn_file});

    } else {
      std::tie(net, marking, pt) = symmetri::readGrml({pn_file});
    }
    m.graph.reset(*createGraph(net, positions));
    // auto g = createGraph(net);
    // m.graph = *g;
    // m.arcs = g->arcs;
    // m.nodes = g->nodes;
    // m.a_idx = g->a_idx;
    // m.n_idx = g->n_idx;
    return m_ptr;
  };
}

void draw_menu_bar(ImGui::FileBrowser &fileDialog) {
  fileDialog.Display();
  if (fileDialog.HasSelected()) {
    rxdispatch::push(updateActiveFile(fileDialog.GetSelected()));
    fileDialog.ClearSelected();
  }
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New")) {
      }
      if (ImGui::MenuItem("Open")) {
        fileDialog.Open();
      }
      // Exit...
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      //...
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window")) {
      //...
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      //...
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
  ImGui::SetNextWindowSize(
      ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
  ImGui::SetNextWindowPos(ImVec2(0, 20));  // fix this
}
