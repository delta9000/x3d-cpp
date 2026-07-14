// MeshBuilder.hpp — M2.5 extraction (Layer A2): geometry node -> MeshData in
// the geometry's LOCAL frame. namespace x3d::runtime::extract. Public value
// types and declarations live here; implementation is compiled in
// x3d_cpp_runtime. Generated nodes remain the source of truth.
//
// SCOPE — every triangle is funnelled through a single addTriangle with
// ok()-bounds guards on EVERY index/count read; an unsupported type yields an
// empty MeshData (positions/indices empty) so the caller can fall through to
// its own proxy/analytic path.
//
// T1 (coord-fed, triangle-bearing):
//   * IndexedFaceSet   : coordIndex, polygon TRIANGLE-FAN (v0,vk,vk+1), -1 ends
//   a face.
//   * IndexedTriangleSet: index, consecutive TRIPLES.
//   * TriangleSet       : implicit consecutive TRIPLES of the coord points.
//
// T2 widened to the remaining indexed/strip/fan/quad sets + the height-grid
// primitive:
//   * IndexedTriangleFanSet  : index, fan (v0,vk,vk+1) per run, -1 separates
//   fans.
//   * IndexedTriangleStripSet: index, strip with PER-TRIANGLE WINDING FLIP
//                              (odd triangle swaps its trailing pair), -1
//                              separates.
//   * IndexedQuadSet         : index, each consecutive 4 -> (0,1,2)+(0,2,3).
//   * TriangleFanSet         : fanCount partitions the implicit coord run into
//   fans.
//   * TriangleStripSet       : stripCount partitions the implicit coord run;
//   flip.
//   * QuadSet                : implicit consecutive quads of 4 coord points.
//   * ElevationGrid          : NO coord node — positions synthesized from
//                              height/xDimension/zDimension/xSpacing/zSpacing;
//                              each grid cell -> 2 triangles.
//
// T3 (this task) adds ATTRIBUTE RESOLUTION + FLAT-NORMAL GENERATION:
//   * Authored Normal (vector: MFVec3f), Color (color: MFColor),
//     ColorRGBA (color: MFColorRGBA) and TextureCoordinate (point: MFVec2f) are
//     resolved per emitted corner, honoring normalPerVertex / colorPerVertex
//     (default true) and the optional normalIndex / colorIndex / texCoordIndex.
//   * SFColor is PROMOTED to SFColorRGBA with alpha = 1.
//   * When NO Normal node is authored the builder GENERATES FLAT per-face
//     normals (the geometric normal of each emitted triangle), so a consumer
//     always has shading normals. (creaseAngle smoothing is deferred.)
//   * MeshData.hasColors is set when a per-vertex/per-face Color or ColorRGBA
//   was
//     resolved; per X3D lighting a per-vertex Color OVERRIDES the Material
//     diffuse (documented at this seam + on MeshData.hasColors).
//   * ccw / solid are carried verbatim from
//   getField<SFBool>(geom,"ccw"/"solid",
//     true) (the X3DComposedGeometryNode defaults).
//
// BUFFER-EXPANSION POLICY: this builder ALWAYS expands — every triangle pushes
// three fresh positions and a trivial 0..N-1 index run. That is the correct
// layout the moment ANY of {flat per-face normals, per-face color/normal,
// *PerVertex=false, per-corner texcoord with a distinct texCoordIndex} breaks
// vertex sharing — which the default (flat normals) already does. Keeping a
// single expanded path is simpler and matches T1/T2's emission contract; a
// future task may add a shared-vertex fast path off the actual *PerVertex
// flags.
//
// B4 adds LINE/POINT TOPOLOGY (MeshData.topology = Lines/Points):
// IndexedLineSet (coordIndex -1-runs -> consecutive segment pairs), LineSet
// (vertexCount partitions the coord run), PointSet (whole point array -> 0..N-1
// GL_POINTS). These carry NO normals, set solid=false, and are ALWAYS unlit
// (vertex Color / baseColor) — see the EXPLICIT CONSUMER CONTRACT on
// MeshData::Topology.
//
// T4 adds PARAMETRIC TESSELLATION of the analytic primitives that
// carry NO coord node — Box / Sphere / Cone / Cylinder — driven by
// MeshBuildOptions density (sphere 16 rings x 16 segments, cone/cylinder 24
// radial slices by default). Each is emitted EXPANDED with CCW-outward winding
// and explicit OUTWARD per-corner normals, so a primitive composes under the
// SAME cull rule as a composed mesh. (PickSystem's primitive proxies are left
// untouched: the opt-in mesh-pick upgrade is a separate deferred follow-up.)
#ifndef X3D_RUNTIME_EXTRACT_MESH_BUILDER_HPP
#define X3D_RUNTIME_EXTRACT_MESH_BUILDER_HPP

#include "AssetResolver.hpp"   // AssetResolver (binary-geometry seam)
#include "FontMetrics.hpp"     // T-TEXT: font-metrics seam (Text branch)
#include "PackedMesh.hpp"      // PackedMesh (Phase 1 binary geometry)
#include "RecursionLimits.hpp" // #21: kMaxGraphWalkVisits (walk budget default)
#include "RenderItem.hpp"      // MeshData
#include "x3d/core/X3Dtypes.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;
using x3d::nodes::X3DNode;

// ---------------------------------------------------------------------------
// GEO-PROJECTION SEAM (B5). The SDK does NO geodesy itself —
// coordinate-reference transforms (GDC/GD/UTM -> a local Cartesian frame) are
// an embedder concern. A consumer that wants geographically ANCHORED terrain
// supplies a GeoProjection through MeshBuildOptions; the builder invokes it per
// lattice vertex. With NO projection wired the builder ships a FLAT-FALLBACK
// (see geoFlatFallback below): the grid renders its SHAPE immediately but is
// geographically UNANCHORED — wrong absolute position/orientation vs a
// GeoViewpoint and mis-placed across tiles if spacing units differ. This is a
// documented shape-visible stopgap; correct placement REQUIRES the embedder
// seam.
// ---------------------------------------------------------------------------

// What the builder knows about the grid's spatial reference, handed verbatim to
// the embedder's GeoProjection so it can pick the right datum/zone transform.
struct GeoSystemDesc {
  std::vector<std::string>
      geoSystem; // raw geoSystem MFString, e.g. {"GD"} / {"UTM","Z17"}.
  SFVec3d geoGridOrigin{0.0, 0.0, 0.0}; // grid origin in geoSystem coordinates.
};

// A geo coordinate (in the grid's geoSystem) + an elevation -> a LOCAL SFVec3f.
// `geoCoord`: the per-vertex geographic coordinate the builder computed by
//   stepping geoGridOrigin by (col*xSpacing, row*zSpacing) in the geoSystem's
//   native axes (lat/long degrees for GD/GDC, easting/northing metres for UTM).
// `elevation`: the height sample (already yScale-applied) for that vertex.
// `sys`: the grid's GeoSystemDesc.
// Returns the vertex position in the geometry's LOCAL Cartesian frame.
using GeoProjection = std::function<SFVec3f(
    const SFVec3d &geoCoord, double elevation, const GeoSystemDesc &sys)>;

// Build options. Carries the parametric-tessellation density knobs for the
// analytic primitives (Box/Sphere/Cone/Cylinder, T4) AND the optional
// GeoProjection seam (B5). Stays a COPYABLE VALUE TYPE (std::function is
// copyable) so SceneExtractor can hold one by value and forward it. Defaults
// match the design: sphere 16 rings x 16 segments, cone/cylinder 24 radial
// slices, NO geo projection (flat-fallback).
struct MeshBuildOptions {
  int sphereRings = 16;    // latitude bands (pole-to-pole), >= 2.
  int sphereSegments = 16; // longitude segments around Y, >= 3.
  int radialSlices = 24;   // cone/cylinder radial subdivisions, >= 3.

  // Embedder geodesy seam (B5). Empty => flat-fallback (geoGridOrigin as planar
  // origin, spacing as planar units, geographically UNANCHORED).
  GeoProjection geoProjection{};

  // Embedder font-metrics seam (T-TEXT). The Text branch resolves glyph
  // advances
  // + atlas UVs through this callback; the SDK NEVER opens font files. Defaults
  // to makeMonospaceStub() (advanceEm=0.6, no atlas UV) so Text layout + bounds
  // are exact without any real font wired. Copyable value type (std::function).
  FontMetrics fontMetrics{makeMonospaceStub()};

  // Embedder external-geometry seam (Phase 1). When set, this is called for any
  // geometry node that buildLocalMesh returns recognized==false. An empty
  // PackedMesh (vertex_count==0) signals Pending (retry next tick); non-empty
  // signals Ready and triggers emitPacked(). Null by default (no ext, firewall
  // intact). Core-typed only: PackedMesh + X3DNode* + AssetResolver, no ext
  // type.
  std::function<PackedMesh(const X3DNode *, AssetResolver)>
      externalGeometryResolver{};

  // #21: ceiling on node-visits per fullSnapshot() walk (extractor + light
  // collection). Bounds an acyclic "doubling DAG" fan-out (2^depth paths) that
  // would otherwise emit billions of RenderItems/LightDescs. Default is far
  // above any legitimate placement count; raise it for a genuinely huge scene
  // (and check SceneExtractor::budgetExceeded() to detect truncation).
  std::size_t maxWalkVisits = kMaxGraphWalkVisits;
};

namespace mesh_detail {

// Promote an SFColor to SFColorRGBA with alpha = 1 (T3 promotion contract).
SFColorRGBA promote(const SFColor &c);

// Geometric (flat) normal of a triangle (a,b,c). Returns {0,0,0} for a
// degenerate triangle so a consumer can detect/skip it; otherwise normalized.
SFVec3f faceNormal(const SFVec3f &a, const SFVec3f &b, const SFVec3f &c);

// ---------------------------------------------------------------------------
// MSH-3: PLANAR EAR-CLIP triangulation for a NON-CONVEX polygon (IFS convex
// field FALSE). A naive fan from vertex 0 produces inverted/overlapping
// triangles on a concave polygon; ear-clipping instead emits a valid fan-free
// tessellation whose triangles all wind the same way and whose total area
// equals the polygon's.
//
// Operates in the polygon's BEST-FIT plane: we sum the Newell normal of the
// (possibly non-planar) ring, project each vertex onto two in-plane basis axes,
// and run the standard O(n^2) ear-clip in that 2D frame. The returned triples
// are LOCAL indices into `poly` (0..poly.size()-1), in CCW order with respect
// to the Newell normal, so the caller maps them back to its Corner stream and
// the existing addTriangle (which computes/flips the face normal) stays the
// single source of truth for winding/normal direction.
std::vector<std::array<int, 3>>
earClipPolygon(const std::vector<SFVec3f> &poly);

// ---------------------------------------------------------------------------
// creaseAngle SMOOTH-NORMAL POST-PASS (B6).
//
// The funnels ALWAYS emit FLAT per-face normals (the geometric normal of each
// triangle, repeated on its three corners) AND, in parallel to `positions`, a
// per-corner SOURCE index (`sourceId`): the coordIndex value for composed sets,
// the row*xDim+col / section*csCount+csVertex lattice id for the grid/extrusion
// emitters. `sourceId` is what lets us re-derive face ADJACENCY in source space
// WITHOUT re-indexing the expanded buffer — two coincident-but-distinct source
// verts (e.g. a hard UV seam authored as separate coordIndex entries) keep
// distinct ids and are NOT fused, while genuinely-shared verts are.
//
// Algorithm (runs ONLY when no Normal node is authored):
//   * Each triangle = a run of 3 consecutive expanded corners. Its flat face
//     normal is mesh.normals[base] (all three corners share it at emit time).
//   * For every source vertex, gather the list of incident triangles.
//   * For each corner of each triangle T at source vertex V, sum the face
//     normals of all triangles incident to V whose face normal is within
//     creaseAngle of T's own face normal (dihedral test: dot >=
//     cos(creaseAngle)), then normalize. A degenerate (zero-length) face normal
//     contributes nothing and a corner whose accumulated sum is zero keeps its
//     original flat normal.
//
// creaseAngle == 0 => cos(0) == 1 => only a triangle's OWN face (and any
// exactly co-planar neighbour) fuses => output is byte-identical to the flat
// fill today (NON-REGRESSIVE). creaseAngle >= PI => everything at a shared
// source vertex fuses => fully smooth.
// ---------------------------------------------------------------------------
void creaseSmoothNormals(MeshData &m,
                         const std::vector<std::uint32_t> &sourceId,
                         float creaseAngle);

// ---------------------------------------------------------------------------
// DEFAULT (implicit) TEXTURE-COORDINATE GENERATION (TC1).
//
// ISO/IEC 19775-1: when a composed/triangle geometry node carries NO
// TextureCoordinate (its texCoord field is NULL) the implicit texture
// coordinates are the NORMATIVE bounding-box projection of the geometry's coord
// points:
//   * Compute the LOCAL bbox of the points; dims = (sizeX,sizeY,sizeZ).
//   * Sdim = the axis of the LARGEST dimension, Tdim = the SECOND-largest.
//     Ties prefer X, then Y, then Z (so equal X&Y => S=X, T=Y).
//   * For an emitted vertex p:
//       s = (p[Sdim] - min[Sdim]) / size[Sdim]              (ranges 0..1)
//       t = (p[Tdim] - min[Tdim]) / size[Sdim]              (ranges
//       0..size[T]/size[S])
//     Both divided by size[Sdim] (the LARGEST dim) so the aspect ratio is
//     preserved. size[Sdim]==0 (a degenerate point/line geometry) guards to 0.
//
// Emits ONE texcoord per ALREADY-EXPANDED position (mesh.positions), so it must
// run AFTER all triangles are pushed and ONLY when no TextureCoordinate was
// authored. An authored TextureCoordinate ALWAYS WINS (this is never called in
// that case) — meshes that already carry texcoords stay byte-unchanged.
// ---------------------------------------------------------------------------
void generateDefaultTexCoords(MeshData &m);

// ---------------------------------------------------------------------------
// DEFAULT (implicit) GRID TEXTURE-COORDINATE GENERATION (TC2).
//
// ISO/IEC 19775-1: an ElevationGrid / GeoElevationGrid whose texCoord field is
// NULL generates implicit texture coordinates by the GRID PARAMETERIZATION —
// for the lattice vertex at column i (in [0,xDim-1]) and row j (in [0,zDim-1]):
//     s = i / (xDim-1),  t = j / (zDim-1).
// This is distinct from the composed-geometry bbox-projection (TC1): a height
// grid carries an explicit (i,j) parameterization, so each vertex's UV is its
// fractional position in the lattice (so the texture maps once across the whole
// grid regardless of spacing/elevation).
//
// The grid emitters expand each cell to fresh corners but retain the SOURCE
// lattice id (lid = j*xDim + i) per corner in MeshData.latticeIndex. We invert
// that here: i = lid % xDim, j = lid / xDim. Guards xDim<=1 / zDim<=1 to 0 (a
// degenerate 1-wide/1-tall grid has no extent along that axis). Emits ONE
// texcoord per ALREADY-EXPANDED position, parallel to positions; runs ONLY when
// no TextureCoordinate is authored (an authored one ALWAYS WINS).
// ---------------------------------------------------------------------------
void generateGridTexCoords(MeshData &m, int xDim, int zDim);

// Resolve an AUTHORED ElevationGrid/GeoElevationGrid TextureCoordinate per
// expanded corner. The spec indexes the texCoord `point` array by lattice
// vertex in the SAME row-major order as height (lid = j*xDim + i), so we look
// up point[lid] per corner via the retained latticeIndex. An out-of-range lid
// (a short point array) falls back to (0,0). Returns whether it populated
// texcoords (false => caller falls through to generateGridTexCoords).
bool resolveGridTexCoords(MeshData &m, const std::vector<SFVec2f> &point);

// Resolved per-emit-corner attribute carrier. coord    = index into the coord
// `point` array (already validated by ok()); pos       = the POSITIONAL index
// of this corner within the index stream (for *Index lookups / per-vertex
// non-indexed order); face      = the running FACE/primitive ordinal (per-face
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
  //   perVertex=false: PER-FACE — pick attr[ attrIndex[corner.face] ] if
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

// ---------------------------------------------------------------------------
// Analytic primitive tessellation (T4): Box / Sphere / Cone / Cylinder.
//
// Every primitive is emitted EXPANDED (three fresh positions + a trivial 0..N-1
// index run per triangle) with EXPLICIT per-corner normals, matching the
// composed-geometry funnel's layout. Winding is CCW as seen from OUTSIDE the
// surface, and the stored normals point OUTWARD, so a primitive composes under
// the exact same cull rule as an extracted mesh (front = CCW, solid => cull
// back). ccw/solid are carried verbatim by buildLocalMesh from the node fields
// (primitive defaults solid=true).
// ---------------------------------------------------------------------------

// Push one CCW triangle (a,b,c) with its three already-outward corner normals
// and its three per-corner DEFAULT texcoords (TC4). The analytic primitives
// carry NO texCoord field, so the spec's per-primitive UV mapping is ALWAYS
// generated here in lockstep with positions/normals (one texcoord per corner,
// parallel to positions).
void emitTri(MeshData &m, const SFVec3f &a, const SFVec3f &b, const SFVec3f &c,
             const SFVec3f &na, const SFVec3f &nb, const SFVec3f &nc,
             const SFVec2f &ta, const SFVec2f &tb, const SFVec2f &tc);

SFVec3f normalize(const SFVec3f &v);

// TXC-1: default longitudinal texture S for the analytic primitives. ISO/IEC
// 19775-1 §13.3.2/3/7 place the vertical texture seam at the BACK (X=0 plane,
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

// ---------------------------------------------------------------------------
// LATTICE-GRID EMITTER (B5). Shared by ElevationGrid and GeoElevationGrid.
//
// A height grid is a regular xDim x zDim lattice of vertices; cell (i,j) is
// split into two triangles (v00,v11,v10)+(v00,v01,v11). That winding is
// CCW as seen from the +Y side, so with the default ccw=TRUE the surface is
// front-facing and its generated normal points UP (+Y) per ISO/IEC 19775-1
// §13.3.4 — a consumer's default back-face cull (solid=TRUE) keeps the terrain
// when viewed from above. The emitter EXPANDS each corner (three fresh
// positions per triangle) BUT ALSO records, per expanded corner, the id of the
// source LATTICE vertex (row*xDim+col) into MeshData.latticeIndex. That
// retained map is what B6 needs to weld creaseAngle-smooth normals in source
// space without re-indexing.
//
// `pos(i,j)` supplies the LOCAL-frame position of lattice vertex (col=i,
// row=j); it is the ONLY thing that differs between a planar ElevationGrid and
// a geo-projected GeoElevationGrid. Caller pre-validates xDim>=2 && zDim>=2.
// ---------------------------------------------------------------------------
template <class PosFn>
inline void emitHeightGrid(MeshData &m, int xDim, int zDim, PosFn pos,
                           float creaseAngle = 0.0f, bool ccw = true) {
  const std::size_t startCorner = m.positions.size();
  auto lid = [&](int i, int j) {
    return static_cast<std::uint32_t>(j * xDim + i);
  };
  const auto addTriangle = [&](int ia, int ja, int ib, int jb, int ic, int jc) {
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
    // GENERATED normal: ccw=FALSE reverses its direction (geometry3D.md §13.3.4
    // — "Setting the ccw field to FALSE reverses the normal direction"). Done
    // by negation so the lattice/index order is preserved.
    SFVec3f n = faceNormal(a, b, c);
    if (!ccw)
      n = SFVec3f{-n.x, -n.y, -n.z};
    m.normals.push_back(n);
    m.normals.push_back(n);
    m.normals.push_back(n);
  };
  for (int j = 0; j + 1 < zDim; ++j)
    for (int i = 0; i + 1 < xDim; ++i) {
      addTriangle(i, j, i + 1, j + 1, i + 1, j); // v00, v11, v10
      addTriangle(i, j, i, j + 1, i + 1, j + 1); // v00, v01, v11
    }
  // creaseAngle smoothing (B6) keyed on the SOURCE LATTICE id (row*xDim+col),
  // not the expanded 0..N-1 run. creaseAngle==0 => byte-identical flat output.
  (void)startCorner; // grids own the whole mesh; smoothing reads it wholesale.
  if (creaseAngle > 0.0f)
    creaseSmoothNormals(m, m.latticeIndex, creaseAngle);
  m.hasNormals = !m.normals.empty();
}

// ---------------------------------------------------------------------------
// EXTRUSION (B3). Sweep a 2D crossSection along a 3D spine, building a
// Spine-aligned Cross-section Coordinate (SCP) frame at every spine point per
// the X3D spec (19.4.5 / 13.3.5):
//   * SCPyAxis = the spine TANGENT — for an interior point the normalized
//     average of the incoming and outgoing edge directions; for the endpoints
//     of an OPEN spine the single adjacent edge; for the endpoints of a CLOSED
//     spine (first==last) the wrap-around average. A spine that is collinear or
//     has coincident points yields a degenerate tangent/Z — those sections
//     INHERIT the previous section's SCP axes (spec rule).
//   * SCPzAxis = derived from the spine's local plane: for an interior point
//   the
//     normal of the plane through (prev, this, next); seeded from the first
//     valid interior section and propagated to coincident/collinear sections.
//     A sign flip (Z·prevZ < 0) is corrected so the frame does not invert.
//   * SCPxAxis = SCPyAxis × SCPzAxis.
// Each crossSection vertex (cx, cy) maps into the SCP plane as
//   cx along SCPxAxis, cy along SCPzAxis (the X3D cross-section X-Z plane),
// THEN per-section scale (x->X, y->Z), THEN per-section orientation (a rotation
// about an arbitrary axis), THEN translate to the spine point.
//
// scale/orientation are SF-or-MF: length 1 applies to every section, otherwise
// the value is clamped to the last for sections beyond the list end.
//
// Side walls are quad bands between consecutive sections (2 tris each, FLAT
// normals — creaseAngle smoothing is B6). beginCap/endCap are fans over the
// (closed) cross-section at the first/last spine point. Winding honors ccw.
//
// CRITICAL for B6: a (section, crossSectionVertex) -> expanded-vertex lattice
// map is retained on MeshData.latticeIndex (lid = section*csCount + csVertex),
// so B6 can re-derive adjacency in source space WITHOUT re-indexing. <2 spine
// points => empty.
//
// TC3 adds DEFAULT (implicit) TEXTURE-COORDINATE generation (genTexCoords).
// Extrusion has NO authored TextureCoordinate field — texcoords are implicit-
// only per ISO/IEC 19775-1, so the dispatcher always requests them. SIDE walls:
// S = accumulated cross-section CHORD length / perimeter, T = accumulated SPINE
// length / total spine length. begin/end CAPS: the cross-section's own 2D
// coords normalized to the cross-section bbox. Emitted per expanded corner,
// parallel to positions (and reordered in lockstep with the ccw winding swap).
// ---------------------------------------------------------------------------

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
// knows how to tessellate (B2). This is the static type-coverage oracle: it
// does NOT imply the mesh is non-empty (a recognized IFS with an empty
// coordIndex tessellates to nothing — legitimately empty, NOT unsupported). The
// two sets (analytic-no-coord + coord-fed) mirror the dispatch arms in
// buildLocalMesh; keep them in lockstep when a new geometry arm lands (B3
// Extrusion/Text/... flips a name from absent->present here as it becomes
// drawable).
bool recognizedGeometryType(const std::string &t);

// buildLocalMesh — extract LOCAL-frame triangle geometry from `geom`.
//
// Returns a MeshData whose `positions` are the triangle corners in emission
// order and whose `indices` are a trivial 0..N-1 run (one index per emitted
// position, triples form triangles). normals/colors/texcoords (when present)
// are parallel to positions — same length, one per corner. When no Normal node
// is authored the builder fills `normals` with flat per-face normals so a
// consumer always has shading data. Unsupported types return an empty mesh.
//
// B2 observability: the optional `recognized` out-param is set to whether
// `geom`'s nodeTypeName is a type this builder knows how to tessellate
// (recognizedGeometry Type). It DISTINGUISHES an UNRECOGNIZED type (unknown
// geometry node, e.g. an Extrusion still unsupported until B3 —
// recognized=false) from a recognized type that legitimately produced no
// triangles (IFS empty coordIndex, a degenerate ElevationGrid grid —
// recognized=true, indices empty). The caller (SceneExtractor) uses it to count
// ONLY genuinely-unsupported drops, never legitimately-empty ones.
MeshData buildLocalMesh(const X3DNode *geom, const MeshBuildOptions &opt = {},
                        bool *recognized = nullptr);

} // namespace x3d::runtime::extract
#endif
