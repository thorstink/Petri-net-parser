#include "position_parsers.h"

#include "tinyxml2/tinyxml2.h"
using namespace tinyxml2;

namespace farbart {

std::map<std::string, model::Coordinate> readPnmlPositions(
    const std::set<std::string> &files) {
  std::map<std::string, model::Coordinate> positions;

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
      auto position =
          child->FirstChildElement("graphics")->FirstChildElement("position");
      positions[place_id] = {std::stof(position->Attribute("x")),
                             std::stof(position->Attribute("y"))};
    }

    // loop transitions
    for (XMLElement *child = levelElement->FirstChildElement("transition");
         child != NULL; child = child->NextSiblingElement("transition")) {
      auto transition_id = child->Attribute("id");
      auto position =
          child->FirstChildElement("graphics")->FirstChildElement("position");
      positions[transition_id] = {std::stof(position->Attribute("x")),
                                  std::stof(position->Attribute("y"))};
    }
  }

  return positions;
}
}  // namespace farbart
