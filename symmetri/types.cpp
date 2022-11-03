#include "symmetri/types.h"

namespace symmetri {

bool MarkingReached(const NetMarking& marking,
                    const NetMarking& final_marking) {
  return std::all_of(
      std::begin(final_marking), std::end(final_marking),
      [&](const auto& p_m) { return marking.at(p_m.first) >= p_m.second; });
}

bool StateNetEquality(const StateNet& net1, const StateNet& net2) {
  if (net1.size() != net2.size()) {
    return false;
  }
  for (const auto& [t1, mut1] : net1) {
    if (net2.contains(t1)) {
      for (const auto& pre : mut1.first) {
        if (mut1.first.size() != net2.at(t1).first.size()) {
          return false;
        }
        if (std::count(std::begin(mut1.first), std::end(mut1.first), pre) !=
            std::count(std::begin(net2.at(t1).first),
                       std::end(net2.at(t1).first), pre)) {
          return false;
        }
      }
      for (const auto& post : mut1.second) {
        if (mut1.second.size() != net2.at(t1).second.size()) {
          return false;
        }
        if (std::count(std::begin(mut1.second), std::end(mut1.second), post) !=
            std::count(std::begin(net2.at(t1).second),
                       std::end(net2.at(t1).second), post)) {
          return false;
        }
      }
    } else {
      return false;
    }
  }

  return true;
}
}  // namespace symmetri
