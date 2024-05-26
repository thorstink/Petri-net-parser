#pragma once
// clang-format off
#include "imgui.h"
// clang-format on

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>

#include "imfilebrowser.h"
#include "petri.h"
namespace model {

struct Model {
  // Model() {};
  // Model(const Model&) = delete;
  // Model& operator=(const Model&) = delete;
  struct shared {
    bool show_grid, context_menu_active;
    ImVec2 scrolling;
    const symmetri::AugmentedToken *selected_arc;
    const std::string *selected_node;

    std::chrono::steady_clock::time_point timestamp;
    std::filesystem::path working_dir;
    std::optional<std::filesystem::path> active_file;
    std::vector<ImVec2> t_positions, p_positions;
    std::vector<size_t> t_view, p_view;
    std::vector<std::string> colors;
    symmetri::Petri::PTNet net;
  };
  std::shared_ptr<shared> data = std::make_shared<shared>();
};

struct ViewModel {
  bool show_grid, context_menu_active;
  ImVec2 scrolling;
  const symmetri::AugmentedToken *selected_arc;
  const std::string *selected_node;

  std::vector<size_t> t_view, p_view;
  std::vector<std::string> colors;

  const symmetri::Petri::PTNet &net;
  const std::vector<ImVec2> &t_positions, p_positions;

  static inline ImGui::FileBrowser file_dialog;
  ViewModel() = delete;
  explicit ViewModel(const Model &m);
};

using Reducer = std::function<Model(Model &&)>;

Model initializeModel();

}  // namespace model
