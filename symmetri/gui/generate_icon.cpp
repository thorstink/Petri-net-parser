#include <cstdlib>
#include <sstream>

#include "color.hpp"
#include "draw_marking.hpp"
#include "tinyxml2/tinyxml2.h"

auto tohex(const auto rgb) {
  std::stringstream ss;
  ss << "rgb(" << rgb[0] << "," << rgb[1] << "," << rgb[2] << ")";
  return ss.str();
};

int main(int argc, char** argv) {
  if (argc != 3) {
    return 1;
  }
  const int count = atoi(argv[1]);
  const std::string file(argv[2]);
  const auto coords = getTokenCoordinates(count);

  using namespace tinyxml2;
  XMLDocument doc;
  XMLElement* svg = doc.NewElement("svg");
  svg->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
  svg->SetAttribute("width", "400");
  svg->SetAttribute("height", "400");
  svg->SetAttribute("viewBox", "-12 -12 24 24");
  svg->SetAttribute("fill", "none");
  doc.InsertFirstChild(svg);

  XMLElement* container_b = doc.NewElement("circle");
  container_b->SetAttribute("cx", 0);
  container_b->SetAttribute("cy", 0);
  container_b->SetAttribute("r", 12);
  container_b->SetAttribute("fill", "black");
  svg->InsertFirstChild(container_b);

  XMLElement* container_a = doc.NewElement("circle");
  container_a->SetAttribute("cx", 0);
  container_a->SetAttribute("cy", 0);
  container_a->SetAttribute("r", 11.8);
  container_a->SetAttribute("fill", "white");
  svg->InsertAfterChild(container_b, container_a);

  auto previous = container_a;
  for (auto token : coords) {
    XMLElement* outers = doc.NewElement("circle");
    outers->SetAttribute("cx", token.x);
    outers->SetAttribute("cy", token.y);
    outers->SetAttribute("r", 2.2);
    outers->SetAttribute("fill", tohex(hsv_to_rgb(ratio(), 1., 1.)).c_str());
    svg->InsertAfterChild(previous, outers);
    previous = outers;
  }

  doc.SaveFile(file.c_str());

  return 0;
}
