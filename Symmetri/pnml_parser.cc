#include "pnml_parser.h"
#include "tinyxml2/tinyxml2.h"
#include <iostream>
using namespace tinyxml2;

// Function to print the
// index of an element
int getIndex(std::vector<std::string> v, const std::string &K) {
  auto it = find(v.begin(), v.end(), K);
  // If element was found
  if (it != v.end()) {
    // calculating the index
    // of K
    int index = it - v.begin();
    return index;
  } else {
    // If the element is not
    // present in the vector
    return -1;
  }
}

bool contains(std::vector<std::string> v, const std::string &K) {
  auto it = find(v.begin(), v.end(), K);
  // If element was found
  return it != v.end();
}

std::tuple<types::TransitionMutation, types::TransitionMutation,
           types::Marking>
constructTransitionMutationMatrices(std::string file) {
  XMLDocument net;
  net.LoadFile(file.c_str());

  std::vector<std::string> places, transitions;
  std::unordered_map<std::string, int> place_initialMarking;

  tinyxml2::XMLElement *levelElement =
      net.FirstChildElement("pnml")->FirstChildElement("net");

  for (tinyxml2::XMLElement *child = levelElement->FirstChildElement("place");
       child != NULL; child = child->NextSiblingElement("place")) {
    // do something with each child element
    auto place_name =
        child->FirstChildElement("name")->FirstChildElement("value")->GetText();

    auto place_id = child->Attribute("id");

    auto initial_marking =
        (child->FirstChildElement("initialMarking") == nullptr)
            ? 0
            : std::stoi(child->FirstChildElement("initialMarking")
                            ->FirstChildElement("value")
                            ->GetText());

    place_initialMarking.insert({place_id, initial_marking});

    std::cout << "place: " << place_name << ", " << place_id << ", "
              << initial_marking << std::endl;

    places.push_back(std::string(place_id));
  }

  for (tinyxml2::XMLElement *child =
           levelElement->FirstChildElement("transition");
       child != NULL; child = child->NextSiblingElement("transition")) {
    // do something with each child element
    auto transition_name =
        child->FirstChildElement("name")->FirstChildElement("value")->GetText();

    auto transition_id = child->Attribute("id");

    std::cout << "transition: " << transition_name << ", " << transition_id
              << std::endl;

    transitions.push_back(transition_id);
  }

  Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> Dp(places.size(),
                                                        transitions.size()),
      Dm(places.size(), transitions.size());

  Dp.setZero();
  Dm.setZero();

  for (tinyxml2::XMLElement *child = levelElement->FirstChildElement("arc");
       child != NULL; child = child->NextSiblingElement("arc")) {
    // do something with each child element

    auto arc_id = child->Attribute("id");
    auto source_id = child->Attribute("source");
    auto target_id = child->Attribute("target");

    if (contains(places, source_id) && contains(transitions, target_id)) {
      auto s_idx = getIndex(places, source_id);
      auto t_idx = getIndex(transitions, target_id);
      // if the source is a place, tokens are consumed.
      std::cout << "place-source: " << arc_id << ", " << source_id << ", "
                << target_id << std::endl;
      Dm(s_idx, t_idx) += 1;
    } else if (contains(places, target_id) &&
               contains(transitions, source_id)) {
      auto s_idx = getIndex(transitions, source_id);
      auto t_idx = getIndex(places, target_id);
      std::cout << "trans-source: " << arc_id << ", " << source_id << ", "
                << target_id << std::endl;
      // if the destination is a place, tokens are produced.
      Dp(t_idx, s_idx) += 1;
    } else {
      std::cout << "error: arc " << arc_id
                << " is not connecting a place to a transition." << std::endl;
    }
  }

  std::cout << Dp - Dm << std::endl;

  int transition_count = transitions.size();
  std::unordered_map<std::string, Eigen::VectorXi> pre_map, post_map;
  Eigen::VectorXi T(transition_count);
  for (const auto &transition_id : transitions) {
    T.setZero();
    T(getIndex(transitions, transition_id)) = 1;
    pre_map.insert({transition_id, (Dm * T).eval()});
    post_map.insert({transition_id, (Dp * T).eval()});
  }

  std::vector<int> initial_marking;
  std::transform(places.begin(), places.end(),
                 std::back_inserter(initial_marking),
                 [&place_initialMarking](const std::string &s) {
                   return place_initialMarking.at(s);
                 });

  Eigen::VectorXi M0 = Eigen::Map<const Eigen::VectorXi>(
      initial_marking.data(), initial_marking.size());

  for (auto [i, dM] : pre_map) {
    std::cout << "deduct " << i << ": " << dM.transpose() << std::endl;
  }

  for (auto [i, dM] : post_map) {
    std::cout << "add " << i << ": " << dM.transpose() << std::endl;
  }

  std::cout << "initial marking: " << M0.transpose() << std::endl;
  return {pre_map, post_map, M0};
}
