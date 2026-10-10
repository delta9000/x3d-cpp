#include "DynamicField.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/Transform.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <x3d/sai/experimental/testing/provider_fixture.hpp>
#include <x3d/sai_provider.hpp>

namespace sai = x3d::sai::experimental;
namespace h = sai::provider;
namespace {
void require(bool value, const char *why) {
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
void eq(sai::vec3f actual, sai::vec3f expected, const char *why) {
  require(actual == expected, why);
}
class inspected_backend : public sai::native::backend {
public:
  using evidence = native_evidence;
  std::vector<evidence> turns;
  bool throw_after_tick = false;
  bool throw_after_activation = false;

protected:
  sai::result<void> do_activate(std::uint64_t scene) override {
    auto result = sai::native::backend::do_activate(scene);
    if (result && throw_after_activation)
      throw std::runtime_error("injected failure after native activation");
    return result;
  }
  backend_result do_turn(std::uint64_t scene, sai::event_time time,
                         const std::vector<admitted_write> &writes) override {
    auto result = sai::native::backend::do_turn(scene, time, writes);
    turns.push_back(inspect(scene));
    if (throw_after_tick)
      throw std::runtime_error("injected failure after native mutation");
    return result;
  }
};
struct fixture {
  h::scene scene;
  h::context context;
  h::node transform, interpolator;
  h::field translation, fraction, output, key, key_value;
};
fixture make(h::service &service) {
  fixture f;
  f.scene = take(sai::testing::make_provider_route_scene(service));
  f.context = take(service.root_context(f.scene));
  f.transform = take(service.named(f.context, "Transform"));
  f.interpolator = take(service.named(f.context, "Interpolator"));
  f.translation = take(service.get_field(f.transform, "translation"));
  f.fraction = take(service.get_field(f.interpolator, "set_fraction"));
  f.output = take(service.get_field(f.interpolator, "value_changed"));
  f.key = take(service.get_field(f.interpolator, "key"));
  f.key_value = take(service.get_field(f.interpolator, "keyValue"));
  return f;
}
void native_route_and_render() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto f = make(service);
  require(take(service.roots(f.context)).size() == 3, "three real roots");
  require(take(service.type_name(f.interpolator)) == "PositionInterpolator",
          "real interpolator type");
  require(take(service.type_name(f.transform)) == "Transform",
          "real Transform type");
  const auto before_identity = f.interpolator;
  auto a = take(service.activate(f.scene));
  require(take(service.named(f.context, "Interpolator")) == before_identity,
          "activation preserves lexical identity");
  auto c = take(service.connect(a));
  std::vector<h::notification> notifications;
  auto output_subscription = take(service.observe(
      a, f.output, [&](const auto &n) { notifications.push_back(n); }));
  auto transform_subscription = take(service.observe(
      a, f.translation, [&](const auto &n) { notifications.push_back(n); }));
  auto ticket = take(service.enqueue(c, {{f.fraction, 0.25f}}));
  eq(std::get<sai::vec3f>(take(service.read(f.translation))), {0, 0, 0},
     "enqueue cannot mutate native node");
  auto first = take(service.pump(a, {7.0}));
  require(first.turn == 1 && first.request == ticket.id(),
          "turn/request identity");
  require(notifications.empty(), "no application callbacks inside native turn");
  const auto &e = native.turns.back();
  require(e.tick == 1 && e.host_time == 7.0 && e.routes == 1,
          "native tick and authored ROUTE bridge");
  require(e.snapshot_items == 1 && e.transformed == 1 && e.geometry == 0 &&
              e.material == 0,
          "real RenderDelta transform-only update");
  require(e.added == 0 && e.removed == 0 && e.rendered_translations.size() == 1,
          "stable render identity");
  eq(e.interpolated, {2.5f, 5, 7.5f}, "actual interpolator output storage");
  eq(e.translation, e.interpolated, "actual Transform storage");
  eq(std::get<sai::vec3f>(take(service.read(f.output))), e.interpolated,
     "portable read uses actual native outputOnly getter");
  eq(e.world_translation, e.interpolated,
     "worldTransform propagated after cascade");
  eq(e.rendered_translations.front(), e.interpolated,
     "render item reflects routed translation");
  require(take(service.dispatch_notifications()) == 2,
          "two captured native notifications");
  require(notifications[0].source == f.output &&
              notifications[1].source == f.translation,
          "source then routed destination causal order");
  require(notifications[0].turn == 1 && notifications[1].turn == 1 &&
              notifications[0].time.seconds == 7.0,
          "notifications carry turn/time");
  eq(std::get<sai::vec3f>(notifications.front().value), e.interpolated,
     "notification owns output copy");
  take(service.enqueue(c, {{f.fraction, 0.75f}}));
  const auto second = take(service.pump(a, {7.0}));
  require(second.turn == 2 && native.turns.back().tick == 2,
          "equal host time creates distinct native turn");
  require(native.turns.back().transformed == 1,
          "equal-time delta not suppressed");
  eq(native.turns.back().world_translation, {7.5f, 15, 22.5f},
     "second equal-time world transform");
  eq(std::get<sai::vec3f>(notifications.front().value), {2.5f, 5, 7.5f},
     "earlier notification not aliased to native storage");
  take(service.dispatch_notifications());
  const auto empty = take(service.pump(a, {7.0}));
  require(empty.turn == 3 && !empty.request && native.turns.back().tick == 3,
          "empty host turn ticks runtime");
  require(native.turns.back().transformed == 0,
          "empty turn no spurious RenderDelta");
}
void all_four_payload_kinds() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto f = make(service);
  // Setup authoring is intentionally distinct from event ingress.
  take(service.author(f.key, std::vector<float>{0, 1}));
  take(service.author(f.key_value,
                      std::vector<sai::vec3f>{{1, 2, 3}, {3, 6, 9}}));
  take(service.author(f.translation, sai::vec3f{1, 1, 1}));
  eq(std::get<sai::vec3f>(take(service.read(f.output))), {1, 2, 3},
     "setup pre-input output getter uses first keyValue");
  take(service.author(f.key_value, std::vector<sai::vec3f>{}));
  eq(std::get<sai::vec3f>(take(service.read(f.output))), {0, 0, 0},
     "empty setup keyValue restores output type default without an event");
  take(service.author(f.key_value,
                      std::vector<sai::vec3f>{{1, 2, 3}, {3, 6, 9}}));
  auto a = take(service.activate(f.scene));
  eq(std::get<sai::vec3f>(take(service.read(f.output))), {1, 2, 3},
     "activation preserves initialized native output readback");
  auto c = take(service.connect(a));
  std::vector<h::notification> notices;
  auto sk = take(
      service.observe(a, f.key, [&](const auto &n) { notices.push_back(n); }));
  auto sv = take(service.observe(a, f.key_value,
                                 [&](const auto &n) { notices.push_back(n); }));
  // Native live batch conversion: MFFloat, MFVec3f and SFFloat. Queue ordering
  // makes new key data visible to the real set_fraction event handler, even
  // when fraction appeared first in the admitted vector.
  take(service.enqueue(
      c, {{f.fraction, 0.25f},
          {f.key, std::vector<float>{0, 0.5f, 1}},
          {f.key_value,
           std::vector<sai::vec3f>{{0, 0, 0}, {4, 8, 12}, {8, 16, 24}}}}));
  take(service.pump(a, {0}));
  eq(native.turns.back().interpolated, {2, 4, 6},
     "live native array inputs reach interpolation handler");
  require(native.turns.back().translation_authored &&
              native.turns.back().key_authored &&
              native.turns.back().key_value_authored,
          "setup authoring preserves native authored-scalar presence marks");
  require(take(service.dispatch_notifications()) == 2,
          "array field events captured");
  require(std::get<std::vector<float>>(notices[0].value) ==
              std::vector<float>({0, 0.5f, 1}),
          "owning MFFloat event copy");
  require(std::get<std::vector<sai::vec3f>>(notices[1].value).at(1) ==
              sai::vec3f{4, 8, 12},
          "owning MFVec3f event copy");
  take(service.enqueue(c, {{f.translation, sai::vec3f{9, 8, 7}}}));
  take(service.pump(a, {0}));
  eq(native.turns.back().translation, {9, 8, 7},
     "live native SFVec3f conversion");
  eq(native.turns.back().world_translation, {9, 8, 7},
     "direct event input propagates native transforms");
}
void independent_scene_retirement() {
  // Constructor-supplied admission bounds reach the shared portable service.
  h::service limited(std::make_unique<inspected_backend>(),
                     h::resource_limits{1, 4096});
  auto bounded = make(limited);
  auto limited_world = take(limited.activate(bounded.scene));
  auto limited_client = take(limited.connect(limited_world));
  auto receipt =
      take(limited.enqueue(limited_client, {{bounded.fraction, .5f}}));
  auto rejected = limited.enqueue(limited_client, {{bounded.fraction, .75f}});
  require(!rejected && rejected.error().code == sai::error_code::resource_limit,
          "native constructor forwards the shared admission budget");
  take(limited.pump(limited_world, {0}));
  require(take(receipt.status()).state == h::request_state::processed,
          "native processing publishes the owning terminal receipt");
  take(limited.enqueue(limited_client, {{bounded.fraction, .75f}}));
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto one = make(service), two = make(service);
  require(!(one.scene == two.scene) && !(one.context == two.context) &&
              !(one.transform == two.transform),
          "separate scene/context/node identities");
  auto a1 = take(service.activate(one.scene)),
       a2 = take(service.activate(two.scene));
  auto c1 = take(service.connect(a1)), c2 = take(service.connect(a2));
  take(service.enqueue(c1, {{one.fraction, 0.2f}}));
  take(service.enqueue(c2, {{two.fraction, 0.8f}}));
  take(service.pump(a1, {100}));
  take(service.pump(a2, {1}));
  eq(std::get<sai::vec3f>(take(service.read(one.translation))), {2, 4, 6},
     "first scene native values independent");
  eq(std::get<sai::vec3f>(take(service.read(two.translation))), {8, 16, 24},
     "second scene clock and native values independent");
  take(service.retire(a1));
  require(take(service.state(one.scene)) == h::activation_state::retired,
          "scene explicitly retired");
  require(!service.read(one.translation), "retired handle rejected");
  take(service.enqueue(c2, {{two.fraction, 0.5f}}));
  take(service.pump(a2, {1}));
  eq(native.turns.back().world_translation, {5, 10, 15},
     "other session works after first retirement");
  require(native.turns.back().tick == 2 && native.turns.back().transformed == 1,
          "other runtime and extractor remain active");
}
void partial_failure_no_replay() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto f = make(service);
  auto a = take(service.activate(f.scene));
  auto c = take(service.connect(a));
  std::size_t notified = 0;
  auto sub = take(
      service.observe(a, f.translation, [&](const auto &) { ++notified; }));
  auto mutation = take(service.enqueue(c, {{f.fraction, 0.5f}}));
  auto later = take(service.enqueue(c, {{f.fraction, 0.9f}}));
  native.throw_after_tick = true;
  const auto report = take(service.pump(a, {3}));
  require(report.state == h::activation_state::faulted &&
              report.effects == h::failure_effect::partial_or_unknown &&
              report.error.has_value(),
          "post-mutation throw faults activation with honest effects");
  eq(native.turns.back().translation, {5, 10, 15},
     "injected fault happened after real mutation");
  eq(std::get<sai::vec3f>(take(service.read(f.translation))), {5, 10, 15},
     "fault does not falsely roll back native mutation");
  require(take(mutation.status()).state == h::request_state::failed,
          "failed request terminal");
  require(take(later.status()).state == h::request_state::cancelled,
          "later batch cancelled after fault");
  require(take(service.dispatch_notifications()) == 0 && notified == 0,
          "partial turn notifications discarded");
  native.throw_after_tick = false;
  require(!service.pump(a, {4}) && native.turns.size() == 1,
          "faulted native turn cannot replay");
  require(!service.enqueue(c, {{f.fraction, 0.1f}}),
          "faulted activation refuses new writes");
  take(service.retire(a));
  auto fresh = make(service);
  auto fresh_a = take(service.activate(fresh.scene));
  auto fresh_c = take(service.connect(fresh_a));
  take(service.enqueue(fresh_c, {{fresh.fraction, 0.25f}}));
  take(service.pump(fresh_a, {0}));
  eq(native.turns.back().translation, {2.5f, 5, 7.5f},
     "new activation contains no failed queued work");
}
void activation_failure_retirement() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto broken = make(service);
  native.throw_after_activation = true;
  const auto failed = service.activate(broken.scene);
  require(!failed && failed.error().code == sai::error_code::callback_failed,
          "activation throw after native construction returns explicit error");
  require(take(service.state(broken.scene)) == h::activation_state::faulted,
          "partially built session cannot masquerade as setup");
  require(!service.activate(broken.scene),
          "partial native activation cannot retry");
  take(service.retire(broken.scene));
  require(take(service.state(broken.scene)) == h::activation_state::retired,
          "scene retirement releases failed native activation");
  native.throw_after_activation = false;
  auto fresh = make(service);
  auto active = take(service.activate(fresh.scene));
  auto client = take(service.connect(active));
  take(service.enqueue(client, {{fresh.fraction, 0.5f}}));
  take(service.pump(active, {0}));
  eq(native.turns.back().translation, {5, 10, 15},
     "subsequent session works after partial activation teardown");
}

void destruction_during_deferred_notification() {
  auto service =
      std::make_unique<h::service>(std::make_unique<inspected_backend>());
  auto f = make(*service);
  auto active = take(service->activate(f.scene));
  auto client = take(service->connect(active));
  std::size_t calls = 0;
  auto destroys_owner =
      take(service->observe(active, f.output, [&](const auto &) {
        ++calls;
        service.reset();
      }));
  auto never_called = take(
      service->observe(active, f.translation, [&](const auto &) { ++calls; }));
  auto completed = take(service->enqueue(client, {{f.fraction, 0.5f}}));
  take(service->pump(active, {0}));
  auto pending = take(service->enqueue(client, {{f.fraction, 0.75f}}));
  require(calls == 0,
          "destructive application callback is deferred beyond native tick");
  auto *owner = service.get();
  require(
      take(owner->dispatch_notifications()) == 1,
      "native session owner can be destroyed during detached callback drain");
  require(!service && calls == 1 && f.transform.expired(),
          "native teardown suppresses pending callbacks and expires authority");
  require(
      take(completed.status()).state == h::request_state::processed &&
          take(pending.status()).state == h::request_state::cancelled,
      "native service destruction preserves consumer-owned terminal receipts");
}

// Input occurrence cardinality and output timestamp cardinality are distinct.
// No portable winner is inferred from the architecture's at-most-once rule.
void ordered_inputs_bounded_outputs() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto f = make(service);
  auto active = take(service.activate(f.scene));
  auto client = take(service.connect(active));
  std::vector<h::notification> outputs, routed;
  auto source = take(service.observe(
      active, f.output, [&](const auto &n) { outputs.push_back(n); }));
  auto sink = take(service.observe(
      active, f.translation, [&](const auto &n) { routed.push_back(n); }));
  take(service.begin_update(client));
  auto first = take(service.enqueue(client, {{f.fraction, .25f}}));
  auto same = take(service.enqueue(client, {{f.fraction, .25f}}));
  auto last = take(service.enqueue(client, {{f.fraction, .75f}}));
  take(service.end_update(client));
  take(service.pump(active, {7}));
  require(native.turns.size() == 1 &&
              native.turns.back().input_occurrences == 3,
          "all ordered external inputOnly occurrences are delivered in one "
          "native tick");
  take(service.dispatch_notifications());
  require(outputs.size() == 1 && routed.size() == 1,
          "one output-field event and one ROUTE delivery per timestamp");
  require(outputs.front().value == routed.front().value &&
              take(service.read(f.output)) == outputs.front().value &&
              take(service.read(f.translation)) == routed.front().value,
          "admitted runtime output, native readback and routed storage are "
          "coherent");
  require(take(first.status()).submission == take(same.status()).submission &&
              take(same.status()).submission == take(last.status()).submission,
          "the three occurrence receipts share one released gate");
}

void unnamed_routes_and_weak_identity() {
  auto owned = std::make_unique<sai::native::backend>();
  auto *native = owned.get();
  h::service service(std::move(owned));
  auto scene = take(service.create_scene());
  auto context = take(service.root_context(scene));
  auto interpolator =
      take(service.create_node(context, "PositionInterpolator"));
  auto transform = take(service.create_node(context, "Transform"));
  // Keep the behavior node detached: provider ownership, not root membership,
  // determines whether its live handle remains executable.
  take(service.append_root(context, transform));
  auto key = take(service.get_field(interpolator, "key")),
       values = take(service.get_field(interpolator, "keyValue"));
  auto fraction = take(service.get_field(interpolator, "set_fraction")),
       output = take(service.get_field(interpolator, "value_changed"));
  auto translation = take(service.get_field(transform, "translation"));
  take(service.author(key, std::vector<float>{0, 1}));
  take(
      service.author(values, std::vector<sai::vec3f>{{0, 0, 0}, {10, 20, 30}}));
  take(service.add_route(context, output, translation));
  auto duplicate = service.add_route(context, output, translation);
  require(!duplicate &&
              duplicate.error().code == sai::error_code::invalid_route,
          "duplicate unnamed ROUTE rejected");
  const auto storage = take(native->native_scene(1));
  require(storage->defs.empty() && storage->rootNodes.size() == 1 &&
              storage->rootNodes[0]->getDEF().empty(),
          "generic ROUTE creates no synthetic public DEF names");
  require(storage->routes.size() == 1 &&
              storage->routes.front().binding ==
                  x3d::runtime::Route::Binding::DirectNodes,
          "ROUTE lives in the authoritative native scene");
  auto retained = std::dynamic_pointer_cast<x3d::nodes::PositionInterpolator>(
      storage->routes.front().from.lock());
  require(bool(retained),
          "direct route retains the actual detached native interpolator");
  auto active = take(service.activate(scene));
  auto client = take(service.connect(active));
  take(service.enqueue(client, {{fraction, .5f}}));
  take(service.pump(active, {0}));
  eq(std::get<sai::vec3f>(take(service.read(translation))), {5, 10, 15},
     "native unnamed endpoint ROUTE executes");
  require(native->inspect(1).routes == 1,
          "native bridge admits the direct weak endpoint route");
  const auto completed = retained->getValue_changed();
  take(service.retire(active));
  retained->onSet_fraction(.75f);
  require(retained->getValue_changed() == completed && !service.read(output),
          "retired detached native callbacks stay inert even when the node is "
          "retained");

  // Default parsed/name-bound routes still refresh from the DEF table. A direct
  // route never rebinds an expired weak identity through a coincidental name.
  x3d::runtime::Scene native_scene;
  auto first = std::make_shared<x3d::nodes::Transform>();
  auto replacement = std::make_shared<x3d::nodes::Transform>();
  native_scene.define("A", first);
  native_scene.define("B", replacement);
  native_scene.routes.emplace_back("A", "translation", "B", "translation");
  auto direct = native_scene.routes.front();
  direct.binding = x3d::runtime::Route::Binding::DirectNodes;
  direct.from = first;
  direct.to = replacement;
  native_scene.routes.push_back(direct);
  native_scene.resolveRoutes();
  require(native_scene.routes[0].from.lock() == first,
          "named ROUTE default resolves as before");
  native_scene.define("A", replacement);
  first.reset();
  native_scene.resolveRoutes();
  require(native_scene.routes[0].from.lock() == replacement,
          "named ROUTE refresh follows DEF replacement");
  require(native_scene.routes[1].from.expired() &&
              native_scene.routes[1].to.lock() == replacement,
          "expired direct endpoint is never resurrected by a reused DEF name");
}

void shared_root_behavior_once() {
  auto owned = std::make_unique<inspected_backend>();
  auto &native = *owned;
  h::service service(std::move(owned));
  auto f = make(service);
  take(service.append_root(f.context, f.interpolator));
  require(take(service.roots(f.context)).size() == 4,
          "repeated root occurrence is preserved before activation");
  auto active = take(service.activate(f.scene));
  auto client = take(service.connect(active));
  std::size_t outputs = 0;
  auto observer =
      take(service.observe(active, f.output, [&](const auto &) { ++outputs; }));
  take(service.enqueue(client, {{f.fraction, .5f}}));
  take(service.pump(active, {0}));
  take(service.dispatch_notifications());
  require(
      outputs == 1 && native.turns.back().input_occurrences == 1,
      "shared/rooted node identity has one behavior attachment and one output");
  require(take(service.roots(f.context)).size() == 4,
          "activation does not fabricate or remove render roots");
}

void native_scene_information() {
  class information_backend : public sai::native::backend {
  public:
    sai::result<std::vector<sai::unit_declaration>> declarations() const {
      return do_units(1);
    }
  };
  auto owned = std::make_unique<information_backend>();
  auto *native = owned.get();
  h::service service(std::move(owned));
  auto scene = take(service.create_scene());
  auto context = take(service.root_context(scene));
  require(take(native->declarations()).empty(),
          "native document stores only explicitly declared units");
  take(service.declare_unit(scene, {"length", "centimetre", .01}));
  take(service.declare_unit(scene, {"angle", "degree", .017453292519943295}));
  take(service.set_metadata(scene, "title", "original"));
  take(service.set_metadata(scene, "Title", "case-sensitive"));
  take(service.set_metadata(scene, "title", "replacement"));
  take(service.set_metadata(scene, "", ""));
  const auto metadata = take(service.metadata(scene));
  require(metadata == std::vector<sai::metadata_entry>{
                          {"title", "replacement"},
                          {"Title", "case-sensitive"}, {"", ""}},
          "native metadata retains order, exact keys and empty values");
  const auto units = take(service.units(context));
  auto transform = take(service.create_node(context, "Transform"));
  auto interpolator = take(service.create_node(context, "PositionInterpolator"));
  auto shape = take(service.create_node(context, "Shape"));
  auto box = take(service.create_node(context, "Box"));
  take(service.define_name(transform, "Root"));
  take(service.define_name(interpolator, "Detached"));
  take(service.define_name(box, "DefaultBox"));
  take(service.append_root(context, transform));
  take(service.author(take(service.get_field(transform, "children")),
                      h::node_values{shape}));
  take(service.author(take(service.get_field(shape, "geometry")),
                      h::node_value{box}));
  const auto translation = take(service.get_field(transform, "translation"));
  const auto rotation = take(service.get_field(transform, "rotation"));
  const auto size = take(service.get_field(box, "size"));
  const auto key = take(service.get_field(interpolator, "key"));
  const auto values = take(service.get_field(interpolator, "keyValue"));
  const auto fraction = take(service.get_field(interpolator, "set_fraction"));
  const auto output = take(service.get_field(interpolator, "value_changed"));
  take(service.author(translation, sai::vec3f{3, 4, 5}));
  const sai::rotation canonical_rotation{0, 1, 0, .5f};
  take(service.author(rotation, canonical_rotation));
  take(service.author(key, std::vector<float>{0, 1}));
  take(service.author(values, std::vector<sai::vec3f>{{2, 4, 6}, {6, 8, 10}}));
  take(service.add_route(context, output, translation));
  {
    const auto storage = take(native->native_scene(1));
    const auto declared = take(native->declarations());
    require(storage->sourceUnits.size() == declared.size(),
            "native document and scene source declarations have matching size");
    for (std::size_t i = 0; i < declared.size(); ++i)
      require(storage->sourceUnits[i].category == declared[i].category &&
                  storage->sourceUnits[i].name == declared[i].name &&
                  storage->sourceUnits[i].conversionFactor ==
                      declared[i].conversion_factor,
              "native document and scene source declarations stay synchronized");
    require(storage->normalizedUnitFields.contains(storage->resolve("Root"),
                                                   "translation") &&
                storage->normalizedUnitFields.contains(storage->resolve("Root"),
                                                       "rotation") &&
                storage->normalizedUnitFields.contains(
                    storage->resolve("Detached"), "keyValue"),
            "all provider-authored dimensional scalars carry canonical marks");
    require(!storage->authoredScalarFields.contains(
                storage->resolve("DefaultBox"), "size"),
            "generated defaults remain unauthored");
  }
  auto active = take(service.activate(scene));
  require(take(service.units(context)) == units &&
              take(service.metadata(scene)) == metadata,
          "native document declarations survive runtime activation");
  eq(std::get<sai::vec3f>(take(service.read(translation))), {3, 4, 5},
     "length declaration never rescales canonical setup translation");
  require(std::get<sai::rotation>(take(service.read(rotation))) ==
              canonical_rotation,
          "angle declaration never rescales canonical setup rotation");
  eq(std::get<sai::vec3f>(take(service.read(size))), {2, 2, 2},
     "length declaration never rescales generated Box defaults");
  eq(std::get<sai::vec3f>(take(service.read(output))), {2, 4, 6},
     "detached initialized output remains canonical at activation");
  auto client = take(service.connect(active));
  take(service.enqueue(client, {{fraction, .5f}}));
  take(service.pump(active, {0}));
  eq(std::get<sai::vec3f>(take(service.read(translation))), {4, 6, 8},
     "detached interpolation and routed values remain canonical");
  take(service.enqueue(client, {{translation, sai::vec3f{9, 10, 11}}}));
  take(service.pump(active, {1}));
  eq(std::get<sai::vec3f>(take(service.read(translation))), {9, 10, 11},
     "live ingress remains canonical with retained source units");
}

} // namespace
int main() {
  try {
    // Leave unrelated author-field state alive while all provider scenes run.
    // The generic provider path must neither register its own dynamic fields
    // nor clear another native scene's explicitly owned author-field entries.
    x3d::runtime::Scene unrelated_scene;
    auto unrelated = std::make_shared<x3d::nodes::Transform>();
    unrelated_scene.rootNodes.push_back(unrelated);
    auto &author_fields = *unrelated_scene.authorFields;
    author_fields.addAuthorField(unrelated,
                                 {"provider_unrelated_sentinel",
                                  x3d::core::X3DFieldType::SFFloat,
                                  x3d::core::AccessType::InputOutput, 42.f});
    const auto author_count = author_fields.entryCount();
    const auto common = sai::testing::run_provider_live_fixture(
        [] { return sai::native::make_service(); });
    require(common.checks > 100, "shared live fixture executed deeply");
    std::cout << "native common live fixture: " << common.checks << " checks\n";
    native_route_and_render();
    all_four_payload_kinds();
    independent_scene_retirement();
    partial_failure_no_replay();
    activation_failure_retirement();
    destruction_during_deferred_notification();
    ordered_inputs_bounded_outputs();
    unnamed_routes_and_weak_identity();
    shared_root_behavior_once();
    native_scene_information();
    require(
        author_fields.entryCount() == author_count,
        "provider fixture does not populate or clear another scene's author "
        "fields");
    require(std::any_cast<float>(author_fields.getValue(
                *unrelated, "provider_unrelated_sentinel")) == 42.f,
            "unrelated native author field survives provider scene retirement");
    author_fields.erase(*unrelated);
    std::cout << "native runtime: six migrated plus input-cardinality, "
                 "unnamed/detached-route "
                 "and shared-root evidence cases passed\n";
  } catch (const std::exception &e) {
    std::cerr << "native provider runtime failed: " << e.what() << '\n';
    return 1;
  }
}
