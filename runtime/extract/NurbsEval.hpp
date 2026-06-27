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

struct SurfaceDef {
  std::vector<SFVec3f> cp;     // uDim*vDim, u-index fastest: cp[i + j*uDim]
  std::vector<double>  w;
  std::vector<double>  uKnot, vKnot;
  int  uDim = 0, vDim = 0, uOrder = 3, vOrder = 3;
  bool uClosed = false, vClosed = false;
};
struct SurfaceSample { SFVec3f p; SFVec3f n; SFVec2f uv; };

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

// Basis funcs N[0..p] AND first derivatives dN[0..p] at span i (PT A2.3, d=1).
// Verified against symbolic D[BSplineBasis] to machine epsilon.
inline void dersBasisFuns1(int i, double u, int order,
                           const std::vector<double>& knot, double* N, double* dN) {
  int p = order - 1;
  std::vector<std::vector<double>> ndu(order, std::vector<double>(order, 0.0));
  std::vector<double> left(order, 0.0), right(order, 0.0);
  ndu[0][0] = 1.0;
  for (int j = 1; j <= p; ++j) {
    left[j]  = u - knot[i + 1 - j];
    right[j] = knot[i + j] - u;
    double saved = 0.0;
    for (int r = 0; r < j; ++r) {
      double denom = right[r + 1] + left[j - r];
      ndu[j][r] = denom;
      double temp = denom != 0.0 ? ndu[r][j - 1] / denom : 0.0;
      ndu[r][j] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    ndu[j][j] = saved;
  }
  for (int j = 0; j <= p; ++j) N[j] = ndu[j][p];
  for (int r = 0; r <= p; ++r) {
    double der = 0.0; int rk = r - 1, pk = p - 1;
    if (r >= 1 && ndu[pk + 1][rk] != 0.0) der += (1.0 / ndu[pk + 1][rk]) * ndu[rk][pk];
    if (r <= pk && ndu[pk + 1][r]  != 0.0) der += (-1.0 / ndu[pk + 1][r]) * ndu[r][pk];
    dN[r] = der * double(p);
  }
}

// Rational surface point + analytic normal at (u,v). Homogeneous accumulation
// then quotient rule: dP = (dS.xyz - dS.w * P) / S.w ; normal = dP/du x dP/dv.
inline SurfaceSample evalSurface(const std::vector<SFVec3f>& cp,
                                 const std::vector<double>& w,
                                 const std::vector<double>& uKnot,
                                 const std::vector<double>& vKnot,
                                 int uDim, int vDim, int uOrder, int vOrder,
                                 double u, double v) {
  int pu = uOrder - 1, pv = vOrder - 1;
  int su = findSpan(uDim, uOrder, u, uKnot);
  int sv = findSpan(vDim, vOrder, v, vKnot);
  std::vector<double> Nu(uOrder), dNu(uOrder), Nv(vOrder), dNv(vOrder);
  dersBasisFuns1(su, u, uOrder, uKnot, Nu.data(), dNu.data());
  dersBasisFuns1(sv, v, vOrder, vKnot, Nv.data(), dNv.data());
  double Sx=0,Sy=0,Sz=0,Sw=0, Ux=0,Uy=0,Uz=0,Uw=0, Vx=0,Vy=0,Vz=0,Vw=0;
  for (int b = 0; b <= pv; ++b) {
    int jv = sv - pv + b;
    for (int a = 0; a <= pu; ++a) {
      int iu = su - pu + a, idx = iu + jv * uDim;
      double wi = w.empty() ? 1.0 : w[idx];
      const SFVec3f& P = cp[idx];
      double hx=wi*P.x, hy=wi*P.y, hz=wi*P.z, hw=wi;
      double nN=Nu[a]*Nv[b], nU=dNu[a]*Nv[b], nV=Nu[a]*dNv[b];
      Sx+=nN*hx; Sy+=nN*hy; Sz+=nN*hz; Sw+=nN*hw;
      Ux+=nU*hx; Uy+=nU*hy; Uz+=nU*hz; Uw+=nU*hw;
      Vx+=nV*hx; Vy+=nV*hy; Vz+=nV*hz; Vw+=nV*hw;
    }
  }
  double inv = 1.0 / Sw;
  SFVec3f Pt{ (float)(Sx*inv), (float)(Sy*inv), (float)(Sz*inv) };
  double dUx=(Ux-Uw*Sx*inv)*inv, dUy=(Uy-Uw*Sy*inv)*inv, dUz=(Uz-Uw*Sz*inv)*inv;
  double dVx=(Vx-Vw*Sx*inv)*inv, dVy=(Vy-Vw*Sy*inv)*inv, dVz=(Vz-Vw*Sz*inv)*inv;
  SFVec3f nrm{ (float)(dUy*dVz-dUz*dVy),
               (float)(dUz*dVx-dUx*dVz),
               (float)(dUx*dVy-dUy*dVx) };
  double len = std::sqrt((double)nrm.x*nrm.x + (double)nrm.y*nrm.y + (double)nrm.z*nrm.z);
  if (len > 1e-20) { nrm.x/=(float)len; nrm.y/=(float)len; nrm.z/=(float)len; }
  return SurfaceSample{ Pt, nrm, SFVec2f{0,0} };
}

inline SurfaceDef prepareSurface(const SurfaceDef& in) {
  SurfaceDef s = in;
  if ((int)s.w.size() != s.uDim * s.vDim) s.w.assign(s.uDim * s.vDim, 1.0);

  // u-direction periodic wrap: append first (uOrder-1) columns of every row.
  if (s.uClosed && s.uDim >= 2) {
    int pu = s.uOrder - 1, nu = s.uDim + pu;
    std::vector<SFVec3f> cp2(nu * s.vDim);
    std::vector<double>  w2(nu * s.vDim);
    for (int j = 0; j < s.vDim; ++j)
      for (int i = 0; i < nu; ++i) {
        int src = (i < s.uDim ? i : i - s.uDim) + j * s.uDim;
        cp2[i + j * nu] = s.cp[src];
        w2[i + j * nu]  = s.w[src];
      }
    s.cp = std::move(cp2); s.w = std::move(w2); s.uDim = nu;
    s.uKnot = periodicKnots(s.uDim, s.uOrder);
  } else if ((int)s.uKnot.size() != s.uDim + s.uOrder) {
    s.uKnot = clampedKnots(s.uDim, s.uOrder);
  }

  // v-direction periodic wrap: append first (vOrder-1) rows (uDim may have grown).
  if (s.vClosed && s.vDim >= 2) {
    int pv = s.vOrder - 1, nv = s.vDim + pv;
    std::vector<SFVec3f> cp2(s.uDim * nv);
    std::vector<double>  w2(s.uDim * nv);
    for (int j = 0; j < nv; ++j) {
      int srcj = (j < s.vDim ? j : j - s.vDim);
      for (int i = 0; i < s.uDim; ++i) {
        cp2[i + j * s.uDim] = s.cp[i + srcj * s.uDim];
        w2[i + j * s.uDim]  = s.w[i + srcj * s.uDim];
      }
    }
    s.cp = std::move(cp2); s.w = std::move(w2); s.vDim = nv;
    s.vKnot = periodicKnots(s.vDim, s.vOrder);
  } else if ((int)s.vKnot.size() != s.vDim + s.vOrder) {
    s.vKnot = clampedKnots(s.vDim, s.vOrder);
  }
  return s;
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

inline std::vector<SurfaceSample> tessellateSurface(const SurfaceDef& in,
                                                    int uSeg, int vSeg) {
  std::vector<SurfaceSample> out;
  SurfaceDef s = detail::prepareSurface(in);
  if (s.uDim < s.uOrder || s.vDim < s.vOrder || uSeg < 1 || vSeg < 1) return out;
  if ((int)s.cp.size() < s.uDim * s.vDim) return out;
  int pu = s.uOrder - 1, pv = s.vOrder - 1;
  double u0 = s.uKnot[pu], u1 = s.uKnot[s.uDim];
  double v0 = s.vKnot[pv], v1 = s.vKnot[s.vDim];
  out.reserve((uSeg + 1) * (vSeg + 1));
  for (int b = 0; b <= vSeg; ++b) {
    double v = v0 + (v1 - v0) * double(b) / double(vSeg);
    for (int a = 0; a <= uSeg; ++a) {
      double u = u0 + (u1 - u0) * double(a) / double(uSeg);
      auto sm = detail::evalSurface(s.cp, s.w, s.uKnot, s.vKnot,
                                    s.uDim, s.vDim, s.uOrder, s.vOrder, u, v);
      sm.uv = SFVec2f{ (float)(double(a)/uSeg), (float)(double(b)/vSeg) };
      out.push_back(sm);
    }
  }
  return out;
}

// X3D tessellation field -> segment count. >0: that many; <0: |t|*numCP; 0: 2*numCP.
inline int tessellationToSegments(int tess, int numCP) {
  int segs = tess > 0 ? tess : (tess < 0 ? (-tess) * numCP : 2 * numCP);
  return segs < 1 ? 1 : segs;
}

} // namespace x3d::runtime::extract::nurbs
#endif
