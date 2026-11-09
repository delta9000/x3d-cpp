// Build geometry in its local frame. Unsupported types return an empty mesh.
// Triangle meshes expand each corner to preserve per-face and per-corner attributes.
#ifndef X3D_RUNTIME_EXTRACT_MESH_BUILDER_HPP
#define X3D_RUNTIME_EXTRACT_MESH_BUILDER_HPP

#include "AssetResolver.hpp"
#include "FontMetrics.hpp"
#include "GeoProjection.hpp"
#include "NurbsEval.hpp"
#include "PackedMesh.hpp"
#include "RecursionLimits.hpp"
#include "RenderItem.hpp"
#include "ShaderOptions.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;
using x3d::nodes::X3DNode;

// Geographic geometry uses the caller's explicit backend in GeoNodes.hpp.
// Mesh build options remain for tessellation and authored resource data.
struct MeshBuildOptions {
  int sphereRings = 16;    // latitude bands (pole-to-pole), >= 2.
  int sphereSegments = 16; // longitude segments around Y, >= 3.
  int radialSlices = 24;   // cone/cylinder radial subdivisions, >= 3.

  // Embedder font-metrics seam. The Text branch resolves glyph
  // advances
  // + atlas UVs through this callback; the SDK never opens font files. Defaults
  // to makeMonospaceStub() (advanceEm=0.6, no atlas UV) so Text layout + bounds
  // are exact without any real font wired. Copyable value type (std::function).
  FontMetrics fontMetrics{makeMonospaceStub()};

  // Embedder external-geometry seam. When set, this is called for any
  // geometry node that buildLocalMesh returns recognized==false. An empty
  // PackedMesh (vertex_count==0) signals Pending (retry next tick); non-empty
  // signals Ready and triggers emitPacked(). Null by default (no ext, firewall
  // intact). Core-typed only: PackedMesh + X3DNode* + AssetResolver, no ext
  // type.
  std::function<PackedMesh(const X3DNode *, AssetResolver)>
      externalGeometryResolver{};

  // ceiling on node-visits per fullSnapshot() walk (extractor + light
  // collection). Bounds an acyclic "doubling DAG" fan-out (2^depth paths) that
  // would otherwise emit billions of RenderItems/LightDescs. Default is far
  // above any legitimate placement count; raise it for a genuinely huge scene
  // (and check SceneExtractor::budgetExceeded() to detect truncation).
  std::size_t maxWalkVisits = kMaxGraphWalkVisits;

  // The HAnimSegment enclosing the geometry being built, or null. When the
  // geometry's coord is that Segment's coord, the Segment's displacers are
  // applied to the points (ISO/IEC 19774-1 §6.6; ADR-0055). Set per build by
  // SceneExtractor; the authored Coordinate is never modified.
  const X3DNode *hanimSegment = nullptr;

  // Premultiplied control points are used by default. Euclidean mode multiplies
  // coordinates by weight; unit-weight content is identical in either mode.
  nurbs::NurbsWeightMode nurbsWeightMode = nurbs::NurbsWeightMode::Premultiplied;

  // §31 shaders: the url resolver for ShaderPart/ShaderProgram sources and the
  // host's compile check. SceneExtractor uses these to select each
  // Appearance's shader (ShaderExtract.hpp); RuntimeSession hands the same
  // options to ShaderSystem so its isSelected/isValid events agree.
  ShaderOptions shaders{};
};

namespace mesh_detail {

// Promote an SFColor to SFColorRGBA with alpha = 1 .
SFColorRGBA promote(const SFColor &c);

// Geometric (flat) normal of a triangle (a,b,c). Returns {0,0,0} for a
// degenerate triangle so a consumer can detect/skip it; otherwise normalized.
SFVec3f faceNormal(const SFVec3f &a, const SFVec3f &b, const SFVec3f &c);

// Ear-clip non-convex polygons in their Newell-normal projection plane.
// Returned triples are local indices into `poly`, counterclockwise relative to
// that normal. The caller maps these to source corners before emission.
std::vector<std::array<int, 3>>
earClipPolygon(const std::vector<SFVec3f> &poly);

// Smooth generated normals using source-index adjacency, preserving distinct
// coincident source vertices. Faces contribute when dot(n0,n1) >= cos(creaseAngle).
// Degenerate faces contribute nothing; a zero sum retains the flat normal.
void creaseSmoothNormals(MeshData &m,
                         const std::vector<std::uint32_t> &sourceId,
                         float creaseAngle);

// default (implicit) texture-coordinate generation.
//
// ISO/IEC 19775-1: when a composed/triangle geometry node carries no
// TextureCoordinate (its texCoord field is NULL) the implicit texture
// coordinates are the normative bounding-box projection of the geometry's coord
// points:
//   * Compute the local bbox of the points; dims = (sizeX,sizeY,sizeZ).
//   * Sdim = the axis of the largest dimension, Tdim = the second-largest.
//     Ties prefer X, then Y, then Z (so equal X&Y => S=X, T=Y).
//   * For an emitted vertex p:
//       s = (p[Sdim] - min[Sdim]) / size[Sdim]              (ranges 0..1)
//       t = (p[Tdim] - min[Tdim]) / size[Sdim]              (ranges
//       0..size[T]/size[S])
//     Both divided by size[Sdim] (the largest dim) so the aspect ratio is
//     preserved. size[Sdim]==0 (a degenerate point/line geometry) guards to 0.
//
// Emits one texcoord per already-expanded position (mesh.positions), so it must
// run after all triangles are pushed and only when no TextureCoordinate was
// authored. An authored TextureCoordinate always wins (this is never called in
// that case) — meshes that already carry texcoords stay byte-unchanged.
void generateDefaultTexCoords(MeshData &m);

// default (implicit) grid texture-coordinate generation.
//
// ISO/IEC 19775-1: an ElevationGrid / GeoElevationGrid whose texCoord field is
// NULL generates implicit texture coordinates by the grid parameterization —
// for the lattice vertex at column i (in [0,xDim-1]) and row j (in [0,zDim-1]):
//     s = i / (xDim-1),  t = j / (zDim-1).
// This is distinct from the composed-geometry bbox-projection: a height
// grid carries an explicit (i,j) parameterization, so each vertex's UV is its
// fractional position in the lattice (so the texture maps once across the whole
// grid regardless of spacing/elevation).
//
// The grid emitters expand each cell to fresh corners but retain the source
// lattice id (lid = j*xDim + i) per corner in MeshData.latticeIndex. We invert
// that here: i = lid % xDim, j = lid / xDim. Guards xDim<=1 / zDim<=1 to 0 (a
// degenerate 1-wide/1-tall grid has no extent along that axis). Emits one
// texcoord per already-expanded position, parallel to positions; runs only when
// no TextureCoordinate is authored (an authored one always wins).
void generateGridTexCoords(MeshData &m, int xDim, int zDim);

// Resolve an authored ElevationGrid/GeoElevationGrid TextureCoordinate per
// expanded corner. The spec indexes the texCoord `point` array by lattice
// vertex in the same row-major order as height (lid = j*xDim + i), so we look
// up point[lid] per corner via the retained latticeIndex. An out-of-range lid
// (a short point array) falls back to (0,0). Returns whether it populated
// texcoords (false => caller falls through to generateGridTexCoords).
bool resolveGridTexCoords(MeshData &m, const std::vector<SFVec2f> &point);

// Resolved per-emit-corner attribute carrier. coord    = index into the coord
// `point` array (already validated by ok()); pos       = the POSITIONAL index
// of this corner within the index stream (for *Index lookups / per-vertex
// non-indexed order); face      = the running face/primitive ordinal (per-face
// attribute lookups). For non-indexed types pos == the running vertex ordinal
// and the *Index resolvers fall back to it.
struct Corner {
  int coord = 0;
  int pos = 0;
  int face = 0;
};

// An attribute resolver bundle: per-corner Normal / Color / TexCoord lookups
// built once from the geometry's reflection. Each lambda returns whether it
// produced a value (so the caller knows to fall through to flat-normal gen / no
// color / no texcoord) and, when it does, writes into the out param.
struct AttrResolvers {
  // Normal node present?
  bool hasNormal = false;
  bool hasColor = false; // Color or ColorRGBA present.
  bool hasTexCoord = false;

  std::vector<SFVec3f> normals;    // authored Normal.vector
  std::vector<SFColorRGBA> colors; // promoted (Color->RGBA) or ColorRGBA
  std::vector<SFVec2f> texcoords;  // authored TextureCoordinate.point
  std::vector<std::vector<SFVec2f>> texcoordSets;
  std::vector<MeshData::VertexAttribute> vertexAttributes;
  std::vector<std::vector<float>> vertexAttributeValues;

  std::vector<int> normalIndex;
  std::vector<int> colorIndex;
  std::vector<int> texCoordIndex;

  bool normalPerVertex = true;
  bool colorPerVertex = true;

  // Generic per-corner value pick for an indexed composed-geometry attribute.
  //
  //   perVertex=true : pick attr[ attrIndex[corner.pos] ] if attrIndex present,
  //                    else attr[ corner.coord ] (coordIndex doubles as the
  //                    attr index — the X3D default when *Index is empty).
  //   perVertex=false: per-face — pick attr[ attrIndex[corner.face] ] if
  //                    attrIndex present, else attr[ corner.face ].
  //
  // Returns -1 when the resolved index is out of range (caller drops the
  // value).
  template <class T>
  static int pickIndex(const std::vector<T> &attr, const std::vector<int> &idx,
                       bool perVertex, const Corner &corner) {
    int sel;
    if (perVertex) {
      if (!idx.empty()) {
        if (corner.pos < 0 || corner.pos >= static_cast<int>(idx.size()))
          return -1;
        sel = idx[corner.pos];
      } else {
        sel = corner.coord;
      }
    } else {
      if (!idx.empty()) {
        if (corner.face < 0 || corner.face >= static_cast<int>(idx.size()))
          return -1;
        sel = idx[corner.face];
      } else {
        sel = corner.face;
      }
    }
    if (sel < 0 || sel >= static_cast<int>(attr.size()))
      return -1;
    return sel;
  }
};

std::vector<SFVec2f> texturePoints2D(const X3DNode &tc);

std::vector<std::vector<SFVec2f>> texturePointSets2D(const X3DNode &tc);

// Build the attribute resolvers from a geometry node's child attribute nodes.
AttrResolvers buildAttrs(const X3DNode &geom);

// Analytic primitive tessellation: Box / Sphere / Cone / Cylinder.
//
// Every primitive is emitted expanded (three fresh positions + a trivial 0..N-1
// index run per triangle) with explicit per-corner normals, matching the
// composed-geometry funnel's layout. Winding is CCW as seen from outside the
// surface, and the stored normals point outward, so a primitive composes under
// the exact same cull rule as an extracted mesh (front = CCW, solid => cull
// back). ccw/solid are carried verbatim by buildLocalMesh from the node fields
// (primitive defaults solid=true).
// Push one CCW triangle (a,b,c) with its three already-outward corner normals
// and its three per-corner default texcoords. The analytic primitives
// carry no texCoord field, so the spec's per-primitive UV mapping is always
// generated here in lockstep with positions/normals (one texcoord per corner,
// parallel to positions).
void emitTri(MeshData &m, const SFVec3f &a, const SFVec3f &b, const SFVec3f &c,
             const SFVec3f &na, const SFVec3f &nb, const SFVec3f &nc,
             const SFVec2f &ta, const SFVec2f &tb, const SFVec2f &tc);

SFVec3f normalize(const SFVec3f &v);

// default longitudinal texture S for the analytic primitives. ISO/IEC
// 19775-1 §13.3.2/3/7 place the vertical texture seam at the back (X=0 plane,
// -Z) and wrap S counterclockwise-from-above. The geometry rings start at +Z
// (j=0), so shift by half a turn: S=0 lands on the -Z meridian (j = n/2 for an
// even n). Callers apply a per-slice wrap fix (if sR <= sL, sR += 1) so the
// slice that closes the loop at the seam reads up to 1.0 instead of wrapping
// back to 0 (which would mirror the texture across that one slice).
float seamShiftedS(int j, int n);

// Box: axis-aligned, centered at origin, full extent = size. 6 faces, each a
// CCW-outward quad split into 2 triangles. 12 triangles, 36 corners.
void tessellateBox(MeshData &m, const SFVec3f &size);

// Sphere: UV sphere of `radius` centered at origin, `rings` latitude bands and
// `segments` longitude divisions. The top and bottom rows are triangle fans to
// the poles; the (rings-2) middle rows are quad bands (2 triangles each).
// Per-vertex normal = the radial (outward) direction. Winding CCW from outside.
void tessellateSphere(MeshData &m, float radius, int rings, int segments);

// Cone: apex at +height/2 on +Y, base circle of `bottomRadius` at -height/2.
// `slices` radial divisions. side => lateral surface (slices triangles to the
// apex); bottom => base cap fan (slices triangles), normal straight down (-Y).
// Winding CCW from outside, normals outward.
void tessellateCone(MeshData &m, float bottomRadius, float height, int slices,
                    bool side, bool bottom);

// Cylinder: axis on Y, `radius`, height centered at origin. `slices` radial
// divisions. side => lateral quads (2 tris each); top => +Y cap fan;
// bottom => -Y cap fan. Side normals radial outward; caps +Y / -Y. CCW outside.
void tessellateCylinder(MeshData &m, float radius, float height, int slices,
                        bool side, bool top, bool bottom);

// lattice-grid emitter. Shared by ElevationGrid and GeoElevationGrid.
//
// A height grid is a regular xDim x zDim lattice of vertices; cell (i,j) is
// split into two triangles (v00,v11,v10)+(v00,v01,v11). That winding is
// CCW as seen from the +Y side, so with the default ccw=TRUE the surface is
// front-facing and its generated normal points up (+Y) per ISO/IEC 19775-1
// §13.3.4 — a consumer's default back-face cull (solid=TRUE) keeps the terrain
// when viewed from above. The emitter expands each corner (three fresh
// positions per triangle) BUT also records, per expanded corner, the id of the
// source lattice vertex (row*xDim+col) into MeshData.latticeIndex. That
// retained map is what  needs to weld creaseAngle-smooth normals in source
// space without re-indexing.
//
// `pos(i,j)` supplies the local-frame position of lattice vertex (col=i,
// row=j); GeoElevationGrid also reverses the triangle winding because its
// rows advance north (local −Z). Caller pre-validates xDim>=2 && zDim>=2.
template <class PosFn>
inline void emitHeightGrid(MeshData &m, int xDim, int zDim, PosFn pos,
                           float creaseAngle = 0.0f, bool ccw = true,
                           const AttrResolvers *attrs = nullptr,
                           bool reverseWinding = false) {
  auto lid = [&](int i, int j) {
    return static_cast<std::uint32_t>(j * xDim + i);
  };
  // authored Color/Normal honor colorPerVertex/normalPerVertex. Per
  // §13.3.4 a per-vertex value indexes the lattice vertex (row*xDim+col); a
  // per-quad (colorPerVertex/normalPerVertex FALSE) value indexes the cell
  // (row*(xDim-1)+col). ElevationGrid carries no *Index fields, so the index is
  // the picked value directly.
  const bool authoredNormal = attrs && attrs->hasNormal;
  const bool authoredColor = attrs && attrs->hasColor;
  const int cellCols = xDim - 1; // cells per row.
  const auto pickVert = [&](int i, int j) { return static_cast<int>(lid(i, j)); };
  // Per-quad (per-cell) attribute index: the cell, indexed row*(xDim-1)+col. The
  // cell is identified by its lower-left lattice corner, so all six corners of a
  // cell's two triangles pick the same entry.
  const auto pickCell = [&](int ci, int cj) { return cj * cellCols + ci; };
  const auto addTriangle = [&](int ci, int cj, int ia, int ja, int ib, int jb,
                               int ic, int jc) {
    if (reverseWinding) { std::swap(ib, ic); std::swap(jb, jc); }
    const SFVec3f a = pos(ia, ja), b = pos(ib, jb), c = pos(ic, jc);
    auto base = static_cast<std::uint32_t>(m.positions.size());
    m.positions.push_back(a);
    m.positions.push_back(b);
    m.positions.push_back(c);
    m.indices.push_back(base);
    m.indices.push_back(base + 1);
    m.indices.push_back(base + 2);
    m.latticeIndex.push_back(lid(ia, ja));
    m.latticeIndex.push_back(lid(ib, jb));
    m.latticeIndex.push_back(lid(ic, jc));
    // Authored Normal node wins per §13.3.4; else generated flat normal, whose
    // direction ccw=FALSE reverses ("Setting the ccw field to FALSE reverses the
    // normal direction"). Negation keeps the lattice/index order.
    const SFVec3f flat = faceNormal(a, b, c);
    if (authoredNormal) {
      const auto pickN = [&](int i, int j) -> SFVec3f {
        const int sel = attrs->normalPerVertex ? pickVert(i, j)
                                               : pickCell(ci, cj);
        return (sel >= 0 && sel < static_cast<int>(attrs->normals.size()))
                   ? attrs->normals[sel]
                   : flat;
      };
      m.normals.push_back(pickN(ia, ja));
      m.normals.push_back(pickN(ib, jb));
      m.normals.push_back(pickN(ic, jc));
    } else {
      SFVec3f n = flat;
      if (!ccw)
        n = SFVec3f{-n.x, -n.y, -n.z};
      m.normals.push_back(n);
      m.normals.push_back(n);
      m.normals.push_back(n);
    }
    if (authoredColor) {
      const auto pickC = [&](int i, int j) -> SFColorRGBA {
        const int sel = attrs->colorPerVertex ? pickVert(i, j)
                                              : pickCell(ci, cj);
        return (sel >= 0 && sel < static_cast<int>(attrs->colors.size()))
                   ? attrs->colors[sel]
                   : SFColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};
      };
      m.colors.push_back(pickC(ia, ja));
      m.colors.push_back(pickC(ib, jb));
      m.colors.push_back(pickC(ic, jc));
    }
  };
  for (int j = 0; j + 1 < zDim; ++j)
    for (int i = 0; i + 1 < xDim; ++i) {
      addTriangle(i, j, i, j, i + 1, j + 1, i + 1, j); // v00, v11, v10
      addTriangle(i, j, i, j, i, j + 1, i + 1, j + 1); // v00, v01, v11
    }
  // creaseAngle smoothing keyed on the source lattice id (row*xDim+col),
  // not the expanded 0..N-1 run. creaseAngle==0 => byte-identical flat output.
  // Runs only when no Normal node is authored (authored normals must survive).
  if (!authoredNormal && creaseAngle > 0.0f)
    creaseSmoothNormals(m, m.latticeIndex, creaseAngle);
  m.hasNormals = !m.normals.empty();
  m.hasColors = !m.colors.empty();
}

// EXTRUSION. Sweep a 2D crossSection along a 3D spine, building a
// Spine-aligned Cross-section Coordinate (SCP) frame at every spine point per
// the X3D spec (19.4.5 / 13.3.5):
//   * SCPyAxis = the spine TANGENT — for an interior point the normalized
//     average of the incoming and outgoing edge directions; for the endpoints
//     of an OPEN spine the single adjacent edge; for the endpoints of a CLOSED
//     spine (first==last) the wrap-around average. A spine that is collinear or
//     has coincident points yields a degenerate tangent/Z — those sections
//     inherit the previous section's SCP axes (spec rule).
//   * SCPzAxis = derived from the spine's local plane: for an interior point
//   the
//     normal of the plane through (prev, this, next); seeded from the first
//     valid interior section and propagated to coincident/collinear sections.
//     A sign flip (Z·prevZ < 0) is corrected so the frame does not invert.
//   * SCPxAxis = SCPyAxis × SCPzAxis.
// Each crossSection vertex (cx, cy) maps into the SCP plane as
//   cx along SCPxAxis, cy along SCPzAxis (the X3D cross-section X-Z plane),
// THEN per-section scale (x->X, y->Z), then per-section orientation (a rotation
// about an arbitrary axis), then translate to the spine point.
//
// scale/orientation are SF-or-MF: length 1 applies to every section, otherwise
// the value is clamped to the last for sections beyond the list end.
//
// Side walls are quad bands between consecutive sections (2 tris each, flat
// normals before crease-angle smoothing). beginCap/endCap are fans over the
// (closed) cross-section at the first/last spine point. Winding honors ccw.
//
// A (section, crossSectionVertex) -> expanded-vertex lattice
// map is retained on MeshData.latticeIndex (lid = section*csCount + csVertex),
// so smoothing can recover adjacency without re-indexing. <2 spine
// points => empty.
//
// genTexCoords enables implicit texture-coordinate generation.
// Extrusion has no authored TextureCoordinate field — texcoords are implicit-
// only per ISO/IEC 19775-1, so the dispatcher always requests them. side walls:
// S = accumulated cross-section chord length / perimeter, T = accumulated spine
// length / total spine length. begin/end caps: the cross-section's own 2D
// coords normalized to the cross-section bbox. Emitted per expanded corner,
// parallel to positions (and reordered in lockstep with the ccw winding swap).
SFVec3f vsub(const SFVec3f &a, const SFVec3f &b);
SFVec3f vcross(const SFVec3f &a, const SFVec3f &b);
float vdot(const SFVec3f &a, const SFVec3f &b);
float vlen(const SFVec3f &a);

// Rotate v about a (already-normalized) axis by `angle` radians (Rodrigues).
SFVec3f rotateAxisAngle(const SFVec3f &v, const SFVec3f &axis, float angle);

void tessellateExtrusion(MeshData &m, const std::vector<SFVec2f> &crossSection,
                         const std::vector<SFVec3f> &spine,
                         const std::vector<SFRotation> &orientation,
                         const std::vector<SFVec2f> &scale, bool beginCap,
                         bool endCap, bool ccw, float creaseAngle = 0.0f,
                         bool genTexCoords = false);

} // namespace mesh_detail

// recognizedGeometryType — true iff `t` is a geometry nodeTypeName this builder
// knows how to tessellate. This is the static type-coverage oracle: it
// does not imply the mesh is non-empty (a recognized IFS with an empty
// coordIndex tessellates to nothing — legitimately empty, not unsupported). The
// two sets (analytic-no-coord + coord-fed) mirror the dispatch arms in
// buildLocalMesh; keep them in lockstep when a new geometry arm lands (
// Extrusion/Text/... flips a name from absent->present here as it becomes
// drawable).
bool recognizedGeometryType(const std::string &t);

// buildLocalMesh — extract local-frame triangle geometry from `geom`.
//
// Returns a MeshData whose `positions` are the triangle corners in emission
// order and whose `indices` are a trivial 0..N-1 run (one index per emitted
// position, triples form triangles). normals/colors/texcoords (when present)
// are parallel to positions — same length, one per corner. When no Normal node
// is authored the builder fills `normals` with flat per-face normals so a
// consumer always has shading data. Unsupported types return an empty mesh.
// The projection is borrowed for this call; world consumers pass their context's
// immutable backend so geometry, bounds, transforms and picking agree.
//
// `recognized` distinguishes an unsupported geometry type from supported
// geometry that emits no triangles, such as an empty or degenerate grid.
MeshData buildLocalMesh(const X3DNode *geom, const geo::GeoProjection &projection,
                        const MeshBuildOptions &opt = {},
                        bool *recognized = nullptr);

/// Process-wide number of buildLocalMesh() tessellations. Count builds per
/// geometry rather than per placement to detect redundant work; see ADR-0045.
// Independent owner-thread scenes share this diagnostic only; it is not a
// scene synchronization mechanism or a per-world counter.
inline std::atomic<std::uint64_t> buildLocalMeshCalls_{0};
inline std::uint64_t buildLocalMeshCallCount() {
  return buildLocalMeshCalls_.load(std::memory_order_relaxed);
}

} // namespace x3d::runtime::extract
#endif
