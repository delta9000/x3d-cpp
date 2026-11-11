#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
struct measure {
  static constexpr auto x3d_name = "Measure";
  static constexpr sai::field_key<measure, std::int32_t> value{
      "value", sai::access_type::input_output};
};

sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "Measure";
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_int32;
  field.access = sai::access_type::input_output;
  field.default_value = std::int32_t{0};
  type.fields.push_back(std::move(field));
  REQUIRE(types.define(std::move(type)));
  return types;
}

sai::node create(const sai::execution_context &scene) {
  auto edit = scene.edit();
  auto owner = edit.create_node("Measure").value();
  REQUIRE(edit.commit());
  return owner;
}

template <class T>
void check_error(const sai::result<T> &result, sai::error_code code,
                 const std::string &operation, const sai::node &owner) {
  REQUIRE_FALSE(result);
  CHECK(result.error().code == code);
  CHECK(result.error().operation == operation);
  CHECK(result.error().generation == owner.generation());
  CHECK(result.error().node == owner.id());
  CHECK_FALSE(result.error().message.empty());
}

void check_node_services(const sai::node &owner, sai::error_code code) {
  check_error(owner.get_type_name(), code, "node.get_type_name", owner);
  check_error(owner.get_field_definitions(), code, "node.get_field_definitions",
              owner);
  check_error(owner.get_field("value"), code, "node.get_field", owner);
  check_error(owner.is_realized(), code, "node.is_realized", owner);
}
} // namespace

TEST_CASE("live node services inspect committed state without changing setup") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  auto owner = edit.create<measure>().value();
  check_node_services(owner.dynamic(), sai::error_code::unknown_node);
  // Authoring has its own staged view before publication.
  REQUIRE(edit.field(owner.dynamic(), "value"));
  REQUIRE(edit.commit());
  const auto before = scene.snapshot();
  const auto revision = before.revision();
  REQUIRE(scene.validate());
  CHECK(owner.get_type_name().value() == "Measure");
  CHECK_FALSE(owner.is_realized().value());
  auto definitions = owner.get_field_definitions();
  REQUIRE(definitions);
  REQUIRE(definitions->size() == 1);
  CHECK(definitions->front().name == "value");
  CHECK(definitions->front().kind == sai::value_kind::sf_int32);
  definitions->front().name = "local copy";
  CHECK(owner.get_field_definitions()->front().name == "value");
  auto field = owner.get_field(measure::value);
  REQUIRE(field);
  CHECK(field->get_definition()->access == sai::access_type::input_output);
  CHECK(before.read(*field).value() == 0);
  CHECK(scene.snapshot().revision() == revision);
  CHECK_FALSE(owner.is_realized().value());
  CHECK(scene.drain().delivered == 0);

  const auto unknown = owner.get_field("missing");
  check_error(unknown, sai::error_code::unknown_field, "node.get_field",
              owner.dynamic());
  CHECK(unknown.error().field == "missing");
  CHECK(unknown.error().base_revision == revision);
  CHECK(unknown.error().current_revision == revision);
  CHECK(scene.snapshot().revision() == revision);

  auto realize = scene.edit();
  REQUIRE(realize.realize(owner));
  CHECK_FALSE(owner.is_realized().value());
  REQUIRE(realize.commit());
  CHECK(owner.is_realized().value());
  CHECK_FALSE(before.is_realized(owner.dynamic()).value());
}

TEST_CASE("node disposal revokes services without changing copies or scene "
          "ownership") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  auto field = owner.get_field("value").value();
  auto sibling = owner;
  auto attach = scene.edit();
  REQUIRE(attach.append_root(owner));
  REQUIRE(attach.define_name("Retained", owner));
  REQUIRE(attach.commit());
  const auto before = scene.snapshot();
  int callbacks = 0;
  auto observer = scene.observe([&](const sai::change_set &) { ++callbacks; });
  const auto identity = owner.id();
  owner.dispose();
  owner.dispose();
  CHECK(owner.disposed());
  CHECK(owner.id() == identity);
  check_node_services(owner, sai::error_code::disposed);
  auto copied_after = owner;
  check_node_services(copied_after, sai::error_code::disposed);
  CHECK_FALSE(sibling.disposed());
  CHECK(sibling.get_type_name().value() == "Measure");
  CHECK(scene.snapshot().named("Retained")->get_type_name().value() ==
        "Measure");
  CHECK(scene.snapshot().roots() == before.roots());
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.drain().delivered == 0);
  CHECK(callbacks == 0);
  CHECK(observer.active());

  // Abstract SAI 6.7.1: fields acquired before node disposal remain usable.
  REQUIRE(field.get_definition());
  auto mutation = scene.edit();
  REQUIRE(mutation.set(field, sai::value{std::int32_t{17}}));
  REQUIRE(mutation.commit());
  CHECK(scene.snapshot().read(field).value() == sai::value{std::int32_t{17}});
  CHECK(before.read(field).value() == sai::value{std::int32_t{0}});
  CHECK(sibling.get_field("value"));
  CHECK(scene.drain().delivered == 1);
}

TEST_CASE("typed node live service projections reject disposed references") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  auto owner = edit.create<measure>().value();
  REQUIRE(edit.commit());
  auto sibling = owner;
  auto retained = owner.get_field(measure::value).value();
  owner.dispose();
  CHECK(owner.get_type_name().error().code == sai::error_code::disposed);
  CHECK(owner.get_field_definitions().error().code ==
        sai::error_code::disposed);
  CHECK(owner.get_field("value").error().code == sai::error_code::disposed);
  CHECK(owner.get_field(measure::value).error().code ==
        sai::error_code::disposed);
  CHECK(owner.is_realized().error().code == sai::error_code::disposed);
  REQUIRE(retained.get_definition());
  REQUIRE(sibling.get_field(measure::value));
}

TEST_CASE(
    "field disposal is local and preserves independently acquired views") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  auto field = owner.get_field("value").value();
  auto sibling = field;
  auto typed = field.as<std::int32_t>().value();
  const auto before = scene.snapshot();
  field.dispose();
  field.dispose();
  CHECK(field.disposed());
  CHECK(field.node() == owner.id());
  CHECK(field.name() == "value");
  check_error(field.get_definition(), sai::error_code::disposed,
              "dynamic_field.get_definition", owner);
  CHECK(field.get_definition().error().field == "value");
  check_error(field.as<std::int32_t>(), sai::error_code::disposed,
              "dynamic_field.as", owner);
  // Disposal takes priority over an incompatible requested typed view.
  CHECK(field.as<std::string>().error().code == sai::error_code::disposed);
  auto disposed_copy = field;
  CHECK(disposed_copy.get_definition().error().code ==
        sai::error_code::disposed);
  CHECK_FALSE(sibling.disposed());
  CHECK_FALSE(typed.disposed());
  REQUIRE(sibling.get_definition());
  REQUIRE(typed.get_definition());
  REQUIRE(owner.get_field("value"));
  CHECK_FALSE(before.read(field));
  CHECK(before.read(typed).value() == 0);
  auto denied = scene.edit();
  REQUIRE_FALSE(denied.set(field, sai::value{std::int32_t{21}}));
  REQUIRE_FALSE(denied.commit());
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.drain().delivered == 0);

  auto typed_copy = typed;
  typed.dispose();
  CHECK(typed.disposed());
  CHECK(typed.get_definition().error().code == sai::error_code::disposed);
  REQUIRE(typed_copy.get_definition());
  auto allowed = scene.edit();
  REQUIRE(allowed.set(typed_copy, std::int32_t{4}));
  REQUIRE(allowed.commit());
  CHECK(scene.snapshot().read(sibling).value() == sai::value{std::int32_t{4}});
}

TEST_CASE(
    "live removal fails distinctly while historical inspection remains valid") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  const auto field = owner.get_field("value").value();
  const auto history = scene.snapshot();
  auto removal = scene.edit();
  REQUIRE(removal.remove_node(owner));
  REQUIRE(removal.commit());
  check_node_services(owner, sai::error_code::unknown_node);
  check_error(field.get_definition(), sai::error_code::unknown_node,
              "dynamic_field.get_definition", owner);
  CHECK(history.describe(owner)->name == "Measure");
  CHECK(history.read(field).value() == sai::value{std::int32_t{0}});
  auto replacement = create(scene);
  CHECK(replacement.id() != owner.id());
  CHECK(replacement.get_type_name().value() == "Measure");
  check_node_services(owner, sai::error_code::unknown_node);
}

TEST_CASE("live retirement fails without invalidating historical inspection") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  const auto field = owner.get_field("value").value();
  const auto history = scene.snapshot();
  const auto replacement = host.create_scene();
  const auto other = create(replacement);
  CHECK(other.id() == owner.id());
  CHECK(other.generation() != owner.generation());
  REQUIRE(host.replace_world(replacement));
  const auto inactive = scene.validate();
  REQUIRE_FALSE(inactive);
  CHECK(inactive.error().code == sai::error_code::stale_handle);
  CHECK(inactive.error().operation == "execution_context.validate");
  CHECK(inactive.error().generation == scene.generation());
  REQUIRE(replacement.validate());
  REQUIRE(other.get_type_name());
  check_node_services(owner, sai::error_code::stale_handle);
  check_error(field.get_definition(), sai::error_code::stale_handle,
              "dynamic_field.get_definition", owner);
  CHECK(history.describe(owner)->name == "Measure");
  CHECK(history.read(field).value() == sai::value{std::int32_t{0}});
  CHECK_FALSE(replacement.snapshot().describe(owner));
  CHECK_FALSE(replacement.snapshot().read(field));
  // Explicit local disposal remains distinguishable even after retirement.
  owner.dispose();
  check_node_services(owner, sai::error_code::disposed);
}

TEST_CASE("live services reject null expired and abandoned node authority") {
  sai::node empty;
  check_node_services(empty, sai::error_code::stale_handle);
  empty.dispose();
  check_node_services(empty, sai::error_code::disposed);
  sai::node expired;
  std::optional<sai::dynamic_field> expired_field;
  {
    sai::browser host{registry()};
    const auto scene = host.current_scene();
    expired = create(scene);
    expired_field = expired.get_field("value").value();
  }
  check_node_services(expired, sai::error_code::stale_handle);
  check_error(expired_field->get_definition(), sai::error_code::stale_handle,
              "dynamic_field.get_definition", expired);

  sai::browser host{registry()};
  const auto scene = host.current_scene();
  sai::node abandoned;
  {
    auto edit = scene.edit();
    abandoned = edit.create_node("Measure").value();
  }
  check_node_services(abandoned, sai::error_code::unknown_node);
  const auto live = create(scene);
  CHECK(live.id() != abandoned.id());
  check_node_services(abandoned, sai::error_code::unknown_node);
}

TEST_CASE(
    "context validation handles a moved-from wrapper without side effects") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  const auto revision = scene.snapshot().revision();
  auto moved = std::move(scene);
  const auto empty = scene.validate();
  REQUIRE_FALSE(empty);
  CHECK(empty.error().code == sai::error_code::stale_handle);
  REQUIRE(moved.validate());
  CHECK(moved.snapshot().revision() == revision);
  CHECK(moved.drain().delivered == 0);
}

static_assert(noexcept(std::declval<sai::dynamic_field &>().dispose()));
static_assert(noexcept(std::declval<sai::field<std::int32_t> &>().dispose()));

TEST_CASE("live field lookup canonicalizes inputOutput event aliases only") {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "AccessKinds";
  for (const auto access :
       {sai::access_type::input_output, sai::access_type::input_only,
        sai::access_type::output_only, sai::access_type::initialize_only}) {
    sai::field_descriptor descriptor;
    descriptor.name = access == sai::access_type::input_output  ? "duplex"
                      : access == sai::access_type::input_only  ? "input"
                      : access == sai::access_type::output_only ? "output"
                                                                : "initial";
    descriptor.kind = sai::value_kind::sf_int32;
    descriptor.access = access;
    descriptor.default_value = std::int32_t{};
    type.fields.push_back(std::move(descriptor));
  }
  // This actual inputOnly declaration takes priority over alias expansion.
  auto explicit_input = type.fields[1];
  explicit_input.name = "set_duplex";
  type.fields.push_back(std::move(explicit_input));
  REQUIRE(types.define(std::move(type)));
  sai::browser host{std::move(types)};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto owner = edit.create_node("AccessKinds").value();
  REQUIRE(edit.commit());
  const auto before = scene.snapshot();
  const auto canonical = owner.get_field("duplex").value();
  const auto output_alias = owner.get_field("duplex_changed").value();
  CHECK(output_alias.name() == canonical.name());
  CHECK(output_alias.node() == canonical.node());
  CHECK(output_alias.access() == sai::access_type::input_output);
  CHECK(owner.get_field("set_duplex")->name() == "set_duplex");
  CHECK(owner.get_field("set_duplex")->access() ==
        sai::access_type::input_only);
  for (const auto *name :
       {"set_input", "input_changed", "set_output", "output_changed",
        "set_initial", "initial_changed", "set_", "_changed"}) {
    const auto rejected = owner.get_field(name);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::unknown_field);
    CHECK(rejected.error().field == name);
  }
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK_FALSE(owner.is_realized().value());
  auto mutation = scene.edit();
  REQUIRE(mutation.set(output_alias, sai::value{std::int32_t{8}}));
  REQUIRE(mutation.commit());
  CHECK(scene.snapshot().read(canonical).value() ==
        sai::value{std::int32_t{8}});

  sai::browser simple{registry()};
  const auto simple_scene = simple.current_scene();
  const auto simple_node = create(simple_scene);
  const auto input_alias = simple_node.get_field("set_value").value();
  CHECK(input_alias.name() == "value");
  CHECK(input_alias.access() == sai::access_type::input_output);
  const auto typed_alias = input_alias.as<std::int32_t>();
  REQUIRE(typed_alias);
  CHECK(simple_scene.snapshot().read(*typed_alias).value() == 0);
}
