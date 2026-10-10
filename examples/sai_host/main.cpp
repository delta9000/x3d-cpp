#include <x3d/sai_presentation.hpp>
#include "cpu_presenter.hpp"
#include "workload.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;
namespace p = sai::provider;

template<class T> T take(sai::result<T> result, const char* operation) {
  if (!result)
    throw std::runtime_error(std::string(operation) + ": " + result.error().message);
  return std::move(*result);
}
void take(sai::result<void> result, const char* operation) {
  if (!result)
    throw std::runtime_error(std::string(operation) + ": " + result.error().message);
}

void require(bool condition, const std::string& message) {
  if (!condition)
    throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }

struct scene_fields {
  p::node root, left, right, shape, small, large, interpolator;
  p::field children, left_translation, right_translation, geometry, fraction, output;
};

scene_fields author_scene(p::service& service, const p::scene& scene) {
  const auto context = take(service.root_context(scene), "root context");
  const auto make = [&](const char* type, const char* name) {
    auto node = take(service.create_node(context, type), "create node");
    take(service.define_name(node, name), "name node");
    return node;
  };
  scene_fields f;
  f.root = make("Transform", "Root");
  f.left = make("Transform", "Left");
  f.right = make("Transform", "Right");
  f.shape = make("Shape", "SharedShape");
  f.small = make("Box", "SmallBox");
  f.large = make("Box", "LargeBox");
  f.interpolator = make("PositionInterpolator", "Movement");
  const auto field = [&](const p::node& n, const char* name) {
    return take(service.get_field(n, name), "get field");
  };
  f.children = field(f.root, "children");
  f.left_translation = field(f.left, "translation");
  f.right_translation = field(f.right, "translation");
  f.geometry = field(f.shape, "geometry");
  f.fraction = field(f.interpolator, "set_fraction");
  f.output = field(f.interpolator, "value_changed");
  take(service.author(field(f.small, "size"), sai::vec3f{1, 1, 1}), "author small Box");
  take(service.author(field(f.large, "size"), sai::vec3f{1.8f, 1.2f, 0.8f}),
       "author large Box before activation");
  take(service.author(f.geometry, p::node_value{f.small}), "share geometry");
  take(service.author(field(f.left, "children"), p::node_values{f.shape}), "left placement");
  take(service.author(field(f.right, "children"), p::node_values{f.shape}), "right placement");
  take(service.author(f.children, p::node_values{f.left, f.right}), "root children");
  take(service.author(f.left_translation, sai::vec3f{-2.2f, -0.7f, 0}), "left pose");
  take(service.author(f.right_translation, sai::vec3f{2, 0, 0}), "right pose");
  take(service.author(field(f.interpolator, "key"), sai::float_list{0, 1}), "interpolation keys");
  take(service.author(field(f.interpolator, "keyValue"),
    sai::vec3f_list{{-2.2f, -0.7f, 0}, {-1.2f, 0.7f, 0}}), "interpolation values");
  take(service.add_route(context, f.output, f.left_translation), "route movement to placement");
  take(service.append_root(context, f.root), "append root");
  // The interpolator is a second root so its behavior belongs to the active scene.
  take(service.append_root(context, f.interpolator), "append interpolator");
  return f;
}

struct expected_scene {
  float fraction = 0;
  bool right_present = true, large = false;
};

// Match the public rendering seam against the host's intended scene state.
// Placement identity and mesh identity have different lifetimes and purposes.
void verify_frame(const sai::native::presentation_frame& frame,
                  const expected_scene& expected, std::uint64_t left_key,
                  std::uint64_t right_key) {
  require(frame.items.size() == (expected.right_present ? 2u : 1u),
          "presentation must reflect removal and re-addition");
  const sai::native::presentation_item* left = nullptr;
  const sai::native::presentation_item* right = nullptr;
  for (const auto& item : frame.items) {
    require(bool(item.mesh) && !item.mesh->positions.empty() &&
            item.mesh->indices.size() == 36, "Box must yield twelve indexed triangles");
    if (item.placement_key == left_key)
      left = &item;
    else if (item.placement_key == right_key)
      right = &item;
    else
      throw std::runtime_error("presentation contains an unexpected placement identity");
    const auto& m = item.world_transform.m;
    require(near(m[0], 1) && near(m[5], 1) && near(m[10], 1) && near(m[15], 1),
            "host expects translation-only world transforms");
    const std::array<float, 3> half = expected.large ?
      std::array<float, 3>{0.9f, 0.6f, 0.4f} : std::array<float, 3>{0.5f, 0.5f, 0.5f};
    std::array<float, 3> lo{1000, 1000, 1000}, hi{-1000, -1000, -1000};
    for (const auto& v : item.mesh->positions) {
      const std::array<float, 3> position{v.x, v.y, v.z};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        lo[axis] = std::min(lo[axis], position[axis]);
        hi[axis] = std::max(hi[axis], position[axis]);
      }
    }
    for (std::size_t axis = 0; axis < 3; ++axis)
      require(near(lo[axis], -half[axis]) && near(hi[axis], half[axis]),
              "mesh bounds must match the currently selected pre-authored Box");
  }
  require(left != nullptr, "left placement must remain present");
  require(near(left->world_transform.m[12], -2.2f + expected.fraction) &&
          near(left->world_transform.m[13], -0.7f + 1.4f * expected.fraction),
          "native route must drive the expected left world transform");
  require((right != nullptr) == expected.right_present, "right placement membership");
  if (right) {
    require(near(right->world_transform.m[12], 2) &&
            near(right->world_transform.m[13], 0), "right placement must keep its own pose");
    require(left->placement_key != right->placement_key &&
            left->mesh_key == right->mesh_key && left->mesh == right->mesh,
            "distinct placements must share one immutable mesh resource");
  }
}

void replay(const std::vector<host::record>& records) {
  p::resource_limits limits;
  limits.pending_requests = 4;
  limits.payload_bytes = 64 * 1024;
  limits.live_registrations = 2;
  limits.queued_notifications = 2;
  limits.notification_bytes = 64 * 1024;
  auto presented = take(sai::native::make_presented_scene(limits), "create presentation");
  auto& service = *presented.service;
  const auto f = author_scene(service, presented.scene);
  const auto active = take(service.activate(presented.scene), "activate");
  const auto client = take(service.connect(active), "connect host");
  std::vector<p::notification> observed;
  std::size_t diagnostic_callbacks = 0;
  auto observer = take(service.observe(active, f.left_translation,
    [&](const p::notification& notification) {
      require(observed.size() < 256, "bounded callback recording exhausted");
      observed.push_back(notification);
    }), "observe routed translation");
  auto diagnostic = take(service.observe(active, f.left_translation,
    [&](const p::notification&) { ++diagnostic_callbacks; }), "observe diagnostic trace");

  // Activation publishes tick zero. Retain that immutable frame through edits,
  // removal, mesh replacement, close and complete service destruction.
  const auto old_frame = take(presented.feed.snapshot(service), "initial frame");
  require(old_frame->native_tick == 0 && !old_frame->host_time,
          "activation frame precedes host turns");
  require(old_frame->source == presented.scene, "frame source must identify the bound scene");
  require(old_frame->items.size() == 2, "initial frame needs two placements");
  const auto* left = &old_frame->items[0];
  const auto* right = &old_frame->items[1];
  if (left->world_transform.m[12] > right->world_transform.m[12])
    std::swap(left, right);
  const auto left_key = left->placement_key, original_right_key = right->placement_key;
  auto right_key = original_right_key;
  auto removed_right_key = original_right_key;
  bool learn_readded_key = false;
  const auto small_mesh_key = left->mesh_key;
  expected_scene expected;
  verify_frame(*old_frame, expected, left_key, right_key);
  host::cpu_presenter presenter;
  const auto old_image = presenter.draw(*old_frame);
  require(presenter.draw(*old_frame) == old_image, "same immutable frame must render deterministically");

  bool saw_equal_time = false, saw_group = false, saw_large = false;
  bool saw_remove = false, saw_readd = false, saw_small_again = false;
  bool saw_sampled_large = false, saw_sampled_removal = false;
  std::size_t backpressure = 0, presentations = 0;
  std::vector<p::ticket> completed;
  std::uint64_t previous_tick = 0;
  std::optional<double> previous_time;
  std::optional<std::uint64_t> last_presented_checksum;
  bool changed_image = false;

  // The application owns both clocks: every record requests one runtime turn;
  // the CPU presenter deliberately samples only every third completed tick.
  for (std::size_t index = 0; index < records.size(); ++index) {
    const auto& record = records[index];
    if (previous_time && *previous_time == record.time)
      saw_equal_time = true;
    std::vector<p::ticket> receipts;
    const auto enqueue = [&](std::vector<p::write> writes) {
      receipts.push_back(take(service.enqueue(client, std::move(writes)), "enqueue host input"));
    };
    switch (record.op) {
    case host::operation::fraction:
      enqueue({{f.fraction, record.first}});
      expected.fraction = record.first;
      break;
    case host::operation::group:
      saw_group = true;
      take(service.begin_update(client), "begin grouped update");
      enqueue({{f.fraction, record.first}, {f.right_translation, sai::vec3f{2.5f, 0, 0}}});
      enqueue({{f.fraction, record.second}, {f.right_translation, sai::vec3f{2, 0, 0}}});
      for (const auto& receipt : receipts)
        require(take(receipt.status(), "held receipt").state == p::request_state::accepted,
                "held calls must stay accepted until the host releases its gate");
      take(service.end_update(client), "release grouped update");
      require(take(receipts[0].status(), "group status").submission ==
              take(receipts[1].status(), "group status").submission,
              "held calls must share one released submission");
      expected.fraction = record.second;
      break;
    case host::operation::swap_large:
      enqueue({{f.geometry, p::node_value{f.large}}});
      expected.large = true;
      saw_large = true;
      break;
    case host::operation::swap_small:
      enqueue({{f.geometry, p::node_value{f.small}}});
      expected.large = false;
      saw_small_again = saw_large;
      break;
    case host::operation::remove_right:
      require(expected.right_present, "cannot remove an absent right placement");
      enqueue({{f.children, p::node_values{f.left}}});
      expected.right_present = false;
      removed_right_key = right_key;
      saw_remove = true;
      break;
    case host::operation::add_right:
      require(!expected.right_present, "cannot add an already present right placement");
      enqueue({{f.children, p::node_values{f.left, f.right}}});
      expected.right_present = true;
      learn_readded_key = true;
      saw_readd = saw_remove;
      break;
    case host::operation::idle:
      break;
    }

    for (const auto& receipt : receipts)
      require(take(receipt.status(), "submitted receipt").state == p::request_state::submitted,
              "released work must wait for an explicit host pump");
    const auto callbacks_before = observed.size();
    auto result = service.pump(active, sai::event_time{record.time});
    require(observed.size() == callbacks_before,
            "pump must defer application callbacks to an explicit drain");
    if (index == 1) {
      require(!result && result.error().code == sai::error_code::resource_limit,
              "full notification queue must backpressure the next observed turn");
      ++backpressure;
      const auto unchanged = take(presented.feed.snapshot(service), "backpressure frame");
      require(unchanged->native_tick == previous_tick,
              "preflight backpressure must not advance native presentation");
      require(take(receipts.front().status(), "backpressured receipt").state ==
              p::request_state::submitted, "backpressure must leave input available for retry");
      const auto drained = take(service.dispatch_notifications(), "drain full notification queue");
      require(drained > 0 && !observed.empty() && diagnostic_callbacks > 0,
              "explicit drain must deliver the earlier deferred callback");
      diagnostic.cancel();
      // Retry the same queued work and host time. No automatic rollback/retry
      // is assumed for failures that happen after native execution begins.
      const auto retry_callbacks = observed.size();
      result = service.pump(active, sai::event_time{record.time});
      require(observed.size() == retry_callbacks, "retry must also defer callbacks");
      std::cout << "backpressure: drain=" << drained << " retry=same-submission\n";
    }
    const auto turn = take(std::move(result), "pump explicit host turn");
    require(!turn.error && turn.effects == p::failure_effect::none &&
            turn.state == p::activation_state::active,
            turn.error ? turn.error->message : "native turn did not complete cleanly");
    require(turn.turn == index + 1 && turn.time.seconds == record.time,
            "logical turn and host time must remain separate");
    for (const auto& receipt : receipts) {
      const auto status = take(receipt.status(), "completed receipt");
      require(status.state == p::request_state::processed && status.turn == turn.turn,
              "receipt must become processed by this explicit turn");
      completed.push_back(receipt);
    }
    require(bool(turn.request) == !receipts.empty(), "idle turns must have no request receipt");

    // The native EventModel may coalesce output events within a timestamp.
    // Two input-only occurrences do not imply two routed results, nor a
    // last-input-wins rule. Verify an admitted interpolation result and exact
    // source/ROUTE agreement without imposing a new event policy on the runtime.
    const auto output = take(service.read(f.output), "read interpolator output");
    if (record.op == host::operation::group) {
      const auto* value = std::get_if<sai::vec3f>(&output);
      const auto matches = [&](float fraction) {
        return value && near(value->x, -2.2f + fraction) &&
               near(value->y, -0.7f + 1.4f * fraction) && near(value->z, 0);
      };
      require(matches(record.first) || matches(record.second),
              "group output must be an interpolation of an admitted occurrence");
      expected.fraction = matches(record.first) ? record.first : record.second;
    }
    // Verify owning portable reads even on ticks the presenter intentionally skips.
    const auto translation = take(service.read(f.left_translation), "read routed state");
    require(translation == output, "ROUTE destination must agree with admitted native output");
    const auto* position = std::get_if<sai::vec3f>(&translation);
    require(position && near(position->x, -2.2f + expected.fraction) &&
            near(position->y, -0.7f + 1.4f * expected.fraction),
            "routed state at turn " + std::to_string(turn.turn) +
            " must reflect fraction " + std::to_string(expected.fraction) +
            "; observed x=" + (position ? std::to_string(position->x) : "wrong payload kind"));

    if (turn.turn % 3 == 0) {
      const auto frame = take(presented.feed.snapshot(service), "present every third tick");
      require(frame->native_tick == turn.turn && frame->host_time &&
              frame->host_time->seconds == record.time, "presentation must carry native tick and host time");
      if (learn_readded_key && expected.right_present) {
        const auto readded = std::find_if(frame->items.begin(), frame->items.end(),
          [&](const auto& item) { return item.placement_key != left_key; });
        require(readded != frame->items.end() && readded->placement_key != removed_right_key,
                "re-added path must receive a fresh placement identity");
        right_key = readded->placement_key;
        learn_readded_key = false;
      }
      verify_frame(*frame, expected, left_key, right_key);
      if (expected.large) {
        saw_sampled_large = true;
        require(frame->items.front().mesh_key != small_mesh_key,
                "geometry replacement must have a different mesh identity");
      }
      saw_sampled_removal = saw_sampled_removal || !expected.right_present;
      const auto image = presenter.draw(*frame);
      require(presenter.draw(*frame) == image, "repeated presentation must be deterministic");
      if (last_presented_checksum && *last_presented_checksum != image.checksum)
        changed_image = true;
      last_presented_checksum = image.checksum;
      ++presentations;
      std::cout << "frame tick=" << frame->native_tick << " time=" << frame->host_time->seconds
                << " placements=" << frame->items.size() << " covered=" << image.covered_pixels
                << " checksum=" << image.checksum << '\n';
    }
    // The first turn intentionally retains notices to demonstrate backpressure.
    // Counts are measured, never equated to input occurrence counts: native
    // EventModel scheduling may coalesce deliveries inside a turn.
    if (index != 0)
      take(service.dispatch_notifications(), "dispatch deferred callbacks");
    previous_tick = turn.turn;
    previous_time = record.time;
  }

  require(saw_equal_time && saw_group && saw_large && saw_remove && saw_readd &&
          saw_small_again && saw_sampled_large && saw_sampled_removal && changed_image,
          "workload must cover equal times, groups, sampled geometry changes and removal/re-addition");
  require(presentations * 3 == records.size() && backpressure == 1,
          "host presentation and backpressure schedule must be exercised");
  require(!observed.empty(), "host must receive deferred route notifications");
  for (const auto& notification : observed)
    require(notification.turn > 0 && notification.turn <= records.size() &&
            notification.time.seconds == records[notification.turn - 1].time,
            "deferred notifications must retain originating turn and host time");

  // Complete receipt status is owning; pending work is cancelled at teardown.
  const auto pending = take(service.enqueue(client, {{f.fraction, 0.375f}}), "enqueue before teardown");
  take(service.close(), "close ordinary service");
  require(!presented.feed.snapshot(service), "closed service must revoke new feed access");
  require(take(pending.status(), "cancelled receipt").state == p::request_state::cancelled,
          "close must cancel pending work");
  presented.service.reset();
  require(f.left.expired(), "destroyed service must retire portable handles");
  require(old_frame->source.expired(), "retained presentation source must not preserve service authority");
  require(old_frame->native_tick == 0 && !old_frame->host_time,
          "retained presentation metadata must remain immutable");
  require(take(completed.front().status(), "retained completed receipt").state ==
          p::request_state::processed, "receipt must survive service destruction");
  verify_frame(*old_frame, expected_scene{}, left_key, original_right_key);
  require(presenter.draw(*old_frame) == old_image,
          "old immutable frame and mesh must remain usable after complete teardown");
  std::cout << "PASS turns=" << records.size() << " presentations=" << presentations
            << " receipts=" << completed.size() << " callbacks=" << observed.size()
            << " backpressure=" << backpressure << " retained-frame=unchanged\n";
}

int main(int argc, char** argv) {
  try {
    if (argc != 2)
      throw std::runtime_error("usage: sai_cpu_host <local-recorded-workload.txt>");
    replay(host::read_workload(argv[1]));
  } catch (const std::exception& e) {
    std::cerr << "sai_cpu_host: " << e.what() << '\n';
    return 1;
  }
}
