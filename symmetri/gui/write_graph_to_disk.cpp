#include "write_graph_to_disk.h"

#include <thread>

#include "reducers.h"
#include "tinyxml2/tinyxml2.h"

namespace farbart {

auto toXml(const symmetri::Petri::PTNet& net,
           const std::vector<model::Coordinate>& p_positions,
           const std::vector<model::Coordinate>& t_positions,
           const std::vector<size_t>& p_view,
           const std::vector<size_t>& t_view) {
  using namespace tinyxml2;
  auto doc = std::make_unique<XMLDocument>();
  XMLElement* pnml = doc->NewElement("pnml");
  pnml->SetAttribute("xmlns", "http://www.pnml.org/version-2009/grammar/pnml");
  doc->InsertFirstChild(pnml);
  auto xmlnet = pnml->InsertFirstChild(doc->NewElement("net"));
  auto root = xmlnet->InsertFirstChild(doc->NewElement("page"));

  for (auto idx : p_view) {
    XMLElement* child = doc->NewElement("place");
    XMLElement* s_child = doc->NewElement("graphics");
    XMLElement* s_s_child = doc->NewElement("position");
    child->SetAttribute("id", net.place[idx].c_str());
    s_s_child->SetAttribute("x", p_positions[idx].x);
    s_s_child->SetAttribute("y", p_positions[idx].y);
    s_child->InsertEndChild(s_s_child);
    child->InsertEndChild(s_child);
    root->InsertEndChild(child);
  }

  for (auto idx : t_view) {
    XMLElement* child = doc->NewElement("transition");
    XMLElement* s_child = doc->NewElement("graphics");
    XMLElement* s_s_child = doc->NewElement("position");
    child->SetAttribute("id", net.transition[idx].c_str());
    s_s_child->SetAttribute("x", t_positions[idx].x);
    s_s_child->SetAttribute("y", t_positions[idx].y);
    s_child->InsertEndChild(s_s_child);
    child->InsertEndChild(s_child);
    root->InsertEndChild(child);

    for (const auto& token : net.input_n[idx]) {
      XMLElement* child = doc->NewElement("arc");
      child->SetAttribute("source", net.place[std::get<size_t>(token)].c_str());
      child->SetAttribute("target", net.transition[idx].c_str());
      child->SetAttribute(
          "color",
          std::string(std::get<symmetri::Token>(token).toString()).c_str());
      root->InsertEndChild(child);
    }
    for (const auto& token : net.output_n[idx]) {
      XMLElement* child = doc->NewElement("arc");
      child->SetAttribute("source", net.transition[idx].c_str());
      child->SetAttribute("target", net.place[std::get<size_t>(token)].c_str());
      child->SetAttribute(
          "color",
          std::string(std::get<symmetri::Token>(token).toString()).c_str());
      root->InsertEndChild(child);
    }
  }
  return doc;
}

model::Computer writeGraphToDisk(const model::ViewModel& vm) {
  return [&net = vm.net, &p_positions = vm.p_positions,
          &t_positions = vm.t_positions, p_view = vm.p_view, t_view = vm.t_view,
          path = vm.active_file]() {
    auto doc = toXml(net, p_positions, t_positions, p_view, t_view);
    auto fut = addViewBlocking(&draw_confirmation_popup);
    fut.get();
    doc->SaveFile(path.c_str());
    return [path](model::Model&& m_ptr) {
      m_ptr.data->active_file = path.c_str();
      return m_ptr;
    };
  };
}

const auto no_move_draw_resize =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

void draw_confirmation_popup(const model::ViewModel&) {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGui::Begin("About", NULL, no_move_draw_resize);
  ImGui::Text("Are you sure you want to save?");

  if (ImGui::Button("Ok")) {
    removeView(&draw_confirmation_popup);
    rxdispatch::push([=](model::Model&& m) {
      if (auto search = m.data->blockers.find(&draw_confirmation_popup);
          search != m.data->blockers.end()) {
        search->second.set_value();
        m.data->blockers.erase(search);
      }
      return m;
    });
  }
  ImGui::End();
}

}  // namespace farbart
