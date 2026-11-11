#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/value_constraints.hpp"

#include "doctest/doctest.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
std::vector<sai::rotation> invalid_axes() {
  const auto inf = std::numeric_limits<float>::infinity();
  const auto nan = std::numeric_limits<float>::quiet_NaN();
  const auto epsilon = std::numeric_limits<float>::epsilon();
  return {{0, 0, 0, 0},
          {0, 0, 0, 1},
          {2, 0, 0, 1},
          {0, -2, 0, 1},
          {0, 0, .5f, 1},
          {1, 1, 0, 1},
          {nan, 0, 1, 1},
          {0, nan, 1, 1},
          {1, 0, nan, 1},
          {inf, 0, 0, 1},
          {0, -inf, 1, 1},
          {0, 1, inf, 1},
          {std::numeric_limits<float>::max(), 0, 0, 1},
          {std::numeric_limits<float>::denorm_min(), 0, 0, 1},
          {1.f - 5.f * epsilon, 0, 0, 1},
          {std::nextafter(1.f, 2.f), 0, 0, 1}};
}

sai::field_descriptor
rotation_field(std::string name, bool many = false,
               sai::access_type access = sai::access_type::input_output) {
  sai::field_descriptor field;
  field.name = std::move(name);
  field.kind =
      many ? sai::value_kind::mf_rotation : sai::value_kind::sf_rotation;
  field.access = access;
  field.default_value = many ? sai::value{sai::rotation_list{sai::rotation{}}}
                             : sai::value{sai::rotation{}};
  if (access == sai::access_type::output_only && many)
    field.default_value = sai::rotation_list{};
  return field;
}

sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "RotationHolder";
  type.fields.push_back(rotation_field("single"));
  type.fields.push_back(rotation_field("many", true));
  type.fields.push_back(
      rotation_field("output", false, sai::access_type::output_only));
  type.fields.push_back(
      rotation_field("outputs", true, sai::access_type::output_only));
  auto field = rotation_field("guard");
  field.kind = sai::value_kind::sf_int32;
  field.default_value = std::int32_t{};
  type.fields.push_back(std::move(field));
  REQUIRE(types.define(std::move(type)));
  return types;
}

struct fixture {
  sai::browser host{registry()};
  sai::execution_context scene{host.current_scene()};
  sai::node owner;
  fixture() {
    auto edit = scene.edit();
    owner = edit.create_node("RotationHolder").value();
    REQUIRE(edit.append_root(owner));
    REQUIRE(edit.commit());
  }
  sai::dynamic_field field(const std::string &name) const {
    return scene.snapshot().field(owner, name).value();
  }
};
} // namespace

TEST_CASE("rotation axis validity checks normalized float precision without "
          "rewriting values") {
  const float epsilon = std::numeric_limits<float>::epsilon();
  const std::vector<sai::rotation> valid{{},
                                         {1, 0, 0, -9},
                                         {0, -1, 0, 12},
                                         {-1, -0.f, 0, 0},
                                         {.6f, .8f, 0, 1},
                                         {.577350f, .577350f, .577350f, 2},
                                         {.707107f, 0, .707107f, 3},
                                         {1.f - 2.f * epsilon, 0, 0, 4}};
  for (const auto &candidate : valid)
    CHECK(sai::detail::valid_rotation_axis(candidate));
  for (const auto &candidate : invalid_axes())
    CHECK_FALSE(sai::detail::valid_rotation_axis(candidate));
  fixture f;
  const auto field = f.field("single");
  for (const auto &candidate : valid) {
    auto edit = f.scene.edit();
    REQUIRE(edit.set(field, sai::value{candidate}));
    REQUIRE(edit.commit());
    CHECK(sai::same_representation(f.scene.snapshot().read(field).value(),
                                   sai::value{candidate}));
  }
}

TEST_CASE("rotation defaults reject malformed axes in node and declaration "
          "descriptors") {
  for (const auto &candidate : invalid_axes()) {
    for (bool many : {false, true}) {
      const sai::value payload =
          many ? sai::value{sai::rotation_list{{}, candidate}}
               : sai::value{candidate};
      sai::type_registry types;
      sai::node_type_descriptor descriptor;
      descriptor.name = "Malformed";
      auto field = rotation_field("value", many);
      field.default_value = payload;
      field.default_origin = sai::default_source::schema;
      descriptor.fields.push_back(field);
      const auto rejected = types.define(std::move(descriptor));
      REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
      CHECK(types.find("Malformed") == nullptr);

      fixture f;
      const auto before = f.scene.snapshot();
      auto edit = f.scene.edit();
      sai::local_declaration_descriptor declaration;
      declaration.name = "MalformedDefault";
      sai::interface_field_descriptor entry;
      entry.name = "value";
      entry.kind = field.kind;
      entry.access = sai::access_type::input_output;
      entry.default_value = payload;
      declaration.interface.push_back(std::move(entry));
      const auto invalid = edit.add_local_declaration(std::move(declaration));
      REQUIRE_FALSE(invalid);
      CHECK(invalid.error().code == sai::error_code::invalid_descriptor);
      CHECK(invalid.error().field == "value");
      REQUIRE_FALSE(edit.commit());
      CHECK(f.scene.snapshot().revision() == before.revision());
      CHECK(f.scene.snapshot().declarations().empty());
    }
  }
}

TEST_CASE(
    "scalar and whole MF rotation writes reject invalid axes atomically") {
  fixture f;
  const auto before = f.scene.snapshot();
  const auto guard = f.field("guard");
  int calls = 0;
  auto observer = f.scene.observe([&](const sai::change_set &) { ++calls; });
  for (const auto &candidate : invalid_axes()) {
    for (bool many : {false, true}) {
      const auto target = f.field(many ? "many" : "single");
      const sai::value payload =
          many ? sai::value{sai::rotation_list{{}, candidate}}
               : sai::value{candidate};
      auto edit = f.scene.edit();
      REQUIRE(edit.set(guard, sai::value{std::int32_t{123}}));
      const auto invalid = edit.set(target, payload);
      REQUIRE_FALSE(invalid);
      CHECK(invalid.error().code == sai::error_code::invalid_value);
      CHECK(invalid.error().node == f.owner.id());
      CHECK(invalid.error().field == target.name());
      REQUIRE_FALSE(edit.commit());
      CHECK(f.scene.snapshot().revision() == before.revision());
      CHECK(f.scene.snapshot().read(guard).value() ==
            sai::value{std::int32_t{0}});
      CHECK(sai::same_representation(f.scene.snapshot().read(target).value(),
                                     before.read(target).value()));
    }
  }
  CHECK(f.scene.drain().delivered == 0);
  CHECK(calls == 0);
}

TEST_CASE(
    "indexed MF rotation mutations reject invalid axes without publication") {
  fixture f;
  const auto before = f.scene.snapshot();
  const auto target = f.field("many").as<sai::rotation_list>().value();
  for (const auto &candidate : invalid_axes()) {
    for (int operation = 0; operation < 4; ++operation) {
      auto edit = f.scene.edit();
      auto field = edit.multi(target).value();
      REQUIRE(field.append(sai::rotation{}));
      const auto rejected = [&]() -> sai::result<void> {
        switch (operation) {
        case 0:
          return field.set(0, candidate);
        case 1:
          return field.insert(0, candidate);
        case 2:
          return field.append(candidate);
        default:
          return field.replace(sai::rotation_list{{}, candidate});
        }
      }();
      REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code == sai::error_code::invalid_value);
      CHECK(rejected.error().field == "many");
      REQUIRE_FALSE(edit.commit());
      CHECK(f.scene.snapshot().revision() == before.revision());
      CHECK(f.scene.snapshot().read(target).value() == sai::rotation_list{{}});
    }
  }
  CHECK(f.scene.drain().delivered == 0);
}

TEST_CASE(
    "rotation event input rejects bad axes before any queued write publishes") {
  fixture f;
  const auto before = f.scene.snapshot();
  const auto guard = f.field("guard");
  int calls = 0;
  auto observer = f.scene.observe([&](const sai::change_set &) { ++calls; });
  for (const auto &candidate : invalid_axes()) {
    for (bool many : {false, true}) {
      const auto target = f.field(many ? "many" : "single");
      const sai::value payload =
          many ? sai::value{sai::rotation_list{{}, candidate}}
               : sai::value{candidate};
      auto events = f.scene.events({10});
      REQUIRE(events.send(guard, sai::value{std::int32_t{123}}));
      const auto invalid = events.send(target, payload);
      REQUIRE_FALSE(invalid);
      CHECK(invalid.error().code == sai::error_code::invalid_value);
      CHECK(invalid.error().field == target.name());
      REQUIRE_FALSE(events.commit());
      CHECK(f.scene.snapshot().revision() == before.revision());
      CHECK(f.scene.snapshot().read(guard).value() ==
            sai::value{std::int32_t{0}});
    }
  }
  CHECK(f.scene.drain().delivered == 0);
  CHECK(calls == 0);
  auto retry = f.scene.events({9});
  REQUIRE(retry.send(guard, sai::value{std::int32_t{7}}));
  REQUIRE(retry.commit());
}

TEST_CASE("rotation runtime output rejects invalid axes with complete cascade "
          "rollback") {
  for (const auto &candidate : invalid_axes()) {
    for (bool many : {false, true}) {
      fixture f;
      const auto before = f.scene.snapshot();
      const auto guard = f.field("guard");
      const auto output = f.field(many ? "outputs" : "output");
      const sai::value payload =
          many ? sai::value{sai::rotation_list{{}, candidate}}
               : sai::value{candidate};
      REQUIRE(f.host.set_runtime_processor(
          f.scene, [&](sai::runtime_step &step) -> sai::result<void> {
            if (step.input())
              return step.emit(output, payload);
            return {};
          }));
      auto events = f.scene.events({1});
      REQUIRE(events.send(guard, sai::value{std::int32_t{123}}));
      const auto invalid = events.commit();
      REQUIRE_FALSE(invalid);
      CHECK(invalid.error().code == sai::error_code::invalid_value);
      CHECK(invalid.error().field == output.name());
      CHECK(f.scene.snapshot().revision() == before.revision());
      CHECK(f.scene.snapshot().read(guard).value() ==
            sai::value{std::int32_t{0}});
      CHECK(sai::same_representation(f.scene.snapshot().read(output).value(),
                                     before.read(output).value()));
      CHECK(f.scene.drain().delivered == 0);
    }
  }
}

TEST_CASE("axis validity preserves arbitrary angle IEEE representations and "
          "generic vectors") {
  fixture f;
  const auto single = f.field("single");
  const auto many = f.field("many");
  const std::vector<float> angles{
      -0.f,
      0.f,
      -10.f,
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      std::bit_cast<float>(std::uint32_t{0x7fc01234})};
  double time = 20;
  for (const auto angle : angles) {
    const sai::rotation candidate{0, 0, -1, angle};
    REQUIRE(sai::detail::valid_rotation_axis(candidate));
    auto edit = f.scene.edit();
    REQUIRE(edit.set(single, sai::value{candidate}));
    REQUIRE(edit.set(many, sai::value{sai::rotation_list{candidate}}));
    REQUIRE(edit.commit());
    CHECK(sai::same_representation(f.scene.snapshot().read(single).value(),
                                   sai::value{candidate}));
    CHECK(sai::same_representation(f.scene.snapshot().read(many).value(),
                                   sai::value{sai::rotation_list{candidate}}));
    auto events = f.scene.events({time++});
    REQUIRE(events.send(single, sai::value{candidate}));
    REQUIRE(events.send(many, sai::value{sai::rotation_list{candidate}}));
    REQUIRE(events.commit());
    CHECK(sai::same_representation(f.scene.snapshot().read(single).value(),
                                   sai::value{candidate}));
  }
  sai::type_registry generic;
  sai::node_type_descriptor descriptor;
  descriptor.name = "ArbitraryVector";
  auto field = rotation_field("vector");
  field.kind = sai::value_kind::sf_vec3f;
  field.default_value =
      sai::vec3f{std::numeric_limits<float>::infinity(), 2, 3};
  descriptor.fields.push_back(std::move(field));
  REQUIRE(generic.define(std::move(descriptor)));
}

TEST_CASE("existing descriptor ranges still constrain otherwise valid rotation "
          "angles") {
  sai::type_registry types;
  sai::node_type_descriptor descriptor;
  descriptor.name = "BoundedRotation";
  auto field = rotation_field("value");
  field.range.minimum = -1.;
  field.range.maximum = 1.;
  descriptor.fields.push_back(std::move(field));
  REQUIRE(types.define(std::move(descriptor)));
  sai::browser host{std::move(types)};
  const auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("BoundedRotation").value();
  REQUIRE(setup.commit());
  const auto target = scene.snapshot().field(node, "value").value();
  for (const auto angle : {2.f, std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::quiet_NaN()}) {
    auto edit = scene.edit();
    const auto denied =
        edit.set(target, sai::value{sai::rotation{0, 0, 1, angle}});
    REQUIRE_FALSE(denied);
    CHECK(denied.error().code == sai::error_code::invalid_value);
    REQUIRE_FALSE(edit.commit());
  }
}
