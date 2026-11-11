#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/session.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace sai = x3d::sai::experimental;
namespace {
struct logical_fixture {
  static sai::type_registry types() {
    sai::type_registry registry;
    sai::node_type_descriptor type;
    type.name = "HostedPort";
    for (const auto &[name, access] : {
             std::pair{"input", sai::access_type::input_only},
             std::pair{"output", sai::access_type::output_only},
             std::pair{"value", sai::access_type::input_output}}) {
      sai::field_descriptor field;
      field.name = name;
      field.kind = sai::value_kind::sf_float;
      field.access = access;
      field.default_value = 0.f;
      type.fields.push_back(std::move(field));
    }
    REQUIRE(registry.define(std::move(type)));
    return registry;
  }
  sai::browser host{types()};
  sai::execution_context scene = host.current_scene();
  sai::node owner;
  logical_fixture() {
    auto edit = scene.edit();
    owner = edit.create_node("HostedPort").value();
    REQUIRE(edit.add_route(edit.field(owner, "output").value(),
                           edit.field(owner, "value").value()));
    REQUIRE(edit.commit());
  }
  sai::dynamic_field field(const char *name) const {
    return scene.snapshot().field(owner, name).value();
  }
  sai::result<sai::event_result> send(double time, float value, bool logical) {
    auto batch = logical ? scene.logical_turn({time}) : scene.events({time});
    auto sent = batch.send(field("value"), sai::value{value});
    REQUIRE(sent);
    return batch.commit();
  }
};
}

TEST_CASE("hosted logical turns share strict event admission without weakening it") {
  logical_fixture f;
  REQUIRE(f.send(1, 1, false));
  REQUIRE(f.send(1, 2, true));
  auto strict_equal = f.send(1, 3, false);
  REQUIRE_FALSE(strict_equal);
  CHECK(strict_equal.error().code == sai::error_code::event_time_regression);
  auto logical_past = f.send(0.5, 3, true);
  REQUIRE_FALSE(logical_past);
  CHECK(logical_past.error().code == sai::error_code::event_time_regression);
  REQUIRE(f.send(2, 3, false));
  REQUIRE(f.send(2, 4, true));
  CHECK(std::get<float>(f.scene.snapshot().read(f.field("value")).value()) == 4);

  sai::external_session client{f.scene};
  auto failed_ticket = client.set_value(f.field("value"), sai::value{5.f});
  REQUIRE(failed_ticket);
  auto strict_session = client.process_next({2});
  REQUIRE(strict_session);
  REQUIRE(strict_session->completions.size() == 1);
  REQUIRE(strict_session->completions.front().error);
  CHECK(strict_session->completions.front().error->code ==
        sai::error_code::event_time_regression);
  REQUIRE(client.set_value(f.field("value"), sai::value{5.f}));
  auto advanced_session = client.process_next({3});
  REQUIRE(advanced_session);
  REQUIRE(advanced_session->completions.size() == 1);
  CHECK_FALSE(advanced_session->completions.front().error);
  REQUIRE(f.send(3, 6, true));
  CHECK_FALSE(f.send(3, 7, false));
}

TEST_CASE("empty hosted logical turns admit finite nondecreasing time") {
  logical_fixture f;
  REQUIRE(f.scene.logical_turn({-2}).commit());
  REQUIRE(f.scene.logical_turn({-2}).commit());
  REQUIRE(f.scene.logical_turn({0}).commit());
  CHECK_FALSE(f.scene.logical_turn({-1}).commit());
  CHECK_FALSE(f.send(0, 1, false));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  for (const double invalid : {nan, inf, -inf}) {
    auto rejected = f.scene.logical_turn({invalid}).commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
    CHECK(rejected.error().operation == "execution_context.logical_turn");
  }
  REQUIRE(f.send(0, 2, true));
  REQUIRE(f.send(1, 3, false));
}

TEST_CASE("equal hosted times produce separate cascades and deferred callbacks") {
  logical_fixture f;
  const auto input = f.field("input");
  const auto output = f.field("output");
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [output](sai::runtime_step &step) -> sai::result<void> {
        if (step.input() && step.input()->field == "input")
          return step.emit(output, step.input()->payload);
        return {};
      }));
  std::vector<sai::event_delivery> observed;
  auto output_subscription = f.scene.observe(output, [&](const auto &event) {
    observed.push_back(event);
  }).value();
  auto sink_subscription = f.scene.observe(f.field("value"), [&](const auto &event) {
    observed.push_back(event);
  }).value();
  for (const float value : {7.f, 7.f, 8.f}) {
    auto batch = f.scene.logical_turn({4});
    REQUIRE(batch.send(input, sai::value{value}));
    auto result = batch.commit();
    REQUIRE(result);
    REQUIRE(result->deliveries.size() == 3);
    CHECK(result->deliveries[0].origin == sai::event_origin::external);
    CHECK(result->deliveries[1].origin == sai::event_origin::runtime);
    CHECK(result->deliveries[1].cause == 0);
    CHECK(result->deliveries[2].origin == sai::event_origin::route);
    CHECK(result->deliveries[2].cause == 1);
    CHECK(result->deliveries[2].route_index == 0);
    CHECK(std::get<float>(result->deliveries[2].payload) == value);
    CHECK(observed.empty());
  }
  auto dispatched = f.scene.drain();
  CHECK(dispatched.errors.empty());
  CHECK(dispatched.delivered == 6);
  REQUIRE(observed.size() == 6);
  for (std::size_t index = 0; index < observed.size(); ++index) {
    CHECK(observed[index].time == sai::event_time{4});
    CHECK(std::get<float>(observed[index].payload) == (index < 4 ? 7.f : 8.f));
  }
  CHECK(f.scene.drain().delivered == 0);
  CHECK_FALSE(f.send(4, 9, false));
}

TEST_CASE("failed hosted logical turns leave state and admission clock unchanged") {
  logical_fixture f;
  REQUIRE(f.send(1, 1, true));
  const auto revision = f.scene.snapshot().revision();
  auto poisoned = f.scene.logical_turn({10});
  REQUIRE_FALSE(poisoned.send(f.field("value"), sai::value{std::string{"bad"}}));
  CHECK_FALSE(poisoned.commit());
  CHECK(f.scene.snapshot().revision() == revision);
  REQUIRE(f.send(1, 2, true));

  bool fail = true;
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&](sai::runtime_step &) -> sai::result<void> {
        if (!fail) return {};
        sai::sai_error error;
        error.code = sai::error_code::callback_failed;
        error.operation = "hosted_logical_turn_test";
        error.message = "deliberate evaluation failure";
        return sai::failure(std::move(error));
      }));
  const auto before_failure = f.scene.snapshot().revision();
  auto rejected = f.send(20, 20, true);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::callback_failed);
  CHECK(f.scene.snapshot().revision() == before_failure);
  CHECK(std::get<float>(f.scene.snapshot().read(f.field("value")).value()) == 2);
  fail = false;
  REQUIRE(f.send(2, 3, false));
  REQUIRE(f.send(2, 4, true));

  auto stale = f.scene.logical_turn({30});
  REQUIRE(stale.send(f.field("value"), sai::value{30.f}));
  REQUIRE(f.send(3, 5, false));
  auto stale_result = stale.commit();
  REQUIRE_FALSE(stale_result);
  CHECK(stale_result.error().code == sai::error_code::stale_revision);
  REQUIRE(f.send(3, 6, true));
  REQUIRE(f.send(4, 7, false));
}

TEST_CASE("logical turns deliver every external inputOnly occurrence without resetting loop breakers") {
  logical_fixture f;
  const auto input = f.field("input");
  const auto output = f.field("output");
  std::vector<float> inputs;
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&, output](sai::runtime_step &step) -> sai::result<void> {
        if (step.input() && step.input()->field == "input") {
          inputs.push_back(std::get<float>(step.input()->payload));
          return step.emit(output, step.input()->payload);
        }
        return {};
      }));
  std::vector<float> observed;
  auto subscription = f.scene.observe(output, [&](const auto &event) {
    observed.push_back(std::get<float>(event.payload));
  }).value();
  auto turn = f.scene.logical_turn({7});
  for (float value : {.25f, .25f, .75f})
    REQUIRE(turn.send(input, sai::value{value}));
  auto result = turn.commit();
  REQUIRE(result);
  // Count real runtime input processing independently of emitted output counts.
  CHECK(inputs == std::vector<float>{.25f, .25f, .75f});
  REQUIRE(result->deliveries.size() == 5);
  for (std::size_t i = 0; i != 3; ++i) {
    CHECK(result->deliveries[i].origin == sai::event_origin::external);
    CHECK(result->deliveries[i].time == sai::event_time{7});
    CHECK(std::get<float>(result->deliveries[i].payload) == inputs[i]);
  }
  CHECK(result->deliveries[3].origin == sai::event_origin::runtime);
  CHECK(result->deliveries[3].cause == 0);
  CHECK(result->deliveries[4].origin == sai::event_origin::route);
  CHECK(result->deliveries[4].cause == 3);
  CHECK(result->deliveries[4].route_index == 0);
  CHECK(std::get<float>(f.scene.snapshot().read(output).value()) == .25f);
  CHECK(std::get<float>(f.scene.snapshot().read(f.field("value")).value()) == .25f);
  CHECK(observed.empty());
  CHECK(f.scene.drain().delivered == 1);
  CHECK(observed == std::vector<float>{.25f});
}

TEST_CASE("logical inputOnly occurrence policy leaves strict and retained duplicate checks intact") {
  logical_fixture f;
  auto strict = f.scene.events({1});
  REQUIRE(strict.send(f.field("input"), sai::value{.25f}));
  auto duplicate = strict.send(f.field("input"), sai::value{.25f});
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::ambiguous_event_seed);
  CHECK_FALSE(strict.commit());
  auto retained = f.scene.logical_turn({1});
  REQUIRE(retained.send(f.field("value"), sai::value{.25f}));
  auto repeated = retained.send(f.field("value"), sai::value{.75f});
  REQUIRE_FALSE(repeated);
  CHECK(repeated.error().code == sai::error_code::ambiguous_event_seed);
  CHECK_FALSE(retained.commit());
}

TEST_CASE("reference construction edits preserve setup references and close on activation") {
  logical_fixture f;
  auto setup = f.host.create_scene();
  auto initial = setup.construction_edit();
  auto owner = initial.create_node("HostedPort").value();
  REQUIRE(initial.commit());
  CHECK_FALSE(setup.snapshot().is_realized(owner).value());
  auto pending = setup.construction_edit();
  REQUIRE(pending.set(pending.field(owner, "value").value(), sai::value{4.f}));
  REQUIRE(f.host.replace_world(setup));
  auto rejected = pending.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::access_denied);
  CHECK(std::get<float>(setup.snapshot().read(setup.snapshot().field(owner, "value").value()).value()) == 0);
  CHECK_FALSE(setup.construction_edit().commit());
  auto ordinary = setup.edit();
  REQUIRE(ordinary.set(ordinary.field(owner, "value").value(), sai::value{5.f}));
  REQUIRE(ordinary.commit());
}

TEST_CASE("construction node references retain ordinary authority and lifecycle validation") {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "ConstructionPort";
  sai::field_descriptor children;
  children.name = "children";
  children.kind = sai::value_kind::mf_node;
  children.access = sai::access_type::input_output;
  children.default_value = sai::node_list{};
  children.accepted_node_types = {"ConstructionPort"};
  children.unique_non_null_nodes = true;
  sai::field_descriptor configuration;
  configuration.name = "configuration";
  configuration.kind = sai::value_kind::sf_float;
  configuration.access = sai::access_type::initialize_only;
  configuration.default_value = 0.f;
  type.fields = {children, configuration};
  REQUIRE(types.define(type));
  sai::browser host{types};
  auto scene = host.create_scene();
  auto initial = scene.construction_edit();
  auto parent = initial.create_node(type.name).value();
  auto child = initial.create_node(type.name).value();
  auto children_field = initial.field(parent, "children").value();
  const std::array<sai::node, 1> nodes{child};
  REQUIRE(initial.set(children_field, std::span<const sai::node>{nodes}));
  REQUIRE(initial.commit());
  CHECK_FALSE(scene.snapshot().is_realized(child).value());
  auto configure = scene.construction_edit();
  REQUIRE(configure.set(configure.field(child, "configuration").value(), sai::value{3.f}));
  REQUIRE(configure.commit());

  auto duplicate = scene.construction_edit();
  const std::array<sai::node, 2> duplicates{child, child};
  auto duplicated = duplicate.set(children_field, std::span<const sai::node>{duplicates});
  if (duplicated) CHECK_FALSE(duplicate.commit());
  else CHECK_FALSE(duplicated);
  CHECK(scene.snapshot().read_nodes(children_field)->size() == 1);

  auto other = host.create_scene();
  auto foreign_edit = other.edit();
  auto foreign = foreign_edit.create_node(type.name).value();
  REQUIRE(foreign_edit.commit());
  auto invalid = scene.construction_edit();
  const std::array<sai::node, 1> outsiders{foreign};
  auto rejected = invalid.set(children_field, std::span<const sai::node>{outsiders});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_context);
  CHECK_FALSE(invalid.commit());

  // Default edits retain their existing eager realization rule.
  auto ordinary = scene.edit();
  REQUIRE(ordinary.set(children_field, std::span<const sai::node>{nodes}));
  REQUIRE(ordinary.commit());
  CHECK(scene.snapshot().is_realized(child).value());
  auto sealed = scene.edit();
  auto late = sealed.set(sealed.field(child, "configuration").value(), sai::value{4.f});
  REQUIRE_FALSE(late);
  CHECK(late.error().code == sai::error_code::access_denied);
  CHECK_FALSE(scene.construction_edit().commit());
}

TEST_CASE("distinct ROUTEs both deliver to inputOnly while output and ROUTE guards stay bounded") {
  logical_fixture f;
  auto edit = f.scene.edit();
  auto first = edit.create_node("HostedPort").value();
  auto second = edit.create_node("HostedPort").value();
  const auto first_source = edit.field(first, "value").value();
  const auto second_source = edit.field(second, "value").value();
  REQUIRE(edit.add_route(first_source, f.field("input")));
  REQUIRE(edit.add_route(second_source, f.field("input")));
  REQUIRE(edit.commit());
  const auto output = f.field("output");
  std::vector<float> inputs;
  REQUIRE(f.host.set_runtime_processor(
      f.scene, [&, output](sai::runtime_step &step) -> sai::result<void> {
        if (step.input() && step.input()->field == "input") {
          inputs.push_back(std::get<float>(step.input()->payload));
          return step.emit(output, step.input()->payload);
        }
        return {};
      }));
  auto batch = f.scene.logical_turn({1});
  REQUIRE(batch.send(first_source, sai::value{1.f}));
  REQUIRE(batch.send(second_source, sai::value{2.f}));
  auto result = batch.commit();
  REQUIRE(result);
  CHECK(inputs == std::vector<float>{1.f, 2.f});
  const auto count = [&](sai::event_origin origin, std::string_view field) {
    return std::count_if(result->deliveries.begin(), result->deliveries.end(),
                         [&](const auto &delivery) {
                           return delivery.origin == origin && delivery.field == field;
                         });
  };
  CHECK(count(sai::event_origin::route, "input") == 2);
  CHECK(count(sai::event_origin::runtime, "output") == 1);
  CHECK(count(sai::event_origin::route, "value") == 1);
  CHECK(std::get<float>(f.scene.snapshot().read(output).value()) == 1.f);
  CHECK(std::get<float>(f.scene.snapshot().read(f.field("value")).value()) == 1.f);
}

TEST_CASE("construction output initialization is validated storage without events or time") {
  logical_fixture f;
  auto scene = f.host.create_scene();
  auto create = scene.construction_edit();
  auto owner = create.create_node("HostedPort").value();
  auto sink = create.create_node("HostedPort").value();
  auto output = create.field(owner, "output").value();
  auto retained = create.field(owner, "value").value();
  auto target = create.field(sink, "value").value();
  REQUIRE(create.add_route(output, target));
  REQUIRE(create.commit());
  const auto before = scene.snapshot().revision();
  std::size_t events = 0;
  auto subscription = scene.observe(output, [&](const auto &) { ++events; }).value();
  auto setup = scene.construction_edit();
  REQUIRE(setup.set(retained, sai::value{4.f}));
  REQUIRE(setup.initialize_output(output, sai::value{9.f}));
  auto initialized = setup.commit();
  REQUIRE(initialized);
  CHECK(initialized->after_revision == before + 1);
  CHECK(initialized->changes.size() == 2);
  CHECK(std::get<float>(scene.snapshot().read(output).value()) == 9.f);
  CHECK(std::get<float>(scene.snapshot().read(target).value()) == 0.f);
  CHECK_FALSE(scene.snapshot().is_realized(owner).value());
  CHECK(scene.drain().delivered == 0);
  CHECK(events == 0);
  auto ordinary = scene.edit();
  auto denied = ordinary.initialize_output(output, sai::value{3.f});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);
  CHECK_FALSE(ordinary.commit());
  auto normal_set = scene.construction_edit();
  CHECK_FALSE(normal_set.set(output, sai::value{3.f}));
  CHECK_FALSE(normal_set.commit());
  auto wrong_field = scene.construction_edit();
  CHECK_FALSE(wrong_field.initialize_output(retained, sai::value{3.f}));
  CHECK_FALSE(wrong_field.commit());
  auto wrong_type = scene.construction_edit();
  auto mismatched = wrong_type.initialize_output(output, sai::value{std::string{"bad"}});
  REQUIRE_FALSE(mismatched);
  CHECK(mismatched.error().code == sai::error_code::type_mismatch);
  CHECK_FALSE(wrong_type.commit());
  REQUIRE(f.host.replace_world(scene));
  auto after_activation = scene.construction_edit();
  CHECK_FALSE(after_activation.initialize_output(output, sai::value{3.f}));
  CHECK_FALSE(after_activation.commit());
  REQUIRE(scene.logical_turn({-100}).commit());
  CHECK(std::get<float>(scene.snapshot().read(output).value()) == 9.f);
  CHECK(events == 0);
}

TEST_CASE("construction output initialization does not admit raw node identities") {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "OutputNodes";
  sai::field_descriptor field;
  field.name = "single";
  field.kind = sai::value_kind::sf_node;
  field.access = sai::access_type::output_only;
  field.default_value = sai::node_id{};
  type.fields.push_back(field);
  field.name = "many";
  field.kind = sai::value_kind::mf_node;
  field.default_value = sai::node_list{};
  type.fields.push_back(field);
  REQUIRE(types.define(type));
  sai::browser host{types};
  auto scene = host.create_scene();
  auto create = scene.construction_edit();
  auto owner = create.create_node(type.name).value();
  REQUIRE(create.commit());
  for (const auto &[name, value] : {
         std::pair{"single", sai::value{sai::node_id{owner.id()}}},
         std::pair{"many", sai::value{sai::node_list{owner.id()}}}}) {
    auto init = scene.construction_edit();
    auto rejected = init.initialize_output(init.field(owner, name).value(), value);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::unsupported_field_type);
    CHECK_FALSE(init.commit());
  }
}
