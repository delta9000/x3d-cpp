#include "X3DDocument.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include "x3d/sai_provider.hpp"
#include <iostream>
#include <limits>

#ifdef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#error "The independent native provider must not include the reference kernel"
#endif
#ifdef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#error "The independent native provider must not include reference metadata"
#endif

namespace sai = x3d::sai::experimental;
static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

// This is intentionally separate from the common backend-blind oracle.
static void native_authority_proof() {
  x3d::runtime::SaiOfflineProvider provider;
  auto scene_result = provider.native_scene();
  require(bool(scene_result), "native extension must return the owned scene");
  auto scene = *scene_result;
  auto created = provider.create_node("Transform");
  require(bool(created), "native creation must succeed");
  require(bool(provider.define_name(*created, "NativeRoot")),
          "native naming must succeed");
  require(bool(provider.append_root(*created)) &&
              bool(provider.append_root(*created)),
          "native root occurrences must append");
  auto native = std::dynamic_pointer_cast<x3d::nodes::Transform>(
      scene->resolve("NativeRoot"));
  require(bool(native), "DEF must resolve to a real native Transform");
  require(
      scene->rootNodes.size() == 2 && scene->rootNodes[0] == native &&
          scene->rootNodes[1] == native,
      "native Scene DEF and both root occurrences must share the same node");
  require(native->getDEF() == "NativeRoot",
          "native node DEF must match Scene naming");
  require(
      bool(provider.write_field(*created, "translation", sai::vec3f{4, 5, 6})),
      "adapter write must succeed");
  require(native->getTranslation() == x3d::core::SFVec3f{4, 5, 6},
          "adapter must mutate the authoritative native node");
  require(scene->authoredScalarFields.contains(native, "translation"),
          "adapter must record native authored-field presence");
  native->setTranslation(x3d::core::SFVec3f{7, 8, 9});
  auto read = provider.read_field(*created, "translation");
  require(
      read && std::get<sai::vec3f>(*read) == sai::vec3f{7, 8, 9},
      "native mutation must be visible immediately without resynchronization");
  auto named = provider.lookup_name("NativeRoot");
  require(named && *named == *created,
          "native lookup must retain common identity");
  sai::result<std::shared_ptr<x3d::runtime::Scene>> off_thread = scene;
  std::thread other([&] { off_thread = provider.native_scene(); });
  other.join();
  require(!off_thread &&
              off_thread.error().code == sai::error_code::wrong_thread,
          "native extension must honor thread confinement too");
  require(bool(provider.close()), "native provider must close");
  require(created->expired() &&
              native->getTranslation() == x3d::core::SFVec3f{7, 8, 9},
          "retained native storage must not preserve SAI authority");
  auto closed = provider.native_scene();
  require(!closed && closed.error().code == sai::error_code::stale_handle,
          "closed provider must reject native extension access");
}

// Application metadata is provider-local sidecar state, never a mirrored node
// graph or an authored X3D field. The real native scene remains authoritative.
static void native_user_data_authority_proof() {
  x3d::runtime::SaiOfflineProvider provider;
  const auto scene = *provider.native_scene();
  const auto node = *provider.create_node("Transform");
  const auto child = *provider.create_node("Transform");
  require(bool(provider.define_name(node, "UserDataOwner")) &&
              bool(provider.append_root(node)) &&
              bool(provider.set_nodes(node, "children", {child, std::nullopt})),
          "native metadata fixture must create a named rooted graph");
  const auto native = std::dynamic_pointer_cast<x3d::nodes::Transform>(
      scene->resolve("UserDataOwner"));
  const auto roots = scene->rootNodes;
  const auto children = native->getChildren();
  const auto definitions = scene->defs;
  const auto translation = native->getTranslation();
  int destroyed = 0;
  require(bool(provider.set_user_data(
              node, sai::user_data_value::make<sai::testing::provider_cleanup_probe>(
                        [&] { ++destroyed; }))) &&
              bool(provider.set_user_data(node, "set_metadata",
                                          sai::user_data_value::make<std::string>("metadata"))) &&
              bool(provider.set_user_data(node, sai::bindings::Transform::addChildren,
                                          sai::user_data_value::make<int>(17))) &&
              bool(provider.set_user_data(node, "translation_changed",
                                          sai::user_data_value::make<int>(23))),
          "native metadata must support nodes, aliases and unsupported value fields");
  require(scene->rootNodes == roots && scene->defs == definitions &&
              native->getChildren() == children && native->getTranslation() == translation &&
              native->getDEF() == "UserDataOwner",
          "user metadata must leave native values, roots, definitions and children unchanged");
  for (const auto &field : native->fields())
    require(!scene->authoredScalarFields.contains(native, field.x3dName),
            "metadata must never record native authored-scalar presence");
  native->setTranslation(x3d::core::SFVec3f{6, 7, 8});
  require(*provider.read_field(node, sai::bindings::Transform::translation) == sai::vec3f{6, 7, 8} &&
              **provider.user_data(node, "translation")->as<int>() == 23,
          "native mutation must be visible without changing canonical metadata");
  const auto field = *provider.field(node, "metadata_changed");
  auto retained = provider.user_data(field);
  require(bool(provider.close()) && destroyed == 1 && node.expired() && field.expired() &&
              native->getChildren() == children && scene->rootNodes == roots &&
              native->getTranslation() == x3d::core::SFVec3f{6, 7, 8} &&
              **retained->as<std::string>() == "metadata",
          "provider close must retire metadata authority while preserving retained native storage and reads");
}

// Every supported scalar lives in the native Transform, including inherited
// bool fields. Read results own their values across native edits and close.
static void native_scalar_authority_proof() {
  x3d::runtime::SaiOfflineProvider provider;
  auto scene = *provider.native_scene();
  auto handle = *provider.create_node("Transform");
  require(bool(provider.append_root(handle)), "scalar fixture root must append");
  auto native = std::dynamic_pointer_cast<x3d::nodes::Transform>(
      scene->rootNodes.front());
  require(bool(native), "scalar fixture must expose a real native Transform");
  std::vector<std::string> authored;
  const auto check_authored = [&] {
    for (const auto &field : native->fields()) {
      const bool expected = std::find(authored.begin(), authored.end(),
                                      field.x3dName) != authored.end();
      require(scene->authoredScalarFields.contains(native, field.x3dName) == expected,
              "each write must preserve the exact native authored-field set");
    }
  };
  const auto author = [&](const char *name, sai::value payload) {
    require(bool(provider.write_field(handle, name, std::move(payload))),
            "all inputOutput scalar writes must succeed");
    authored.emplace_back(name);
    check_authored();
  };
  auto rejected_flag = provider.write_field(handle, "visible", std::int32_t{0});
  auto rejected_rotation = provider.write_field(handle, "scaleOrientation", false);
  require(!rejected_flag && !rejected_rotation &&
              rejected_flag.error().code == sai::error_code::type_mismatch &&
              rejected_rotation.error().code == sai::error_code::type_mismatch &&
              native->getVisible() &&
              native->getScaleOrientation() == x3d::core::SFRotation{0, 0, 1, 0} &&
              !scene->authoredScalarFields.contains(native, "visible") &&
              !scene->authoredScalarFields.contains(native, "scaleOrientation"),
          "failed writes must preserve default values and absent authored marks");
  check_authored();
  for (const auto invalid : {sai::rotation{0, 0, 0, 0},
                             sai::rotation{0, 2, 0, 1},
                             sai::rotation{std::numeric_limits<float>::infinity(),
                                           0, 0, 1}}) {
    const auto rejected = provider.write_field(handle, "rotation", invalid);
    require(!rejected && rejected.error().code == sai::error_code::invalid_value &&
                native->getRotation() == x3d::core::SFRotation{0, 0, 1, 0},
            "invalid rotation axes must fail before native mutation");
    check_authored();
  }
  author("center", sai::vec3f{1, 2, 3});
  author("scale", sai::vec3f{-2, 0, 4});
  author("translation", sai::vec3f{4, 5, 6});
  author("rotation", sai::rotation{0, 1, 0, 1});
  author("scaleOrientation", sai::rotation{1, 0, 0, 2});
  author("visible", false);
  author("bboxDisplay", true);
  require(native->getCenter() == x3d::core::SFVec3f{1, 2, 3} &&
              native->getScale() == x3d::core::SFVec3f{-2, 0, 4} &&
              native->getTranslation() == x3d::core::SFVec3f{4, 5, 6} &&
              native->getRotation() == x3d::core::SFRotation{0, 1, 0, 1} &&
              native->getScaleOrientation() == x3d::core::SFRotation{1, 0, 0, 2} &&
              !native->getVisible() && native->getBboxDisplay(),
          "all scalar writes, including negative and zero scale, must store exactly");
  for (const char *name : {"center", "scale", "translation", "rotation",
                           "scaleOrientation", "visible", "bboxDisplay"})
    require(scene->authoredScalarFields.contains(native, name),
            "all scalar writes must record native authored presence");
  for (const char *name : {"bboxCenter", "bboxSize", "children", "metadata"})
    require(!scene->authoredScalarFields.contains(native, name),
            "scalar writes must not author unrelated fields");

  const auto retained_rotation = *provider.read_field(handle, "rotation");
  const auto retained_visible = *provider.read_field(handle, "visible");
  native->setCenter(x3d::core::SFVec3f{-1, -2, -3});
  native->setScale(x3d::core::SFVec3f{5, 6, 7});
  native->setTranslation(x3d::core::SFVec3f{8, 9, 10});
  native->setRotation(x3d::core::SFRotation{0, 0, 1, 3});
  // Direct native writes can bypass SAI validity. Reads must reject such
  // storage without silently normalizing or repairing it.
  native->setScaleOrientation(x3d::core::SFRotation{0, 0, 0, 4});
  native->setVisible(true);
  native->setBboxDisplay(false);
  native->setBboxCenterUnchecked(x3d::core::SFVec3f{11, 12, 13});
  native->setBboxSizeUnchecked(x3d::core::SFVec3f{14, 15, 16});
  const auto read_equals = [&](const char *name, sai::value expected) {
    const auto value = provider.read_field(handle, name);
    require(value && *value == expected,
            "native scalar edits must be visible through owning reads");
  };
  read_equals("center", sai::vec3f{-1, -2, -3});
  read_equals("scale", sai::vec3f{5, 6, 7});
  read_equals("translation", sai::vec3f{8, 9, 10});
  read_equals("rotation", sai::rotation{0, 0, 1, 3});
  const auto malformed = provider.read_field(handle, "scaleOrientation");
  require(!malformed && malformed.error().code == sai::error_code::invalid_value &&
              native->getScaleOrientation() == x3d::core::SFRotation{0, 0, 0, 4},
          "malformed native rotation reads must fail without changing native state");
  check_authored();
  require(bool(provider.write_field(handle, "scaleOrientation",
                                     sai::rotation{0, 1, 0, 4})),
          "a valid scalar write must repair its malformed native field");
  require(native->getScaleOrientation() == x3d::core::SFRotation{0, 1, 0, 4},
          "repair must publish the exact valid native rotation");
  check_authored();
  read_equals("scaleOrientation", sai::rotation{0, 1, 0, 4});
  read_equals("visible", true);
  read_equals("bboxDisplay", false);
  read_equals("bboxCenter", sai::vec3f{11, 12, 13});
  read_equals("bboxSize", sai::vec3f{14, 15, 16});

  const auto wrong_bool = provider.write_field(handle, "visible", 1.0f);
  const auto wrong_rotation = provider.write_field(handle, "rotation",
                                                     sai::vec4f{0, 0, 1, 2});
  const auto wrong_vector = provider.write_field(handle, "translation",
                                                   sai::rotation{0, 0, 1, 2});
  require(!wrong_bool && !wrong_rotation && !wrong_vector &&
              wrong_bool.error().code == sai::error_code::type_mismatch &&
              wrong_rotation.error().code == sai::error_code::type_mismatch &&
              wrong_vector.error().code == sai::error_code::type_mismatch,
          "scalar writes must reject numerically compatible but wrong kinds");
  check_authored();
  read_equals("visible", true);
  read_equals("rotation", sai::rotation{0, 0, 1, 3});
  read_equals("translation", sai::vec3f{8, 9, 10});
  auto initialization = provider.write_field(handle, "bboxSize", sai::vec3f{1, 2, 3});
  require(!initialization &&
              initialization.error().code == sai::error_code::unsupported_operation,
          "initializeOnly writes must remain explicitly unavailable");
  read_equals("bboxSize", sai::vec3f{14, 15, 16});
  require(!scene->authoredScalarFields.contains(native, "bboxSize"),
          "rejected writes must not mark native authored presence");
  check_authored();
  require(bool(provider.close()), "scalar fixture must close");
  require(std::get<sai::rotation>(retained_rotation) == sai::rotation{0, 1, 0, 1} &&
              !std::get<bool>(retained_visible),
          "owning scalar results must survive native mutation and provider close");
}

// Retain every node so deliberately installed shared_ptr cycles can always be
// removed, including on a failed assertion. The native registry is authoritative
// for unnamed detached nodes too; root/DEF membership is not an ownership test.
struct native_graph_fixture {
  x3d::runtime::SaiOfflineProvider provider;
  std::shared_ptr<x3d::runtime::Scene> scene = *provider.native_scene();
  std::vector<sai::provider_node> handles;
  std::vector<std::shared_ptr<x3d::nodes::Transform>> nodes;

  sai::provider_node create() {
    auto handle = provider.create_node("Transform");
    require(bool(handle), "native graph fixture creation must succeed");
    require(bool(provider.append_root(*handle)), "fixture node must be exposed");
    auto native = std::dynamic_pointer_cast<x3d::nodes::Transform>(
        scene->rootNodes.back());
    require(bool(native), "fixture node must be a real Transform");
    scene->rootNodes.pop_back();
    handles.push_back(*handle);
    nodes.push_back(std::move(native));
    return *handle;
  }

  ~native_graph_fixture() {
    for (const auto &node : nodes)
      node->setChildren(x3d::core::MFNode{});
  }

  struct snapshot {
    std::vector<x3d::core::MFNode> children;
    std::vector<x3d::core::SFVec3f> vectors;
    std::vector<x3d::core::SFRotation> rotations;
    std::vector<bool> flags;
    std::vector<std::string> names;
    std::vector<bool> authored;
    x3d::core::MFNode roots;
    decltype(x3d::runtime::Scene::defs) defs;
    friend bool operator==(const snapshot &, const snapshot &) = default;
  };

  snapshot capture() const {
    snapshot result;
    result.roots = scene->rootNodes;
    result.defs = scene->defs;
    for (const auto &node : nodes) {
      result.children.push_back(node->getChildren());
      result.vectors.push_back(node->getTranslation());
      result.vectors.push_back(node->getScale());
      result.vectors.push_back(node->getCenter());
      result.vectors.push_back(node->getBboxCenter());
      result.vectors.push_back(node->getBboxSize());
      result.rotations.push_back(node->getRotation());
      result.rotations.push_back(node->getScaleOrientation());
      result.flags.push_back(node->getVisible());
      result.flags.push_back(node->getBboxDisplay());
      result.names.push_back(node->getDEF());
      for (const auto &field : node->fields())
        result.authored.push_back(
            scene->authoredScalarFields.contains(node, field.x3dName));
    }
    return result;
  }

  template <class Action>
  void unchanged_error(Action action, sai::error_code expected,
                       const char *message) const {
    const auto before = capture();
    auto outcome = action();
    require(!outcome && outcome.error().code == expected, message);
    require(capture() == before,
            "returned native error must preserve all observed native state");
  }
};

static void native_children_proof() {
  native_graph_fixture f;
  auto &provider = f.provider;
  const auto parent = f.create();
  const auto other_parent = f.create();
  const auto first = f.create();
  const auto second = f.create();
  const auto detached = f.create();
  const auto detached_peer = f.create();
  const auto &p = f.nodes[0];
  const auto &q = f.nodes[1];
  const auto &a = f.nodes[2];
  const auto &b = f.nodes[3];
  const auto &d = f.nodes[4];
  const auto &e = f.nodes[5];
  require(bool(provider.define_name(parent, "ChildrenRoot")) &&
              bool(provider.append_root(parent)) &&
              bool(provider.append_root(parent)),
          "native children fixture must retain DEF and repeated roots");
  require(bool(provider.write_field(parent, "translation", sai::vec3f{1, 2, 3})),
          "native children fixture must retain scalar state");

  const sai::provider_node_list initial{first, std::nullopt, second, std::nullopt};
  require(bool(provider.set_nodes(parent, sai::bindings::Transform::children,
                                  initial)),
          "generated children setter must succeed");
  require(p->getChildren() == x3d::core::MFNode{a, nullptr, b, nullptr},
          "owned setter must publish the exact native pointer sequence");
  auto read = provider.read_nodes(parent, "children");
  require(read && *read == initial,
          "native read must preserve owned identity, order and NULL positions");
  require(!f.scene->authoredScalarFields.contains(p, "children") &&
              f.scene->authoredScalarFields.contains(p, "translation"),
          "children must not be recorded as an authored scalar");
  require(bool(provider.set_nodes(other_parent, "children", {first})),
          "one native child may be shared across separate parent lists");
  require(q->getChildren() == x3d::core::MFNode{a},
          "cross-parent aliases must share the actual native pointer");
  p->setChildren(x3d::core::MFNode{b, nullptr, a});
  read = provider.read_nodes(parent, sai::bindings::Transform::children);
  require(read && *read == sai::provider_node_list{second, std::nullopt, first},
          "serial native edits must be visible through generated owned reads");

  f.unchanged_error(
      [&] { return provider.set_nodes(parent, "children", {first, first}); },
      sai::error_code::invalid_value,
      "same-list repeated non-NULL identity must be rejected");
  f.unchanged_error(
      [&] { return provider.set_nodes(first, "children", {parent}); },
      sai::error_code::containment_cycle,
      "candidate multi-node cycles must be rejected before mutation");
  f.unchanged_error(
      [&] { return provider.set_nodes(detached, "children", {detached}); },
      sai::error_code::containment_cycle,
      "candidate self-cycle on an unnamed detached node must be rejected");

  // A foreign native child makes all children queries fail closed, including
  // queries/edits of unrelated valid nodes. Repair validates the candidate list.
  auto foreign = std::make_shared<x3d::nodes::Transform>();
  d->setChildren(x3d::core::MFNode{foreign});
  f.unchanged_error([&] { return provider.read_nodes(detached, "children"); },
                    sai::error_code::invalid_context,
                    "foreign native pointer must never acquire provider authority");
  f.unchanged_error([&] { return provider.read_nodes(parent, "children"); },
                    sai::error_code::invalid_context,
                    "read must check foreign pointers in detached components");
  f.unchanged_error(
      [&] { return provider.set_nodes(parent, "children", {second}); },
      sai::error_code::invalid_context,
      "unrelated setter must reject a detached foreign pointer unchanged");
  require(bool(provider.set_nodes(detached, "children", {std::nullopt, first})),
          "candidate replacement must repair a foreign target list");
  require(d->getChildren() == x3d::core::MFNode{nullptr, a},
          "repair must replace the authoritative native list");

  d->setChildren(x3d::core::MFNode{a, nullptr, a});
  f.unchanged_error([&] { return provider.read_nodes(parent, "children"); },
                    sai::error_code::invalid_value,
                    "read must fail on a detached native duplicate");
  f.unchanged_error(
      [&] { return provider.set_nodes(parent, "children", {second}); },
      sai::error_code::invalid_value,
      "unrelated setter must reject a native duplicate unchanged");
  require(bool(provider.set_nodes(detached, "children", {first, std::nullopt})),
          "candidate replacement must repair a native duplicate");

  d->setChildren(x3d::core::MFNode{e});
  e->setChildren(x3d::core::MFNode{d});
  f.unchanged_error([&] { return provider.read_nodes(parent, "children"); },
                    sai::error_code::containment_cycle,
                    "read must reject cycles in detached native components");
  f.unchanged_error(
      [&] { return provider.set_nodes(parent, "children", {second}); },
      sai::error_code::containment_cycle,
      "unrelated setter must reject a detached native cycle unchanged");
  require(bool(provider.set_nodes(detached, "children", {})),
          "candidate replacement must break an existing detached cycle");
  require(d->getChildren().empty() && e->getChildren() == x3d::core::MFNode{d},
          "cycle repair must alter only the target list");
  d->setChildren(x3d::core::MFNode{d});
  f.unchanged_error([&] { return provider.read_nodes(detached, "children"); },
                    sai::error_code::containment_cycle,
                    "read must reject native self cycles");
  require(bool(provider.set_nodes(detached, "children", {std::nullopt})),
          "candidate replacement must repair a native self cycle");

  // Replacing one bad list cannot conceal another invalid detached component.
  d->setChildren(x3d::core::MFNode{foreign});
  e->setChildren(x3d::core::MFNode{a, a});
  f.unchanged_error(
      [&] { return provider.set_nodes(detached, "children", {}); },
      sai::error_code::invalid_value,
      "candidate repair must still validate every other registered list");
  e->setChildren(x3d::core::MFNode{});
  require(bool(provider.set_nodes(detached, "children", {})),
          "repair must succeed once the entire candidate graph is valid");
  require(bool(provider.set_nodes(parent, "children", {})) &&
              p->getChildren().empty(),
          "an empty owned list must clear native children");
  require(f.scene->rootNodes == x3d::core::MFNode{p, p} &&
              f.scene->resolve("ChildrenRoot") == p &&
              p->getTranslation() == x3d::core::SFVec3f{1, 2, 3},
          "children writes must preserve roots, DEF identity and scalar fields");
  for (const auto &node : f.nodes)
    require(!f.scene->authoredScalarFields.contains(node, "children"),
            "successful and failed children edits must not author scalar marks");
}

static void native_deep_detached_graph_proof() {
  native_graph_fixture f;
  constexpr std::size_t depth = 4096;
  for (std::size_t i = 0; i < depth; ++i)
    f.create();
  for (std::size_t i = 1; i < depth; ++i)
    f.nodes[i - 1]->setChildren(x3d::core::MFNode{f.nodes[i]});
  require(f.scene->rootNodes.empty() && f.scene->defs.empty(),
          "deep fixture must consist entirely of unnamed detached nodes");
  auto read = f.provider.read_nodes(f.handles.front(), "children");
  require(read && *read == sai::provider_node_list{f.handles[1]},
          "deep detached chains must be traversed iteratively");
  f.nodes.back()->setChildren(x3d::core::MFNode{f.nodes.front()});
  auto rejected = f.provider.read_nodes(f.handles.front(), "children");
  require(!rejected &&
              rejected.error().code == sai::error_code::containment_cycle,
          "deep detached cycles must fail without recursive traversal");
  require(bool(f.provider.set_nodes(f.handles.back(), "children", {})),
          "deep detached cycle must be repairable with a candidate replacement");
}

int main() {
  try {
    const auto report = sai::testing::run_provider_fixture(
        []() -> sai::result<std::unique_ptr<sai::offline_provider>> {
          return std::unique_ptr<sai::offline_provider>{
              std::make_unique<x3d::runtime::SaiOfflineProvider>()};
        });
    native_authority_proof();
    native_scalar_authority_proof();
    native_user_data_authority_proof();
    native_children_proof();
    native_deep_detached_graph_proof();
    std::cout << "native offline provider: " << report.checks
              << " common checks plus native scalar/metadata authority and containment "
                 "proofs passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
