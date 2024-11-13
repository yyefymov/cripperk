#include "../header/mathutils.h"
#include <limits>
#include <cmath>

float MathUtils::log2_combination(int n, int k) {
  if (k > n || k < 0) return -std::numeric_limits<float>::infinity();
  return (lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)) / std::log(2);
}