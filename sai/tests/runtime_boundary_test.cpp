#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"

#include <array>
#include <future>
#include <optional>
#include <stdexcept>

namespace sai = x3d::sai::experimental;

namespace {
template <class T>
sai::field_descriptor port(std::string name, sai::access_type access,
                           T initial) {
  return {.name = std::move(name),
          .kind = sai::value_traits<T>::kind,
          .access = access,
          .default_value = std::move(initial),
          .accepted_node_types = {},
          .unit_category = std::nullopt};
}
sai::type_registry registry() {
  sai::type_registry types;
  REQUIRE(types.define(
      {.name = "Port",
       .fields = {port("input", sai::access_type::input_only, float{}),
                  port("value", sai::access_type::input_output, float{}),
                  port("output", sai::access_type::output_only, float{}),
                  port("node", sai::access_type::output_only, sai::node_id{}),
                  port("nodes", sai::access_type::output_only,
                       sai::node_list{})},
       .component = {},
       .interfaces = {}}));
  return types;
}
struct fixture {
  sai::browser host{registry()};
  sai::execution_context scene = host.current_scene();
  sai::node source;
  sai::node sink;
  fixture() {
    auto edit = scene.edit();
    source = edit.create_node("Port").value();
    sink = edit.create_node("Port").value();
    REQUIRE(edit.add_route(edit.field(source, "output").value(),
                           edit.field(sink, "value").value()));
    REQUIRE(edit.commit());
  }
  sai::dynamic_field field(const sai::node &n, const char *name) const {
    return scene.snapshot().field(n, name).value();
  }
  float read(const sai::node &n, const char *name) const {
    return std::get<float>(scene.snapshot().read(field(n, name)).value());
  }
};
} // namespace

TEST_CASE("runtime outputs settle with causal routes in one revision") {
  fixture f;
  const auto input = f.field(f.source, "input").as<float>().value();
  const auto output = f.field(f.source, "output").as<float>().value();
  const auto retained = f.field(f.sink, "value").as<float>().value();
  const auto before = f.scene.snapshot();
  int observed = 0;
  auto subscription =
      f.scene
          .observe(output,
                   [&](const sai::event_delivery &event) {
                     ++observed;
                     CHECK(event.origin == sai::event_origin::runtime);
                     CHECK(f.read(f.sink, "value") == 6);
                   })
          .value();
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input())
          return {};
        if (step.input()->target == f.source.id() &&
            step.input()->field == "input") {
          CHECK(step.time() == sai::event_time{1});
          CHECK(step.read(retained).value() == 0);
          CHECK(step.emit(output, std::get<float>(step.input()->payload) * 2));
        }
        if (step.input()->target == f.sink.id()) {
          CHECK(step.read(retained).value() == 6);
          CHECK(step.read(output).value() == 6);
          CHECK(f.scene.snapshot().revision() == before.revision());
        }
        return {};
      }));
  auto batch = f.scene.events({1});
  REQUIRE(batch.send(input, 3.f));
  auto result = batch.commit();
  REQUIRE(result);
  REQUIRE(result->deliveries.size() == 3);
  CHECK(result->deliveries[0].origin == sai::event_origin::external);
  CHECK(result->deliveries[1].origin == sai::event_origin::runtime);
  CHECK(result->deliveries[1].cause == 0);
  CHECK(result->deliveries[2].origin == sai::event_origin::route);
  CHECK(result->deliveries[2].cause == 1);
  CHECK(result->deliveries[2].route_index == 0);
  for (const auto &event : result->deliveries)
    CHECK(event.time == sai::event_time{1});
  CHECK(result->after_revision == before.revision() + 1);
  CHECK(f.read(f.source, "output") == 6);
  CHECK(f.read(f.sink, "value") == 6);
  CHECK(std::get<float>(before.read(output.dynamic()).value()) == 0);
  CHECK(observed == 0);
  CHECK(f.scene.drain().delivered == 1);
  CHECK(observed == 1);
  auto external = f.scene.events({2});
  auto denied = external.send(output, 4.f);
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);
}

TEST_CASE("failed runtime evaluation publishes no partial frame") {
  fixture f;
  const auto output = f.field(f.source, "output");
  const auto input = f.field(f.source, "input");
  const auto before = f.scene.snapshot();
  auto observer = f.scene.observe(
      [](const sai::change_set &) { FAIL("failed frame notified"); });
  SUBCASE("exception") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          if (!step.input())
            return {};
          REQUIRE(step.emit(output, sai::value{8.f}));
          throw std::runtime_error("native evaluation failed");
        }));
  }
  SUBCASE("ignored failed emit still poisons the frame") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          if (!step.input())
            return {};
          REQUIRE(step.emit(output, sai::value{8.f}));
          CHECK_FALSE(step.emit(input, sai::value{1.f}));
          return {};
        }));
  }
  SUBCASE("returned error") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          if (!step.input())
            return {};
          REQUIRE(step.emit(output, sai::value{8.f}));
          return sai::failure({.code = sai::error_code::callback_failed,
                               .operation = "test-runtime",
                               .message = "native evaluation refused",
                               .node = std::nullopt,
                               .declaration = std::nullopt,
                               .field = {}});
        }));
  }
  SUBCASE("routed sink fails after source output was applied") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          if (!step.input())
            return {};
          if (step.input()->target == f.source.id())
            return step.emit(output, sai::value{8.f});
          CHECK(std::get<float>(step.read(output).value()) == 8);
          return sai::failure({.code = sai::error_code::callback_failed,
                               .operation = "test-runtime",
                               .message = "sink refused the native result",
                               .node = std::nullopt,
                               .declaration = std::nullopt,
                               .field = {}});
        }));
  }
  auto batch = f.scene.events({1});
  REQUIRE(batch.send(input, sai::value{3.f}));
  CHECK_FALSE(batch.commit());
  CHECK(f.scene.snapshot().revision() == before.revision());
  CHECK_FALSE(f.scene.snapshot().is_realized(f.source).value());
  CHECK(f.read(f.source, "output") == 0);
  CHECK(f.read(f.sink, "value") == 0);
  CHECK(f.scene.drain().delivered == 0);
  observer.cancel();
  REQUIRE(f.host.set_runtime_processor(f.scene, {}));
  auto retry = f.scene.events({1});
  REQUIRE(retry.send(input, sai::value{3.f}));
  CHECK(retry.commit());
}

TEST_CASE("runtime output authority expires and validates every payload") {
  fixture f;
  const auto output = f.field(f.source, "output");
  const auto node_out = f.field(f.source, "node");
  const auto nodes_out = f.field(f.source, "nodes");
  std::optional<sai::runtime_step> saved;
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
        if (step.input())
          return {};
        saved = step;
        REQUIRE(step.emit(node_out, sai::node{}));
        const std::array payload{f.sink, sai::node{}, f.source};
        REQUIRE(step.emit(nodes_out, payload));
        return step.emit(output, sai::value{7.f});
      }));
  REQUIRE(f.scene.events({1}).commit());
  REQUIRE(saved);
  CHECK(saved->time() == sai::event_time{1});
  CHECK(saved->input() == nullptr);
  CHECK_FALSE(saved->read(output));
  auto expired = saved->emit(output, sai::value{9.f});
  REQUIRE_FALSE(expired);
  CHECK(expired.error().code == sai::error_code::stale_revision);
  auto snapshot = f.scene.snapshot();
  CHECK(snapshot.read_node(node_out).value().is_null());
  const auto nodes = snapshot.read_nodes(nodes_out).value();
  REQUIRE(nodes.size() == 3);
  CHECK(nodes[0].id() == f.sink.id());
  CHECK(nodes[1].is_null());
  CHECK(nodes[2].id() == f.source.id());
  sai::browser other{registry()};
  auto foreign_edit = other.current_scene().edit();
  auto foreign = foreign_edit.create_node("Port").value();
  REQUIRE(foreign_edit.commit());
  SUBCASE("foreign owner cannot install") {
    auto installed = other.set_runtime_processor(
        f.scene, [](sai::runtime_step &) -> sai::result<void> { return {}; });
    REQUIRE_FALSE(installed);
    CHECK(installed.error().code == sai::error_code::invalid_context);
  }
  SUBCASE("foreign node ID collision is checked before loop suppression") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          REQUIRE(step.emit(node_out, f.source));
          return step.emit(node_out, foreign);
        }));
    CHECK_FALSE(f.scene.events({2}).commit());
  }
  SUBCASE("raw node identity does not grant output authority") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          return step.emit(node_out, sai::value{f.source.id()});
        }));
    CHECK_FALSE(f.scene.events({2}).commit());
  }
  SUBCASE("representation mismatch") {
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          return step.emit(output, sai::value{std::string{"wrong"}});
        }));
    CHECK_FALSE(f.scene.events({2}).commit());
  }
  CHECK(f.read(f.source, "output") == 7);
}

TEST_CASE("runtime callbacks can inspect but cannot publish reentrantly") {
  fixture f;
  auto replacement = f.host.create_scene();
  auto ticket = f.host.begin_load();
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
        CHECK(f.scene.snapshot().revision() > 0);
        auto edit = f.scene.edit();
        REQUIRE(edit.set(f.field(f.sink, "value"), sai::value{5.f}));
        auto busy_edit = edit.commit();
        REQUIRE_FALSE(busy_edit);
        CHECK(busy_edit.error().code == sai::error_code::operation_in_progress);
        auto nested = f.scene.events({10}).commit();
        REQUIRE_FALSE(nested);
        CHECK(nested.error().code == sai::error_code::operation_in_progress);
        CHECK_FALSE(f.host.replace_world(replacement));
        CHECK_FALSE(f.host.complete_load(ticket, replacement));
        auto concurrent = std::async(std::launch::async, [&] {
                            return f.scene.events({100}).commit();
                          }).get();
        REQUIRE_FALSE(concurrent);
        CHECK(concurrent.error().code ==
              sai::error_code::operation_in_progress);
        CHECK_FALSE(f.host.set_runtime_processor(f.scene, {}));
        auto observe = f.scene.observe([](const sai::change_set &) {});
        if (step.input())
          return {};
        return step.emit(f.field(f.source, "output"), sai::value{2.f});
      }));
  REQUIRE(f.scene.events({1}).commit());
  CHECK(f.read(f.sink, "value") == 2);
  REQUIRE(f.host.set_runtime_processor(f.scene, {}));
  bool retired_outside_lock = false;
  auto lifetime = std::shared_ptr<int>{
      new int, [&](int *value) {
        delete value;
        CHECK(f.host.current_scene().generation() == replacement.generation());
        retired_outside_lock = true;
      }};
  auto retirement = f.scene.observe([lifetime](const sai::change_set &) {});
  lifetime.reset();
  REQUIRE(f.host.complete_load(ticket, replacement));
  CHECK(retired_outside_lock);
  CHECK(f.host.current_scene().generation() == replacement.generation());
}

TEST_CASE("ignored native inputs do not retain echo or notify") {
  fixture f;
  auto edit = f.scene.edit();
  const auto retained = f.field(f.source, "value").as<float>().value();
  REQUIRE(
      edit.add_route(retained, f.field(f.sink, "value").as<float>().value()));
  REQUIRE(edit.commit());
  bool ignore = true;
  float observed_value = 0;
  auto observed = f.scene
                      .observe(retained,
                               [&](const sai::event_delivery &event) {
                                 CHECK_FALSE(ignore);
                                 observed_value =
                                     std::get<float>(event.payload);
                               })
                      .value();
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input() || step.input()->target != f.source.id())
          return {};
        REQUIRE(step.read(retained));
        CHECK(step.read(retained).value() == 12);
        if (!ignore)
          return step.emit(retained, 7.f);
        REQUIRE(step.ignore_input());
        CHECK(step.read(retained).value() == 0);
        return {};
      }));
  auto batch = f.scene.events({1});
  REQUIRE(batch.send(retained, 12.f));
  auto result = batch.commit();
  REQUIRE(result);
  REQUIRE(result->deliveries.size() == 1);
  CHECK(result->deliveries[0].ignored);
  CHECK(f.read(f.source, "value") == 0);
  CHECK(f.read(f.sink, "value") == 0);
  CHECK(f.scene.drain().delivered == 0);
  CHECK(f.scene.snapshot().is_realized(f.source).value());
  ignore = false;
  auto adjusted = f.scene.events({2});
  REQUIRE(adjusted.send(retained, 12.f));
  auto processed = adjusted.commit();
  REQUIRE(processed);
  REQUIRE(processed->deliveries.size() == 3);
  CHECK(processed->deliveries[1].origin == sai::event_origin::runtime);
  CHECK(processed->deliveries[1].cause == 0);
  CHECK(processed->deliveries[2].cause == 1);
  CHECK(f.read(f.source, "value") == 7);
  CHECK(f.read(f.sink, "value") == 7);
  CHECK(f.scene.drain().delivered == 1);
  CHECK(observed_value == 7);
}

TEST_CASE("autonomous runtime output advances time and breaks feedback") {
  fixture f;
  auto edit = f.scene.edit();
  const auto output = f.field(f.source, "output").as<float>().value();
  const auto input = f.field(f.source, "input").as<float>().value();
  REQUIRE(edit.add_route(output, input));
  REQUIRE(edit.commit());
  int calls = 0;
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
        ++calls;
        return step.emit(output, static_cast<float>(step.time().seconds));
      }));
  auto tick = f.scene.events({1}).commit();
  REQUIRE(tick);
  CHECK(tick->deliveries.size() == 3);
  CHECK(calls == 3); // prepare, retained sink, input feedback
  CHECK(f.read(f.source, "output") == 1);
  auto backwards = f.scene.events({1}).commit();
  REQUIRE_FALSE(backwards);
  CHECK(backwards.error().code == sai::error_code::event_time_regression);
  CHECK(calls == 3);
  REQUIRE(f.scene.events({2}).commit());
  CHECK(f.read(f.source, "output") == 2);
}

TEST_CASE("world replacement retains the old mutex and retires the processor "
          "outside locks") {
  sai::browser host{registry()};
  auto replacement = host.create_scene();
  bool retired = false;
  auto lifetime = std::shared_ptr<int>{
      new int, [&](int *value) {
        delete value;
        CHECK(host.current_scene().generation() == replacement.generation());
        retired = true;
      }};
  REQUIRE(host.set_runtime_processor(
      host.current_scene(),
      [lifetime](sai::runtime_step &) -> sai::result<void> { return {}; }));
  lifetime.reset();
  // No caller holds the old context alive. Replacement must pin it while
  // unlocking, and callable destruction can reenter browser inspection.
  REQUIRE(host.replace_world(replacement));
  CHECK(retired);
}

TEST_CASE("runtime reads and outputs convert authored units exactly once") {
  auto types = registry();
  sai::node_type_descriptor measured;
  measured.name = "Measured";
  auto input = port("input", sai::access_type::input_output, float{});
  input.unit_category = "length";
  auto output = port("output", sai::access_type::output_only, float{});
  output.unit_category = "length";
  measured.fields = {input, output};
  REQUIRE(types.define(std::move(measured)));
  sai::browser host{std::move(types)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  REQUIRE(edit.declare_unit(
      {.category = "length", .name = "centimetre", .conversion_factor = 0.01}));
  auto source = edit.create_node("Measured").value();
  auto sink = edit.create_node("Measured").value();
  auto out = edit.field(source, "output").value().as<float>().value();
  auto in = edit.field(sink, "input").value().as<float>().value();
  REQUIRE(edit.add_route(out, in));
  REQUIRE(edit.commit());
  REQUIRE(host.set_runtime_processor(
      scene, [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input()) {
          return step.emit(out, 100.f, sai::value_space::authored);
        }
        CHECK(step.read(out).value() == 1);
        CHECK(step.read(in).value() == 1);
        CHECK(step.read(out, sai::value_space::authored).value() == 100);
        CHECK(
            std::get<float>(
                step.read(out.dynamic(), sai::value_space::authored).value()) ==
            100);
        CHECK(step.read(in, sai::value_space::authored).value() == 100);
        return {};
      }));
  auto tick = scene.events({1}).commit();
  REQUIRE(tick);
  REQUIRE(tick->deliveries.size() == 2);
  CHECK(std::get<float>(tick->deliveries[0].payload) == 1);
  CHECK(std::get<float>(tick->deliveries[1].payload) == 1);
  CHECK(scene.snapshot().read(out).value() == 1);
  CHECK(scene.snapshot().read(in).value() == 1);
  CHECK(scene.snapshot().read(in, sai::value_space::authored).value() == 100);
}

TEST_CASE("escaped runtime input facts survive destruction of their host") {
  std::optional<sai::runtime_step> saved;
  std::optional<sai::dynamic_field> output;
  {
    fixture f;
    output = f.field(f.source, "output");
    REQUIRE(f.host.set_runtime_processor(
        f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
          if (step.input())
            saved = step;
          return {};
        }));
    auto batch = f.scene.events({11});
    REQUIRE(batch.send(f.field(f.source, "input"), sai::value{99.f}));
    REQUIRE(batch.commit());
  }
  REQUIRE(saved);
  REQUIRE(saved->input());
  CHECK(saved->input()->field == "input");
  CHECK(std::get<float>(saved->input()->payload) == 99);
  CHECK(saved->time() == sai::event_time{11});
  CHECK_FALSE(saved->read(*output));
  CHECK_FALSE(saved->emit(*output, sai::value{100.f}));
}
