#include "symmetri/types.h"

#include <algorithm>
#include <numeric>
namespace symmetri {

bool stateNetEquality(const Net& net1, const Net& net2) {
  if (net1.size() != net2.size()) {
    return false;
  }
  for (const auto& [t1, mut1] : net1) {
    if (net2.find(t1) != net2.end()) {
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

size_t calculateTrace(const Eventlog& event_log) noexcept {
  // calculate a trace-id, in a simple way.
  return std::hash<std::string>{}(std::accumulate(
      event_log.begin(), event_log.end(), std::string(""),
      [](const auto& acc, const Event& n) {
        constexpr auto success = "o";
        constexpr auto fail = "x";
        return n.state == State::Completed ? acc + n.transition + success
                                           : acc + fail;
      }));
}

std::string printState(symmetri::State s) noexcept {
  std::string ret;
  switch (s) {
    case State::Scheduled:
      ret = "Scheduled";
      break;
    case State::Started:
      ret = "Started";
      break;
    case State::Completed:
      ret = "Completed";
      break;
    case State::Deadlock:
      ret = "Deadlock";
      break;
    case State::UserExit:
      ret = "UserExit";
      break;
    case State::Paused:
      ret = "Paused";
      break;
    case State::Error:
      ret = "Error";
      break;
  }

  return ret;
}

}  // namespace symmetri
