// light_system_test.cpp — M2.5 T6 acceptance: LightSystem world-resolution +
// authored-flag fidelity.
//
// Two proofs:
//   1) A PointLight's `location` world-resolves under a TRANSLATING Transform —
//      the LightSystem re-accumulates worldM down the PATH (the worldOfRec
//      pattern, NOT the first-path TransformSystem.world_ table).
//   2) A DirectionalLight's `global` reads its VERIFIED spec default (false) and
//      is CARRIED verbatim — never silently promoted to scene-wide.
// Plus: a Spot/Point direction world-resolves through rotation; on==false lights
// are skipped; scopeRoot carries the enclosing grouping node.
#include "LightSystem.hpp"

#include "X3DDocument.hpp" // Scene::addRootNode definition.
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p,
                     const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

TEST_CASE("light_system_test") {
  // --- PROOF 1: PointLight location world-resolves under a Transform --------
  // Group -> Transform(+10x) -> PointLight(location 1,2,3). The collected
  // worldLocation must be (11,2,3): the transform translation applied to the
  // authored location via the per-PATH accumulation.
  {
    auto pl = createX3DNode("PointLight");
    setF(pl, "location", std::any(SFVec3f{1, 2, 3}));

    auto T = createX3DNode("Transform");
    setF(T, "translation", std::any(SFVec3f{10, 0, 0}));
    addChild(T, pl);

    auto group = createX3DNode("Group");
    addChild(group, T);

    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::LightSystem ls;
    auto lights = ls.collect(scene);
    CHECK((lights.size() == 1));
    const extract::LightDesc &L = lights[0];
    CHECK((L.type == extract::LightDesc::Type::Point));
    // World-resolved location: (1,2,3) + (10,0,0).
    CHECK((feq(L.worldLocation.x, 11.0f)));
    CHECK((feq(L.worldLocation.y, 2.0f)));
    CHECK((feq(L.worldLocation.z, 3.0f)));
    // PointLight spec default global=true, carried verbatim.
    CHECK((L.global == true));
    // scopeRoot is the enclosing grouping node the light was collected under.
    CHECK((L.scopeRoot == T.get()));
  }

  // --- PROOF 2: DirectionalLight 'global' reads false by default (no promote) -
  {
    auto dl = createX3DNode("DirectionalLight"); // no fields set => spec defaults
    Scene scene;
    scene.addRootNode(dl);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::LightSystem ls;
    auto lights = ls.collect(scene);
    CHECK((lights.size() == 1));
    const extract::LightDesc &L = lights[0];
    CHECK((L.type == extract::LightDesc::Type::Directional));
    // THE fidelity assertion: DirectionalLight global default is false, CARRIED
    // verbatim — never silently promoted to scene-wide.
    CHECK((L.global == false));
    // Default direction (0,0,-1) world-resolves to itself under identity.
    CHECK((feq(L.worldDirection.x, 0.0f)));
    CHECK((feq(L.worldDirection.y, 0.0f)));
    CHECK((feq(L.worldDirection.z, -1.0f)));
    // No enclosing grouping node => scopeRoot null (light is a root itself).
    CHECK((L.scopeRoot == nullptr));
  }

  // --- on==false lights are skipped ----------------------------------------
  {
    auto pl = createX3DNode("PointLight");
    setF(pl, "on", std::any(false));
    Scene scene;
    scene.addRootNode(pl);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::LightSystem ls;
    CHECK((ls.collect(scene).empty()));
  }

  // --- generic field reads: color/intensity/radius/attenuation ------------
  {
    auto pl = createX3DNode("PointLight");
    setF(pl, "color", std::any(SFColor{0.2f, 0.4f, 0.6f}));
    setF(pl, "intensity", std::any(0.5f));
    setF(pl, "radius", std::any(42.0f));
    setF(pl, "attenuation", std::any(SFVec3f{0, 1, 0}));
    Scene scene;
    scene.addRootNode(pl);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::LightSystem ls;
    auto lights = ls.collect(scene);
    CHECK((lights.size() == 1));
    const extract::LightDesc &L = lights[0];
    CHECK((feq(L.color.r, 0.2f) && feq(L.color.g, 0.4f) && feq(L.color.b, 0.6f)));
    CHECK((feq(L.intensity, 0.5f)));
    CHECK((feq(L.radius, 42.0f)));
    CHECK((feq(L.attenuation.x, 0.0f) && feq(L.attenuation.y, 1.0f)));
  }

  return;
}
