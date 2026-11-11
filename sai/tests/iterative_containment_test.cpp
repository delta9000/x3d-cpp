#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
constexpr std::size_t deep_count = 4096;

sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "Link";
  sai::field_descriptor child;
  child.name = "child";
  child.kind = sai::value_kind::sf_node;
  child.access = sai::access_type::input_output;
  child.default_value = sai::node_id{};
  child.containment = true;
  type.fields.push_back(child);
  child.name = "children";
  child.kind = sai::value_kind::mf_node;
  child.default_value = sai::node_list{};
  type.fields.push_back(child);
  child.name = "reference";
  child.kind = sai::value_kind::sf_node;
  child.default_value = sai::node_id{};
  child.containment = false;
  type.fields.push_back(child);
  REQUIRE(types.define(std::move(type)));
  return types;
}

std::vector<sai::node> chain(sai::scene_edit &edit, std::size_t count) {
  std::vector<sai::node> nodes;
  nodes.reserve(count);
  for (std::size_t i = 0; i < count; ++i)
    nodes.push_back(edit.create_node("Link").value());
  // Construct the whole graph in one edit; publication is not quadratic in
  // chain depth and each new target is realized only once.
  for (std::size_t i = 0; i + 1 < count; ++i) {
    if (i % 2 == 0) {
      REQUIRE(edit.set(edit.field(nodes[i], "child").value(), nodes[i + 1]));
    } else {
      const std::array<sai::node, 2> values{sai::node{}, nodes[i + 1]};
      REQUIRE(edit.set(edit.field(nodes[i], "children").value(),
                       std::span<const sai::node>{values}));
    }
  }
  return nodes;
}
} // namespace

TEST_CASE("deep containment validation and occurrence inspection use bounded "
          "call stacks") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto nodes = chain(edit, deep_count);
  // A reference back to an ancestor is legal when it is not containment.
  REQUIRE(
      edit.set(edit.field(nodes.back(), "reference").value(), nodes.front()));
  REQUIRE(edit.append_root(nodes.front()));
  REQUIRE(edit.validate());
  REQUIRE(edit.commit());
  const auto snapshot = scene.snapshot();
  const auto occurrences = snapshot.occurrences();
  REQUIRE(occurrences.size() == deep_count);
  CHECK_FALSE(occurrences.front().parent_occurrence);
  CHECK(occurrences.front().path == std::vector<std::size_t>{0});
  for (std::size_t i = 1; i < occurrences.size(); ++i) {
    CHECK(occurrences[i].node == nodes[i].id());
    CHECK(occurrences[i].parent_occurrence == i - 1);
    CHECK(occurrences[i].container_field ==
          ((i - 1) % 2 == 0 ? "child" : "children"));
    CHECK(occurrences[i].index == (i - 1) % 2);
  }
  std::vector<std::size_t> expected_path{0};
  for (std::size_t i = 1; i < deep_count; ++i)
    expected_path.push_back((i - 1) % 2);
  CHECK(occurrences.back().path == expected_path);
  CHECK(snapshot.read_node(snapshot.field(nodes.back(), "reference").value())
            ->id() == nodes.front().id());
}

TEST_CASE("deep template closure validation remains iterative and scope safe") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto nodes = chain(edit, deep_count);
  REQUIRE(
      edit.set(edit.field(nodes.back(), "reference").value(), nodes.front()));
  sai::local_declaration_descriptor descriptor;
  descriptor.name = "DeepTemplate";
  const std::array<sai::node, 1> roots{nodes.front()};
  auto declaration = edit.add_local_declaration(
      std::move(descriptor), std::span<const sai::node>{roots});
  REQUIRE(declaration);
  REQUIRE(edit.validate());
  REQUIRE(edit.commit());
  const auto snapshot = scene.snapshot();
  const auto description = snapshot.describe(*declaration);
  REQUIRE(description);
  CHECK(std::get<sai::local_declaration_descriptor>(description->payload)
            .body_roots == sai::node_list{nodes.front().id()});
  CHECK(snapshot.roots().empty());
  CHECK(snapshot.occurrences().empty());
  auto invalid = scene.edit();
  const auto denied = invalid.append_root(nodes.back());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(invalid.commit());
  CHECK(scene.snapshot().revision() == snapshot.revision());
}

TEST_CASE("deep authoring containment cycles retain deterministic diagnostics "
          "and rollback") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto before = scene.snapshot();
  int calls = 0;
  auto observer = scene.observe([&](const sai::change_set &) { ++calls; });
  auto edit = scene.edit();
  const auto nodes = chain(edit, deep_count);
  REQUIRE(edit.set(edit.field(nodes.back(), "child").value(), nodes.front()));
  REQUIRE(edit.append_root(nodes.front()));
  const auto invalid = edit.validate();
  REQUIRE_FALSE(invalid);
  CHECK(invalid.error().code == sai::error_code::containment_cycle);
  CHECK(invalid.error().node == nodes.back().id());
  CHECK(invalid.error().field == "child");
  CHECK(invalid.error().operation == "scene_edit.validate");
  const auto retry = edit.validate();
  REQUIRE_FALSE(retry);
  CHECK(retry.error().node == invalid.error().node);
  CHECK(retry.error().field == invalid.error().field);
  CHECK(retry.error().message == invalid.error().message);
  const auto failed = edit.commit();
  REQUIRE_FALSE(failed);
  CHECK(failed.error().code == sai::error_code::containment_cycle);
  CHECK(failed.error().node == invalid.error().node);
  CHECK(failed.error().field == invalid.error().field);
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.snapshot().roots().empty());
  CHECK_FALSE(scene.snapshot().lookup(nodes.front().id()));
  CHECK(scene.drain().delivered == 0);
  CHECK(calls == 0);
}

TEST_CASE("deep template containment cycles fail before claiming node scopes") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto before = scene.snapshot();
  auto edit = scene.edit();
  const auto nodes = chain(edit, deep_count);
  REQUIRE(edit.set(edit.field(nodes.back(), "child").value(), nodes.front()));
  sai::local_declaration_descriptor descriptor;
  descriptor.name = "CyclicTemplate";
  const std::array<sai::node, 1> roots{nodes.front()};
  const auto rejected = edit.add_local_declaration(
      std::move(descriptor), std::span<const sai::node>{roots});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::containment_cycle);
  CHECK(rejected.error().message ==
        "template body contains a containment cycle");
  REQUIRE_FALSE(edit.commit());
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.snapshot().declarations().empty());
  CHECK_FALSE(scene.snapshot().lookup(nodes.front().id()));
  CHECK(scene.drain().delivered == 0);
}

TEST_CASE("deep runtime containment cycle rejection leaves published state and "
          "time unchanged") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto nodes = chain(edit, deep_count);
  REQUIRE(edit.append_root(nodes.front()));
  REQUIRE(edit.commit());
  const auto before = scene.snapshot();
  const auto terminal = before.field(nodes.back(), "child").value();
  int calls = 0;
  auto observer = scene.observe([&](const sai::change_set &) { ++calls; });
  auto events = scene.events({10});
  REQUIRE(events.send(terminal, nodes.front()));
  const auto failed = events.commit();
  REQUIRE_FALSE(failed);
  CHECK(failed.error().code == sai::error_code::containment_cycle);
  CHECK(failed.error().node == nodes.back().id());
  CHECK(failed.error().field == "child");
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.snapshot().read_node(terminal)->is_null());
  CHECK(scene.drain().delivered == 0);
  CHECK(calls == 0);
  // A failed cascade does not advance the clock to its requested time.
  auto retry = scene.events({9});
  REQUIRE(retry.send(terminal, std::nullopt));
  REQUIRE(retry.commit());
  CHECK(scene.snapshot().read_node(terminal)->is_null());
}

TEST_CASE("iterative occurrences preserve preorder fields indexes sharing and "
          "root order") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto root = edit.create_node("Link").value();
  const auto first = edit.create_node("Link").value();
  const auto second = edit.create_node("Link").value();
  const auto shared = edit.create_node("Link").value();
  REQUIRE(edit.set(edit.field(root, "child").value(), first));
  const std::array<sai::node, 3> children{sai::node{}, second, shared};
  REQUIRE(edit.set(edit.field(root, "children").value(),
                   std::span<const sai::node>{children}));
  REQUIRE(edit.set(edit.field(first, "child").value(), shared));
  REQUIRE(edit.set(edit.field(second, "reference").value(), root));
  REQUIRE(edit.append_root(root));
  REQUIRE(edit.append_root(shared));
  REQUIRE(edit.commit());
  const auto occurrences = scene.snapshot().occurrences();
  REQUIRE(occurrences.size() == 6);
  const std::array<sai::node_id, 6> expected{root.id(),   first.id(),
                                             shared.id(), second.id(),
                                             shared.id(), shared.id()};
  for (std::size_t i = 0; i < expected.size(); ++i)
    CHECK(occurrences[i].node == expected[i]);
  CHECK(occurrences[1].parent_occurrence == 0);
  CHECK(occurrences[2].parent_occurrence == 1);
  CHECK(occurrences[3].parent_occurrence == 0);
  CHECK(occurrences[4].parent_occurrence == 0);
  CHECK_FALSE(occurrences[5].parent_occurrence);
  CHECK(occurrences[2].path == std::vector<std::size_t>{0, 0, 0});
  CHECK(occurrences[3].path == std::vector<std::size_t>{0, 1});
  CHECK(occurrences[4].path == std::vector<std::size_t>{0, 2});
  CHECK(occurrences[5].path == std::vector<std::size_t>{1});
  CHECK(occurrences[3].container_field == "children");
  CHECK(occurrences[4].index == 2);
}
