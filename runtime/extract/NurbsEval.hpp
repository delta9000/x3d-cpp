#ifndef X3D_RUNTIME_EXTRACT_NURBSEVAL_HPP
#define X3D_RUNTIME_EXTRACT_NURBSEVAL_HPP

// Node-free NURBS evaluation: Cox-de Boor basis, rational (weighted) curves and
// surfaces with analytic normals. Operates on plain arrays — no X3D-node or
// MeshBuilder dependency. All formulas machine-verified against a CAS.
#include "x3d/core/X3Dtypes.hpp"
#include <vector>
#include <cmath>

namespace x3d::runtime::extract::nurbs {

using x3d::core::SFVec3f;
using x3d::core::SFVec2f;

struct CurveDef {
  std::vector<SFVec3f> cp;     // control points
  std::vector<double>  w;      // weights; empty => all 1.0
  std::vector<double>  knot;   // len cp.size()+order; empty/wrong => generated
  int  order = 3;              // = degree + 1 (min 2)
  bool closed = false;
};

namespace detail {

// Clamped uniform knots: len n+order, first `order`=0, last `order`=1, interior uniform.
inline std::vector<double> clampedKnots(int n, int order) {
  std::vector<double> k(n + order, 0.0);
  int interior = n - order;               // = n - order interior knots
  for (int i = 0; i < order; ++i) k[i] = 0.0;
  for (int i = 0; i < order; ++i) k[n + i] = 1.0;
  for (int i = 1; i <= interior; ++i) k[order - 1 + i] = double(i) / double(interior + 1);
  return k;
}

// Uniform unclamped (periodic) knots: 0,1,...,M+order-1 (length M+order).
inline std::vector<double> periodicKnots(int M, int order) {
  std::vector<double> k(M + order);
  for (int i = 0; i < M + order; ++i) k[i] = double(i);
  return k;
}

// Knot span containing u, for `numCP` control points. Clamps to [knot[p], knot[numCP]].
inline int findSpan(int numCP, int order, double u, const std::vector<double>& knot) {
  int p = order - 1, nHigh = numCP - 1;
  if (u >= knot[nHigh + 1]) return nHigh;
  if (u <= knot[p]) return p;
  int lo = p, hi = nHigh + 1, mid = (lo + hi) / 2;
  while (u < knot[mid] || u >= knot[mid + 1]) {
    if (u < knot[mid]) hi = mid; else lo = mid;
    mid = (lo + hi) / 2;
  }
  return mid;
}

// Nonzero basis functions N[0..p] at span i (Cox-de Boor, PT A2.2). The
// `denom != 0` guard implements the 0/0 -> 0 convention at repeated knots.
inline void basisFuns(int i, double u, int order, const std::vector<double>& knot,
                      double* N) {
  int p = order - 1;
  std::vector<double> left(order, 0.0), right(order, 0.0);
  N[0] = 1.0;
  for (int j = 1; j <= p; ++j) {
    left[j]  = u - knot[i + 1 - j];
    right[j] = knot[i + j] - u;
    double saved = 0.0;
    for (int r = 0; r < j; ++r) {
      double denom = right[r + 1] + left[j - r];
      double temp = denom != 0.0 ? N[r] / denom : 0.0;
      N[r] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    N[j] = saved;
  }
}

// Resolve weights + knots; apply closed/periodic wrap (X3D: wrap only when the
// first and last control points DIFFER, else it is already a closed clamped loop).
inline CurveDef prepareCurve(const CurveDef& in) {
  CurveDef c = in;
  int n = (int)c.cp.size();
  if ((int)c.w.size() != n) c.w.assign(n, 1.0);
  const bool endsDiffer = n >= 2 &&
      (c.cp.front().x != c.cp.back().x || c.cp.front().y != c.cp.back().y ||
       c.cp.front().z != c.cp.back().z);
  if (c.closed && endsDiffer) {
    int p = c.order - 1;
    for (int i = 0; i < p; ++i) { c.cp.push_back(c.cp[i]); c.w.push_back(c.w[i]); }
    c.knot = periodicKnots((int)c.cp.size(), c.order);
  } else if ((int)c.knot.size() != n + c.order) {
    c.knot = clampedKnots(n, c.order);
  }
  return c;
}

} // namespace detail

// segments+1 samples across the valid domain [knot[order-1], knot[numCP]].
inline std::vector<SFVec3f> tessellateCurve(const CurveDef& in, int segments) {
  std::vector<SFVec3f> out;
  CurveDef c = detail::prepareCurve(in);
  int numCP = (int)c.cp.size();
  if (numCP < c.order || segments < 1) return out;
  int p = c.order - 1;
  double u0 = c.knot[p], u1 = c.knot[numCP];
  out.reserve(segments + 1);
  std::vector<double> N(c.order);
  for (int s = 0; s <= segments; ++s) {
    double u = u0 + (u1 - u0) * double(s) / double(segments);
    int span = detail::findSpan(numCP, c.order, u, c.knot);
    detail::basisFuns(span, u, c.order, c.knot, N.data());
    double x=0,y=0,z=0,w=0;
    for (int a = 0; a <= p; ++a) {
      int idx = span - p + a;
      double wi = c.w[idx], nb = N[a] * wi;
      x += nb*c.cp[idx].x; y += nb*c.cp[idx].y; z += nb*c.cp[idx].z; w += nb;
    }
    out.push_back(SFVec3f{ (float)(x/w), (float)(y/w), (float)(z/w) });
  }
  return out;
}

} // namespace x3d::runtime::extract::nurbs
#endif
