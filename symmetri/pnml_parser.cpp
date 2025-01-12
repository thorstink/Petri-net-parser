#include <map>
#include <stdexcept>
#include <string>

#include "symmetri/colors.hpp"
#include "symmetri/parsers.h"
#include "tinyxml2/tinyxml2.h"
using namespace tinyxml2;

namespace symmetri {

std::tuple<Net, Marking> readPnml(const std::set<std::string> &files) {
  std::set<std::string> places, transitions;
  Marking place_initialMarking;
  Net state_net;

  for (auto file : files) {
    XMLDocument net;
    net.LoadFile(file.c_str());

    XMLElement *levelElement = net.FirstChildElement("pnml")
                                   ->FirstChildElement("net")
                                   ->FirstChildElement("page");

    // loop places.
    for (XMLElement *child = levelElement->FirstChildElement("place");
         child != NULL; child = child->NextSiblingElement("place")) {
      auto place_id = child->Attribute("id");
      auto initial_marking =
          (child->FirstChildElement("initialMarking") == nullptr)
              ? 0
              : std::stoi(child->FirstChildElement("initialMarking")
                              ->FirstChildElement("text")
                              ->GetText());
      for (int i = 0; i < initial_marking; i++) {
        place_initialMarking.push_back({place_id, Success});
      }
      places.insert(std::string(place_id));
    }

    // loop transitions
    for (XMLElement *child = levelElement->FirstChildElement("transition");
         child != NULL; child = child->NextSiblingElement("transition")) {
      auto transition_id = child->Attribute("id");
      transitions.insert(transition_id);
    }

    // loop arcs
    for (XMLElement *child = levelElement->FirstChildElement("arc");
         child != NULL; child = child->NextSiblingElement("arc")) {
      // do something with each child element

      const auto source_id = child->Attribute("source");
      const auto target_id = child->Attribute("target");
      const auto color = child->Attribute("color");
      const auto multiplicity =
          (child->FirstChildElement("inscription") == nullptr)
              ? 1
              : std::stoi(child->FirstChildElement("inscription")
                              ->FirstChildElement("text")
                              ->GetText());

      for (int i = 0; i < multiplicity; i++) {
        const auto arc_color = NULL == color ? Success : Token(color);
        if (places.find(source_id) != places.end()) {
          // if the source is a place, tokens are consumed.
          if (state_net.find(target_id) != state_net.end()) {
            state_net.find(target_id)->second.first.push_back(
                {source_id, arc_color});
          } else {
            state_net.insert({target_id, {{{source_id, arc_color}}, {}}});
          }
        } else if (transitions.find(source_id) != transitions.end()) {
          // if the destination is a place, tokens are produced.
          if (state_net.find(source_id) != state_net.end()) {
            state_net.find(source_id)->second.second.push_back(
                {target_id, arc_color});
          } else {
            state_net.insert({source_id, {{}, {{target_id, arc_color}}}});
          }
        } else {
          const auto arc_id = child->Attribute("id");
          throw std::runtime_error(
              std::string("error: arc ") + arc_id +
              std::string("is not connecting a place to a transition."));
        }
      }
    }
  }

  return {state_net, place_initialMarking};
}
}  // namespace symmetri
