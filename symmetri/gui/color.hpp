#pragma once
#include <array>
#include <cmath>

// https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
inline double ratio() {
  const static double phi = (std::sqrt(5) + 1.0) / 2.0;
  static double n = 0;
  return std::fmod(++n * phi, 1.0);
}

// https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
inline std::array<int, 3> hsv_to_rgb(float h, float s, float v) {
  float H = h * 360.f;
  float C = s * v;
  float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
  float m = v - C;
  float r, g, b;
  if (H >= 0 && H < 60) {
    r = C, g = X, b = 0;
  } else if (H >= 60 && H < 120) {
    r = X, g = C, b = 0;
  } else if (H >= 120 && H < 180) {
    r = 0, g = C, b = X;
  } else if (H >= 180 && H < 240) {
    r = 0, g = X, b = C;
  } else if (H >= 240 && H < 300) {
    r = X, g = 0, b = C;
  } else {
    r = C, g = 0, b = X;
  }
  int R = (r + m) * 255;
  int G = (g + m) * 255;
  int B = (b + m) * 255;
  return {R, G, B};
}
