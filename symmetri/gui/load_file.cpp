#include "load_file.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <ranges>
#include <vector>

#include "petri.h"
#include "position_parsers.h"
#include "reducers.h"
#include "rxdispatch.h"
#include "symmetri/parsers.h"
#include "symmetri/symmetri.h"

void loadPetriNet(const std::filesystem::path& file) {
  rxdispatch::push([=](model::Model&& model) {
    auto& m = *model.data;
    m.selected_arc_idxs.reset();
    m.selected_node_idx.reset();
    m.active_file = file;
    if (not m.active_file.has_value()) {
      return model;
    }
    symmetri::Net net;
    symmetri::Marking marking;
    symmetri::PriorityTable pt;
    std::map<std::string, model::Coordinate> positions;
    const std::filesystem::path pn_file = m.active_file.value();
    if (pn_file.extension() == std::string(".pnml")) {
      std::tie(net, marking) = symmetri::readPnml({pn_file});
      positions = farbart::readPnmlPositions({pn_file});
    } else {
      std::tie(net, marking, pt) = symmetri::readGrml({pn_file});
    }

    symmetri::Petri::PTNet new_net;
    std::tie(new_net.transition, new_net.place, new_net.store) =
        symmetri::convert(net);

    std::tie(new_net.input_n, new_net.output_n) =
        symmetri::populateIoLookups(net, new_net.place);

    new_net.priority = symmetri::createPriorityLookup(new_net.transition, pt);

    m.t_positions.reserve(m.t_positions.size() + new_net.transition.size());
    for (const auto& transition : new_net.transition) {
      m.t_positions.push_back(positions.at(transition));
    }

    m.p_positions.reserve(m.p_positions.size() + new_net.place.size());
    for (const auto& place : new_net.place) {
      m.p_positions.push_back(positions.at(place));
    }

    const auto old_place_count = m.net.place.size();
    const auto old_transition_count = m.net.transition.size();
    const auto new_place_count = new_net.place.size();
    const auto new_transition_count = new_net.transition.size();

    // update lookups:
    for (auto& inputs : new_net.input_n) {
      for (auto& [idx, color] : inputs) {
        idx += old_place_count;
      }
    }
    for (auto& outputs : new_net.output_n) {
      for (auto& [idx, color] : outputs) {
        idx += old_place_count;
      }
    }

    m.colors = symmetri::Token::getColors();

    // For GCC, we copy the priority table because of some weird GCC13/14 bug...
    // should eventually just become: append(std::move(new_net.priority),
    // m.net.priority); ...
    // m.net.priority.insert(m.net.priority.end(), new_net.priority.begin(),
    //                       new_net.priority.end());
    std::ranges::move(new_net.priority, std::back_inserter(m.net.priority));
    std::ranges::move(new_net.transition, std::back_inserter(m.net.transition));
    std::ranges::move(new_net.place, std::back_inserter(m.net.place));
    std::ranges::move(new_net.output_n, std::back_inserter(m.net.output_n));
    std::ranges::move(new_net.input_n, std::back_inserter(m.net.input_n));
    std::ranges::move(new_net.store, std::back_inserter(m.net.store));

    m.net.p_to_ts_n = createReversePlaceToTransitionLookup(
        m.net.place.size(), m.net.transition.size(), m.net.input_n);

    m.tokens.clear();
    for (const auto& [p, c] : marking) {
      m.tokens.push_back(
          {symmetri::toIndex(new_net.place, p) + old_place_count, c});
    }

    m.t_view.resize(new_transition_count);
    m.p_view.resize(new_place_count);

    std::iota(m.t_view.begin(), m.t_view.end(), old_transition_count);
    std::iota(m.p_view.begin(), m.p_view.end(), old_place_count);

    m.active_file = file;

    resetNetView();  // convenient to view new graph

    return model;
  });
}
