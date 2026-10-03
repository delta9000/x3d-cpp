#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include "doctest/doctest.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
sai::node_type_descriptor descriptor(std::string name, bool abstract = false) {
  sai::node_type_descriptor type;
  type.name = std::move(name);
  type.component = "Example";
  type.component_level = 1;
  type.interfaces = {"X3DNode"};
  type.abstract = abstract;
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_float;
  field.access = sai::access_type::input_output;
  field.default_value = 3.f;
  field.unit_category = "length";
  type.fields.push_back(field);
  field.name = "initial";
  field.kind = sai::value_kind::sf_int32;
  field.access = sai::access_type::initialize_only;
  field.default_value = std::int32_t{1};
  field.unit_category.reset();
  type.fields.push_back(field);
  field.name = "children";
  field.kind = sai::value_kind::mf_node;
  field.access = sai::access_type::input_output;
  field.default_value = sai::node_list{};
  field.containment = true;
  field.accepted_node_types = {type.name};
  type.fields.push_back(std::move(field));
  return type;
}
} // namespace

TEST_CASE("node vocabulary discovery owns ordered configured descriptors") {
  std::vector<sai::node_type_descriptor> retained;
  {
    sai::type_registry registry;
    CHECK(registry.node_types().empty());
    REQUIRE(registry.define(descriptor("Zeta")));
    REQUIRE(registry.define(descriptor("Alpha", true)));
    REQUIRE(registry.define(descriptor("Beta")));
    auto types = registry.node_types();
    REQUIRE(types.size() == 3);
    CHECK(types[0].name == "Alpha");
    CHECK(types[1].name == "Beta");
    CHECK(types[2].name == "Zeta");
    REQUIRE(types[0].fields.size() == 3);
    CHECK(types[0].fields[0].name == "value");
    CHECK(types[0].fields[1].name == "initial");
    CHECK(types[0].fields[2].name == "children");
    CHECK(types[0].abstract);
    CHECK(types[0].component == "Example");
    CHECK(types[0].component_level == 1);
    CHECK(types[0].interfaces == std::vector<std::string>{"X3DNode"});
    CHECK(types[0].fields[0].unit_category == "length");
    types[0].name = "Changed";
    types[0].fields[0].default_value = 90.f;
    types[0].fields[2].accepted_node_types.clear();
    CHECK(registry.find("Alpha")->fields[0].default_value == sai::value{3.f});
    CHECK(registry.find("Alpha")->fields[2].accepted_node_types ==
          std::vector<std::string>{"Alpha"});
    REQUIRE(registry.define(descriptor("Gamma")));
    CHECK(types.size() == 3);
    CHECK(registry.node_types().size() == 4);

    sai::browser host{std::move(registry)};
    auto scene = host.current_scene();
    const auto history = scene.snapshot();
    int notifications = 0;
    auto observer = scene.observe([&](const auto &) { ++notifications; });
    auto discovered = history.node_types();
    REQUIRE(discovered.size() == 4);
    discovered[1].fields[0].default_value = 77.f;
    auto edit = scene.edit();
    const auto beta = edit.create_node(discovered[1].name).value();
    const auto value = edit.field(beta, "value").value();
    REQUIRE(edit.commit());
    CHECK(scene.snapshot().read(value).value() == sai::value{3.f});
    CHECK(scene.drain().delivered == 1);
    notifications = 0;
    retained = scene.snapshot().node_types();
    CHECK(retained[0].name == "Alpha");
    CHECK(retained[1].name == "Beta");
    CHECK(retained[2].name == "Gamma");
    CHECK(retained[3].name == "Zeta");
    CHECK(history.node_types().size() == 4);
    CHECK(history.revision() == 0);
    CHECK(scene.snapshot().revision() == 1);
    CHECK(scene.drain().delivered == 0);
    CHECK(notifications == 0);
    REQUIRE(host.replace_world(host.create_scene()));
    CHECK(history.node_types()[1].fields[0].default_value == sai::value{3.f});
    CHECK(host.current_scene().snapshot().node_types().size() == 4);

    sai::browser empty{sai::type_registry{}};
    CHECK(empty.current_scene().snapshot().node_types().empty());
  }
  REQUIRE(retained.size() == 4);
  CHECK(retained[1].fields[0].default_value == sai::value{3.f});
  CHECK(retained[1].fields[2].accepted_node_types ==
        std::vector<std::string>{"Beta"});
}

TEST_CASE("generated node vocabulary predicts creation and field inspection") {
  const auto catalog = sai::generated_metadata_catalog();
  std::vector<std::string_view> names;
  for (const auto &type : catalog.nodes)
    names.push_back(type.name);
  auto registry = sai::generated_type_registry(names);
  REQUIRE(registry);
  REQUIRE(registry->schema_fingerprint());
  const auto fingerprint = std::string{*registry->schema_fingerprint()};
  CHECK(registry->node_types().size() == catalog.nodes.size());
  CHECK(*registry->schema_fingerprint() == fingerprint);
  sai::browser host{std::move(*registry)};
  auto scene = host.current_scene();
  const auto before = scene.snapshot();
  const auto types = before.node_types();
  REQUIRE(types.size() == catalog.nodes.size());
  CHECK(std::ranges::is_sorted(types, {}, &sai::node_type_descriptor::name));
  CHECK_FALSE(host.capabilities().live);
  auto edit = scene.edit();
  std::vector<std::pair<sai::node, std::size_t>> created;
  std::size_t abstract_count = 0;
  for (std::size_t index = 0; index < types.size(); ++index) {
    const auto &type = types[index];
    CAPTURE(type.name);
    if (type.abstract) {
      ++abstract_count;
      auto rejected_edit = scene.edit();
      const auto rejected = rejected_edit.create_node(type.name);
      REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code == sai::error_code::abstract_type);
      CHECK_FALSE(rejected_edit.commit());
    } else {
      const auto node = edit.create_node(type.name);
      REQUIRE(node);
      created.emplace_back(*node, index);
    }
  }
  CHECK(abstract_count > 0);
  REQUIRE_FALSE(created.empty());
  REQUIRE(edit.commit());
  const auto snapshot = scene.snapshot();
  for (const auto &[node, index] : created) {
    const auto &type = types[index];
    CAPTURE(type.name);
    const auto actual = snapshot.describe(node);
    REQUIRE(actual);
    CHECK(actual->name == type.name);
    CHECK(actual->component == type.component);
    CHECK(actual->component_level == type.component_level);
    CHECK(actual->interfaces == type.interfaces);
    REQUIRE(actual->fields.size() == type.fields.size());
    for (std::size_t field_index = 0; field_index < type.fields.size(); ++field_index) {
      const auto &expected = type.fields[field_index];
      CAPTURE(expected.name);
      CHECK(actual->fields[field_index].name == expected.name);
      const auto field = snapshot.field(node, expected.name);
      REQUIRE(field);
      CHECK(field->kind() == expected.kind);
      CHECK(field->access() == expected.access);
      if (expected.access == sai::access_type::input_only) {
        CHECK_FALSE(snapshot.readable(*field).value());
        const auto unreadable = snapshot.read(*field);
        REQUIRE_FALSE(unreadable);
        CHECK(unreadable.error().code == sai::error_code::access_denied);
      } else {
        CHECK(snapshot.readable(*field).value());
        const auto stored = snapshot.read(*field);
        REQUIRE(stored);
        CHECK(sai::same_representation(*stored, expected.default_value));
      }
    }
  }
  const std::array<std::string_view, 2> selected{"Group", "X3DNode"};
  auto subset = sai::generated_type_registry(selected);
  REQUIRE(subset);
  sai::browser limited{std::move(*subset)};
  auto limited_scene = limited.current_scene();
  const auto available = limited_scene.snapshot().node_types();
  REQUIRE(available.size() == selected.size());
  CHECK(available[0].name == "Group");
  CHECK(available[1].name == "X3DNode");
  CHECK(available[1].abstract);
  auto excluded = limited_scene.edit().create_node("Transform");
  REQUIRE_FALSE(excluded);
  CHECK(excluded.error().code == sai::error_code::unknown_type);
  CHECK(before.revision() == 0);
  CHECK(before.node_types().size() == types.size());
}
