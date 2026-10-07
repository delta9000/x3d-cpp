#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <x3d/sai_presentation.hpp>

namespace sai = x3d::sai::experimental;
namespace p = sai::provider;
namespace native = sai::native;
namespace extract = x3d::runtime::extract;
namespace {
std::size_t checks = 0;
void require(bool value, const char *why) {
  ++checks;
  if (!value)
    throw std::runtime_error(why);
}
template <class T> T take(sai::result<T> value) {
  if (!value)
    throw std::runtime_error(value.error().operation + ": " +
                             value.error().message);
  return std::move(*value);
}
void take(sai::result<void> value) {
  if (!value)
    throw std::runtime_error(value.error().operation + ": " +
                             value.error().message);
}
template <class T>
void rejects(sai::result<T> value, sai::error_code expected, const char *why) {
  require(!value && value.error().code == expected, why);
}

// No native scene, node, RuntimeSession, backend subclass, or testing helper is
// needed by a consumer. All authoring and turns use the ordinary service.
struct graph {
  p::context context;
  p::node left, right, shape, box, replacement;
  p::field left_translation, right_translation, left_children, right_children;
  p::field geometry;
};
graph author_graph(native::presented_scene &world, bool repeat_root = false) {
  auto &service = *world.service;
  graph g;
  g.context = take(service.root_context(world.scene));
  g.left = take(service.create_node(g.context, "Transform"));
  g.right = take(service.create_node(g.context, "Transform"));
  g.shape = take(service.create_node(g.context, "Shape"));
  g.box = take(service.create_node(g.context, "Box"));
  g.replacement = take(service.create_node(g.context, "Box"));
  g.left_translation = take(service.get_field(g.left, "translation"));
  g.right_translation = take(service.get_field(g.right, "translation"));
  g.left_children = take(service.get_field(g.left, "children"));
  g.right_children = take(service.get_field(g.right, "children"));
  g.geometry = take(service.get_field(g.shape, "geometry"));
  take(service.author(g.left_translation, sai::vec3f{1, 2, 3}));
  take(service.author(g.right_translation, sai::vec3f{10, 20, 30}));
  take(service.author(take(service.get_field(g.box, "size")),
                      sai::vec3f{2, 4, 6}));
  take(service.author(take(service.get_field(g.replacement, "size")),
                      sai::vec3f{8, 10, 12}));
  take(service.author(g.geometry, p::node_value{g.box}));
  take(service.author(g.left_children, p::node_values{g.shape}));
  take(service.author(g.right_children, p::node_values{g.shape}));
  take(service.append_root(g.context, g.left));
  take(service.append_root(g.context, g.right));
  if (repeat_root)
    take(service.append_root(g.context, g.left));
  return g;
}
const native::presentation_item &at_x(const native::presentation_frame &frame,
                                      float x) {
  const auto found = std::find_if(
      frame.items.begin(), frame.items.end(),
      [&](const auto &item) { return item.world_transform.m[12] == x; });
  require(found != frame.items.end(),
          "expected placement world translation exists");
  return *found;
}

// Compare every MeshData member by value, without pointer addresses or struct
// padding. This remains meaningful after all service and native owners die.
std::string mesh_value(const extract::MeshData &mesh) {
  std::ostringstream out;
  out << std::hexfloat;
  const auto values = [&](const auto &range, const auto &write) {
    out << range.size() << ':';
    for (const auto &v : range) {
      write(v);
      out << ';';
    }
  };
  const auto scalar = [&](const auto &v) { out << v; };
  const auto vec2 = [&](const auto &v) { out << v.x << ',' << v.y; };
  const auto vec3 = [&](const auto &v) {
    out << v.x << ',' << v.y << ',' << v.z;
  };
  values(mesh.positions, vec3);
  values(mesh.indices, scalar);
  values(mesh.normals, vec3);
  values(mesh.texcoords, vec2);
  values(mesh.texcoordSets, [&](const auto &set) { values(set, vec2); });
  values(mesh.colors, [&](const auto &v) {
    out << v.r << ',' << v.g << ',' << v.b << ',' << v.a;
  });
  values(mesh.vertexAttributes, [&](const auto &v) {
    out << v.name.size() << ':' << v.name << ':' << v.components << ':';
    values(v.values, scalar);
  });
  values(mesh.latticeIndex, scalar);
  values(mesh.sourceCoordIndex, scalar);
  values(mesh.sourceNormalIndex, scalar);
  out << static_cast<int>(mesh.topology) << ',' << mesh.ccw << ',' << mesh.solid
      << ',' << mesh.hasNormals << ',' << mesh.hasColors << ','
      << mesh.isGlyphMesh;
  return out.str();
}
std::string frame_value(const native::presentation_frame &frame) {
  std::ostringstream out;
  out << std::hexfloat << frame.native_tick << ':'
      << frame.host_time.has_value() << ':';
  if (frame.host_time)
    out << frame.host_time->seconds;
  out << ':' << frame.items.size() << ':';
  for (const auto &item : frame.items) {
    out << item.placement_key << ':' << item.mesh_key << ':';
    for (auto value : item.world_transform.m)
      out << value << ',';
    require(bool(item.mesh), "presentation mesh is non-null");
    out << mesh_value(*item.mesh) << '|';
  }
  return out.str();
}
void turn(p::service &service, const p::activation &active,
          const p::client &client, std::vector<p::write> writes, double time) {
  auto receipt = take(service.enqueue(client, std::move(writes)));
  const auto report = take(service.pump(active, {time}));
  require(report.state == p::activation_state::active && !report.error,
          "ordinary native turn succeeds");
  require(take(receipt.status()).state == p::request_state::processed,
          "ordinary request completes");
}

void binding_lifecycle_and_scene_namespace() {
  auto first = take(native::make_presented_scene());
  auto first_graph = author_graph(first);
  auto second = take(native::make_presented_scene());
  author_graph(second);
  rejects(first.feed.snapshot(*first.service), sai::error_code::access_denied,
          "setup scene has no published frame");
  rejects(native::render_feed{}.snapshot(*first.service),
          sai::error_code::stale_handle, "default feed carries no authority");
  rejects(first.feed.snapshot(*second.service),
          sai::error_code::invalid_context,
          "feed rejects another ordinary service");
  const auto active = take(first.service->activate(first.scene));
  take(second.service->activate(second.scene));
  const auto a = take(first.feed.snapshot(*first.service));
  const auto b = take(second.feed.snapshot(*second.service));
  require(
      a->source == first.scene && b->source == second.scene &&
          !(a->source == b->source),
      "frame carries exact opaque scene identity, including factory namespace");
  require(a->native_tick == 0 && !a->host_time && b->native_tick == 0 &&
              !b->host_time,
          "activation snapshot has native tick zero and no invented host time");
  require(a->items.size() == 2 && b->items.size() == 2,
          "two different Transform paths produce two placements");
  require(at_x(*a, 1).placement_key == at_x(*b, 1).placement_key &&
              at_x(*a, 1).mesh_key == at_x(*b, 1).mesh_key,
          "equal numeric keys in independent factories require the scene "
          "namespace");
  require(
      at_x(*a, 1).placement_key != at_x(*a, 10).placement_key &&
          at_x(*a, 1).mesh_key == at_x(*a, 10).mesh_key &&
          at_x(*a, 1).mesh == at_x(*a, 10).mesh,
      "shared Shape geometry has distinct placements and one immutable mesh");
  std::optional<sai::error_code> off_thread;
  std::thread reader([&] {
    auto result = first.feed.snapshot(*first.service);
    if (!result)
      off_thread = result.error().code;
  });
  reader.join();
  require(off_thread == sai::error_code::wrong_thread,
          "feed pull obeys ordinary service owner thread");
  const auto saved = frame_value(*a);
  take(first.service->retire(active));
  rejects(first.feed.snapshot(*first.service), sai::error_code::stale_handle,
          "retired scene feed is stale");
  require(frame_value(*a) == saved, "retirement cannot alter a retained frame");
  require(take(second.feed.snapshot(*second.service))->source == second.scene,
          "retirement does not affect another factory");
  take(second.service->close());
  rejects(second.feed.snapshot(*second.service), sai::error_code::stale_handle,
          "closed service feed is stale");
  first.service.reset();
  second.service.reset();
  require(first_graph.shape.expired() && a->source.expired() &&
              b->source.expired(),
          "retained presentation does not retain ordinary service authority");
  require(frame_value(*a) == saved,
          "retained frame survives complete service destruction");
}

void immutable_frames_and_latest_full_snapshot() {
  auto world = take(native::make_presented_scene());
  const auto g = author_graph(world);
  auto &service = *world.service;
  const auto active = take(service.activate(world.scene));
  const auto client = take(service.connect(active));
  const auto initial = take(world.feed.snapshot(service));
  const auto initial_value = frame_value(*initial);
  const auto retained_mesh = at_x(*initial, 1).mesh;
  const auto retained_mesh_value = mesh_value(*retained_mesh);
  const auto left_key = at_x(*initial, 1).placement_key;
  const auto right_key = at_x(*initial, 10).placement_key;
  const auto old_mesh_key = at_x(*initial, 1).mesh_key;
  turn(service, active, client, {{g.left_translation, sai::vec3f{4, 5, 6}}}, 7);
  const auto moved = take(world.feed.snapshot(service));
  const auto moved_value = frame_value(*moved);
  require(moved->native_tick == 1 && moved->host_time &&
              moved->host_time->seconds == 7,
          "frame records completed native tick and exact host time");
  require(at_x(*moved, 4).placement_key == left_key &&
              at_x(*moved, 10).placement_key == right_key &&
              at_x(*moved, 4).mesh_key == old_mesh_key &&
              at_x(*moved, 4).mesh == retained_mesh,
          "transform-only turn preserves placement and mesh identities");
  turn(service, active, client, {{g.geometry, p::node_value{g.replacement}}},
       7);
  const auto changed = take(world.feed.snapshot(service));
  const auto changed_value = frame_value(*changed);
  require(changed->native_tick == 2 && changed->host_time &&
              changed->host_time->seconds == 7,
          "equal host times still have distinct native ticks");
  require(at_x(*changed, 4).placement_key == left_key &&
              at_x(*changed, 10).placement_key == right_key,
          "geometry replacement rebuild preserves unchanged complete paths");
  require(at_x(*changed, 4).mesh_key != old_mesh_key &&
              at_x(*changed, 4).mesh != retained_mesh &&
              at_x(*changed, 4).mesh_key == at_x(*changed, 10).mesh_key &&
              at_x(*changed, 4).mesh == at_x(*changed, 10).mesh,
          "new geometry gets new content identity shared by both placements");
  require(mesh_value(*at_x(*changed, 4).mesh) != retained_mesh_value,
          "geometry replacement has different extracted data");
  require(frame_value(*initial) == initial_value &&
              frame_value(*moved) == moved_value &&
              mesh_value(*retained_mesh) == retained_mesh_value,
          "later transform and geometry turns never mutate retained values");
  // No pull between these turns: a slow host receives one complete latest
  // frame, including the unchanged placement, rather than a missed delta.
  turn(service, active, client, {{g.left_translation, sai::vec3f{8, 5, 6}}}, 8);
  turn(service, active, client, {{g.left_translation, sai::vec3f{9, 5, 6}}}, 9);
  const auto latest = take(world.feed.snapshot(service));
  require(latest->native_tick == 4 && latest->host_time &&
              latest->host_time->seconds == 9 && latest->items.size() == 2 &&
              at_x(*latest, 9).placement_key == left_key &&
              at_x(*latest, 10).placement_key == right_key,
          "slow host obtains latest full frame with unchanged placements");
  require(take(world.feed.snapshot(service)) == latest,
          "repeated pull without a turn returns the same immutable frame");
  world.service.reset();
  require(g.left.expired() && initial->source.expired(),
          "service destruction expires handles while values remain owned");
  require(
      frame_value(*initial) == initial_value &&
          frame_value(*moved) == moved_value &&
          frame_value(*changed) == changed_value &&
          mesh_value(*retained_mesh) == retained_mesh_value,
      "all retained transform and geometry generations outlive native service");
}

void removal_readdition_and_empty_snapshot() {
  auto world = take(native::make_presented_scene());
  const auto g = author_graph(world);
  auto &service = *world.service;
  const auto active = take(service.activate(world.scene));
  const auto client = take(service.connect(active));
  const auto initial = take(world.feed.snapshot(service));
  const auto old_left = at_x(*initial, 1).placement_key;
  const auto old_right = at_x(*initial, 10).placement_key;
  turn(service, active, client, {{g.left_children, p::node_values{}}}, 1);
  const auto removed = take(world.feed.snapshot(service));
  require(removed->items.size() == 1 &&
              at_x(*removed, 10).placement_key == old_right,
          "structural rebuild removes one path and preserves unchanged path "
          "identity");
  turn(service, active, client, {{g.left_children, p::node_values{g.shape}}},
       2);
  const auto restored = take(world.feed.snapshot(service));
  require(restored->items.size() == 2 &&
              at_x(*restored, 1).placement_key != old_left &&
              at_x(*restored, 1).placement_key != old_right &&
              at_x(*restored, 10).placement_key == old_right,
          "removed and later readded path gets a fresh key without aliasing "
          "survivors");
  turn(service, active, client,
       {{g.left_children, p::node_values{}},
        {g.right_children, p::node_values{}}},
       3);
  const auto empty = take(world.feed.snapshot(service));
  require(empty->items.empty() && empty->native_tick == 3 && empty->host_time &&
              empty->host_time->seconds == 3,
          "a complete empty snapshot is still a successful native frame");
  turn(service, active, client, {{g.left_children, p::node_values{g.shape}}},
       4);
  const auto reborn = take(world.feed.snapshot(service));
  require(reborn->items.size() == 1 &&
              at_x(*reborn, 1).placement_key !=
                  at_x(*restored, 1).placement_key &&
              at_x(*reborn, 1).placement_key != old_right,
          "empty intervals do not permit placement key reuse");
}

void repeated_identical_root_paths_follow_extractor_contract() {
  auto world = take(native::make_presented_scene());
  const auto g = author_graph(world, true);
  require(take(world.service->roots(g.context)).size() == 3,
          "ordinary root list retains the repeated root occurrence");
  const auto active = take(world.service->activate(world.scene));
  const auto frame = take(world.feed.snapshot(*world.service));
  require(frame->items.size() == 2, "identical root-to-Shape paths collapse "
                                    "under existing extractor identity");
  require(at_x(*frame, 1).placement_key != at_x(*frame, 10).placement_key,
          "distinct ancestor paths remain distinct despite a repeated "
          "identical root");
  const auto empty_turn = take(world.service->pump(active, {0}));
  require(empty_turn.state == p::activation_state::active &&
              take(world.feed.snapshot(*world.service))->items.size() == 2,
          "repeated identical root paths do not fault later snapshots");
}

void frontend_notification_overflow_hides_native_candidate() {
  p::resource_limits limits;
  limits.queued_notifications = 1;
  auto world = take(native::make_presented_scene(limits));
  const auto g = author_graph(world);
  auto &service = *world.service;
  const auto active = take(service.activate(world.scene));
  const auto client = take(service.connect(active));
  // First retain a successfully completed host frame before installing the
  // observers. Overflow is discovered during native event capture, after the
  // frontend's empty-queue preflight; native do_turn itself still completes.
  turn(service, active, client, {{g.left_translation, sai::vec3f{4, 5, 6}}}, 1);
  const auto good = take(world.feed.snapshot(service));
  const auto good_value = frame_value(*good);
  std::size_t callbacks = 0;
  auto one = take(service.observe(active, g.left_translation,
                                  [&](const auto &) { ++callbacks; }));
  auto two = take(service.observe(active, g.left_translation,
                                  [&](const auto &) { ++callbacks; }));
  const auto receipt = take(
      service.enqueue(client, {{g.left_translation, sai::vec3f{99, 5, 6}}}));
  const auto report = take(service.pump(active, {2}));
  require(report.state == p::activation_state::faulted && report.error &&
              report.error->code == sai::error_code::resource_limit &&
              report.effects == p::failure_effect::partial_or_unknown,
          "two deliveries overflow one notification slot and fault completed "
          "native turn");
  require(std::get<sai::vec3f>(take(service.read(g.left_translation))) ==
              sai::vec3f{99, 5, 6},
          "native mutation really happened before frontend publication failed");
  require(take(receipt.status()).state == p::request_state::failed,
          "overflowed turn owns a failed request receipt");
  rejects(world.feed.snapshot(service), sai::error_code::access_denied,
          "feed rejects candidate from a frontend-faulted native turn");
  require(take(service.dispatch_notifications()) == 0 && callbacks == 0,
          "failed turn publishes no deferred notifications");
  require(good->native_tick == 1 && frame_value(*good) == good_value,
          "previous good frame remains unchanged after frontend-only fault");
  world.service.reset();
  require(
      frame_value(*good) == good_value,
      "previous good frame remains valid after faulted service destruction");
}

p::node author_wide_dag(p::service &service, const p::context &context) {
  // Only 44 native nodes, no meshes: distinct siblings satisfy MFNode
  // uniqueness while shared descendants create over a million path visits.
  std::array<p::node, 2> next;
  for (unsigned level = 0; level < 22; ++level) {
    std::array<p::node, 2> current{
        take(service.create_node(context, "Transform")),
        take(service.create_node(context, "Transform"))};
    if (level)
      for (const auto &node : current)
        take(service.author(take(service.get_field(node, "children")),
                            p::node_values{next[0], next[1]}));
    next = std::move(current);
  }
  return next[0];
}
void truncated_native_walk_cannot_publish_complete_frame() {
  {
    auto world = take(native::make_presented_scene());
    auto &service = *world.service;
    const auto context = take(service.root_context(world.scene));
    const auto root = author_wide_dag(service, context);
    take(service.append_root(context, root));
    const auto activated = service.activate(world.scene);
    rejects(
        activated, sai::error_code::resource_limit,
        "initial path-walk truncation cannot publish a successful empty frame");
    require(take(service.state(world.scene)) == p::activation_state::faulted,
            "truncated initial native snapshot faults activation");
    rejects(world.feed.snapshot(service), sai::error_code::access_denied,
            "initial truncated frame is not exposed through the feed");
    take(service.retire(world.scene));
  }
  {
    auto world = take(native::make_presented_scene());
    auto &service = *world.service;
    const auto g = author_graph(world);
    // The wide graph is owned but detached at activation, so frame zero is
    // complete. Attaching it later trips the extractor's bounded rebuild.
    const auto wide = author_wide_dag(service, g.context);
    const auto active = take(service.activate(world.scene));
    const auto client = take(service.connect(active));
    const auto good = take(world.feed.snapshot(service));
    const auto good_value = frame_value(*good);
    const auto receipt = take(service.enqueue(
        client, {{g.left_children, p::node_values{g.shape, wide}}}));
    const auto report = take(service.pump(active, {1}));
    require(report.state == p::activation_state::faulted && report.error &&
                report.error->code == sai::error_code::resource_limit &&
                report.effects == p::failure_effect::partial_or_unknown,
            "truncated live rebuild cannot claim successful full-frame "
            "publication");
    require(take(receipt.status()).state == p::request_state::failed,
            "truncated turn fails its request receipt");
    rejects(world.feed.snapshot(service), sai::error_code::access_denied,
            "truncated live frame is not exposed through the feed");
    require(frame_value(*good) == good_value,
            "truncated live rebuild leaves retained complete frame unchanged");
    world.service.reset();
    require(frame_value(*good) == good_value,
            "complete frame survives truncated scene teardown");
  }
}

void presentation_depth_preflight_is_conservative_and_nonmutating() {
  auto world = take(native::make_presented_scene());
  auto &service = *world.service;
  const auto context = take(service.root_context(world.scene));
  const auto box = take(service.create_node(context, "Box"));
  const auto shape = take(service.create_node(context, "Shape"));
  take(service.author(take(service.get_field(shape, "geometry")),
                      p::node_value{box}));
  // The native traversal limit is 1000 registered nodes on a path. Its
  // presentation preflight deliberately counts geometry and detached nodes,
  // even if the extractor would never visit some of those paths. Keep this
  // exact-boundary fixture in sync if the native traversal limit changes.
  constexpr std::size_t native_depth_limit = 1000;
  std::vector<p::node> chain;
  chain.reserve(native_depth_limit - 2);
  auto child = shape;
  for (std::size_t i = 0; i < native_depth_limit - 2; ++i) {
    const auto parent = take(service.create_node(context, "Transform"));
    take(service.author(take(service.get_field(parent, "children")),
                        p::node_values{child}));
    chain.push_back(parent);
    child = parent;
  }
  const auto root = chain.back();
  const auto below_root = chain[chain.size() - 2];
  const auto extra = take(service.create_node(context, "Transform"));
  const auto extra_children = take(service.get_field(extra, "children"));
  const auto root_children = take(service.get_field(root, "children"));
  take(service.append_root(context, root));
  // A detached extra ancestor makes the full owned graph one node too deep.
  take(service.author(extra_children, p::node_values{root}));
  rejects(service.activate(world.scene), sai::error_code::resource_limit,
          "presentation activation rejects excessive owned-graph depth before "
          "mutation");
  require(take(service.state(world.scene)) == p::activation_state::setup,
          "depth-preflight rejection leaves setup repairable");
  take(service.author(extra_children, p::node_values{}));
  const auto active = take(service.activate(world.scene));
  const auto client = take(service.connect(active));
  const auto good = take(world.feed.snapshot(service));
  const auto good_value = frame_value(*good);
  require(good->native_tick == 0 && good->items.size() == 1,
          "exactly bounded path activates with its complete mesh placement");
  // Insert the detached node between root and its existing child, in one final
  // candidate graph. That graph is acyclic but one node beyond the depth cap.
  rejects(
      service.enqueue(client, {{root_children, p::node_values{extra}},
                               {extra_children, p::node_values{below_root}}}),
      sai::error_code::resource_limit,
      "live depth violation is rejected at admission without native mutation");
  require(
      std::get<p::node_values>(take(service.read(root_children))) ==
              p::node_values{below_root} &&
          std::get<p::node_values>(take(service.read(extra_children))).empty(),
      "rejected depth candidate preserves native node-valued fields");
  take(service.begin_update(client));
  const auto first =
      take(service.enqueue(client, {{root_children, p::node_values{extra}}}));
  const auto second = take(
      service.enqueue(client, {{extra_children, p::node_values{below_root}}}));
  rejects(service.end_update(client), sai::error_code::resource_limit,
          "held writes enforce final graph depth at gate release");
  require(
      take(first.status()).state == p::request_state::failed &&
          take(second.status()).state == p::request_state::failed &&
          take(first.status()).effects == p::failure_effect::none,
      "rejected depth gate gives both receipts an honest pre-mutation failure");
  require(
      take(service.state(world.scene)) == p::activation_state::active &&
          take(world.feed.snapshot(service)) == good &&
          frame_value(*good) == good_value,
      "depth rejection keeps active scene and complete published frame intact");
}
} // namespace

int main() {
  static_assert(std::is_same_v<extract::MeshRef,
                               std::shared_ptr<const extract::MeshData>>);
  static_assert(std::is_trivially_copyable_v<x3d::runtime::Mat4>);
  static_assert(sizeof(x3d::runtime::Mat4) == sizeof(float) * 16);
  try {
    binding_lifecycle_and_scene_namespace();
    immutable_frames_and_latest_full_snapshot();
    removal_readdition_and_empty_snapshot();
    repeated_identical_root_paths_follow_extractor_contract();
    frontend_notification_overflow_hides_native_candidate();
    truncated_native_walk_cannot_publish_complete_frame();
    presentation_depth_preflight_is_conservative_and_nonmutating();
    std::cout << "native owned presentation: " << checks
              << " runtime checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << "native owned presentation failed: " << error.what() << '\n';
    return 1;
  }
}
