// code in this file by Karl Yerkes

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using std::vector;

// https://en.wikipedia.org/wiki/Trilinear_interpolation
template <class F>
inline F trilinear(F xd, F yd, F zd,               //
                   F c000, F c001, F c010, F c011, //
                   F c100, F c101, F c110, F c111) {
  F c00 = c000 * (1 - xd) + c100 * xd;
  F c01 = c001 * (1 - xd) + c101 * xd;
  F c10 = c010 * (1 - xd) + c110 * xd;
  F c11 = c011 * (1 - xd) + c111 * xd;
  F c0 = c00 * (1 - yd) + c10 * yd;
  F c1 = c01 * (1 - yd) + c11 * yd;
  return c0 * (1 - zd) + c1 * zd;
}

// index into a dense cube of data
inline int index(int N, int x, int y, int z) { return x + y * N + z * N * N; }

template <class F> F interpolate(const F *data, size_t N, F x, F y, F z) {
  F x0, y0, z0;
  F xd = modf(x, &x0);
  F yd = modf(y, &y0);
  F zd = modf(z, &z0);
  int xi = static_cast<int>(x0) % N;
  int yi = static_cast<int>(y0) % N;
  int zi = static_cast<int>(z0) % N;
  return trilinear(xd, yd, zd, //
                   data[index(N, xi + 0, yi + 0, zi + 0)],
                   data[index(N, xi + 0, yi + 0, zi + 1)],
                   data[index(N, xi + 0, yi + 1, zi + 0)],
                   data[index(N, xi + 0, yi + 1, zi + 1)],
                   data[index(N, xi + 1, yi + 0, zi + 0)],
                   data[index(N, xi + 1, yi + 0, zi + 1)],
                   data[index(N, xi + 1, yi + 1, zi + 0)],
                   data[index(N, xi + 1, yi + 1, zi + 1)]);
}

// const int N = 100;
// int main() {
//   vector<float> field;
//   field.resize(N * N * N);
//   for (int i = 0; i < N * N * N; i++)  //
//     field[i] = 1.0f * i / (N * N * N);
//   float f = interpolate(field.data(), N, 0.1f, 3.3f, 9.5f);
//   printf("%f\n", f);
// }
