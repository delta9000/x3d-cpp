// RoundTripOracle.hpp — shared by the import round-trip tests: export an X3D
// scene to glb with the SDK's writer, import it with a chosen backend, run both
// documents through one tick schedule and compare each frame's world-space
// triangles (with surface colour), lights and bound camera.
#ifndef X3D_IMPORT_TESTS_ROUND_TRIP_ORACLE_HPP
#define X3D_IMPORT_TESTS_ROUND_TRIP_ORACLE_HPP

#include "x3d/sdk.hpp"

#include "doctest/doctest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

namespace roundtrip {

namespace sdk = x3d::sdk;
namespace imp = x3d::runtime::importer;
namespace fs = std::filesystem;


struct Tri {
  std::array<x3d::core::SFVec3f, 3> p;
  std::array<float, 4> colour; ///< surface colour and opacity
};

struct Frame {
  std::vector<Tri> tris;
  std::vector<sdk::LightDesc> lights;
  sdk::CameraDesc camera;
  std::size_t lineSegments = 0;
  std::size_t points = 0;
};

inline std::array<float, 4> colourOf(const sdk::MaterialDesc &m) {
  x3d::core::SFColor c = m.model == sdk::MaterialModel::Physical ? m.physical.baseColor
                         : m.model == sdk::MaterialModel::Unlit  ? m.emissive
                                                                 : m.phong.diffuse;
  return {c.r, c.g, c.b, 1.0f - m.transparency};
}

inline Frame frameOf(sdk::RuntimeSession &s) {
  Frame f;
  auto &ex = s.extractor();
  const auto snap = ex.fullSnapshot();
  for (auto id : snap.added) {
    const auto &item = ex.item(id);
    const sdk::MeshData mesh = item.skin ? ex.deformedMesh(id) : *item.mesh;
    if (mesh.topology == sdk::Topology::Lines) {
      f.lineSegments += mesh.indices.size() / 2;
      continue;
    }
    if (mesh.topology == sdk::Topology::Points) {
      f.points += mesh.positions.size();
      continue;
    }
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
      Tri t;
      for (int k = 0; k < 3; ++k)
        t.p[static_cast<std::size_t>(k)] =
            item.worldTransform.transformPoint(mesh.positions[mesh.indices[i + static_cast<std::size_t>(k)]]);
      t.colour = colourOf(item.material);
      f.tris.push_back(t);
    }
  }
  f.lights = ex.lights();
  f.camera = ex.camera();
  return f;
}

inline float dist(const x3d::core::SFVec3f &a, const x3d::core::SFVec3f &b) {
  return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

inline bool sameTri(const Tri &a, const Tri &b, float tol) {
  for (int c = 0; c < 4; ++c)
    if (std::fabs(a.colour[static_cast<std::size_t>(c)] - b.colour[static_cast<std::size_t>(c)]) > 2e-3f) return false;
  for (int r = 0; r < 3; ++r) { // same winding, any starting corner
    bool all = true;
    for (int k = 0; k < 3 && all; ++k)
      all = dist(a.p[static_cast<std::size_t>(k)], b.p[static_cast<std::size_t>((k + r) % 3)]) <= tol;
    if (all) return true;
  }
  return false;
}

/// Every triangle of `a` matches a distinct triangle of `b`.
inline void checkSameTriangles(const Frame &a, const Frame &b, float tol) {
  REQUIRE(a.tris.size() == b.tris.size());
  std::vector<bool> used(b.tris.size(), false);
  std::size_t unmatched = 0;
  for (const Tri &t : a.tris) {
    bool found = false;
    for (std::size_t j = 0; j < b.tris.size() && !found; ++j)
      if (!used[j] && sameTri(t, b.tris[j], tol)) used[j] = found = true;
    if (!found) ++unmatched;
  }
  CHECK(unmatched == 0);
}

inline void checkSameLights(const Frame &a, const Frame &b) {
  REQUIRE(a.lights.size() == b.lights.size());
  auto key = [](const sdk::LightDesc &l) {
    return std::make_tuple(static_cast<int>(l.type), l.worldLocation.x, l.worldLocation.y, l.worldLocation.z,
                           l.worldDirection.x, l.worldDirection.y, l.worldDirection.z);
  };
  auto la = a.lights, lb = b.lights;
  auto order = [&](const sdk::LightDesc &x, const sdk::LightDesc &y) { return key(x) < key(y); };
  std::sort(la.begin(), la.end(), order);
  std::sort(lb.begin(), lb.end(), order);
  for (std::size_t i = 0; i < la.size(); ++i) {
    CAPTURE(i);
    CHECK(la[i].type == lb[i].type);
    CHECK(dist(la[i].worldLocation, lb[i].worldLocation) < 1e-3f);
    CHECK(dist(la[i].worldDirection, lb[i].worldDirection) < 1e-3f);
    CHECK(la[i].color.r == doctest::Approx(lb[i].color.r).epsilon(1e-3));
    CHECK(la[i].color.g == doctest::Approx(lb[i].color.g).epsilon(1e-3));
    CHECK(la[i].color.b == doctest::Approx(lb[i].color.b).epsilon(1e-3));
    CHECK(la[i].intensity == doctest::Approx(lb[i].intensity).epsilon(1e-3));
    if (la[i].type == sdk::LightDesc::Type::Spot) {
      CHECK(la[i].cutOffAngle == doctest::Approx(lb[i].cutOffAngle).epsilon(1e-3));
      CHECK(la[i].beamWidth == doctest::Approx(lb[i].beamWidth).epsilon(1e-3));
    }
  }
}

inline void checkSameCamera(const Frame &a, const Frame &b) {
  for (int i = 0; i < 16; ++i)
    CHECK(a.camera.viewMatrix.m[i] == doctest::Approx(b.camera.viewMatrix.m[i]).epsilon(1e-3));
  CHECK(a.camera.fieldOfView == doctest::Approx(b.camera.fieldOfView).epsilon(1e-3));
  CHECK(a.camera.ortho == b.camera.ortho);
}

struct RoundTrip {
  sdk::X3DDocument original;
  imp::ImportResult imported;
  sdk::GltfWriteReport exportReport;
};

inline RoundTrip roundTrip(const std::string &body, const std::string &name,
                           const imp::BackendRegistry &registry = imp::builtinBackends(),
                           const std::string &backend = "") {
  const std::string text = "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
  RoundTrip rt;
  rt.original = sdk::parseDocument(text, sdk::Encoding::XML);
  sdk::X3DDocument copy = sdk::parseDocument(text, sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(copy.scene);
  ctx.buildFrom(copy.scene);
  sdk::SceneExtractor ex(ctx, copy.scene);
  const auto frame = ex.fullSnapshot();
  // An empty export would make the whole round trip vacuous.
  REQUIRE(!frame.added.empty());
  const std::string glb = sdk::writeGlb(ex, frame.added, {}, &rt.exportReport);
  const fs::path dir = fs::temp_directory_path() / "x3d_import_roundtrip";
  fs::create_directories(dir);
  const fs::path file = dir / (name + ".glb");
  std::ofstream(file, std::ios::binary) << glb;
  imp::ImportOptions opts;
  opts.outputPath = (dir / (name + ".x3d")).string();
  opts.autoplay = imp::Autoplay::All;
  opts.backend = backend;
  rt.imported = imp::importFile(file.string(), opts, registry);
  return rt;
}

/// Runs both documents through the same tick schedule and compares frames.
inline void compareOver(RoundTrip &rt, const std::vector<double> &times, float tol = 2e-3f) {
  // Re-serialise the import so the comparison also covers the XML writer
  // and reader.
  const std::string xml = sdk::XmlWriter{}.writeDocument(rt.imported.document);
  auto a = sdk::RuntimeSession::create(std::move(rt.original));
  auto b = sdk::RuntimeSession::create(sdk::parseDocument(xml, sdk::Encoding::XML));
  CHECK(b->routes().rejected.empty());
  for (double t : times) {
    CAPTURE(t);
    a->tick(t);
    b->tick(t);
    const Frame fa = frameOf(*a), fb = frameOf(*b);
    // A source that renders nothing would make every comparison vacuous.
    REQUIRE(!fa.tris.empty());
    checkSameTriangles(fa, fb, tol);
    CHECK(fa.lineSegments == fb.lineSegments);
    CHECK(fa.points == fb.points);
    checkSameLights(fa, fb);
    checkSameCamera(fa, fb);
  }
}

} // namespace roundtrip

#endif // X3D_IMPORT_TESTS_ROUND_TRIP_ORACLE_HPP
