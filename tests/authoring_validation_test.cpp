#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"

#include <array>
#include <utility>

namespace sai = x3d::sai::experimental;
namespace {
sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor group;
  group.name = "Group";
  sai::field_descriptor field;
  field.name = "children";
  field.kind = sai::value_kind::mf_node;
  field.access = sai::access_type::input_output;
  field.default_value = sai::node_list{};
  field.containment = true;
  group.fields.push_back(field);
  field.name = "value";
  field.kind = sai::value_kind::sf_float;
  field.default_value = 0.f;
  field.containment = false;
  group.fields.push_back(field);
  REQUIRE(types.define(std::move(group)));
  return types;
}
} // namespace

TEST_CASE("authoring validation preserves staged work and published state") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  const auto before = scene.snapshot();
  int calls = 0;
  auto observer = scene.observe([&](const auto &) { ++calls; });
  auto edit = scene.edit();
  auto first = edit.create_node("Group").value();
  auto value = edit.field(first, "value").value();
  REQUIRE(edit.set(value, sai::value{4.f}));
  const auto &inspect = std::as_const(edit);
  REQUIRE(inspect.validate());
  REQUIRE(inspect.validate());
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK_FALSE(scene.snapshot().lookup(first.id()));
  CHECK(scene.drain().delivered == 0);
  CHECK(calls == 0);
  auto second = edit.create_node("Group").value();
  CHECK(second.id().value == first.id().value + 1);
  REQUIRE(inspect.validate());
  REQUIRE(edit.commit());
  CHECK(scene.snapshot().read(value).value() == sai::value{4.f});
  CHECK_FALSE(scene.snapshot().is_realized(first).value());
  CHECK(scene.drain().delivered == 1);
  CHECK(calls == 1);
  auto consumed = inspect.validate();
  REQUIRE_FALSE(consumed);
  CHECK(consumed.error().code == sai::error_code::poisoned_edit);
  CHECK(consumed.error().operation == "scene_edit.validate");
}

TEST_CASE("authoring validation reports stable cycle endpoints") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  auto a = edit.create_node("Group").value();
  auto b = edit.create_node("Group").value();
  auto children = edit.field(a, "children").value();
  auto others = edit.field(b, "children").value();
  REQUIRE(edit.set(children, std::array{b}));
  REQUIRE(edit.set(others, std::array{a}));
  for (int attempt = 0; attempt != 2; ++attempt) {
    const auto rejected = edit.validate();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::containment_cycle);
    CHECK(rejected.error().operation == "scene_edit.validate");
    CHECK(rejected.error().generation == scene.generation());
    CHECK(rejected.error().base_revision == 0);
    CHECK(rejected.error().current_revision == 0);
    CHECK(rejected.error().node == b.id());
    CHECK(rejected.error().field == "children");
    CHECK(scene.snapshot().revision() == 0);
    CHECK(scene.drain().delivered == 0);
  }
  auto unpublished = edit.commit();
  REQUIRE_FALSE(unpublished);
  CHECK(unpublished.error().code == sai::error_code::containment_cycle);
  CHECK(unpublished.error().node == b.id());
  CHECK(unpublished.error().field == "children");
  REQUIRE(edit.set(others, std::span<const sai::node>{}));
  REQUIRE(edit.validate());
  REQUIRE(edit.commit());
  CHECK(scene.snapshot().read(children).value() ==
        sai::value{sai::node_list{b.id()}});
  CHECK(scene.snapshot().read(others).value() == sai::value{sai::node_list{}});
}

TEST_CASE("authoring validation retains the original mutation diagnostic") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  auto owner = edit.create_node("Group").value();
  auto field = edit.field(owner, "value").value();
  const auto invalid = edit.set(field, sai::value{std::string{"wrong kind"}});
  REQUIRE_FALSE(invalid);
  const auto rejected = edit.validate();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == invalid.error().code);
  CHECK(rejected.error().operation == invalid.error().operation);
  CHECK(rejected.error().node == owner.id());
  CHECK(rejected.error().field == "value");
  CHECK_FALSE(edit.commit());
  CHECK(scene.snapshot().revision() == 0);
}

TEST_CASE("authoring validation rechecks publication admission") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto create = scene.edit();
  auto owner = create.create_node("Group").value();
  auto field = create.field(owner, "value").value();
  REQUIRE(create.commit());
  auto edit = scene.edit();
  const auto base = scene.snapshot().revision();
  REQUIRE(edit.validate());

  SUBCASE("an intervening revision invalidates preflight") {
    auto other = scene.edit();
    REQUIRE(other.set(field, sai::value{2.f}));
    REQUIRE(other.commit());
    auto rejected = edit.validate();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::stale_revision);
    CHECK(rejected.error().base_revision == base);
    CHECK(rejected.error().current_revision == base + 1);
    CHECK_FALSE(edit.commit());
  }
  SUBCASE("world retirement invalidates preflight") {
    REQUIRE(host.replace_world(host.create_scene()));
    auto rejected = edit.validate();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::stale_handle);
    CHECK_FALSE(edit.commit());
  }
  SUBCASE("imports recheck the source export") {
    auto source = host.create_scene();
    auto source_edit = source.edit();
    auto exported = source_edit.create_node("Group").value();
    REQUIRE(source_edit.export_node("public", exported));
    REQUIRE(source_edit.commit());
    REQUIRE(edit.import_node("local", source, "public"));
    REQUIRE(edit.validate());
    auto unexport = source.edit();
    REQUIRE(unexport.remove_export(source.snapshot().exports().front()));
    REQUIRE(unexport.commit());
    auto rejected = edit.validate();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::stale_aperture);
    CHECK(rejected.error().node == exported.id());
    CHECK(scene.snapshot().imports().empty());
    CHECK_FALSE(edit.commit());
  }
  SUBCASE("new field observers can retain a staged removal") {
    REQUIRE(edit.remove_node(owner));
    REQUIRE(edit.validate());
    auto observer = scene.observe(field, [](const auto &) {}).value();
    auto rejected = edit.validate();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::node_in_use);
    CHECK(rejected.error().node == owner.id());
    CHECK(rejected.error().field == "value");
    observer.cancel();
    REQUIRE(edit.validate());
    REQUIRE(edit.commit());
    CHECK_FALSE(scene.snapshot().lookup(owner.id()));
  }
  SUBCASE("runtime evaluation excludes publication admission") {
    int calls = 0;
    REQUIRE(host.set_runtime_processor(
        scene, [&](sai::runtime_step &) -> sai::result<void> {
          ++calls;
          auto rejected = edit.validate();
          REQUIRE_FALSE(rejected);
          CHECK(rejected.error().code ==
                sai::error_code::operation_in_progress);
          CHECK(rejected.error().operation == "scene_edit.validate");
          return {};
        }));
    REQUIRE(scene.events({1}).commit());
    CHECK(calls == 1);
    REQUIRE(host.set_runtime_processor(scene, {}));
    REQUIRE(edit.validate());
    REQUIRE(edit.commit());
  }
}
