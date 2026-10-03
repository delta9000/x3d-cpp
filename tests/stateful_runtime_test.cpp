#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"
#include <memory>
#include <stdexcept>

namespace sai = x3d::sai::experimental;
namespace {
struct checkpoint {
  std::unique_ptr<int> counter;
  explicit checkpoint(int n) : counter(std::make_unique<int>(n)) {}
};
struct frame {
  sai::dynamic_field output;
  sai::scene_snapshot base;
  int count;
  bool fail_process = false;
  bool fail_finish = false;
  int *destroyed = nullptr;
  const sai::execution_context *scene_for_cleanup = nullptr;
  std::unique_ptr<int> resource = std::make_unique<int>(0);
  frame(sai::dynamic_field port, sai::scene_snapshot snapshot, int n,
        bool process_failure = false, bool finish_failure = false,
        int *destruction = nullptr,
        const sai::execution_context *cleanup_scene = nullptr)
      : output(std::move(port)), base(std::move(snapshot)), count(n),
        fail_process(process_failure), fail_finish(finish_failure),
        destroyed(destruction), scene_for_cleanup(cleanup_scene) {}
  frame(frame &&) noexcept = default;
  ~frame() noexcept {
    if (resource && destroyed) {
      CHECK(base.revision() > 0);
      if (scene_for_cleanup)
        CHECK(scene_for_cleanup->snapshot().revision() > 0);
      ++*destroyed;
    }
  }
  sai::result<void> process(sai::runtime_step &step) {
    if (!step.input()) {
      ++count;
      return step.emit(output, sai::value{static_cast<float>(count)});
    }
    if (fail_process)
      throw std::runtime_error("late process failure");
    return {};
  }
  sai::result<checkpoint> finish() {
    if (fail_finish)
      throw std::runtime_error("checkpoint failure");
    return checkpoint{count};
  }
};
struct fixture {
  sai::browser host;
  sai::execution_context scene;
  sai::node source;
  sai::node sink;
  explicit fixture() : host{types()}, scene(host.current_scene()) {
    auto edit = scene.edit();
    source = edit.create_node("Port").value();
    sink = edit.create_node("Port").value();
    REQUIRE(edit.add_route(edit.field(source, "output").value(),
                           edit.field(sink, "input").value()));
    REQUIRE(edit.commit());
  }
  static sai::type_registry types() {
    sai::type_registry registry;
    sai::node_type_descriptor type;
    type.name = "Port";
    sai::field_descriptor input;
    input.name = "input";
    input.kind = sai::value_kind::sf_float;
    input.access = sai::access_type::input_output;
    input.default_value = 0.f;
    sai::field_descriptor output = input;
    output.name = "output";
    output.access = sai::access_type::output_only;
    type.fields = {input, output};
    REQUIRE(registry.define(std::move(type)));
    return registry;
  }
  sai::dynamic_field output() const {
    return scene.snapshot().field(source, "output").value();
  }
};
static_assert(sai::runtime_frame_adapter<frame>);
static_assert(!std::copy_constructible<frame>);
static_assert(!std::copy_constructible<checkpoint>);
static_assert(!std::copy_constructible<sai::runtime_frame>);
struct invalid_frame {
  void process(sai::runtime_step &);
  int finish();
};
static_assert(!sai::runtime_frame_adapter<invalid_frame>);
} // namespace

TEST_CASE(
    "stateful runtime checkpoints publish with fields and retain history") {
  fixture f;
  const auto empty = f.scene.snapshot();
  CHECK_FALSE(empty.runtime_state<checkpoint>().value());
  REQUIRE(f.host.set_runtime_factory(
      f.scene,
      [&](const sai::scene_snapshot &base,
          sai::event_time) -> sai::result<sai::runtime_frame> {
        auto previous = base.runtime_state<checkpoint>();
        if (!previous)
          return sai::failure(previous.error());
        return sai::runtime_frame{
            frame{f.output(), base, *previous ? *(*previous)->counter : 0}};
      }));
  int observed = 0;
  auto observer = f.scene.observe([&](const sai::change_set &change) {
    ++observed;
    CHECK(f.scene.snapshot().revision() == change.after_revision);
    CHECK(*f.scene.snapshot().runtime_state<checkpoint>().value()->counter ==
          1);
  });
  auto first = f.scene.events({1}).commit();
  REQUIRE(first);
  REQUIRE(first->state_changes);
  CHECK(first->after_revision == empty.revision() + 1);
  const auto snapshot = f.scene.snapshot();
  auto old = snapshot.runtime_state<checkpoint>().value();
  CHECK(*old->counter == 1);
  CHECK(std::get<float>(snapshot.read(f.output()).value()) == 1);
  auto mismatched = snapshot.runtime_state<int>();
  REQUIRE_FALSE(mismatched);
  CHECK(mismatched.error().code == sai::error_code::type_mismatch);
  const auto *change = std::get_if<sai::runtime_state_change>(
      &first->state_changes->changes.back());
  REQUIRE(change);
  CHECK_FALSE(change->before<checkpoint>().value());
  CHECK(change->after<checkpoint>().value().get() == old.get());
  CHECK(sai::change_kind_of(first->state_changes->changes.back()) ==
        sai::semantic_change_kind{sai::runtime_change_kind::checkpoint});
  CHECK(f.scene.drain().delivered == 1);
  observer.cancel();
  REQUIRE(f.scene.events({2}).commit());
  CHECK(*f.scene.snapshot().runtime_state<checkpoint>().value()->counter == 2);
  CHECK(*old->counter == 1);
  CHECK(snapshot.runtime_state<checkpoint>().value().get() == old.get());
  CHECK_FALSE(empty.runtime_state<checkpoint>().value());
}

TEST_CASE(
    "stateful runtime failures abort the checkpoint and the settled cascade") {
  fixture f;
  int destroyed = 0;
  bool process_failure = false;
  bool finish_failure = false;
  REQUIRE(f.host.set_runtime_factory(
      f.scene,
      [&](const sai::scene_snapshot &base,
          sai::event_time) -> sai::result<sai::runtime_frame> {
        auto state = base.runtime_state<checkpoint>().value();
        return sai::runtime_frame{
            frame{f.output(), base, state ? *state->counter : 0,
                  process_failure, finish_failure, &destroyed, &f.scene}};
      }));
  REQUIRE(f.scene.events({1}).commit());
  CHECK(destroyed == 1);
  auto before = f.scene.snapshot();
  auto state = before.runtime_state<checkpoint>().value();
  SUBCASE("late input processing") { process_failure = true; }
  SUBCASE("finalization after all routed fields settled") {
    finish_failure = true;
  }
  auto rejected = f.scene.events({2}).commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::callback_failed);
  CHECK(destroyed == 2);
  CHECK(f.scene.snapshot().revision() == before.revision());
  CHECK(f.scene.snapshot().runtime_state<checkpoint>().value().get() ==
        state.get());
  CHECK(std::get<float>(f.scene.snapshot().read(f.output()).value()) == 1);
  CHECK(f.scene.drain().delivered == 0);
  process_failure = false;
  finish_failure = false;
  REQUIRE(f.scene.events({2}).commit());
  CHECK(*f.scene.snapshot().runtime_state<checkpoint>().value()->counter == 2);
  CHECK(destroyed == 3);
}

TEST_CASE("stateful-only frames revise and stateless processing clears the "
          "checkpoint") {
  fixture f;
  struct quiet_frame {
    sai::result<void> process(sai::runtime_step &) { return {}; }
    sai::result<checkpoint> finish() { return checkpoint{42}; }
  };
  REQUIRE(f.host.set_runtime_factory(
      f.scene,
      [](const sai::scene_snapshot &,
         sai::event_time) -> sai::result<sai::runtime_frame> {
        return sai::runtime_frame{quiet_frame{}};
      }));
  auto before = f.scene.snapshot();
  auto completed = f.scene.events({1}).commit();
  REQUIRE(completed);
  CHECK(completed->deliveries.empty());
  CHECK(completed->after_revision == before.revision() + 1);
  REQUIRE(completed->state_changes);
  CHECK(completed->state_changes->changes.size() == 1);
  auto pinned = f.scene.snapshot();
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [](sai::runtime_step &) -> sai::result<void> { return {}; }));
  auto cleared = f.scene.events({2}).commit();
  REQUIRE(cleared);
  CHECK(cleared->after_revision == completed->after_revision + 1);
  CHECK_FALSE(f.scene.snapshot().runtime_state<checkpoint>().value());
  CHECK(*pinned.runtime_state<checkpoint>().value()->counter == 42);
  const auto &change = std::get<sai::runtime_state_change>(
      cleared->state_changes->changes.front());
  CHECK(*change.before<checkpoint>().value()->counter == 42);
  CHECK_FALSE(change.after<checkpoint>().value());
}

TEST_CASE(
    "runtime factory and finalizer failures preserve state and diagnostics") {
  fixture f;
  struct quiet_frame {
    bool reject;
    sai::result<void> process(sai::runtime_step &) { return {}; }
    sai::result<checkpoint> finish() {
      if (!reject)
        return checkpoint{7};
      sai::sai_error error;
      error.code = sai::error_code::invalid_value;
      error.operation = "test.finish";
      error.message = "checkpoint was rejected";
      return sai::failure(std::move(error));
    }
  };
  bool reject_factory = false;
  bool reject_finish = false;
  bool throw_factory = false;
  bool throw_unknown = false;
  REQUIRE(f.host.set_runtime_factory(
      f.scene,
      [&](const sai::scene_snapshot &base,
          sai::event_time) -> sai::result<sai::runtime_frame> {
        CHECK(base.revision() == f.scene.snapshot().revision());
        auto forbidden = f.scene.edit().commit();
        REQUIRE_FALSE(forbidden);
        CHECK(forbidden.error().code == sai::error_code::operation_in_progress);
        if (throw_factory)
          throw std::runtime_error("factory failed");
        if (throw_unknown)
          throw 7;
        if (reject_factory) {
          sai::sai_error error;
          error.code = sai::error_code::invalid_value;
          error.operation = "test.factory";
          return sai::failure(std::move(error));
        }
        return sai::runtime_frame{quiet_frame{reject_finish}};
      }));
  REQUIRE(f.scene.events({1}).commit());
  const auto before = f.scene.snapshot();
  const auto pinned = before.runtime_state<checkpoint>().value();
  SUBCASE("factory returned error") { reject_factory = true; }
  SUBCASE("factory standard exception") { throw_factory = true; }
  SUBCASE("factory non-standard exception") { throw_unknown = true; }
  SUBCASE("finalizer returned error") { reject_finish = true; }
  auto rejected = f.scene.events({2}).commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == ((reject_factory || reject_finish)
                                      ? sai::error_code::invalid_value
                                      : sai::error_code::callback_failed));
  if (reject_factory)
    CHECK(rejected.error().operation == "test.factory");
  if (reject_finish)
    CHECK(rejected.error().operation == "test.finish");
  CHECK(f.scene.snapshot().revision() == before.revision());
  CHECK(f.scene.snapshot().runtime_state<checkpoint>().value().get() ==
        pinned.get());
  CHECK(f.scene.drain().delivered == 0);
  reject_factory = reject_finish = throw_factory = throw_unknown = false;
  REQUIRE(f.scene.events({2}).commit());
  // An authoring edit preserves the prior checkpoint without running its host.
  auto edit = f.scene.edit();
  REQUIRE(edit.set(edit.field(f.sink, "input").value(), sai::value{8.f}));
  auto state = f.scene.snapshot().runtime_state<checkpoint>().value();
  REQUIRE(edit.commit());
  CHECK(f.scene.snapshot().runtime_state<checkpoint>().value().get() ==
        state.get());
}

TEST_CASE(
    "retired native checkpoints release resources outside browser locks") {
  fixture f;
  struct owned_checkpoint {
    std::unique_ptr<int> resource = std::make_unique<int>(0);
    sai::browser *host;
    sai::generation_id old_generation;
    bool *destroyed;
    owned_checkpoint(sai::browser *browser, sai::generation_id generation,
                     bool *done)
        : host(browser), old_generation(generation), destroyed(done) {}
    owned_checkpoint(owned_checkpoint &&) noexcept = default;
    ~owned_checkpoint() noexcept {
      if (resource) {
        CHECK(host->current_scene().generation() != old_generation);
        *destroyed = true;
      }
    }
  };
  struct quiet_frame {
    sai::browser *host;
    sai::generation_id generation;
    bool *destroyed;
    sai::result<void> process(sai::runtime_step &) { return {}; }
    sai::result<owned_checkpoint> finish() {
      return owned_checkpoint{host, generation, destroyed};
    }
  };
  bool destroyed = false;
  {
    auto old = f.host.create_scene();
    REQUIRE(f.host.replace_world(old));
    const auto generation = old.generation();
    REQUIRE(f.host.set_runtime_factory(
        old,
        [&, generation](const sai::scene_snapshot &,
                        sai::event_time) -> sai::result<sai::runtime_frame> {
          return sai::runtime_frame{
              quiet_frame{&f.host, generation, &destroyed}};
        }));
    REQUIRE(old.events({1}).commit());
  }
  REQUIRE_FALSE(destroyed);
  REQUIRE(f.host.replace_world(f.host.create_scene()));
  CHECK(destroyed);
}

TEST_CASE(
    "queued runtime checkpoint facts survive later frames and nested drains") {
  fixture f;
  REQUIRE(f.host.set_runtime_factory(
      f.scene,
      [&](const sai::scene_snapshot &base,
          sai::event_time) -> sai::result<sai::runtime_frame> {
        auto previous = base.runtime_state<checkpoint>().value();
        return sai::runtime_frame{
            frame{f.output(), base, previous ? *previous->counter : 0}};
      }));
  std::vector<int> before_values;
  std::vector<int> after_values;
  auto observer = f.scene.observe([&](const sai::change_set &changes) {
    for (const auto &change : changes.changes) {
      if (const auto *native =
              std::get_if<sai::runtime_state_change>(&change)) {
        const auto before = native->before<checkpoint>().value();
        const auto after = native->after<checkpoint>().value();
        before_values.push_back(before ? *before->counter : 0);
        after_values.push_back(*after->counter);
      }
    }
    const auto nested = f.scene.drain();
    CHECK(nested.already_draining);
    CHECK(nested.delivered == 0);
  });
  REQUIRE(f.scene.events({1}).commit());
  REQUIRE(f.scene.events({2}).commit());
  CHECK(*f.scene.snapshot().runtime_state<checkpoint>().value()->counter == 2);
  const auto drained = f.scene.drain();
  CHECK_FALSE(drained.already_draining);
  CHECK(drained.delivered == 2);
  CHECK(before_values == std::vector<int>{0, 1});
  CHECK(after_values == std::vector<int>{1, 2});
}
