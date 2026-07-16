#include "MeshBuilder.hpp"

#include "GeometryBounds.hpp"
#include "NurbsEval.hpp"
#include "TextExtract.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace x3d::runtime::extract::mesh_detail {

SFColorRGBA promote(const SFColor &c) {
  return SFColorRGBA{c.r, c.g, c.b, 1.0f};
}

SFVec3f faceNormal(const SFVec3f &a, const SFVec3f &b, const SFVec3f &c) {
  const SFVec3f u{b.x - a.x, b.y - a.y, b.z - a.z};
  const SFVec3f v{c.x - a.x, c.y - a.y, c.z - a.z};
  SFVec3f n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
            u.x * v.y - u.y * v.x};
  const float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  if (len <= 0.0f)
    return SFVec3f{0.0f, 0.0f, 0.0f};
  return SFVec3f{n.x / len, n.y / len, n.z / len};
}

std::vector<std::array<int, 3>>
earClipPolygon(const std::vector<SFVec3f> &poly) {
  std::vector<std::array<int, 3>> tris;
  const int n = static_cast<int>(poly.size());
  if (n < 3)
    return tris;

  // Newell normal of the ring (robust for non-planar / concave polygons).
  SFVec3f nrm{0, 0, 0};
  for (int i = 0; i < n; ++i) {
    const SFVec3f &a = poly[i];
    const SFVec3f &b = poly[(i + 1) % n];
    nrm.x += (a.y - b.y) * (a.z + b.z);
    nrm.y += (a.z - b.z) * (a.x + b.x);
    nrm.z += (a.x - b.x) * (a.y + b.y);
  }
  const float nl = std::sqrt(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);
  if (nl <= 0.0f)
    return tris; // fully degenerate ring.
  nrm = SFVec3f{nrm.x / nl, nrm.y / nl, nrm.z / nl};

  // Build an in-plane orthonormal basis (u,v) so v = nrm x u; a (u,v)
  // coordinate pair preserves the polygon's CCW orientation about nrm.
  SFVec3f ref = (std::fabs(nrm.x) < 0.9f) ? SFVec3f{1, 0, 0} : SFVec3f{0, 1, 0};
  SFVec3f u{ref.y * nrm.z - ref.z * nrm.y, ref.z * nrm.x - ref.x * nrm.z,
            ref.x * nrm.y - ref.y * nrm.x};
  const float ul = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
  u = SFVec3f{u.x / ul, u.y / ul, u.z / ul};
  SFVec3f v{nrm.y * u.z - nrm.z * u.y, nrm.z * u.x - nrm.x * u.z,
            nrm.x * u.y - nrm.y * u.x};

  struct P2 {
    float x, y;
  };
  std::vector<P2> pp(n);
  for (int i = 0; i < n; ++i)
    pp[i] = P2{poly[i].x * u.x + poly[i].y * u.y + poly[i].z * u.z,
               poly[i].x * v.x + poly[i].y * v.y + poly[i].z * v.z};

  auto crossZ = [](const P2 &a, const P2 &b, const P2 &c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  };
  // Point-in-triangle (inclusive) via consistent signs.
  auto inTri = [&](const P2 &p, const P2 &a, const P2 &b, const P2 &c) {
    const float d1 = crossZ(a, b, p), d2 = crossZ(b, c, p),
                d3 = crossZ(c, a, p);
    const bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    const bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(neg && pos);
  };

  std::vector<int> rem(n);
  for (int i = 0; i < n; ++i)
    rem[i] = i;

  int guard = 0;
  while (static_cast<int>(rem.size()) > 3 && guard++ < 4 * n) {
    bool clipped = false;
    const int m = static_cast<int>(rem.size());
    for (int i = 0; i < m; ++i) {
      const int ia = rem[(i + m - 1) % m], ib = rem[i], ic = rem[(i + 1) % m];
      const P2 &a = pp[ia], &b = pp[ib], &c = pp[ic];
      // Convex corner w.r.t. CCW orientation (cross >= 0). Skip reflex.
      if (crossZ(a, b, c) <= 0.0f)
        continue;
      // No other remaining vertex may lie inside the candidate ear.
      bool ear = true;
      for (int j = 0; j < m; ++j) {
        const int vj = rem[j];
        if (vj == ia || vj == ib || vj == ic)
          continue;
        if (inTri(pp[vj], a, b, c)) {
          ear = false;
          break;
        }
      }
      if (!ear)
        continue;
      tris.push_back({ia, ib, ic});
      rem.erase(rem.begin() + i);
      clipped = true;
      break;
    }
    if (!clipped)
      break; // numerically stuck — bail with what we have.
  }
  if (rem.size() == 3)
    tris.push_back({rem[0], rem[1], rem[2]});
  return tris;
}

void creaseSmoothNormals(MeshData &m,
                         const std::vector<std::uint32_t> &sourceId,
                         float creaseAngle) {
  const std::size_t n = m.normals.size();
  if (n == 0 || sourceId.size() != n || n % 3 != 0)
    return;
  const std::size_t triCount = n / 3;

  // Per-triangle flat face normal == the (shared) normal of its first corner.
  // Capture BEFORE we mutate, so accumulation reads the pristine flat normals.
  std::vector<SFVec3f> faceN(triCount);
  for (std::size_t t = 0; t < triCount; ++t)
    faceN[t] = m.normals[t * 3];

  // Bucket triangles by source vertex id (one entry per incident corner; the
  // triangle ordinal is corner/3). Keyed on the source id so distinct-but-
  // coincident verts never cross-pollinate.
  std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> incident;
  incident.reserve(triCount * 2);
  for (std::size_t c = 0; c < n; ++c)
    incident[sourceId[c]].push_back(static_cast<std::uint32_t>(c / 3));

  // cos(creaseAngle): clamp the angle to [0, PI]. A NEGATIVE creaseAngle (or 0)
  // gives cosTheta >= 1 so only exactly-equal face normals fuse (faceted).
  const float pi = 3.14159265358979323846f;
  float ca = creaseAngle;
  if (ca < 0.0f)
    ca = 0.0f;
  if (ca > pi)
    ca = pi;
  const float cosTheta = std::cos(ca);

  std::vector<SFVec3f> out(n);
  for (std::size_t c = 0; c < n; ++c) {
    const std::size_t tri = c / 3;
    const SFVec3f &fn = faceN[tri];
    SFVec3f acc{0.0f, 0.0f, 0.0f};
    const auto &tris = incident[sourceId[c]];
    for (std::uint32_t ot : tris) {
      const SFVec3f &on = faceN[ot];
      // Dihedral test against THIS corner's face. A >= cosTheta dot means the
      // angle between the two face normals is <= creaseAngle. ot == tri always
      // passes (dot 1 >= cosTheta for any clamped angle) so the corner's own
      // face is always counted. Degenerate (zero) normals dot to 0 and are
      // naturally excluded unless creaseAngle >= PI/2.
      const float d = fn.x * on.x + fn.y * on.y + fn.z * on.z;
      if (d >= cosTheta - 1e-6f) {
        acc.x += on.x;
        acc.y += on.y;
        acc.z += on.z;
      }
    }
    const float len = std::sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
    out[c] = (len > 1e-12f)
                 ? SFVec3f{acc.x / len, acc.y / len, acc.z / len}
                 : fn; // keep the flat normal if nothing accumulated.
  }
  m.normals.swap(out);
}

void generateDefaultTexCoords(MeshData &m) {
  if (m.positions.empty())
    return;

  // Local bbox over the EXPANDED positions (same set of distinct points as the
  // coord array, just possibly repeated — min/max are identical either way).
  SFVec3f lo = m.positions[0], hi = m.positions[0];
  for (const SFVec3f &p : m.positions) {
    lo.x = std::min(lo.x, p.x);
    hi.x = std::max(hi.x, p.x);
    lo.y = std::min(lo.y, p.y);
    hi.y = std::max(hi.y, p.y);
    lo.z = std::min(lo.z, p.z);
    hi.z = std::max(hi.z, p.z);
  }
  const float size[3] = {hi.x - lo.x, hi.y - lo.y, hi.z - lo.z};
  const float mn[3] = {lo.x, lo.y, lo.z};

  // Sdim = largest dimension, Tdim = second-largest. Tie-break preferring
  // X(0) then Y(1) then Z(2): a strict '>' comparison walking 0,1,2 keeps the
  // earlier axis on a tie.
  int sDim = 0;
  for (int a = 1; a < 3; ++a)
    if (size[a] > size[sDim])
      sDim = a;
  int tDim = -1;
  for (int a = 0; a < 3; ++a) {
    if (a == sDim)
      continue;
    if (tDim < 0 || size[a] > size[tDim])
      tDim = a;
  }

  const float sSize = size[sDim];
  const float invS = (sSize > 0.0f) ? 1.0f / sSize : 0.0f;

  m.texcoords.clear();
  m.texcoords.reserve(m.positions.size());
  for (const SFVec3f &p : m.positions) {
    const float pc[3] = {p.x, p.y, p.z};
    const float s = (pc[sDim] - mn[sDim]) * invS;
    const float t = (pc[tDim] - mn[tDim]) * invS;
    m.texcoords.push_back(SFVec2f{s, t});
  }
}

void generateGridTexCoords(MeshData &m, int xDim, int zDim) {
  if (m.positions.empty() || m.latticeIndex.size() != m.positions.size())
    return;
  const float invX = (xDim > 1) ? 1.0f / static_cast<float>(xDim - 1) : 0.0f;
  const float invZ = (zDim > 1) ? 1.0f / static_cast<float>(zDim - 1) : 0.0f;
  m.texcoords.clear();
  m.texcoords.reserve(m.positions.size());
  for (std::uint32_t lid : m.latticeIndex) {
    const int i = (xDim > 0) ? static_cast<int>(lid) % xDim : 0;
    const int j = (xDim > 0) ? static_cast<int>(lid) / xDim : 0;
    m.texcoords.push_back(
        SFVec2f{static_cast<float>(i) * invX, static_cast<float>(j) * invZ});
  }
}

bool resolveGridTexCoords(MeshData &m, const std::vector<SFVec2f> &point) {
  if (point.empty() || m.positions.empty() ||
      m.latticeIndex.size() != m.positions.size())
    return false;
  m.texcoords.clear();
  m.texcoords.reserve(m.positions.size());
  for (std::uint32_t lid : m.latticeIndex)
    m.texcoords.push_back(lid < point.size() ? point[lid]
                                             : SFVec2f{0.0f, 0.0f});
  return true;
}

std::vector<SFVec2f> texturePoints2D(const X3DNode &tc) {
  using namespace ::x3d::runtime::geombounds;
  const std::string type = tc.nodeTypeName();
  if (type == "MultiTextureCoordinate") {
    const auto children = getField<MFNode>(tc, "texCoord", {});
    for (const auto &child : children) {
      if (!child)
        continue;
      auto points = texturePoints2D(*child);
      if (!points.empty())
        return points;
    }
    return {};
  }
  if (type == "TextureCoordinate3D") {
    std::vector<SFVec2f> out;
    auto points = getField<std::vector<SFVec3f>>(tc, "point", {});
    out.reserve(points.size());
    for (const auto &p : points)
      out.push_back(SFVec2f{p.x, p.y});
    return out;
  }
  if (type == "TextureCoordinate4D") {
    std::vector<SFVec2f> out;
    auto points = getField<std::vector<SFVec4f>>(tc, "point", {});
    out.reserve(points.size());
    for (const auto &p : points) {
      const float invW = (p.w != 0.0f) ? (1.0f / p.w) : 1.0f;
      out.push_back(SFVec2f{p.x * invW, p.y * invW});
    }
    return out;
  }
  return getField<std::vector<SFVec2f>>(tc, "point", {});
}

std::vector<std::vector<SFVec2f>> texturePointSets2D(const X3DNode &tc) {
  using namespace ::x3d::runtime::geombounds;
  if (tc.nodeTypeName() != "MultiTextureCoordinate")
    return {texturePoints2D(tc)};
  std::vector<std::vector<SFVec2f>> out;
  const auto children = getField<MFNode>(tc, "texCoord", {});
  out.reserve(children.size());
  for (const auto &child : children)
    out.push_back(child ? texturePoints2D(*child) : std::vector<SFVec2f>{});
  return out;
}

AttrResolvers buildAttrs(const X3DNode &geom) {
  using namespace ::x3d::runtime::geombounds;
  AttrResolvers a;

  if (auto nrm = getNode(geom, "normal")) {
    a.normals = getField<std::vector<SFVec3f>>(*nrm, "vector", {});
    a.hasNormal = !a.normals.empty();
  }

  // Color OR ColorRGBA via the single "color" slot; ColorRGBA carried as-is,
  // SFColor promoted to alpha=1.
  if (auto col = getNode(geom, "color")) {
    const std::string ct = col->nodeTypeName();
    if (ct == "ColorRGBA") {
      a.colors = getField<std::vector<SFColorRGBA>>(*col, "color", {});
    } else { // "Color" (or any color-bearing node) -> promote MFColor.
      auto rgb = getField<std::vector<SFColor>>(*col, "color", {});
      a.colors.reserve(rgb.size());
      for (const auto &c : rgb)
        a.colors.push_back(promote(c));
    }
    a.hasColor = !a.colors.empty();
  }

  if (auto tc = getNode(geom, "texCoord")) {
    a.texcoordSets = texturePointSets2D(*tc);
    for (const auto &set : a.texcoordSets) {
      if (!set.empty()) {
        a.texcoords = set;
        break;
      }
    }
    a.hasTexCoord = !a.texcoords.empty();
  }

  a.normalIndex = getField<std::vector<int>>(geom, "normalIndex", {});
  a.colorIndex = getField<std::vector<int>>(geom, "colorIndex", {});
  a.texCoordIndex = getField<std::vector<int>>(geom, "texCoordIndex", {});

  a.normalPerVertex = getField<SFBool>(geom, "normalPerVertex", true);
  a.colorPerVertex = getField<SFBool>(geom, "colorPerVertex", true);
  return a;
}

void emitTri(MeshData &m, const SFVec3f &a, const SFVec3f &b, const SFVec3f &c,
             const SFVec3f &na, const SFVec3f &nb, const SFVec3f &nc,
             const SFVec2f &ta, const SFVec2f &tb, const SFVec2f &tc) {
  auto base = static_cast<std::uint32_t>(m.positions.size());
  m.positions.push_back(a);
  m.positions.push_back(b);
  m.positions.push_back(c);
  m.indices.push_back(base);
  m.indices.push_back(base + 1);
  m.indices.push_back(base + 2);
  m.normals.push_back(na);
  m.normals.push_back(nb);
  m.normals.push_back(nc);
  m.texcoords.push_back(ta);
  m.texcoords.push_back(tb);
  m.texcoords.push_back(tc);
}

SFVec3f normalize(const SFVec3f &v) {
  const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  if (len <= 0.0f)
    return SFVec3f{0.0f, 0.0f, 0.0f};
  return SFVec3f{v.x / len, v.y / len, v.z / len};
}

float seamShiftedS(int j, int n) {
  float s = static_cast<float>(j) / static_cast<float>(n) + 0.5f;
  return s - std::floor(s);
}

void tessellateBox(MeshData &m, const SFVec3f &size) {
  const float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
  // Each face: outward normal + its four corners listed CCW as seen from
  // outside (so the geometric CCW normal == the face normal).
  struct Face {
    SFVec3f n;
    SFVec3f v0, v1, v2, v3;
  };
  const Face faces[6] = {
      // +X
      {{1, 0, 0}, {hx, -hy, hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {hx, hy, hz}},
      // -X
      {{-1, 0, 0},
       {-hx, -hy, -hz},
       {-hx, -hy, hz},
       {-hx, hy, hz},
       {-hx, hy, -hz}},
      // +Y
      {{0, 1, 0}, {-hx, hy, hz}, {hx, hy, hz}, {hx, hy, -hz}, {-hx, hy, -hz}},
      // -Y
      {{0, -1, 0},
       {-hx, -hy, -hz},
       {hx, -hy, -hz},
       {hx, -hy, hz},
       {-hx, -hy, hz}},
      // +Z
      {{0, 0, 1}, {-hx, -hy, hz}, {hx, -hy, hz}, {hx, hy, hz}, {-hx, hy, hz}},
      // -Z
      {{0, 0, -1},
       {hx, -hy, -hz},
       {-hx, -hy, -hz},
       {-hx, hy, -hz},
       {hx, hy, -hz}},
  };
  // DEFAULT texcoords (TC4): each face maps the unit square [0,1]x[0,1] across
  // its 4 corners, listed in the SAME CCW v0,v1,v2,v3 order as above:
  //   v0->(0,0)  v1->(1,0)  v2->(1,1)  v3->(0,1).
  const SFVec2f t0{0.0f, 0.0f}, t1{1.0f, 0.0f}, t2{1.0f, 1.0f}, t3{0.0f, 1.0f};
  for (const Face &f : faces) {
    emitTri(m, f.v0, f.v1, f.v2, f.n, f.n, f.n, t0, t1, t2);
    emitTri(m, f.v0, f.v2, f.v3, f.n, f.n, f.n, t0, t2, t3);
  }
}

void tessellateSphere(MeshData &m, float radius, int rings, int segments) {
  if (rings < 2)
    rings = 2;
  if (segments < 3)
    segments = 3;
  const float pi = 3.14159265358979323846f;
  // Lattice of (rings+1) x (segments+1) positions; phi from +Y pole (0) down.
  const auto vert = [&](int i, int j) {
    const float phi = pi * static_cast<float>(i) / static_cast<float>(rings);
    const float theta =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(segments);
    const float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
    // Outward radial direction; +Y at i=0, -Y at i=rings.
    return SFVec3f{sinPhi * std::sin(theta), cosPhi, sinPhi * std::cos(theta)};
  };
  // DEFAULT texcoords (TC4 + TXC-1). s wraps 0..1 CCW-from-above with the seam
  // at the BACK (-Z meridian, §13.3.7), via seamShiftedS + the per-slice wrap
  // fix. t = 0 at the -Y (south) pole .. 1 at the +Y (north) pole. Since phi
  // runs 0 at +Y to pi at -Y, t = 1 - i/rings.
  const auto tFor = [&](int i) {
    return 1.0f - static_cast<float>(i) / static_cast<float>(rings);
  };
  for (int i = 0; i < rings; ++i)
    for (int j = 0; j < segments; ++j) {
      const SFVec3f n00 = vert(i, j), n01 = vert(i, j + 1);
      const SFVec3f n10 = vert(i + 1, j), n11 = vert(i + 1, j + 1);
      float sL = seamShiftedS(j, segments), sR = seamShiftedS(j + 1, segments);
      if (sR <= sL)
        sR += 1.0f; // seam slice closes to 1.0 (no texture reversal)
      const float tT = tFor(i), tB = tFor(i + 1);
      const SFVec2f t00{sL, tT}, t01{sR, tT};
      const SFVec2f t10{sL, tB}, t11{sR, tB};
      const auto P = [&](const SFVec3f &n) {
        return SFVec3f{n.x * radius, n.y * radius, n.z * radius};
      };
      // CCW from outside. The top row (i==0) is a fan to the +Y pole and the
      // bottom row (i==rings-1) a fan to the -Y pole — each contributes ONE
      // triangle; the (rings-2) middle bands contribute the full quad (two
      // triangles). Total = S*2*(rings-1).
      // Top row (i==0): n00 and n01 coincide at the +Y pole, so the
      //   (n00,n11,n01) half is degenerate — keep only (n00,n10,n11).
      // Bottom row (i==rings-1): n10 and n11 coincide at the -Y pole, so the
      //   (n00,n10,n11) half is degenerate — keep only (n00,n11,n01).
      if (i != rings - 1)
        emitTri(m, P(n00), P(n10), P(n11), n00, n10, n11, t00, t10, t11);
      if (i != 0)
        emitTri(m, P(n00), P(n11), P(n01), n00, n11, n01, t00, t11, t01);
    }
}

void tessellateCone(MeshData &m, float bottomRadius, float height, int slices,
                    bool side, bool bottom) {
  if (slices < 3)
    slices = 3;
  const float pi = 3.14159265358979323846f;
  const float top = height * 0.5f, bot = -height * 0.5f;
  const SFVec3f apex{0.0f, top, 0.0f};
  // Side-slant for outward normals: the lateral normal tilts up by the cone's
  // half-angle. nY component = bottomRadius / slant; radial component scaled.
  const float slant = std::sqrt(bottomRadius * bottomRadius + height * height);
  const float nY = (slant > 0.0f) ? bottomRadius / slant : 0.0f;
  const float nR = (slant > 0.0f) ? height / slant : 1.0f;
  const auto ring = [&](int j) {
    const float a =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
    return SFVec3f{bottomRadius * std::sin(a), bot, bottomRadius * std::cos(a)};
  };
  // Outward lateral normal at angle a.
  const auto sideNormal = [&](int j) {
    const float a =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
    return normalize(SFVec3f{nR * std::sin(a), nY, nR * std::cos(a)});
  };
  // DEFAULT texcoords (TC4 + TXC-1). Side: s wraps CCW-from-above with the seam
  // at the BACK (-Z, §13.3.2) via seamShiftedS + per-slice wrap fix; t = 0 at
  // the base rim .. 1 at the apex. Bottom cap: a circle inscribed in [0,1]^2
  // centred at (0.5,0.5); the cap is right side up when the cone's top is
  // rotated toward -Z, so the -Z rim maps to t=1 and the +Z rim to t=0 (capUv
  // y-scale = -0.5).
  const auto capUv = [&](int j, float y) {
    const float a =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
    return SFVec2f{0.5f + 0.5f * std::sin(a), 0.5f + y * std::cos(a)};
  };
  if (side)
    for (int j = 0; j < slices; ++j) {
      const SFVec3f b0 = ring(j), b1 = ring(j + 1);
      const SFVec3f nb0 = sideNormal(j), nb1 = sideNormal(j + 1);
      // Apex normal = average of the two adjacent base normals (seam-stable).
      const SFVec3f na = normalize(
          SFVec3f{(nb0.x + nb1.x) * 0.5f, nb0.y, (nb0.z + nb1.z) * 0.5f});
      float sL = seamShiftedS(j, slices), sR = seamShiftedS(j + 1, slices);
      if (sR <= sL)
        sR += 1.0f; // seam slice closes to 1.0 (no texture reversal)
      // CCW from outside: apex (t=1), b0 (t=0), b1 (t=0). Apex s = midpoint.
      const SFVec2f ta{(sL + sR) * 0.5f, 1.0f};
      const SFVec2f t0{sL, 0.0f}, t1{sR, 0.0f};
      emitTri(m, apex, b0, b1, na, nb0, nb1, ta, t0, t1);
    }
  if (bottom) {
    const SFVec3f dn{0.0f, -1.0f, 0.0f};
    const SFVec3f cen{0.0f, bot, 0.0f};
    const SFVec2f cuv{0.5f, 0.5f};
    for (int j = 0; j < slices; ++j) {
      const SFVec3f b0 = ring(j), b1 = ring(j + 1);
      // Seen from below (outside), CCW order is center, b1, b0.
      emitTri(m, cen, b1, b0, dn, dn, dn, cuv, capUv(j + 1, -0.5f),
              capUv(j, -0.5f));
    }
  }
}

void tessellateCylinder(MeshData &m, float radius, float height, int slices,
                        bool side, bool top, bool bottom) {
  if (slices < 3)
    slices = 3;
  const float pi = 3.14159265358979323846f;
  const float ty = height * 0.5f, by = -height * 0.5f;
  const auto dir = [&](int j) {
    const float a =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
    return SFVec3f{std::sin(a), 0.0f, std::cos(a)};
  };
  const auto at = [&](const SFVec3f &d, float y) {
    return SFVec3f{d.x * radius, y, d.z * radius};
  };
  // DEFAULT texcoords (TC4 + TXC-1). Side: s wraps CCW-from-above with the seam
  // at the BACK (-Z, §13.3.3) via seamShiftedS + per-slice wrap fix; t = 0 at
  // the bottom rim .. 1 at the top rim. Caps: a circle inscribed in [0,1]^2
  // centred at (0.5,0.5). The top is right side up when the cylinder top tilts
  // toward +Z
  // (+Z rim -> t=1, capUv y-scale = +0.5); the bottom is right side up when it
  // tilts toward -Z (-Z rim -> t=1, capUv y-scale = -0.5).
  const auto capUv = [&](int j, float yScale) {
    const float a =
        2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
    return SFVec2f{0.5f + 0.5f * std::sin(a), 0.5f + yScale * std::cos(a)};
  };
  if (side)
    for (int j = 0; j < slices; ++j) {
      const SFVec3f d0 = dir(j), d1 = dir(j + 1);
      const SFVec3f t0 = at(d0, ty), t1 = at(d1, ty);
      const SFVec3f b0 = at(d0, by), b1 = at(d1, by);
      float s0 = seamShiftedS(j, slices), s1 = seamShiftedS(j + 1, slices);
      if (s1 <= s0)
        s1 += 1.0f; // seam slice closes to 1.0 (no texture reversal)
      const SFVec2f uB0{s0, 0.0f}, uB1{s1, 0.0f}, uT0{s0, 1.0f}, uT1{s1, 1.0f};
      // CCW from outside: (b0, b1, t1) + (b0, t1, t0).
      emitTri(m, b0, b1, t1, d0, d1, d1, uB0, uB1, uT1);
      emitTri(m, b0, t1, t0, d0, d1, d0, uB0, uT1, uT0);
    }
  if (top) {
    const SFVec3f up{0.0f, 1.0f, 0.0f};
    const SFVec3f cen{0.0f, ty, 0.0f};
    const SFVec2f cuv{0.5f, 0.5f};
    for (int j = 0; j < slices; ++j) {
      const SFVec3f a = at(dir(j), ty), b = at(dir(j + 1), ty);
      // Seen from above (outside), CCW order is center, a, b.
      emitTri(m, cen, a, b, up, up, up, cuv, capUv(j, 0.5f),
              capUv(j + 1, 0.5f));
    }
  }
  if (bottom) {
    const SFVec3f dn{0.0f, -1.0f, 0.0f};
    const SFVec3f cen{0.0f, by, 0.0f};
    const SFVec2f cuv{0.5f, 0.5f};
    for (int j = 0; j < slices; ++j) {
      const SFVec3f a = at(dir(j), by), b = at(dir(j + 1), by);
      // Seen from below (outside), CCW order is center, b, a.
      emitTri(m, cen, b, a, dn, dn, dn, cuv, capUv(j + 1, -0.5f),
              capUv(j, -0.5f));
    }
  }
}

SFVec3f vsub(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.x - b.x, a.y - b.y, a.z - b.z};
}

SFVec3f vcross(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
}

float vdot(const SFVec3f &a, const SFVec3f &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vlen(const SFVec3f &a) { return std::sqrt(vdot(a, a)); }

SFVec3f rotateAxisAngle(const SFVec3f &v, const SFVec3f &axis, float angle) {
  const float len = vlen(axis);
  if (len <= 1e-12f || angle == 0.0f)
    return v; // identity rotation.
  const SFVec3f k{axis.x / len, axis.y / len, axis.z / len};
  const float c = std::cos(angle), s = std::sin(angle);
  const SFVec3f kxv = vcross(k, v);
  const float kdv = vdot(k, v);
  return SFVec3f{v.x * c + kxv.x * s + k.x * kdv * (1.0f - c),
                 v.y * c + kxv.y * s + k.y * kdv * (1.0f - c),
                 v.z * c + kxv.z * s + k.z * kdv * (1.0f - c)};
}

void tessellateExtrusion(MeshData &m, const std::vector<SFVec2f> &crossSection,
                         const std::vector<SFVec3f> &spine,
                         const std::vector<SFRotation> &orientation,
                         const std::vector<SFVec2f> &scale, bool beginCap,
                         bool endCap, bool ccw, float creaseAngle,
                         bool genTexCoords) {
  const int ns = static_cast<int>(spine.size());
  const int nc = static_cast<int>(crossSection.size());
  if (ns < 2 || nc < 2)
    return;

  // Closed-spine detection (first point coincides with last) and closed
  // cross-section (first vertex coincides with last) per the spec.
  const float eps = 1e-7f;
  const auto coincident3 = [&](const SFVec3f &a, const SFVec3f &b) {
    return vlen(vsub(a, b)) < eps;
  };
  const bool spineClosed = coincident3(spine[0], spine[ns - 1]);
  const bool csClosed =
      std::fabs(crossSection[0].x - crossSection[nc - 1].x) < eps &&
      std::fabs(crossSection[0].y - crossSection[nc - 1].y) < eps;

  // SF-or-MF clamp-to-last picker.
  const auto pickScale = [&](int i) -> SFVec2f {
    if (scale.empty())
      return SFVec2f{1.0f, 1.0f};
    return scale[i < static_cast<int>(scale.size())
                     ? i
                     : static_cast<int>(scale.size()) - 1];
  };
  const auto pickOrient = [&](int i) -> SFRotation {
    if (orientation.empty())
      return SFRotation{0.0f, 0.0f, 1.0f, 0.0f};
    return orientation[i < static_cast<int>(orientation.size())
                           ? i
                           : static_cast<int>(orientation.size()) - 1];
  };

  // --- Build the per-section SCP frame (Y tangent, Z plane-normal, X = YxZ).
  // --
  std::vector<SFVec3f> axX(ns), axY(ns), axZ(ns);
  SFVec3f prevY{0, 1, 0}, prevZ{0, 0, 1};
  bool haveY = false, haveZ = false;

  // Tangent (Y) at section i.
  const auto tangentAt = [&](int i) -> SFVec3f {
    SFVec3f t{0, 0, 0};
    if (i == 0) {
      if (spineClosed) // wrap: avg(last-edge, first-edge)
        t = vsub(spine[1], spine[ns - 2]);
      else
        t = vsub(spine[1], spine[0]);
    } else if (i == ns - 1) {
      if (spineClosed)
        t = vsub(spine[1], spine[ns - 2]);
      else
        t = vsub(spine[ns - 1], spine[ns - 2]);
    } else {
      t = vsub(spine[i + 1], spine[i - 1]); // avg of in/out edges
    }
    return t;
  };

  // First pass: tangents (Y). Degenerate => inherit previous.
  for (int i = 0; i < ns; ++i) {
    SFVec3f t = tangentAt(i);
    if (vlen(t) < eps) {
      axY[i] = haveY ? prevY : SFVec3f{0, 1, 0};
    } else {
      const float l = vlen(t);
      axY[i] = SFVec3f{t.x / l, t.y / l, t.z / l};
      prevY = axY[i];
      haveY = true;
    }
  }
  // If NO tangent was ever valid, all Y default to +Y (already set).

  // Second pass: plane-normal Z. For an interior point the normal of the plane
  // through (prev,this,next); endpoints inherit the adjacent interior Z; a
  // collinear/degenerate plane inherits the previous Z. A sign flip relative to
  // the previous Z is corrected (consistent frame).
  for (int i = 0; i < ns; ++i) {
    SFVec3f z{0, 0, 0};
    if (i > 0 && i < ns - 1) {
      z = vcross(vsub(spine[i + 1], spine[i]), vsub(spine[i - 1], spine[i]));
    } else if (spineClosed) {
      // endpoints of a closed spine: use the wrap neighbours.
      const int ip = (i == 0) ? ns - 2 : i - 1;
      const int in = (i == ns - 1) ? 1 : i + 1;
      z = vcross(vsub(spine[in], spine[i]), vsub(spine[ip], spine[i]));
    }
    if (vlen(z) < eps) {
      axZ[i] = haveZ ? prevZ : SFVec3f{0, 0, 0}; // resolve below if never seen.
    } else {
      const float l = vlen(z);
      SFVec3f zn{z.x / l, z.y / l, z.z / l};
      if (haveZ && vdot(zn, prevZ) < 0.0f)
        zn = SFVec3f{-zn.x, -zn.y, -zn.z}; // keep frame from inverting.
      axZ[i] = zn;
      prevZ = zn;
      haveZ = true;
    }
  }
  // If NO plane normal was ever valid (straight spine): pick a Z perpendicular
  // to the (common) tangent and propagate it to every section.
  if (!haveZ) {
    const SFVec3f y = axY[0];
    SFVec3f ref = (std::fabs(y.y) < 0.9f) ? SFVec3f{0, 1, 0} : SFVec3f{1, 0, 0};
    SFVec3f z = vcross(ref, y);
    const float l = vlen(z);
    z = (l > eps) ? SFVec3f{z.x / l, z.y / l, z.z / l} : SFVec3f{0, 0, 1};
    for (int i = 0; i < ns; ++i)
      axZ[i] = z;
  } else {
    // Backfill any sections that inherited a still-empty Z (leading
    // degenerate).
    SFVec3f fill{0, 0, 0};
    for (int i = 0; i < ns; ++i)
      if (vlen(axZ[i]) >= eps) {
        fill = axZ[i];
        break;
      }
    if (vlen(fill) < eps)
      fill = SFVec3f{0, 0, 1};
    for (int i = 0; i < ns; ++i)
      if (vlen(axZ[i]) < eps)
        axZ[i] = fill;
  }

  // X = Y x Z (re-orthogonalize so the frame stays right-handed even when the
  // raw spine-plane Z is not exactly perpendicular to the averaged tangent Y).
  for (int i = 0; i < ns; ++i) {
    SFVec3f x = vcross(axY[i], axZ[i]);
    float l = vlen(x);
    if (l < eps) { // Y and Z parallel — rebuild an arbitrary perpendicular.
      const SFVec3f &y = axY[i];
      SFVec3f ref =
          (std::fabs(y.y) < 0.9f) ? SFVec3f{0, 1, 0} : SFVec3f{1, 0, 0};
      x = vcross(y, ref);
      l = vlen(x);
    }
    axX[i] = (l > eps) ? SFVec3f{x.x / l, x.y / l, x.z / l} : SFVec3f{1, 0, 0};
    // Re-derive Z = X x Y to guarantee orthonormality.
    SFVec3f z = vcross(axX[i], axY[i]);
    float lz = vlen(z);
    axZ[i] = (lz > eps) ? SFVec3f{z.x / lz, z.y / lz, z.z / lz} : axZ[i];
  }

  // --- Place every (section, crossSection vertex) into world-local space. ----
  // vertex[i*nc + k] = the position of cross-section vertex k at spine point i.
  std::vector<SFVec3f> verts(static_cast<std::size_t>(ns) * nc);
  for (int i = 0; i < ns; ++i) {
    const SFVec2f sc = pickScale(i);
    const SFRotation rot = pickOrient(i);
    const SFVec3f rax{rot.x, rot.y, rot.z};
    for (int k = 0; k < nc; ++k) {
      const SFVec2f &cv = crossSection[k];
      // Scale in cross-section space (x,y) then map into the SCP plane:
      // cross-section X -> SCPxAxis, cross-section Y -> SCPzAxis.
      const float sx = cv.x * sc.x;
      const float sy = cv.y * sc.y;
      SFVec3f local{axX[i].x * sx + axZ[i].x * sy,
                    axX[i].y * sx + axZ[i].y * sy,
                    axX[i].z * sx + axZ[i].z * sy};
      // Per-section orientation about its axis/angle, then translate to spine.
      local = rotateAxisAngle(local, rax, rot.angle);
      verts[static_cast<std::size_t>(i) * nc + k] = SFVec3f{
          local.x + spine[i].x, local.y + spine[i].y, local.z + spine[i].z};
    }
  }

  // --- DEFAULT (implicit) TEXTURE-COORDINATE PARAMETERIZATION (TC3). ---------
  // ISO/IEC 19775-1 (Extrusion, texCoord NULL — Extrusion has NO authored
  // TextureCoordinate field, so this is the ONLY mapping):
  //   * SIDE walls: S runs AROUND the cross-section by ACCUMULATED CHORD length
  //       sParam[k] = (sum |crossSection[m]-crossSection[m-1]|, m<=k) /
  //       perimeter;
  //     T runs ALONG the spine by ACCUMULATED spine length
  //       tParam[i] = (spine length up to section i) / total spine length.
  //   * begin/end CAPS: the cross-section's OWN 2D coordinates normalized to
  //   the
  //     cross-section's 2D bbox -> (s,t) in [0,1] across the cap.
  // Guards a zero-length spine / cross-section to 0 (no div-by-zero).
  // genTexCoords is the dispatcher's "no authored TextureCoordinate" switch
  // (always true for Extrusion); when false (B3/B6 paths that don't want UVs)
  // texcoords stay empty.
  std::vector<float> sParam(
      nc, 0.0f); // per cross-section vertex, around-perimeter.
  std::vector<float> tParam(ns, 0.0f); // per section, along-spine.
  SFVec2f csLo = crossSection[0],
          csHi = crossSection[0]; // cross-section 2D bbox.
  if (genTexCoords) {
    // S: accumulated cross-section chord length, normalized to total perimeter.
    float csAccum = 0.0f;
    for (int k = 1; k < nc; ++k) {
      const float dx = crossSection[k].x - crossSection[k - 1].x;
      const float dy = crossSection[k].y - crossSection[k - 1].y;
      csAccum += std::sqrt(dx * dx + dy * dy);
      sParam[k] = csAccum;
    }
    const float csTotal = csAccum;
    const float invCs = (csTotal > 0.0f) ? 1.0f / csTotal : 0.0f;
    for (int k = 0; k < nc; ++k)
      sParam[k] *= invCs;
    // T: accumulated spine length, normalized to total spine length.
    float spAccum = 0.0f;
    for (int i = 1; i < ns; ++i) {
      spAccum += vlen(vsub(spine[i], spine[i - 1]));
      tParam[i] = spAccum;
    }
    const float spTotal = spAccum;
    const float invSp = (spTotal > 0.0f) ? 1.0f / spTotal : 0.0f;
    for (int i = 0; i < ns; ++i)
      tParam[i] *= invSp;
    // Cross-section 2D bbox for the cap mapping.
    for (int k = 0; k < nc; ++k) {
      csLo.x = std::min(csLo.x, crossSection[k].x);
      csLo.y = std::min(csLo.y, crossSection[k].y);
      csHi.x = std::max(csHi.x, crossSection[k].x);
      csHi.y = std::max(csHi.y, crossSection[k].y);
    }
  }
  const float csSizeX = csHi.x - csLo.x, csSizeY = csHi.y - csLo.y;
  // Cap mapping per ISO/IEC 19775-1 (Extrusion): the cross-section is UNIFORMLY
  // scaled so the LARGER dimension (X or Z) ranges 0..1 — aspect-preserved, NOT
  // a per-axis stretch. The smaller axis then ranges 0..(smaller/larger).
  const float csMax = (std::max)(csSizeX, csSizeY);
  const float invCs = (csMax > 0.0f) ? 1.0f / csMax : 0.0f;
  // Side-wall UV for the corner at (section i, cross-section vertex k).
  const auto sideUv = [&](int i, int k) -> SFVec2f {
    return SFVec2f{sParam[k], tParam[i]};
  };
  const auto capUv = [&](int k) -> SFVec2f {
    return SFVec2f{(crossSection[k].x - csLo.x) * invCs,
                   (crossSection[k].y - csLo.y) * invCs};
  };

  const auto lid = [&](int i, int k) {
    return static_cast<std::uint32_t>(i * nc + k);
  };
  // Emit a flat-normal triangle (a,b,c) honoring ccw (swap b/c when !ccw),
  // retaining the lattice id of each source (section,crossSectionVertex) AND,
  // when genTexCoords, the per-corner UV (ua,ub,uc) parallel to positions. The
  // ccw swap reorders b/c, the lattice ids AND the UVs in lockstep.
  const auto emit = [&](int ia, int ka, int ib, int kb, int ic, int kc,
                        SFVec2f ua, SFVec2f ub, SFVec2f uc) {
    SFVec3f a = verts[lid(ia, ka)], b = verts[lid(ib, kb)],
            c = verts[lid(ic, kc)];
    std::uint32_t la = lid(ia, ka), lb = lid(ib, kb), lc = lid(ic, kc);
    if (!ccw) {
      std::swap(b, c);
      std::swap(lb, lc);
      std::swap(ub, uc);
    }
    auto base = static_cast<std::uint32_t>(m.positions.size());
    m.positions.push_back(a);
    m.positions.push_back(b);
    m.positions.push_back(c);
    m.indices.push_back(base);
    m.indices.push_back(base + 1);
    m.indices.push_back(base + 2);
    m.latticeIndex.push_back(la);
    m.latticeIndex.push_back(lb);
    m.latticeIndex.push_back(lc);
    const SFVec3f n =
        faceNormal(verts[lid(ia, ka)], verts[lid(ib, kb)], verts[lid(ic, kc)]);
    m.normals.push_back(n);
    m.normals.push_back(n);
    m.normals.push_back(n);
    if (genTexCoords) {
      m.texcoords.push_back(ua);
      m.texcoords.push_back(ub);
      m.texcoords.push_back(uc);
    }
  };

  // --- Side walls: quad band between section i and i+1, vertex k and k+1.
  // ----- CCW-as-authored quad (k,i)-(k+1,i)-(k+1,i+1)-(k,i+1) split into two
  // tris. Each corner's UV is the side parameterization at its (section,
  // csVertex).
  for (int i = 0; i + 1 < ns; ++i)
    for (int k = 0; k + 1 < nc; ++k) {
      emit(i, k, i, k + 1, i + 1, k + 1, sideUv(i, k), sideUv(i, k + 1),
           sideUv(i + 1, k + 1));
      emit(i, k, i + 1, k + 1, i + 1, k, sideUv(i, k), sideUv(i + 1, k + 1),
           sideUv(i + 1, k));
    }

  // --- Caps: fan over the closed cross-section at the first/last spine point.
  // - The cap winding is the spec's: beginCap faces -Y (toward the spine
  // start), endCap faces +Y. We fan v0,vk,vk+1 over the cross-section; ccw is
  // honored by emit(). beginCap uses reversed order so its outward face opposes
  // endCap. Cap UVs use the cross-section's own 2D coords normalized to the
  // bbox.
  const int capCount = csClosed ? nc - 1 : nc; // skip duplicate closing vertex.
  if (beginCap && capCount >= 3) {
    for (int k = 1; k + 1 < capCount; ++k)
      emit(0, 0, 0, k + 1, 0, k, capUv(0), capUv(k + 1),
           capUv(k)); // reversed fan (faces away from endCap).
  }
  if (endCap && capCount >= 3) {
    const int last = ns - 1;
    for (int k = 1; k + 1 < capCount; ++k)
      emit(last, 0, last, k, last, k + 1, capUv(0), capUv(k), capUv(k + 1));
  }

  // creaseAngle smoothing (B6) keyed on the SOURCE lattice id (section*nc +
  // crossSectionVertex), not the expanded run. creaseAngle==0 => flat output.
  if (creaseAngle > 0.0f)
    creaseSmoothNormals(m, m.latticeIndex, creaseAngle);
  m.hasNormals = !m.normals.empty();
}

} // namespace x3d::runtime::extract::mesh_detail

namespace x3d::runtime::extract {

bool recognizedGeometryType(const std::string &t) {
  return
      // analytic primitives (no coord node)
      t == "ElevationGrid" || t == "GeoElevationGrid" || t == "Box" ||
      t == "Sphere" || t == "Cone" || t == "Cylinder" || t == "Extrusion" ||
      // T-TEXT: Text (glyph-quad mesh via the font-metrics seam)
      t == "Text" ||
      // coord-fed indexed / strip / fan / quad sets
      t == "IndexedFaceSet" || t == "IndexedTriangleSet" ||
      t == "TriangleSet" || t == "IndexedTriangleFanSet" ||
      t == "IndexedTriangleStripSet" || t == "IndexedQuadSet" ||
      t == "TriangleFanSet" || t == "TriangleStripSet" || t == "QuadSet" ||
      // B4 line/point sets (Topology::Lines / Topology::Points; always unlit)
      t == "IndexedLineSet" || t == "LineSet" || t == "PointSet" ||
      // NURBS (T5): curve -> Lines, patch surface -> Triangles
      t == "NurbsCurve" || t == "NurbsPatchSurface";
}

MeshData buildLocalMesh(const X3DNode *geom, const MeshBuildOptions &opt,
                        bool *recognized) {
  using namespace mesh_detail;
  ++buildLocalMeshCalls_;
  MeshData mesh;
  if (recognized)
    *recognized = false;
  if (!geom)
    return mesh;

  const std::string t = geom->nodeTypeName();
  if (recognized)
    *recognized = recognizedGeometryType(t);

  // ccw / solid carried verbatim (X3DComposedGeometryNode defaults true).
  mesh.ccw = geombounds::getField<SFBool>(*geom, "ccw", true);
  mesh.solid = geombounds::getField<SFBool>(*geom, "solid", true);

  if (t == "NurbsCurve") {
    auto cpNode = geombounds::getNode(*geom, "controlPoint");
    if (!cpNode)
      return mesh; // recognized, empty
    nurbs::CurveDef c;
    c.cp = geombounds::getPointsLenient(*cpNode, "point");
    c.w = geombounds::getField<std::vector<double>>(*geom, "weight", {});
    c.knot = geombounds::getField<std::vector<double>>(*geom, "knot", {});
    c.order = geombounds::getField<SFInt32>(*geom, "order", 3);
    c.closed = geombounds::getField<SFBool>(*geom, "closed", false);
    if ((int)c.cp.size() < c.order)
      return mesh; // recognized, empty
    int segs = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "tessellation", 0),
        (int)c.cp.size());
    auto pts = nurbs::tessellateCurve(c, segs);
    if (pts.size() < 2)
      return mesh;
    for (std::size_t k = 0; k + 1 < pts.size(); ++k) {
      mesh.positions.push_back(pts[k]);
      mesh.positions.push_back(pts[k + 1]);
    }
    mesh.indices.resize(mesh.positions.size());
    for (std::uint32_t i = 0; i < mesh.indices.size(); ++i)
      mesh.indices[i] = i;
    mesh.topology = Topology::Lines;
    mesh.solid = false;
    return mesh;
  }

  if (t == "NurbsPatchSurface") {
    auto cpNode = geombounds::getNode(*geom, "controlPoint");
    if (!cpNode)
      return mesh;
    nurbs::SurfaceDef s;
    s.cp = geombounds::getPointsLenient(*cpNode, "point");
    s.w = geombounds::getField<std::vector<double>>(*geom, "weight", {});
    s.uKnot = geombounds::getField<std::vector<double>>(*geom, "uKnot", {});
    s.vKnot = geombounds::getField<std::vector<double>>(*geom, "vKnot", {});
    s.uDim = geombounds::getField<SFInt32>(*geom, "uDimension", 0);
    s.vDim = geombounds::getField<SFInt32>(*geom, "vDimension", 0);
    s.uOrder = geombounds::getField<SFInt32>(*geom, "uOrder", 3);
    s.vOrder = geombounds::getField<SFInt32>(*geom, "vOrder", 3);
    s.uClosed = geombounds::getField<SFBool>(*geom, "uClosed", false);
    s.vClosed = geombounds::getField<SFBool>(*geom, "vClosed", false);
    if (s.uDim < s.uOrder || s.vDim < s.vOrder ||
        (int)s.cp.size() < s.uDim * s.vDim)
      return mesh; // recognized, empty
    int uSeg = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "uTessellation", 0), s.uDim);
    int vSeg = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "vTessellation", 0), s.vDim);
    auto grid = nurbs::tessellateSurface(s, uSeg, vSeg);
    if (grid.empty())
      return mesh;
    int gw = uSeg + 1;
    for (int j = 0; j < vSeg; ++j)
      for (int i = 0; i < uSeg; ++i) {
        const nurbs::SurfaceSample *quad[6] = {&grid[j * gw + i],
                                               &grid[j * gw + (i + 1)],
                                               &grid[(j + 1) * gw + (i + 1)],
                                               &grid[j * gw + i],
                                               &grid[(j + 1) * gw + (i + 1)],
                                               &grid[(j + 1) * gw + i]};
        for (auto *v : quad) {
          mesh.positions.push_back(v->p);
          mesh.normals.push_back(v->n);
          mesh.texcoords.push_back(v->uv);
        }
      }
    mesh.indices.resize(mesh.positions.size());
    for (std::uint32_t k = 0; k < mesh.indices.size(); ++k)
      mesh.indices[k] = k;
    mesh.topology = Topology::Triangles;
    mesh.hasNormals = true;
    return mesh; // mesh.solid was carried from getField at the top
  }

  // T-TEXT: Text carries NO coord node — its glyph-quad mesh is synthesized
  // from the §15 layout engine + the embedder's FontMetrics seam (held on opt).
  // The Text node's outputOnly fields (textBounds/lineBounds/origin) are set by
  // the SceneExtractor (it owns the non-const node); buildLocalMesh produces
  // only the geometry here. Returns isGlyphMesh=true, two-sided.
  if (t == "Text") {
    return buildTextMesh(*geom, opt.fontMetrics);
  }

  // ElevationGrid is special: it carries NO coord node — positions are
  // synthesized from the height grid. Handle it before the coord guard, with
  // its own funnel that pushes synthesized vertices (+ flat normals).
  if (t == "ElevationGrid") {
    int xd = geombounds::getField<int>(*geom, "xDimension", 0);
    int zd = geombounds::getField<int>(*geom, "zDimension", 0);
    const float xs = geombounds::getField<float>(*geom, "xSpacing", 1.0f);
    const float zs = geombounds::getField<float>(*geom, "zSpacing", 1.0f);
    const auto h =
        geombounds::getField<std::vector<float>>(*geom, "height", {});
    // Some conformance scenes (ConformanceNist ElevationGrid/test_{x,z}Spacing)
    // author only ONE dimension and leave the other at its 0 default, relying
    // on the renderer to infer it from height.size(). Both are required per
    // spec, but the NIST reference renders expect the inference, so derive the
    // missing dimension from the height array when it divides evenly.
    const std::size_t hn = h.size();
    if (zd < 2 && xd >= 2 && hn > 0 && hn % static_cast<std::size_t>(xd) == 0)
      zd = static_cast<int>(hn / static_cast<std::size_t>(xd));
    else if (xd < 2 && zd >= 2 && hn > 0 &&
             hn % static_cast<std::size_t>(zd) == 0)
      xd = static_cast<int>(hn / static_cast<std::size_t>(zd));
    if (xd >= 2 && zd >= 2 &&
        static_cast<std::size_t>(xd) * static_cast<std::size_t>(zd) <=
            h.size()) {
      // Lattice-index-retaining emission (B5). Synthesize each lattice vertex
      // (col=i, row=j) from height + spacing; the shared emitter records the
      // (row*xDim+col) source id per expanded corner (for B6 smoothing).
      const float crease =
          geombounds::getField<float>(*geom, "creaseAngle", 0.0f);
      emitHeightGrid(
          mesh, xd, zd,
          [&](int i, int j) {
            return SFVec3f{static_cast<float>(i) * xs,
                           h[static_cast<std::size_t>(j) * xd + i],
                           static_cast<float>(j) * zs};
          },
          crease, mesh.ccw);
      // DEFAULT (implicit) grid texcoords (TC2). An authored TextureCoordinate
      // ALWAYS WINS (resolved per lattice vertex, lid = j*xDim+i); only when
      // texCoord is NULL do we generate the NORMATIVE s=i/(xDim-1),
      // t=j/(zDim-1) grid parameterization. Either path leaves texcoords
      // parallel to positions.
      std::vector<SFVec2f> authored;
      if (auto tc = geombounds::getNode(*geom, "texCoord"))
        authored = geombounds::getField<std::vector<SFVec2f>>(*tc, "point", {});
      if (!resolveGridTexCoords(mesh, authored))
        generateGridTexCoords(mesh, xd, zd);
    }
    return mesh;
  }

  // GeoElevationGrid (B5) — the single biggest corpus unlock (~72% of files;
  // all Savage/Locations + Geospatial terrain tiles). Synthesizes the same
  // height lattice as ElevationGrid but the per-vertex geographic coordinate is
  // mapped to a LOCAL position by the embedder's GeoProjection seam. With NO
  // projection wired we FLAT-FALLBACK: geoGridOrigin as a planar origin,
  // spacing as planar units, so terrain renders its SHAPE immediately but
  // geographically UNANCHORED (wrong absolute placement/orientation vs a
  // GeoViewpoint; tiles in different units mis-register). This is a documented
  // shape-visible stopgap.
  if (t == "GeoElevationGrid") {
    // EXACT reflected field types (GeoElevationGrid stores SFInt32/SFDouble/
    // MFDouble/SFVec3d/SFFloat); getField returns the default on a type
    // mismatch, so requesting the wrong T would silently zero the grid.
    const int xd = geombounds::getField<int>(*geom, "xDimension", 0);
    const int zd = geombounds::getField<int>(*geom, "zDimension", 0);
    const double xs = geombounds::getField<double>(*geom, "xSpacing", 1.0);
    const double zs = geombounds::getField<double>(*geom, "zSpacing", 1.0);
    const float yScale = geombounds::getField<float>(*geom, "yScale", 1.0f);
    const auto h =
        geombounds::getField<std::vector<double>>(*geom, "height", {});
    if (xd >= 2 && zd >= 2 &&
        static_cast<std::size_t>(xd) * static_cast<std::size_t>(zd) <=
            h.size()) {
      GeoSystemDesc sys;
      sys.geoSystem = geombounds::getField<std::vector<std::string>>(
          *geom, "geoSystem", {});
      sys.geoGridOrigin = geombounds::getField<SFVec3d>(*geom, "geoGridOrigin",
                                                        SFVec3d{0, 0, 0});

      // GeoElevationGrid stores creaseAngle as SFDouble (NOT SFFloat).
      const float crease = static_cast<float>(
          geombounds::getField<double>(*geom, "creaseAngle", 0.0));
      const GeoProjection &proj = opt.geoProjection;
      // Spec lattice order: height index = row*xDim + col, row j advances along
      // the FIRST geoSystem axis (north/latitude), col i along the SECOND
      // (east/longitude). geoGridOrigin.x = first axis, .y = second axis.
      emitHeightGrid(
          mesh, xd, zd,
          [&](int i, int j) -> SFVec3f {
            const double elev = h[static_cast<std::size_t>(j) * xd + i] *
                                static_cast<double>(yScale);
            if (proj) {
              const SFVec3d geoCoord{
                  sys.geoGridOrigin.x + static_cast<double>(j) * zs,
                  sys.geoGridOrigin.y + static_cast<double>(i) * xs, elev};
              return proj(geoCoord, elev, sys);
            }
            // FLAT-FALLBACK: planar (X=east/col, Z=north/row), Y=elevation. The
            // geoGridOrigin x/y are folded in as a planar offset so multi-tile
            // shapes keep relative layout (still geographically unanchored).
            return SFVec3f{static_cast<float>(sys.geoGridOrigin.y +
                                              static_cast<double>(i) * xs),
                           static_cast<float>(elev),
                           static_cast<float>(sys.geoGridOrigin.x +
                                              static_cast<double>(j) * zs)};
          },
          crease, mesh.ccw);
      // DEFAULT (implicit) grid texcoords (TC2) — same parameterization as
      // ElevationGrid (s=i/(xDim-1), t=j/(zDim-1)); an authored
      // TextureCoordinate ALWAYS WINS, resolved per lattice vertex via
      // latticeIndex.
      std::vector<SFVec2f> authored;
      if (auto tc = geombounds::getNode(*geom, "texCoord"))
        authored = geombounds::getField<std::vector<SFVec2f>>(*tc, "point", {});
      if (!resolveGridTexCoords(mesh, authored))
        generateGridTexCoords(mesh, xd, zd);
    }
    return mesh;
  }

  // Analytic primitives (T4) — no coord node; tessellated parametrically with
  // CCW outward winding + outward per-corner normals (compose under the same
  // cull rule as composed meshes). Dispatched before the coord guard.
  if (t == "Box") {
    const SFVec3f size =
        geombounds::getField<SFVec3f>(*geom, "size", SFVec3f{2.0f, 2.0f, 2.0f});
    tessellateBox(mesh, size);
    mesh.hasNormals = !mesh.normals.empty();
    return mesh;
  }
  if (t == "Sphere") {
    const float radius = geombounds::getField<SFFloat>(*geom, "radius", 1.0f);
    tessellateSphere(mesh, radius, opt.sphereRings, opt.sphereSegments);
    mesh.hasNormals = !mesh.normals.empty();
    return mesh;
  }
  if (t == "Cone") {
    const float br = geombounds::getField<SFFloat>(*geom, "bottomRadius", 1.0f);
    const float h = geombounds::getField<SFFloat>(*geom, "height", 2.0f);
    const bool side = geombounds::getField<SFBool>(*geom, "side", true);
    const bool bottom = geombounds::getField<SFBool>(*geom, "bottom", true);
    tessellateCone(mesh, br, h, opt.radialSlices, side, bottom);
    mesh.hasNormals = !mesh.normals.empty();
    return mesh;
  }
  if (t == "Cylinder") {
    const float radius = geombounds::getField<SFFloat>(*geom, "radius", 1.0f);
    const float h = geombounds::getField<SFFloat>(*geom, "height", 2.0f);
    const bool side = geombounds::getField<SFBool>(*geom, "side", true);
    const bool top = geombounds::getField<SFBool>(*geom, "top", true);
    const bool bottom = geombounds::getField<SFBool>(*geom, "bottom", true);
    tessellateCylinder(mesh, radius, h, opt.radialSlices, side, top, bottom);
    mesh.hasNormals = !mesh.normals.empty();
    return mesh;
  }

  // Extrusion (B3) — no coord node; swept solid. Dispatched before the coord
  // guard. Reads crossSection/spine/orientation/scale/beginCap/endCap/ccw,
  // builds the Spine-aligned Cross-section frame and emits side walls + caps in
  // lattice-index-retaining form (for B6). creaseAngle deferred to B6.
  if (t == "Extrusion") {
    // X3D spec defaults: crossSection = unit square (closed), spine =
    // origin+up. The generated binding now carries these full multi-point
    // defaults, so an UNauthored field already arrives complete; the .empty()
    // fallback stays as a safety net. An AUTHORED degenerate (1-point)
    // crossSection/spine is left as-is so tessellateExtrusion's ns>=2/nc>=2
    // guard culls it (the documented
    // "< 2 spine points => empty" contract).
    std::vector<SFVec2f> crossSection =
        geombounds::getField<std::vector<SFVec2f>>(*geom, "crossSection", {});
    if (crossSection.empty())
      crossSection = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {1, 1}};
    std::vector<SFVec3f> spine =
        geombounds::getField<std::vector<SFVec3f>>(*geom, "spine", {});
    if (spine.empty())
      spine = {{0, 0, 0}, {0, 1, 0}};
    const auto orientation =
        geombounds::getField<std::vector<SFRotation>>(*geom, "orientation", {});
    const auto scale =
        geombounds::getField<std::vector<SFVec2f>>(*geom, "scale", {});
    const bool beginCap = geombounds::getField<SFBool>(*geom, "beginCap", true);
    const bool endCap = geombounds::getField<SFBool>(*geom, "endCap", true);
    const float crease =
        geombounds::getField<float>(*geom, "creaseAngle", 0.0f);
    // DEFAULT (implicit) texcoords (TC3): Extrusion has NO authored
    // TextureCoordinate field (texcoords are implicit-only per the spec), so we
    // ALWAYS generate the NORMATIVE side-wall (S around the cross-section by
    // accumulated chord length, T along the spine by accumulated length) + cap
    // (cross-section coords normalized to the cross-section bbox)
    // parameterization, emitted parallel to positions inside
    // tessellateExtrusion.
    tessellateExtrusion(mesh, crossSection, spine, orientation, scale, beginCap,
                        endCap, mesh.ccw, crease, /*genTexCoords=*/true);
    mesh.hasNormals = !mesh.normals.empty();
    return mesh;
  }

  auto coord = geombounds::getNode(*geom, "coord");
  if (!coord)
    return mesh;
  const auto pts = geombounds::getPointsLenient(*coord, "point");
  if (pts.empty())
    return mesh;

  const AttrResolvers attrs = buildAttrs(*geom);

  const auto ok = [&](int i) {
    return i >= 0 && i < static_cast<int>(pts.size());
  };

  // B6 source-index adjacency: when no Normal is authored, record per emitted
  // corner the COORD index it came from, so a creaseAngle post-pass can fuse
  // face normals across shared coordinate verts WITHOUT re-indexing. Only
  // IndexedFaceSet carries creaseAngle among composed sets; the funnel always
  // captures this (cheap) and the post-pass runs solely for IFS.
  std::vector<std::uint32_t> sourceId;

  // Single triangle funnel. Each Corner carries its coord index, its positional
  // index within the index stream (for *Index / per-vertex order), and the face
  // ordinal (for per-face attributes). Positions are pushed expanded; authored
  // attributes resolved per corner; flat normal computed when no Normal node.
  const auto addTriangle = [&](const Corner &ca, const Corner &cb,
                               const Corner &cc) {
    const SFVec3f &pa = pts[ca.coord];
    const SFVec3f &pb = pts[cb.coord];
    const SFVec3f &pc = pts[cc.coord];
    auto base = static_cast<std::uint32_t>(mesh.positions.size());
    mesh.positions.push_back(pa);
    mesh.positions.push_back(pb);
    mesh.positions.push_back(pc);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    sourceId.push_back(static_cast<std::uint32_t>(ca.coord));
    sourceId.push_back(static_cast<std::uint32_t>(cb.coord));
    sourceId.push_back(static_cast<std::uint32_t>(cc.coord));

    // Normals: authored Normal node honored per corner; else FLAT per-face.
    if (attrs.hasNormal) {
      const SFVec3f flat = faceNormal(pa, pb, pc); // fallback for OOB corners.
      const auto pickN = [&](const Corner &cr) {
        int s = AttrResolvers::pickIndex(attrs.normals, attrs.normalIndex,
                                         attrs.normalPerVertex, cr);
        return s < 0 ? flat : attrs.normals[s];
      };
      mesh.normals.push_back(pickN(ca));
      mesh.normals.push_back(pickN(cb));
      mesh.normals.push_back(pickN(cc));
    } else {
      // GENERATED face normal. Per rendering.md §11.4.x / geometry3D.md
      // §13.3.x, ccw=FALSE reverses the direction of the GENERATED normal. We
      // negate here (rather than swapping the winding) so the index/position
      // order stays the authored order; the Extrusion path achieves the same
      // net normal by swapping b/c. mesh.ccw is still reported verbatim, and
      // authored Normal nodes (the branch above) are NEVER flipped.
      SFVec3f n = faceNormal(pa, pb, pc);
      if (!mesh.ccw)
        n = SFVec3f{-n.x, -n.y, -n.z};
      mesh.normals.push_back(n);
      mesh.normals.push_back(n);
      mesh.normals.push_back(n);
    }

    // Colors: per-vertex/per-face Color or ColorRGBA (Color promoted alpha=1).
    if (attrs.hasColor) {
      const auto pickC = [&](const Corner &cr) -> SFColorRGBA {
        int s = AttrResolvers::pickIndex(attrs.colors, attrs.colorIndex,
                                         attrs.colorPerVertex, cr);
        return s < 0 ? SFColorRGBA{1.0f, 1.0f, 1.0f, 1.0f} : attrs.colors[s];
      };
      mesh.colors.push_back(pickC(ca));
      mesh.colors.push_back(pickC(cb));
      mesh.colors.push_back(pickC(cc));
    }

    // TexCoords: per-vertex only (texCoordIndex or coordIndex fallback).
    if (attrs.hasTexCoord) {
      if (!attrs.texcoordSets.empty() && mesh.texcoordSets.empty())
        mesh.texcoordSets.resize(attrs.texcoordSets.size());
      const auto pickT = [&](const Corner &cr) -> SFVec2f {
        int s = AttrResolvers::pickIndex(attrs.texcoords, attrs.texCoordIndex,
                                         /*perVertex=*/true, cr);
        return s < 0 ? SFVec2f{0.0f, 0.0f} : attrs.texcoords[s];
      };
      mesh.texcoords.push_back(pickT(ca));
      mesh.texcoords.push_back(pickT(cb));
      mesh.texcoords.push_back(pickT(cc));
      for (std::size_t setIndex = 0; setIndex < attrs.texcoordSets.size();
           ++setIndex) {
        const auto &set = attrs.texcoordSets[setIndex];
        const auto pickSetT = [&](const Corner &cr) -> SFVec2f {
          int s = AttrResolvers::pickIndex(set, attrs.texCoordIndex,
                                           /*perVertex=*/true, cr);
          return s < 0 ? SFVec2f{0.0f, 0.0f} : set[s];
        };
        mesh.texcoordSets[setIndex].push_back(pickSetT(ca));
        mesh.texcoordSets[setIndex].push_back(pickSetT(cb));
        mesh.texcoordSets[setIndex].push_back(pickSetT(cc));
      }
    }
  };

  if (t == "IndexedFaceSet") {
    const auto idx =
        geombounds::getField<std::vector<int>>(*geom, "coordIndex", {});
    // MSH-3: the convex field (SFBool, default TRUE). When TRUE we keep the
    // existing naive fan (byte-identical to before). When FALSE a face may be
    // concave, so we ear-clip in its best-fit plane to avoid the fan's
    // inverted/overlapping triangles.
    const bool convex = geombounds::getField<SFBool>(*geom, "convex", true);
    // A face is a maximal run between -1 separators. `pos` tracks the
    // positional offset within `idx` (so normalIndex/colorIndex/texCoordIndex
    // line up with coordIndex element-for-element). face counts faces (per-face
    // attributes).
    std::vector<Corner> face;
    int faceNo = 0;
    const auto flush = [&] {
      if (convex) {
        for (std::size_t k = 1; k + 1 < face.size(); ++k)
          if (ok(face[0].coord) && ok(face[k].coord) && ok(face[k + 1].coord))
            addTriangle(face[0], face[k], face[k + 1]);
      } else {
        // Gather the in-range corners (preserving order), ear-clip their
        // positions, then emit each ear via addTriangle (which owns
        // normal/ccw/attribute resolution).
        std::vector<Corner> valid;
        std::vector<SFVec3f> poly;
        valid.reserve(face.size());
        poly.reserve(face.size());
        for (const Corner &c : face)
          if (ok(c.coord)) {
            valid.push_back(c);
            poly.push_back(pts[c.coord]);
          }
        for (const auto &tri : earClipPolygon(poly))
          addTriangle(valid[tri[0]], valid[tri[1]], valid[tri[2]]);
      }
      face.clear();
      ++faceNo;
    };
    for (std::size_t p = 0; p < idx.size(); ++p) {
      if (idx[p] < 0)
        flush();
      else
        face.push_back(Corner{idx[p], static_cast<int>(p), faceNo});
    }
    if (face.size() >= 3)
      flush();
    // creaseAngle smoothing (B6) — IFS is the only composed set with
    // creaseAngle. Runs ONLY when no Normal node is authored (attrs.hasNormal
    // false) and creaseAngle>0; keyed on the COORD source id so
    // coincident-distinct verts stay distinct. creaseAngle==0 => byte-identical
    // flat output (non-regressive).
    if (!attrs.hasNormal) {
      const float crease =
          geombounds::getField<float>(*geom, "creaseAngle", 0.0f);
      if (crease > 0.0f)
        creaseSmoothNormals(mesh, sourceId, crease);
    }
  } else if (t == "IndexedTriangleSet") {
    const auto idx = geombounds::getField<std::vector<int>>(*geom, "index", {});
    int faceNo = 0;
    for (std::size_t i = 0; i + 2 < idx.size(); i += 3, ++faceNo)
      if (ok(idx[i]) && ok(idx[i + 1]) && ok(idx[i + 2]))
        addTriangle(Corner{idx[i], static_cast<int>(i), faceNo},
                    Corner{idx[i + 1], static_cast<int>(i + 1), faceNo},
                    Corner{idx[i + 2], static_cast<int>(i + 2), faceNo});
  } else if (t == "TriangleSet") {
    int faceNo = 0;
    for (std::size_t i = 0; i + 2 < pts.size(); i += 3, ++faceNo)
      addTriangle(
          Corner{static_cast<int>(i), static_cast<int>(i), faceNo},
          Corner{static_cast<int>(i + 1), static_cast<int>(i + 1), faceNo},
          Corner{static_cast<int>(i + 2), static_cast<int>(i + 2), faceNo});
  } else if (t == "IndexedTriangleFanSet") {
    const auto idx = geombounds::getField<std::vector<int>>(*geom, "index", {});
    std::vector<Corner> fan;
    int faceNo = 0;
    const auto flush = [&] {
      for (std::size_t k = 1; k + 1 < fan.size(); ++k)
        if (ok(fan[0].coord) && ok(fan[k].coord) && ok(fan[k + 1].coord))
          addTriangle(fan[0], fan[k], fan[k + 1]);
      if (!fan.empty())
        ++faceNo; // EXT-002: one color/normal per FAN, not per triangle
      fan.clear();
    };
    for (std::size_t p = 0; p < idx.size(); ++p) {
      if (idx[p] < 0)
        flush();
      else
        fan.push_back(Corner{idx[p], static_cast<int>(p), faceNo});
    }
    if (fan.size() >= 3)
      flush();
  } else if (t == "IndexedTriangleStripSet") {
    const auto idx = geombounds::getField<std::vector<int>>(*geom, "index", {});
    std::vector<Corner> strip;
    int faceNo = 0;
    const auto flush = [&] {
      for (std::size_t k = 0; k + 2 < strip.size(); ++k) {
        Corner a = strip[k], b = strip[k + 1], c = strip[k + 2];
        if (k & 1)
          std::swap(b, c); // winding flip on odd triangles.
        if (ok(a.coord) && ok(b.coord) && ok(c.coord))
          addTriangle(a, b, c);
      }
      if (!strip.empty())
        ++faceNo; // EXT-002: one color/normal per STRIP
      strip.clear();
    };
    for (std::size_t p = 0; p < idx.size(); ++p) {
      if (idx[p] < 0)
        flush();
      else
        strip.push_back(Corner{idx[p], static_cast<int>(p), faceNo});
    }
    if (strip.size() >= 3)
      flush();
  } else if (t == "IndexedQuadSet") {
    const auto idx = geombounds::getField<std::vector<int>>(*geom, "index", {});
    int faceNo = 0;
    for (std::size_t i = 0; i + 3 < idx.size(); i += 4, ++faceNo) {
      Corner a{idx[i], static_cast<int>(i), faceNo};
      Corner b{idx[i + 1], static_cast<int>(i + 1), faceNo};
      Corner c{idx[i + 2], static_cast<int>(i + 2), faceNo};
      Corner d{idx[i + 3], static_cast<int>(i + 3), faceNo};
      if (ok(a.coord) && ok(b.coord) && ok(c.coord))
        addTriangle(a, b, c);
      if (ok(a.coord) && ok(c.coord) && ok(d.coord))
        addTriangle(a, c, d);
    }
  } else if (t == "TriangleFanSet") {
    const auto counts =
        geombounds::getField<std::vector<int>>(*geom, "fanCount", {});
    int v0 = 0, faceNo = 0;
    for (int c : counts) {
      for (int k = 1; k + 1 < c; ++k)
        if (ok(v0) && ok(v0 + k) && ok(v0 + k + 1))
          addTriangle(Corner{v0, v0, faceNo}, Corner{v0 + k, v0 + k, faceNo},
                      Corner{v0 + k + 1, v0 + k + 1, faceNo});
      v0 += c;
      ++faceNo; // EXT-002: one color/normal per FAN, not per triangle
    }
  } else if (t == "TriangleStripSet") {
    const auto counts =
        geombounds::getField<std::vector<int>>(*geom, "stripCount", {});
    int v0 = 0, faceNo = 0;
    for (int c : counts) {
      for (int k = 0; k + 2 < c; ++k) {
        int ai = v0 + k, bi = v0 + k + 1, di = v0 + k + 2;
        if (k & 1)
          std::swap(bi, di); // winding flip on odd triangles.
        if (ok(ai) && ok(bi) && ok(di))
          addTriangle(Corner{ai, ai, faceNo}, Corner{bi, bi, faceNo},
                      Corner{di, di, faceNo});
      }
      v0 += c;
      ++faceNo; // EXT-002: one color/normal per STRIP, not per triangle
    }
  } else if (t == "QuadSet") {
    int faceNo = 0;
    for (std::size_t i = 0; i + 3 < pts.size(); i += 4, ++faceNo) {
      int a = static_cast<int>(i), b = static_cast<int>(i + 1);
      int c = static_cast<int>(i + 2), d = static_cast<int>(i + 3);
      addTriangle(Corner{a, a, faceNo}, Corner{b, b, faceNo},
                  Corner{c, c, faceNo});
      addTriangle(Corner{a, a, faceNo}, Corner{c, c, faceNo},
                  Corner{d, d, faceNo});
    }
  } else if (t == "IndexedLineSet" || t == "LineSet" || t == "PointSet") {
    // ----------------------------------------------------------------------
    // B4 — line/point topology. These produce NO triangles and NO normals;
    // they are ALWAYS unlit (vertex Color when present, else baseColor). The
    // producer marks the mesh non-solid so a consumer's cull-disable path and
    // the topology!=Triangles unlit selector both cover it.
    //
    // Each emitted index is its own EXPANDED corner (one fresh position + a
    // 0..N-1 index entry), matching the triangle funnel's layout so the GPU
    // upload path is identical. Color is resolved per corner via the shared
    // AttrResolvers (colorPerVertex / colorIndex honored); X3D line/point sets
    // carry Color/ColorRGBA but never Normal/TexCoord, so those resolvers are
    // simply absent.
    // ----------------------------------------------------------------------
    mesh.solid = false; // line/point meshes are double-sided + unlit.
    const auto emitVertex = [&](const Corner &cr) {
      mesh.positions.push_back(pts[cr.coord]);
      mesh.indices.push_back(static_cast<std::uint32_t>(mesh.indices.size()));
      if (attrs.hasColor) {
        int s = AttrResolvers::pickIndex(attrs.colors, attrs.colorIndex,
                                         attrs.colorPerVertex, cr);
        mesh.colors.push_back(s < 0 ? SFColorRGBA{1.0f, 1.0f, 1.0f, 1.0f}
                                    : attrs.colors[s]);
      }
    };

    if (t == "IndexedLineSet") {
      // coordIndex: each -1-delimited run is a POLYLINE; emit it as consecutive
      // vertex PAIRS (segment endpoints) so a GL_LINES draw renders the whole
      // polyline (v0-v1, v1-v2, ...). colorPerVertex=false => one color per
      // polyline (face); colorPerVertex=true => per coordIndex element.
      mesh.topology = Topology::Lines;
      const auto idx =
          geombounds::getField<std::vector<int>>(*geom, "coordIndex", {});
      std::vector<Corner> run;
      int faceNo = 0;
      const auto flush = [&] {
        for (std::size_t k = 0; k + 1 < run.size(); ++k)
          if (ok(run[k].coord) && ok(run[k + 1].coord)) {
            emitVertex(run[k]);
            emitVertex(run[k + 1]);
          }
        run.clear();
        ++faceNo;
      };
      for (std::size_t p = 0; p < idx.size(); ++p) {
        if (idx[p] < 0)
          flush();
        else
          run.push_back(Corner{idx[p], static_cast<int>(p), faceNo});
      }
      if (run.size() >= 2)
        flush();
    } else if (t == "LineSet") {
      // vertexCount: MFInt32 partitioning the implicit coord run into polylines
      // (no -1 separators, no coordIndex). Each count>=2 polyline -> segment
      // pairs. colorPerVertex is implicitly per-vertex for LineSet (no per-face
      // color concept in the spec); resolve by coord/pos.
      mesh.topology = Topology::Lines;
      const auto counts =
          geombounds::getField<std::vector<int>>(*geom, "vertexCount", {});
      int v0 = 0, faceNo = 0;
      for (int c : counts) {
        for (int k = 0; k + 1 < c; ++k) {
          int ai = v0 + k, bi = v0 + k + 1;
          if (ok(ai) && ok(bi)) {
            emitVertex(Corner{ai, ai, faceNo});
            emitVertex(Corner{bi, bi, faceNo});
          }
        }
        v0 += c;
        ++faceNo;
      }
    } else { // PointSet
      // The whole coord `point` array is one 0..N-1 run of GL_POINTS. Color is
      // per-vertex by coord index (PointSet has no colorIndex/colorPerVertex).
      mesh.topology = Topology::Points;
      for (std::size_t i = 0; i < pts.size(); ++i)
        emitVertex(Corner{static_cast<int>(i), static_cast<int>(i), 0});
    }
  }

  // MSH-2: VERTEX-AVERAGED normals for the triangle/quad-set family. Unlike IFS
  // and ElevationGrid (which carry a creaseAngle field — default 0 = faceted —
  // and run their own creaseAngle pass above), these sets have NO creaseAngle
  // and normalPerVertex defaults TRUE, so rendering.md §11.3.2/§11.4.9 require
  // each GENERATED per-vertex normal to be the AVERAGE of all triangles sharing
  // that vertex. We reuse the creaseSmoothNormals fuser keyed on the COORD
  // source id with angle PI (every incident face at a shared coord fuses). Runs
  // ONLY when no Normal node is authored AND normalPerVertex is TRUE; an
  // authored Normal or normalPerVertex=FALSE (faceted) leaves the flat per-face
  // fill untouched. The ccw negation already baked into addTriangle survives
  // averaging (the summed normals keep their reversed orientation).
  {
    static const std::array<const char *, 6> averagedSets = {
        "IndexedTriangleSet",    "TriangleSet", "IndexedTriangleStripSet",
        "IndexedTriangleFanSet", "QuadSet",     "IndexedQuadSet"};
    const bool isAveragedSet =
        std::find_if(averagedSets.begin(), averagedSets.end(),
                     [&](const char *s) { return t == s; }) !=
        averagedSets.end();
    if (isAveragedSet && !attrs.hasNormal && attrs.normalPerVertex &&
        !mesh.normals.empty())
      creaseSmoothNormals(mesh, sourceId, 3.14159265358979323846f);
  }

  // DEFAULT (implicit) texcoords (TC1). When the geometry authored NO
  // TextureCoordinate node, generate the NORMATIVE bounding-box-projection UVs
  // so a consumer's texture path works even with no authored coords. An
  // authored TextureCoordinate ALWAYS WINS (attrs.hasTexCoord true => skip,
  // byte-unchanged — the per-corner pickT path above already populated
  // mesh.texcoords). Only for triangle topology: line/point sets carry no
  // texcoords by spec.
  if (!attrs.hasTexCoord && mesh.topology == Topology::Triangles &&
      !mesh.positions.empty())
    generateDefaultTexCoords(mesh);
  if (mesh.texcoordSets.empty() && !mesh.texcoords.empty())
    mesh.texcoordSets.push_back(mesh.texcoords);

  mesh.hasNormals = !mesh.normals.empty();
  mesh.hasColors = !mesh.colors.empty();
  return mesh;
}

} // namespace x3d::runtime::extract
