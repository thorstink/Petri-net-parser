#include <map>
#include <stdexcept>
#include <string>

#include "symmetri/colors.hpp"
#include "symmetri/parsers.h"
#include "tinyxml2/tinyxml2.h"
using namespace tinyxml2;

namespace symmetri {

std::tuple<Net, Marking, PriorityTable> readGrml(
    const std::set<std::string> &files) {
  std::set<std::string> places, transitions;
  PriorityTable priorities;
  std::map<int, std::string> id_lookup_table;

  Marking place_initialMarking;
  Net state_net;

  for (auto file : files) {
    XMLDocument net;
    net.LoadFile(file.c_str());

    XMLElement *levelElement = net.FirstChildElement("model");
    for (XMLElement *child = levelElement->FirstChildElement("node");
         child != NULL; child = child->NextSiblingElement("node")) {
      const auto type = std::string(child->Attribute("nodeType"));
      const int id = std::stoi(child->Attribute("id"));
      if (type == "place") {
        // loop places & initial values
        std::string place_id;
        uint16_t initial_marking = 0;
        for (XMLElement *attribute = child->FirstChildElement("attribute");
             attribute != NULL;
             attribute = attribute->NextSiblingElement("attribute")) {
          const std::string child_attribute =
              std::string(attribute->Attribute("name"));
          if (child_attribute == "name") {
            place_id = std::string(attribute->GetText());
          } else if (child_attribute == "marking") {
            initial_marking =
                std::stoi(attribute->FirstChildElement("attribute")
                              ->FirstChildElement("attribute")
                              ->GetText());
          }
        }
        for (int i = 0; i < initial_marking; i++) {
          place_initialMarking.push_back({place_id, Success});
        }
        places.insert(place_id);
        id_lookup_table.insert({id, place_id});
      } else if (type == "transition") {
        // loop transitions
        std::string transition_id;
        int8_t priority = 0;
        for (XMLElement *attribute = child->FirstChildElement("attribute");
             attribute != NULL;
             attribute = attribute->NextSiblingElement("attribute")) {
          const std::string child_attribute =
              std::string(attribute->Attribute("name"));
          if (child_attribute == "name") {
            transition_id = std::string(attribute->GetText());
          } else if (child_attribute == "priority") {
            priority = std::stoi(attribute->FirstChildElement("attribute")
                                     ->FirstChildElement("attribute")
                                     ->GetText());
          }
        }
        transitions.insert(transition_id);
        id_lookup_table.insert({id, transition_id});
        if (priority != 0) {
          priorities.push_back({transition_id, priority});
        }
      }
    }
    for (XMLElement *child = levelElement->FirstChildElement("arc");
         child != NULL; child = child->NextSiblingElement("arc")) {
      const auto source_id =
          id_lookup_table[std::stoi(child->Attribute("source"))];
      const auto target_id =
          id_lookup_table[std::stoi(child->Attribute("target"))];
      const auto color = child->Attribute("color");

      const auto multiplicity = std::stoi(child->FirstChildElement("attribute")
                                              ->FirstChildElement("attribute")
                                              ->FirstChildElement("attribute")
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
          throw std::runtime_error(std::string(
              "error: arc is not connecting a place to a transition."));
        }
      }
    }
  }

  return {state_net, place_initialMarking, priorities};
}

}  // namespace symmetri
