#define IMGUI_DEFINE_MATH_OPERATORS
#include "reducers.h"

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <tuple>

#include "draw_context_menu.h"
#include "imgui_internal.h"
#include "petri.h"
#include "rxdispatch.h"
void moveView(const ImVec2& d) {
  rxdispatch::push([=](model::Model&& m) mutable {
    m.data->scrolling.x += d.x;
    m.data->scrolling.y += d.y;
    return m;
  });
}

void moveNode(bool is_place, size_t idx, const ImVec2& d) {
  rxdispatch::push([=](model::Model&& m) mutable {
    (is_place ? m.data->p_positions[idx] : m.data->t_positions[idx]) +=
        model::Coordinate{d.x, d.y};
    return m;
  });
}

void showGrid(bool show_grid) {
  rxdispatch::push([=](model::Model&& m) mutable {
    m.data->show_grid = show_grid;
    return m;
  });
}

std::string viewContainsNameAlready(const std::vector<size_t>& view,
                                    const std::vector<std::string>& names,
                                    std::string&& name, size_t j = 0) {
  for (size_t i : view) {
    if (names[i] == name) {
      return viewContainsNameAlready(
          view, names, std::move(name) + "_" + std::to_string(j), j + 1);
    }
  }
  return std::move(name);
}

void addNode(bool is_place, ImVec2 pos) {
  rxdispatch::push([=](model::Model&& m) mutable {
    if (is_place) {
      m.data->net.place.push_back(
          viewContainsNameAlready(m.data->p_view, m.data->net.place, "place"));
      m.data->net.p_to_ts_n.push_back({});
      m.data->p_positions.push_back(model::Coordinate{pos.x, pos.y});
      m.data->p_view.push_back(m.data->net.place.size() - 1);
    } else {
      m.data->net.transition.push_back(viewContainsNameAlready(
          m.data->t_view, m.data->net.transition, "transition"));
      m.data->net.output_n.push_back({});
      m.data->net.input_n.push_back({});
      m.data->net.priority.push_back(0);
      m.data->net.store.emplace_back(
          symmetri::identity<symmetri::DirectMutation>{});
      m.data->t_positions.push_back(model::Coordinate{pos.x, pos.y});
      m.data->t_view.push_back(m.data->net.transition.size() - 1);
    }
    std::erase(m.data->drawables, &draw_context_menu);
    return m;
  });
}

void removeArc(bool is_input, size_t transition_idx, size_t sub_idx) {
  rxdispatch::push([=](model::Model&& m) mutable {
    // remove transition from view
    m.data->t_view.erase(std::remove(m.data->t_view.begin(),
                                     m.data->t_view.end(), transition_idx),
                         m.data->t_view.end());

    const size_t new_idx = m.data->net.transition.size();
    // add it again to the view...
    m.data->net.transition.push_back(m.data->net.transition[transition_idx]);
    m.data->net.store.emplace_back(
        symmetri::identity<symmetri::DirectMutation>{});
    m.data->net.priority.push_back(m.data->net.priority[transition_idx]);
    m.data->t_positions.push_back(m.data->t_positions[transition_idx]);
    m.data->t_view.push_back(new_idx);

    // now copy the arcs, except for the one we want to delete
    if (is_input) {
      m.data->net.output_n.push_back(m.data->net.output_n[transition_idx]);
      m.data->net.input_n.push_back({});
      std::copy_if(m.data->net.input_n[transition_idx].begin(),
                   m.data->net.input_n[transition_idx].end(),
                   std::back_inserter(m.data->net.input_n.back()),
                   [=, i = size_t(0)](auto) mutable { return i++ != sub_idx; });
    } else {
      m.data->net.input_n.push_back(m.data->net.input_n[transition_idx]);
      m.data->net.output_n.push_back({});
      std::copy_if(m.data->net.output_n[transition_idx].begin(),
                   m.data->net.output_n[transition_idx].end(),
                   std::back_inserter(m.data->net.output_n.back()),
                   [=, i = size_t(0)](auto) mutable { return i++ != sub_idx; });
    }
    std::erase(m.data->drawables, &draw_context_menu);

    return m;
  });
}

void addArc(bool is_place, size_t source, size_t target,
            symmetri::Token color) {
  rxdispatch::push([=](model::Model&& m) mutable {
    const size_t transition_idx = is_place ? target : source;
    // remove transition from view
    m.data->t_view.erase(std::remove(m.data->t_view.begin(),
                                     m.data->t_view.end(), transition_idx),
                         m.data->t_view.end());
    // add it again to the view...
    const size_t new_transition_idx = m.data->net.transition.size();
    m.data->net.transition.push_back(m.data->net.transition[transition_idx]);
    m.data->net.output_n.push_back(m.data->net.output_n[transition_idx]);
    m.data->net.input_n.push_back(m.data->net.input_n[transition_idx]);
    m.data->net.priority.push_back(m.data->net.priority[transition_idx]);
    m.data->net.store.emplace_back(
        symmetri::identity<symmetri::DirectMutation>{});

    m.data->t_positions.push_back(m.data->t_positions[transition_idx]);
    m.data->t_view.push_back(new_transition_idx);

    // add the arc
    if (is_place) {
      auto& p_to_ts = m.data->net.p_to_ts_n[source];
      m.data->net.input_n[new_transition_idx].push_back({source, color});
      if (std::find(p_to_ts.begin(), p_to_ts.end(), transition_idx) ==
          p_to_ts.end()) {
        p_to_ts.push_back(new_transition_idx);
      }
    } else {
      m.data->net.output_n[new_transition_idx].push_back({target, color});
    }

    std::erase(m.data->drawables, &draw_context_menu);

    return m;
  });
}

void removePlace(size_t idx) {
  rxdispatch::push([=](model::Model&& m) mutable {
    // remove node from view
    m.data->p_view.erase(
        std::remove(m.data->p_view.begin(), m.data->p_view.end(), idx),
        m.data->p_view.end());
    // remove selection of this node
    m.data->selected_node_idx.reset();
    // remove marking for this node
    auto [b, e] = std::ranges::remove_if(
        m.data->tokens, [idx](const symmetri::AugmentedToken at) {
          return std::get<size_t>(at) == idx;
        });
    m.data->tokens.erase(b, e);
    std::erase(m.data->drawables, &draw_context_menu);

    return m;
  });
}

void removeTransition(size_t idx) {
  rxdispatch::push([=](model::Model&& m) mutable {
    m.data->t_view.erase(
        std::remove(m.data->t_view.begin(), m.data->t_view.end(), idx),
        m.data->t_view.end());
    m.data->selected_node_idx.reset();
    std::erase(m.data->drawables, &draw_context_menu);

    return m;
  });
}

void updateArcColor(bool is_input, size_t idx, size_t sub_idx,
                    const symmetri::Token color) {
  rxdispatch::push([=](model::Model&& m) mutable {
    std::get<symmetri::Token>(
        (is_input ? m.data->net.input_n : m.data->net.output_n)[idx][sub_idx]) =
        color;
    return m;
  });
}

ImGuiInputTextCallback updatePriority(const size_t id) {
  static size_t idx;
  idx = id;
  return [](ImGuiInputTextCallbackData* data) -> int {
    data->ClearSelection();
    if ((data->Buf != NULL) && (data->Buf[0] == '\0')) {
    } else if (strcmp(data->Buf, "-") == 0) {
    } else {
      const auto priority =
          std::clamp(std::stoi(data->Buf), INT8_MIN, INT8_MAX);
      rxdispatch::push([=](model::Model&& m) mutable {
        m.data->net.priority[idx] = priority;
        return m;
      });
    }
    return 0;
  };
}

ImGuiInputTextCallback updatePlaceName(const size_t id) {
  static size_t idx;
  idx = id;

  return [](ImGuiInputTextCallbackData* data) -> int {
    data->ClearSelection();
    rxdispatch::push(
        [=, name = std::string(data->Buf)](model::Model&& m) mutable {
          m.data->net.place[idx] = name;
          return m;
        });
    return 0;
  };
}

ImGuiInputTextCallback updateTransitionName(const size_t id) {
  static size_t idx;
  idx = id;
  return [](ImGuiInputTextCallbackData* data) -> int {
    data->ClearSelection();
    rxdispatch::push(
        [=, name = std::string(data->Buf)](model::Model&& m) mutable {
          m.data->net.transition[idx] = name;
          return m;
        });
    return 0;
  };
}

void setContextMenuActive() {
  rxdispatch::push([](model::Model&& m) {
    m.data->drawables.push_back(&draw_context_menu);
    return m;
  });
}

void setContextMenuInactive() {
  rxdispatch::push([](model::Model&& m) {
    std::erase(m.data->drawables, &draw_context_menu);
    m.data->selected_target_node_idx.reset();
    return m;
  });
}

void setSelectedNode(bool is_place, size_t idx) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->selected_node_idx = {is_place, idx};
    m.data->selected_arc_idxs.reset();
    return m;
  });
};

void setSelectedTargetNode(bool is_place, size_t idx) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->selected_target_node_idx = {is_place, idx};
    return m;
  });
};

void resetSelection() {
  rxdispatch::push([](model::Model&& m) {
    m.data->selected_node_idx.reset();
    m.data->selected_arc_idxs.reset();
    return m;
  });
};

void setSelectedArc(bool is_input, size_t idx, size_t sub_idx) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->selected_node_idx.reset();
    m.data->selected_arc_idxs = {is_input, idx, sub_idx};
    return m;
  });
};

void renderNodeEntry(bool is_place, const std::string& name, size_t idx,
                     bool selected) {
  ImGui::PushID(idx);
  if (ImGui::Selectable(name.c_str(), selected)) {
    setSelectedNode(is_place, idx);
  }
  ImGui::PopID();
}

int updateActiveFile(ImGuiInputTextCallbackData* data) {
  data->ClearSelection();
  if (data->Buf != nullptr) {
    rxdispatch::push([file_name = std::string(data->Buf)](model::Model&& m) {
      m.data->active_file = std::filesystem::path(file_name);
      return m;
    });
  }
  return 0;
}

void updateColorTable() {
  rxdispatch::push([](model::Model&& m) {
    m.data->colors = symmetri::Token::getColors();
    return m;
  });
}

void addTokenToPlace(symmetri::AugmentedToken token) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->tokens.push_back(token);
    std::erase(m.data->drawables, &draw_context_menu);
    return m;
  });
}

void removeTokenFromPlace(symmetri::AugmentedToken token) {
  rxdispatch::push([=](model::Model&& m) {
    if (auto it = std::ranges::find(m.data->tokens, token);
        it != m.data->tokens.end()) {
      m.data->tokens.erase(it);
    }
    return m;
  });
}

void resetSelectedTargetNode() {
  rxdispatch::push([](model::Model&& m) {
    m.data->selected_arc_idxs.reset();
    return m;
  });
}

void resetNetView() {
  rxdispatch::push([=](model::Model&& model) {
    auto& m = *model.data;
    const auto getMinX = [&](const auto& in_view, const auto& pos) {
      return std::ranges::min(in_view | std::views::transform([&](auto idx) {
                                return pos[idx].x;
                              }));
    };
    const auto getMinY = [&](const auto& in_view, const auto& pos) {
      return std::ranges::min(in_view | std::views::transform([&](auto idx) {
                                return pos[idx].y;
                              }));
    };

    float min_x = std::min(getMinX(m.t_view, m.t_positions),
                           getMinX(m.p_view, m.p_positions));
    float min_y = std::min(getMinY(m.t_view, m.t_positions),
                           getMinY(m.p_view, m.p_positions));

    m.scrolling = {400 - min_x, 120 - min_y};

    return model;
  });
}

void tryFire(size_t transition_idx) {
  rxdispatch::push([=](model::Model&& m) {
    if (canFire(m.data->net.input_n[transition_idx], m.data->tokens)) {
      // deduct
      deductMarking(m.data->tokens, m.data->net.input_n[transition_idx]);
      // add
      const auto& lookup_t = m.data->net.output_n[transition_idx];
      m.data->tokens.reserve(m.data->tokens.size() + lookup_t.size());
      for (const auto& [p, c] : lookup_t) {
        m.data->tokens.push_back({p, fire(m.data->net.store[transition_idx])});
      }
    }
    return m;
  });
}
void updateTransitionOutputColor(size_t transition_idx, symmetri::Token color) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->net.store[transition_idx] = [=] { return color; };
    return m;
  });
}
