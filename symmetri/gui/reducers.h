#pragma once

#include <future>
#include <memory>

#include "imgui.h"
#include "rxdispatch.h"
#include "symmetri/colors.hpp"

void moveView(const ImVec2& d);

void moveNode(bool is_place, size_t idx, const ImVec2& d);

void addNode(bool is_place, ImVec2 pos);

void removeArc(bool is_input, size_t transition_idx, size_t sub_idx);

void addArc(bool is_place, size_t source, size_t target, symmetri::Token color);

void removePlace(size_t idx);

void removeTransition(size_t idx);

void showGrid(bool show_grid);

void updateArcColor(bool is_input, size_t idx, size_t sub_idx,
                    const symmetri::Token color);

ImGuiInputTextCallback updatePlaceName(const size_t id);
ImGuiInputTextCallback updateTransitionName(const size_t id);
ImGuiInputTextCallback updatePriority(const size_t id);

void setContextMenuActive();

void setContextMenuInactive();

void setSelectedNode(bool is_place, size_t idx);

void setSelectedTargetNode(bool is_place, size_t idx);

void resetSelectedTargetNode();

void resetSelection();

void resetNetView();

void setSelectedArc(bool is_input, size_t idx, size_t sub_idx);

void renderNodeEntry(bool is_place, const std::string& name, size_t idx,
                     bool selected);

int updateActiveFile(ImGuiInputTextCallbackData* data);

void updateColorTable();

void addTokenToPlace(symmetri::AugmentedToken);

void removeTokenFromPlace(symmetri::AugmentedToken);

void tryFire(size_t transition_idx);

void updateTransitionOutputColor(size_t transition_idx, symmetri::Token color);

auto addViewBlocking(auto&& v) {
  auto accumulate_promise = std::make_shared<std::promise<void>>();
  auto fut = accumulate_promise->get_future();
  rxdispatch::push([q = std::move(accumulate_promise), v](model::Model&& m) {
    m.data->drawables.push_back(v);
    m.data->blockers.emplace(v, std::move(*q));
    return m;
  });
  return fut;
};

void addView(auto&& v) {
  rxdispatch::push([=](model::Model&& m) {
    m.data->drawables.push_back(std::move(v));
    return m;
  });
};

void removeView(auto&& v) {
  rxdispatch::push([=](model::Model&& m) {
    std::erase(m.data->drawables, std::move(v));
    return m;
  });
};
