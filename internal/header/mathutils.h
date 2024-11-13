#ifndef MATHUTILS_H
#define MATHUTILS_H

namespace MathUtils
{
  static long long factorial(int n) {
    if (n < 0 ) {
        return 0;
    }
    return !n ? 1 : n * factorial(n - 1);
  }

  float log2_combination(int n, int k);
}

#endif