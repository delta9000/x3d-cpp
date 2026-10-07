#include "DynamicField.hpp"
#include "x3d/nodes/Transform.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <x3d/sai/experimental/testing/hosted_fixture.hpp>
#include <x3d/sai_hosted.hpp>

namespace sai = x3d::sai::experimental;
namespace h = sai::hosted;
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
class inspected_service : public x3d::runtime::SaiHostedService {
public:
  using evidence = native_evidence;
  std::vector<evidence> turns;
  bool throw_after_tick = false;
  bool throw_after_activation = false;

protected:
  sai::result<void> do_activate(std::uint64_t scene) override {
    auto result = SaiHostedService::do_activate(scene);
    if (result && throw_after_activation)
      throw std::runtime_error("injected failure after native activation");
    return result;
  }
  backend_result do_turn(std::uint64_t scene, sai::event_time time,
                         const std::vector<admitted_write> &writes) override {
    auto result = SaiHostedService::do_turn(scene, time, writes);
    turns.push_back(inspect_builtin(scene));
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
  f.scene = take(service.create_route_fixture());
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
  inspected_service service;
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
  require(first.turn == 1 && first.request == ticket.id,
          "turn/request identity");
  require(notifications.empty(), "no application callbacks inside native turn");
  const auto &e = service.turns.back();
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
  require(second.turn == 2 && service.turns.back().tick == 2,
          "equal host time creates distinct native turn");
  require(service.turns.back().transformed == 1,
          "equal-time delta not suppressed");
  eq(service.turns.back().world_translation, {7.5f, 15, 22.5f},
     "second equal-time world transform");
  eq(std::get<sai::vec3f>(notifications.front().value), {2.5f, 5, 7.5f},
     "earlier notification not aliased to native storage");
  take(service.dispatch_notifications());
  const auto empty = take(service.pump(a, {7.0}));
  require(empty.turn == 3 && !empty.request && service.turns.back().tick == 3,
          "empty host turn ticks runtime");
  require(service.turns.back().transformed == 0,
          "empty turn no spurious RenderDelta");
}
void all_four_payload_kinds() {
  inspected_service service;
  auto f = make(service);
  // Setup authoring is intentionally distinct from event ingress.
  take(service.author(f.key, std::vector<float>{0, 1}));
  take(service.author(f.key_value,
                      std::vector<sai::vec3f>{{1, 2, 3}, {3, 6, 9}}));
  take(service.author(f.translation, sai::vec3f{1, 1, 1}));
  auto a = take(service.activate(f.scene));
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
  eq(service.turns.back().interpolated, {2, 4, 6},
     "live native array inputs reach interpolation handler");
  require(service.turns.back().translation_authored &&
              service.turns.back().key_authored &&
              service.turns.back().key_value_authored,
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
  eq(service.turns.back().translation, {9, 8, 7},
     "live native SFVec3f conversion");
  eq(service.turns.back().world_translation, {9, 8, 7},
     "direct event input propagates native transforms");
}
void independent_scene_retirement() {
  inspected_service service;
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
  eq(service.turns.back().world_translation, {5, 10, 15},
     "other session works after first retirement");
  require(service.turns.back().tick == 2 &&
              service.turns.back().transformed == 1,
          "other runtime and extractor remain active");
}
void partial_failure_no_replay() {
  inspected_service service;
  auto f = make(service);
  auto a = take(service.activate(f.scene));
  auto c = take(service.connect(a));
  std::size_t notified = 0;
  auto sub = take(
      service.observe(a, f.translation, [&](const auto &) { ++notified; }));
  auto mutation = take(service.enqueue(c, {{f.fraction, 0.5f}}));
  auto later = take(service.enqueue(c, {{f.fraction, 0.9f}}));
  service.throw_after_tick = true;
  const auto report = take(service.pump(a, {3}));
  require(report.state == h::activation_state::faulted &&
              report.effects == h::failure_effect::partial_or_unknown &&
              report.error.has_value(),
          "post-mutation throw faults activation with honest effects");
  eq(service.turns.back().translation, {5, 10, 15},
     "injected fault happened after real mutation");
  eq(std::get<sai::vec3f>(take(service.read(f.translation))), {5, 10, 15},
     "fault does not falsely roll back native mutation");
  require(take(service.status(c, mutation.id)).state ==
              h::request_state::failed,
          "failed request terminal");
  require(take(service.status(c, later.id)).state ==
              h::request_state::cancelled,
          "later batch cancelled after fault");
  require(take(service.dispatch_notifications()) == 0 && notified == 0,
          "partial turn notifications discarded");
  service.throw_after_tick = false;
  require(!service.pump(a, {4}) && service.turns.size() == 1,
          "faulted native turn cannot replay");
  require(!service.enqueue(c, {{f.fraction, 0.1f}}),
          "faulted activation refuses new writes");
  take(service.retire(a));
  auto fresh = make(service);
  auto fresh_a = take(service.activate(fresh.scene));
  auto fresh_c = take(service.connect(fresh_a));
  take(service.enqueue(fresh_c, {{fresh.fraction, 0.25f}}));
  take(service.pump(fresh_a, {0}));
  eq(service.turns.back().translation, {2.5f, 5, 7.5f},
     "new activation contains no failed queued work");
}
void activation_failure_retirement() {
  inspected_service service;
  auto broken = make(service);
  service.throw_after_activation = true;
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
  service.throw_after_activation = false;
  auto fresh = make(service);
  auto active = take(service.activate(fresh.scene));
  auto client = take(service.connect(active));
  take(service.enqueue(client, {{fresh.fraction, 0.5f}}));
  take(service.pump(active, {0}));
  eq(service.turns.back().translation, {5, 10, 15},
     "subsequent session works after partial activation teardown");
}

void destruction_during_deferred_notification() {
  auto service = std::make_unique<inspected_service>();
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
  take(service->enqueue(client, {{f.fraction, 0.5f}}));
  take(service->pump(active, {0}));
  require(calls == 0,
          "destructive application callback is deferred beyond native tick");
  auto *owner = service.get();
  require(
      take(owner->dispatch_notifications()) == 1,
      "native session owner can be destroyed during detached callback drain");
  require(!service && calls == 1 && f.transform.expired(),
          "native teardown suppresses pending callbacks and expires authority");
}

} // namespace
int main() {
  try {
    // Leave unrelated author-field state alive while all hosted scenes run.
    // The bounded built-in path must neither register its own dynamic fields
    // nor clear another native scene's process-wide author-field entries.
    auto unrelated = std::make_shared<x3d::nodes::Transform>();
    auto &author_fields = x3d::runtime::dynamicFieldStore();
    author_fields.addAuthorField(unrelated,
                                 {"hosted_unrelated_sentinel",
                                  x3d::core::X3DFieldType::SFFloat,
                                  x3d::core::AccessType::InputOutput, 42.f});
    const auto author_count = author_fields.entryCount();
    const auto common = sai::testing::run_hosted_fixture(
        []() -> sai::result<std::unique_ptr<h::service>> {
          return std::unique_ptr<h::service>{
              new x3d::runtime::SaiHostedService};
        });
    require(common.checks > 100, "shared hosted fixture executed deeply");
    std::cout << "native common hosted fixture: " << common.checks
              << " checks\n";
    native_route_and_render();
    all_four_payload_kinds();
    independent_scene_retirement();
    partial_failure_no_replay();
    activation_failure_retirement();
    destruction_during_deferred_notification();
    require(author_fields.entryCount() == author_count,
            "hosted fixture does not populate or clear mutable global author "
            "fields");
    require(std::any_cast<float>(author_fields.getValue(
                *unrelated, "hosted_unrelated_sentinel")) == 42.f,
            "unrelated native author field survives hosted scene retirement");
    author_fields.erase(*unrelated);
    std::cout << "hosted native runtime: six evidence cases passed\n";
  } catch (const std::exception &e) {
    std::cerr << "hosted native runtime failed: " << e.what() << '\n';
    return 1;
  }
}
