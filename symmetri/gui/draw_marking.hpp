#pragma once

#include <vector>

#include "imgui.h"
std::vector<ImVec2> getTokenCoordinates(size_t count) {
  switch (count) {
    case 1:
      return {{ImVec2(0, 0)}};
    case 2: {
      double a = 4;
      return {{ImVec2(-a, 0), ImVec2(a, 0)}};
    }
    case 3: {
      double a = 4;
      double b = std::sqrt(1.5) * a;
      return {{ImVec2(0, -0.5 * b), ImVec2(-a, 0.5 * b), ImVec2(a, 0.5 * b)}};
    }
    case 4: {
      double a = 4;
      return {{ImVec2(-a, a), ImVec2(a, a), ImVec2(-a, -a), ImVec2(a, -a)}};
    }
    case 5: {
      // https://mathworld.wolfram.com/RegularPentagon.html
      static double s = 6;
      static double c1 = s * 0.25 * (std::sqrt(5) - 1);
      static double c2 = s * 0.25 * (std::sqrt(5) + 1);
      static double s1 = s * 0.25 * std::sqrt((10 + 2 * std::sqrt(5)));
      static double s2 = s * 0.25 * std::sqrt((10 - 2 * std::sqrt(5)));
      return {{ImVec2(0, -s), ImVec2(s1, -c1), ImVec2(s2, c2), ImVec2(-s2, c2),
               ImVec2(-s1, -c1)}};
    }
    case 6: {
      static double s = 7;
      static double c1 = s * 0.4 * (std::sqrt(5) - 1);
      static double s1 = s * 0.20 * std::sqrt((10 + 2 * std::sqrt(5)));
      return {{ImVec2(0, s), ImVec2(0, -s), ImVec2(s1, c1), ImVec2(-s1, c1),
               ImVec2(-s1, -c1), ImVec2(s1, -c1)}};
    }
    case 7: {
      static double s = 7;
      static double c1 = s * 0.4 * (std::sqrt(5) - 1);
      static double s1 = s * 0.20 * std::sqrt((10 + 2 * std::sqrt(5)));
      return {{ImVec2(0, 0), ImVec2(0, s), ImVec2(0, -s), ImVec2(s1, c1),
               ImVec2(-s1, c1), ImVec2(-s1, -c1), ImVec2(s1, -c1)}};
    }
    case 8: {
      static double c1 = 8;
      static double c2 = 0.5 * c1 * std::sqrt(2.0);
      return {{ImVec2(c1, 0), ImVec2(c2, c2), ImVec2(0, c1), ImVec2(-c2, c2),
               ImVec2(-c1, 0), ImVec2(-c2, -c2), ImVec2(0, -c1),
               ImVec2(c2, -c2)}};
    }
    case 9: {
      static double c1 = 9;
      static double c2 = 0.5 * c1;
      return {{ImVec2(0, 0), ImVec2(0, c1), ImVec2(0, -c1), ImVec2(-c1, 0),
               ImVec2(c1, 0), ImVec2(c2, c2), ImVec2(c2, -c2), ImVec2(-c2, -c2),
               ImVec2(-c2, c2)}};
    }
    default:
      return {{}};
  }
}
