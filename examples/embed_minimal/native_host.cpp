// A renderer-free, installed-package host integration contract. This is a CPU
// mirror of the render feed, not a Vulkan renderer or a CAVEOS adapter.
#include "x3d/sdk.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdk = x3d::sdk;
using namespace x3d::core;

namespace {
void require(bool ok, const std::string &message) {
  if (!ok) throw std::runtime_error(message); // also checked in Release/NDEBUG
}
bool near(float a, float b) { return std::abs(a - b) < 0.00001f; }

// Host-owned command queue: all writes occur INSIDE tick(), after the SDK has
// cleared the previous tick's dirty set. Retain node ownership until delivery.
// A System may update multiple times during one event cascade; drain once.
class HostWrites final : public sdk::System {
public:
  struct Write { std::shared_ptr<sdk::X3DNode> node; std::string field; std::any value; };
  std::vector<Write> pending;
  std::vector<sdk::FieldWriteResult> results;
  void attach(sdk::X3DNode *, sdk::X3DExecutionContext &) override {}
  void update(double, sdk::X3DExecutionContext &ctx) override {
    auto writes = std::move(pending);
    pending.clear();
    for (auto &w : writes)
      results.push_back(ctx.writeField(w.node.get(), w.field, std::move(w.value)));
  }
  void enqueue(std::shared_ptr<sdk::X3DNode> node, std::string field, std::any value) {
    pending.push_back({std::move(node), std::move(field), std::move(value)});
  }
  void check() {
    for (auto r : results)
      require(r == sdk::FieldWriteResult::Ok, sdk::fieldWriteResultName(r));
    results.clear();
  }
};

// Borrowed RenderItem references never escape extraction. Copy the fields the
// renderer needs; immutable mesh/pixel payloads can be retained by shared_ptr.
// The path/node pointers below are opaque IN-PROCESS comparison keys only.
struct Placement {
  sdk::PathKey path;
  sdk::Mat4 world;
  sdk::GeomId geometry;
  std::shared_ptr<const sdk::MeshData> mesh;
  sdk::MaterialDesc material;
};
Placement copy(const sdk::RenderItem &item) {
  require(!item.skin && !item.geometry_ext.is_packed(), "fixture uses static AoS geometry");
  return {item.path, item.worldTransform, item.geometry, item.mesh, item.material};
}
struct HostMirror {
  std::map<sdk::RenderItemId, Placement> placements;
  std::uint64_t epoch = 0; // host-local namespace, NOT an SDK or wire identifier
  std::uint64_t frame = 0;
  bool baselineValid = false;

  void apply(const sdk::RenderDelta &d, const sdk::SceneExtractor &ex) {
    if (ex.budgetExceeded()) {
      baselineValid = false;
      throw std::runtime_error("partial extraction rejected; fresh snapshot required");
    }
    require(baselineValid, "no accepted baseline; fresh snapshot required");
    // Any copy/allocation/check failure leaves this frame unpublished. The
    // caller must not present placements until a fresh snapshot is accepted.
    baselineValid = false;
    // A topology rebuild may REMOVE and ADD the same numeric id in one delta.
    if (!d.removed.empty()) ++epoch; // conservatively invalidate external ID caches
    for (auto id : d.removed) placements.erase(id);
    for (auto id : d.added) placements.insert_or_assign(id, copy(ex.item(id)));
    auto update = [&](const std::vector<sdk::RenderItemId> &ids) {
      for (auto id : ids) {
        if (std::find(d.removed.begin(), d.removed.end(), id) != d.removed.end() &&
            std::find(d.added.begin(), d.added.end(), id) == d.added.end()) continue;
        require(placements.count(id) != 0, "delta updates unknown placement " + std::to_string(id) + " at host frame " + std::to_string(frame));
        placements.insert_or_assign(id, copy(ex.item(id)));
      }
    };
    update(d.updatedTransform); update(d.updatedGeometry); update(d.updatedMaterial);
    require(d.updatedSkinPose.empty(), "fixture has no skin");
    // Camera/light/fog/background pull surfaces belong to the host too. This
    // fixture has static lighting; a renderer must honor their change flags.
    if (d.cameraChanged) (void)ex.camera();
    if (d.lightsChanged) (void)ex.lights();
    if (d.backgroundChanged) (void)ex.background();
    ++frame;
    baselineValid = true;
  }
  void replace(sdk::RuntimeSession &session) {
    baselineValid = false;
    const auto snapshot = session.fullSnapshot();
    if (session.extractor().budgetExceeded()) {
      baselineValid = false;
      throw std::runtime_error("partial snapshot rejected; fresh snapshot required");
    }
    ++epoch;
    placements.clear(); // a snapshot is authoritative; old dense ids are invalid
    baselineValid = true;
    apply(snapshot, session.extractor());
  }
  void unload() {
    ++epoch;
    baselineValid = false;
    placements.clear(); // real GPU objects retire after the host's fences signal
  }
};

bool sameMesh(const sdk::MeshData &a, const sdk::MeshData &b) {
  return a.positions == b.positions && a.indices == b.indices &&
         a.normals == b.normals && a.texcoords == b.texcoords &&
         a.topology == b.topology && a.solid == b.solid && a.ccw == b.ccw;
}
void agreesWithSnapshot(const HostMirror &host, sdk::RuntimeSession &s) {
  // A separate extractor is the oracle: it cannot overwrite the incremental
  // extractor's caches or baseline. Compare by path, not independent dense ids.
  sdk::SceneExtractor oracle(s.context(), s.scene());
  const auto snapshot = oracle.fullSnapshot();
  require(snapshot.added.size() == host.placements.size(), "snapshot/delta placement count");
  for (auto id : snapshot.added) {
    const auto &expected = oracle.item(id);
    auto found = std::find_if(host.placements.begin(), host.placements.end(),
        [&](const auto &p) { return p.second.path == expected.path; });
    require(found != host.placements.end(), "snapshot/delta path identity");
    const auto &actual = found->second;
    for (int i = 0; i != 16; ++i)
      require(near(actual.world.m[i], expected.worldTransform.m[i]), "snapshot/delta transform");
    require(actual.geometry.node == expected.geometry.node, "snapshot/delta geometry identity");
    require(sameMesh(*actual.mesh, *expected.mesh), "snapshot/delta mesh content");
    require(actual.material.model == expected.material.model &&
            actual.material.phong.diffuse == expected.material.phong.diffuse &&
            near(actual.material.transparency, expected.material.transparency),
            "snapshot/delta material");
  }
}

// Explicit deny resolvers prevent the parse path's DEFAULT local-file loaders
// from running. A real host substitutes its own confined synchronous cache.
sdk::X3DDocument parseMemory(const std::string &text) {
  return sdk::parseDocument(text, sdk::Encoding::Unknown, "mem:/scene.x3d",
      [](const auto &, const auto &) -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
        return nullptr;
      },
      [](const auto &, const auto &) -> std::shared_ptr<sdk::Scene> { return nullptr; });
}
const char *sceneText = R"(<X3D profile='Immersive' version='4.0'><Scene>
  <Viewpoint DEF='View' position='0 0 0'/>
  <Group DEF='Root'>
    <Transform DEF='Left' translation='-3 0 0'>
      <Shape DEF='A'><Appearance DEF='Appearance'><Material DEF='Material'/></Appearance>
        <IndexedFaceSet DEF='Mesh' coordIndex='0 1 2 -1'>
          <Coordinate DEF='Points' point='0 0 0 1 0 0 0 1 0'/>
        </IndexedFaceSet>
      </Shape>
    </Transform>
    <Transform DEF='Right' translation='3 0 0'>
      <Shape DEF='B'><Appearance USE='Appearance'/><IndexedFaceSet USE='Mesh'/></Shape>
    </Transform>
  </Group>
  <KeySensor DEF='Keys'/>
  <TimeSensor DEF='Clock' cycleInterval='4' loop='true'/>
  <PositionInterpolator DEF='Motion' key='0 1' keyValue='-3 0 0 -1 0 0'/>
  <ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Motion' toField='set_fraction'/>
  <ROUTE fromNode='Motion' fromField='value_changed' toNode='Left' toField='set_translation'/>
</Scene></X3D>)";

void exerciseLifecycle() {
  HostMirror host;
  std::shared_ptr<const sdk::MeshData> retainedMesh;
  for (int load = 0; load != 3; ++load) {
    auto session = sdk::RuntimeSession::create(parseMemory(sceneText));
    require(session->routes().routesAdded == 2 && session->routes().rejected.empty(), "ROUTE wiring");
    auto writes = std::make_shared<HostWrites>();
    session->context().addSystem(writes);
    host.replace(*session);
    require(host.placements.size() == 2, "two shared-geometry placements");
    const auto first = host.placements.begin(), second = std::next(first);
    require(first->first != second->first && first->second.path != second->second.path,
            "placement ids and paths are distinct");
    require(first->second.geometry == second->second.geometry &&
            first->second.mesh == second->second.mesh, "mesh is shared, placement is not");
    retainedMesh = first->second.mesh;
    agreesWithSnapshot(host, *session);

    auto advance = [&](double now) {
      const auto generation = session->context().tickGeneration();
      session->tick(now);
      require(session->context().tickGeneration() == generation + 1, "host drives every tick");
      writes->check();
      auto d = session->delta();
      host.apply(d, session->extractor());
      agreesWithSnapshot(host, *session);
      return d;
    };
    require(session->context().writeField(nullptr, "translation", SFVec3f{}) ==
            sdk::FieldWriteResult::NullNode, "null write is explicit");
    require(session->context().writeField(session->scene().resolve("Left").get(),
            "translation", SFString{"wrong type"}) == sdk::FieldWriteResult::TypeMismatch,
            "wrong-type write is explicit and atomic");
    require(session->context().writeField(session->scene().resolve("Left").get(),
            "notAField", SFVec3f{}) == sdk::FieldWriteResult::UnknownField,
            "unknown-field write is explicit");
    advance(0);
    auto motion = advance(1);
    require(!motion.updatedTransform.empty(), "standard runtime animates authored ROUTEs");
    require(host.placements.begin()->second.mesh == retainedMesh,
            "TRS-only ticks reuse immutable mesh payloads");
    session->context().setHeadPose({0, 0, 5}, {0, 1, 0, 0});
    session->context().pushKeyCharacter("k", true);
    session->context().setPointer({{0, 0, 5}, {0, 0, -1}});
    session->context().setPointerPresent(true);
    session->context().setPointerButton(false);
    advance(1); // repeated timestamp is a distinct host tick
    require(near(session->context().cameraWorldPosition().z, 5),
            "host tracking pose composes with the bound viewpoint");
    auto keys = session->scene().resolve("Keys");
    bool keyDelivered = false;
    for (const auto &f : keys->fields())
      if (f.x3dName == "keyPress") keyDelivered = std::any_cast<SFString>(f.get(*keys)) == "k";
    require(keyDelivered, "host character input reaches KeySensor");

    writes->enqueue(session->scene().resolve("Points"), "point",
                    MFVec3f{{0,0,0}, {2,0,0}, {0,2,0}});
    const auto geometry = advance(1);
    require(geometry.updatedGeometry.size() == 2, "shared geometry updates both placements");
    require(retainedMesh->positions != host.placements.begin()->second.mesh->positions,
            "retained immutable old mesh survives replacement");
    writes->enqueue(session->scene().resolve("Material"), "diffuseColor", SFColor{1,0,0});
    const auto material = advance(1);
    require(material.updatedMaterial.size() == 2, "shared material updates both placements");

    // A geometry-node replacement changes content without changing Shape path.
    auto replacementDoc = parseMemory("<X3D version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>");
    std::shared_ptr<sdk::X3DNode> replacement;
    for (const auto &f : replacementDoc.scene.rootNodes.front()->fields())
      if (f.x3dName == "geometry") replacement = std::any_cast<std::shared_ptr<sdk::X3DNode>>(f.get(*replacementDoc.scene.rootNodes.front()));
    require(replacement != nullptr, "replacement geometry exists");
    auto sharedBeforeStructure = host.placements.rbegin()->second.mesh;
    writes->enqueue(session->scene().resolve("A"), "geometry", replacement);
    advance(1);
    auto sharedAfterStructure = host.placements.rbegin()->second.mesh;
    require(sharedBeforeStructure != sharedAfterStructure &&
            sameMesh(*sharedBeforeStructure, *sharedAfterStructure),
            "structural baseline rebuilds even unchanged mesh payloads");

    writes->enqueue(session->scene().resolve("A"), "geometry", std::shared_ptr<sdk::X3DNode>{});
    const auto removed = advance(1);
    require(removed.removed.size() == 2 && host.placements.size() == 1, "null geometry removes placement");
    writes->enqueue(session->scene().resolve("A"), "geometry", replacement);
    const auto added = advance(1);
    require(!added.added.empty() && host.placements.size() == 2, "geometry reattachment adds placement");

    // Simulate a host that lost its render cache: a full snapshot replaces it.
    host.placements.clear();
    host.replace(*session);
    agreesWithSnapshot(host, *session);
    auto duplicate = session->delta();
    require(duplicate.added.empty() && duplicate.removed.empty() &&
            duplicate.updatedTransform.empty() && duplicate.updatedGeometry.empty(), "duplicate delta is empty");

    // Failed prepare must not replace a working session.
    bool failed = false;
    try { auto rejected = sdk::RuntimeSession::create(parseMemory("not an X3D document")); }
    catch (const std::exception &) { failed = true; }
    require(failed && host.placements.size() == 2, "failed load preserves old host state");
    host.unload();
    session.reset();
    require(host.placements.empty() && !retainedMesh->positions.empty(), "unload clears ids; retained mesh owns bytes");
  }

}

void exerciseResourcesAndDiagnostics() {
  int deniedIncludes = 0;
  auto doc = sdk::parseDocument(R"(<X3D version='4.0'><Scene>
    <Inline url='"unapproved.x3d"'/>
    <TimeSensor DEF='Clock'/><Transform DEF='Target'/>
    <ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Target' toField='set_translation'/>
  </Scene></X3D>)", sdk::Encoding::XML, "mem:/scene.x3d",
      [](const auto &, const auto &) -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
        return nullptr;
      },
      [&](const auto &, const auto &) -> std::shared_ptr<sdk::Scene> {
        ++deniedIncludes;
        return nullptr;
      });
  require(deniedIncludes != 0, "host explicitly rejects external scene resolution");
  auto denied = sdk::RuntimeSession::create(std::move(doc));
  require(!denied->routes().rejected.empty(), "parse success does not imply valid ROUTEs");

  struct Cache { bool ready = false; int deniedUrls = 0; int decoded = 0; int assets = 0; };
  auto cache = std::make_shared<Cache>();
  sdk::SessionOptions options;
  options.textureResolver = [cache](const std::string &url) {
    if (url != "mem:pixel") {
      ++cache->deniedUrls;
      return sdk::TexturePixelResult::makeFailed();
    }
    if (!cache->ready) return sdk::TexturePixelResult::makePending();
    ++cache->decoded;
    return sdk::TexturePixelResult::makeReady(sdk::TexturePixels{1, 1, {255, 0, 0, 255}});
  };
  options.assetResolver = [cache](const std::string &url, sdk::AssetKind) {
    ++cache->assets;
    return url == "mem:pixel" && cache->ready ? sdk::AssetResult::makeReady({1})
                                             : sdk::AssetResult::makeFailed();
  };
  auto textured = sdk::RuntimeSession::create(parseMemory(R"(<X3D version='4.0'><Scene>
    <Shape><Appearance DEF='App'><Material/>
      <ImageTexture DEF='Tex' url='"unapproved.png" "mem:pixel"'/>
    </Appearance><Box DEF='Box'/></Shape>
    <Transform translation='3 0 0'><Shape><Appearance USE='App'/><Box USE='Box'/></Shape></Transform>
    <LoadSensor><ImageTexture USE='Tex' containerField='children'/></LoadSensor>
  </Scene></X3D>)"), std::move(options));
  HostMirror host;
  host.replace(*textured);
  require(host.placements.size() == 2, "textured fixture placements");
  require(host.placements.begin()->second.material.textures.front().resolvedPixels.pending(),
          "pending texture is explicit");
  cache->ready = true; // the host's resource work completed outside the SDK
  textured->tick(0);
  host.apply(textured->delta(), textured->extractor());
  // Completion is a deliberate rebaseline. Pending does not schedule work and
  // an unchanged material need not be re-extracted by delta().
  host.replace(*textured);
  const auto &a = host.placements.begin()->second.material.textures.front().resolvedPixels;
  const auto &b = std::next(host.placements.begin())->second.material.textures.front().resolvedPixels;
  require(a.ready() && a.pixels == b.pixels && cache->decoded == 1,
          "ready texture bytes are shared across placements");
  require(cache->deniedUrls > 0 && cache->assets > 0, "explicit fetch and decode policies were exercised");
  auto retainedPixels = a.pixels;
  host.unload(); textured.reset();
  require(retainedPixels->rgba == std::vector<std::uint8_t>{255, 0, 0, 255},
          "retained immutable texture owns bytes after unload");
}

void exercisePartialSnapshotRecovery() {
  sdk::SessionOptions limited;
  limited.meshOptions.maxWalkVisits = 1;
  auto partial = sdk::RuntimeSession::create(parseMemory(sceneText), limited);
  HostMirror host;
  bool rejected = false;
  try { host.replace(*partial); }
  catch (const std::runtime_error &) { rejected = true; }
  require(rejected && !host.baselineValid && host.placements.empty(),
          "partial snapshot is rejected before publishing host state");
  partial->tick(0);
  rejected = false;
  try { host.apply(partial->delta(), partial->extractor()); }
  catch (const std::runtime_error &) { rejected = true; }
  require(rejected, "deltas cannot resume without an accepted baseline");
  // Reconfigure/replace the session with an adequate budget, then explicitly
  // accept a fresh authoritative snapshot. Never apply a rejected delta later.
  auto recovered = sdk::RuntimeSession::create(parseMemory(sceneText));
  host.replace(*recovered);
  require(host.baselineValid && host.placements.size() == 2, "fresh baseline recovers");
  recovered->tick(0);
  host.apply(recovered->delta(), recovered->extractor());
  agreesWithSnapshot(host, *recovered);

  // A host-side check failure after one mutation must not publish a partial
  // frame as valid. This synthetic bad delta exercises the same exception
  // boundary as a failed descriptor copy/allocation.
  sdk::RenderDelta invalid;
  invalid.removed.push_back(host.placements.begin()->first);
  invalid.updatedTransform.push_back(sdk::kInvalidRenderItemId);
  rejected = false;
  try { host.apply(invalid, recovered->extractor()); }
  catch (const std::runtime_error &) { rejected = true; }
  require(rejected && !host.baselineValid, "failed apply invalidates the whole host frame");
  host.replace(*recovered);
  agreesWithSnapshot(host, *recovered);
}
} // namespace

int main() {
  try {
    exerciseLifecycle(); exerciseResourcesAndDiagnostics(); exercisePartialSnapshotRecovery();
    std::cout << "native host contract: lifecycle, input, time, resources, deltas and snapshots passed\n";
    return 0;
  }
  catch (const std::exception &e) { std::cerr << "native host contract: " << e.what() << '\n'; return 1; }
}
