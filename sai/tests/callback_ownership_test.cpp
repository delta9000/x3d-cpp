#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"

#include <future>
#include <memory>
#include <stdexcept>
#include <utility>

namespace sai = x3d::sai::experimental;
namespace {
sai::type_registry types() {
  sai::type_registry registry;
  sai::node_type_descriptor type;
  type.name = "Port";
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_float;
  field.access = sai::access_type::input_output;
  field.default_value = 0.f;
  type.fields.push_back(field);
  field.name = "output";
  field.access = sai::access_type::output_only;
  type.fields.push_back(std::move(field));
  REQUIRE(registry.define(std::move(type)));
  return registry;
}
struct fixture {
  sai::browser host{types()};
  sai::execution_context scene = host.current_scene();
  sai::node port;
  fixture() {
    auto edit = scene.edit();
    port = edit.create_node("Port").value();
    REQUIRE(edit.commit());
  }
  sai::dynamic_field value() const {
    return scene.snapshot().field(port, "value").value();
  }
  sai::dynamic_field output() const {
    return scene.snapshot().field(port, "output").value();
  }
};
struct checkpoint {
  std::unique_ptr<int> count;
  explicit checkpoint(int n) : count(std::make_unique<int>(n)) {}
};
struct frame {
  sai::dynamic_field output;
  int count;
  float scale;
  sai::result<void> process(sai::runtime_step &step) {
    if (!step.input())
      return step.emit(output, sai::value{scale * count});
    return {};
  }
  sai::result<checkpoint> finish() { return checkpoint{count}; }
};
struct cleanup_probe {
  int destroyed = 0;
  std::function<void()> action;
};
struct resource {
  std::weak_ptr<cleanup_probe> probe;
  explicit resource(const std::shared_ptr<cleanup_probe> &owner)
      : probe(owner) {}
  ~resource() noexcept {
    if (auto owner = probe.lock()) {
      ++owner->destroyed;
      owner->action();
    }
  }
};
} // namespace

TEST_CASE("a move-only change observer can own a packaged task") {
  fixture f;
  std::packaged_task<sai::revision_id(const sai::change_set &)> task{
      [](const sai::change_set &change) { return change.after_revision; }};
  auto future = task.get_future();
  auto observer = f.scene.observe(std::move(task));
  REQUIRE(observer.active());
  auto edit = f.scene.edit();
  REQUIRE(edit.set(f.value(), sai::value{2.f}));
  const auto committed = edit.commit();
  REQUIRE(committed);
  const auto report = f.scene.drain();
  CHECK(report.delivered == 1);
  CHECK(report.errors.empty());
  CHECK(future.get() == committed->after_revision);
  observer.cancel();
}

TEST_CASE(
    "move-only field observers own promises through typed and dynamic access") {
  fixture f;
  std::promise<float> signal;
  auto future = signal.get_future();
  auto callback =
      [signal = std::move(signal)](const sai::event_delivery &event) mutable {
        signal.set_value(std::get<float>(event.payload));
      };
  static_assert(!std::copy_constructible<decltype(callback)>);
  sai::result<sai::subscription> registered = sai::failure({});
  SUBCASE("dynamic field") {
    registered = f.scene.observe(f.value(), std::move(callback));
  }
  SUBCASE("typed field") {
    registered =
        f.scene.observe(f.value().as<float>().value(), std::move(callback));
  }
  REQUIRE(registered);
  auto events = f.scene.events({1});
  REQUIRE(events.send(f.value(), sai::value{3.f}));
  REQUIRE(events.commit());
  const auto report = f.scene.drain();
  CHECK(report.delivered == 1);
  CHECK(report.errors.empty());
  CHECK(future.get() == 3.f);
}

TEST_CASE("a move-only runtime processor keeps its resource across frames") {
  fixture f;
  auto processor = [count = std::make_unique<int>(0), output = f.output()](
                       sai::runtime_step &step) mutable -> sai::result<void> {
    if (!step.input())
      return step.emit(output, sai::value{float(++*count)});
    return {};
  };
  static_assert(!std::copy_constructible<decltype(processor)>);
  REQUIRE(f.host.set_runtime_processor(f.scene, std::move(processor)));
  REQUIRE(f.scene.events({1}).commit());
  CHECK(std::get<float>(f.scene.snapshot().read(f.output()).value()) == 1.f);
  REQUIRE(f.scene.events({2}).commit());
  CHECK(std::get<float>(f.scene.snapshot().read(f.output()).value()) == 2.f);
  REQUIRE(f.host.set_runtime_processor(f.scene, {}));
}

TEST_CASE(
    "a move-only runtime factory owns its backend and publishes checkpoints") {
  fixture f;
  auto factory = [scale = std::make_unique<const float>(2.5f),
                  output = f.output()](
                     const sai::scene_snapshot &base,
                     sai::event_time) -> sai::result<sai::runtime_frame> {
    auto previous = base.runtime_state<checkpoint>();
    if (!previous)
      return sai::failure(previous.error());
    const int count = *previous ? *(*previous)->count + 1 : 1;
    return sai::runtime_frame{frame{output, count, *scale}};
  };
  static_assert(!std::copy_constructible<decltype(factory)>);
  REQUIRE(f.host.set_runtime_factory(f.scene, std::move(factory)));
  REQUIRE(f.scene.events({1}).commit());
  const auto historical = f.scene.snapshot();
  REQUIRE(f.scene.events({2}).commit());
  CHECK(*historical.runtime_state<checkpoint>().value()->count == 1);
  CHECK(*f.scene.snapshot().runtime_state<checkpoint>().value()->count == 2);
  CHECK(std::get<float>(f.scene.snapshot().read(f.output()).value()) == 5.f);
  REQUIRE(f.host.set_runtime_factory(f.scene, {}));
}

TEST_CASE("move-only callback resources retire outside internal locks") {
  fixture f;
  auto probe = std::make_shared<cleanup_probe>();
  probe->action = [&] {
    CHECK(f.scene.snapshot().revision() > 0);
    CHECK(f.host.current_scene().generation() > 0);
  };
  auto owned = std::make_unique<resource>(probe);
  SUBCASE("cancel change observation") {
    auto observer =
        f.scene.observe([owned = std::move(owned)](const sai::change_set &) {});
    CHECK(probe->destroyed == 0);
    observer.cancel();
  }
  SUBCASE("cancel field observation") {
    auto observer = f.scene.observe(
        f.value(), [owned = std::move(owned)](const sai::event_delivery &) {});
    REQUIRE(observer);
    CHECK(probe->destroyed == 0);
    observer->cancel();
  }
  SUBCASE("remove processor") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene,
        [owned = std::move(owned)](sai::runtime_step &) -> sai::result<void> {
          return {};
        }));
    CHECK(probe->destroyed == 0);
    REQUIRE(f.host.set_runtime_processor(f.scene, {}));
  }
  SUBCASE("retire world with factory") {
    REQUIRE(f.host.set_runtime_factory(
        f.scene,
        [owned = std::move(owned), output = f.output()](
            const sai::scene_snapshot &,
            sai::event_time) -> sai::result<sai::runtime_frame> {
          return sai::runtime_frame{frame{output, 1, 1}};
        }));
    CHECK(probe->destroyed == 0);
    REQUIRE(f.host.replace_world(f.host.create_scene()));
  }
  SUBCASE("reject foreign runtime owner") {
    sai::browser foreign{types()};
    auto installed = f.host.set_runtime_processor(
        foreign.current_scene(),
        [owned = std::move(owned)](sai::runtime_step &) -> sai::result<void> {
          return {};
        });
    REQUIRE_FALSE(installed);
    CHECK(installed.error().code == sai::error_code::invalid_context);
  }
  CHECK(probe->destroyed == 1);
}

TEST_CASE("a throwing move-only observer does not starve later observers") {
  fixture f;
  auto throwing = f.scene.observe(
      [owned = std::make_unique<int>(0)](const sai::change_set &) mutable {
        ++*owned;
        throw std::runtime_error("owned observer failed");
      });
  int calls = 0;
  auto later = f.scene.observe([&](const sai::change_set &) { ++calls; });
  auto edit = f.scene.edit();
  REQUIRE(edit.set(f.value(), sai::value{1.f}));
  REQUIRE(edit.commit());
  const auto report = f.scene.drain();
  CHECK(report.delivered == 1);
  REQUIRE(report.errors.size() == 1);
  CHECK(report.errors.front().code == sai::error_code::callback_failed);
  CHECK(calls == 1);
}
