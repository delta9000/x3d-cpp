#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include "doctest/doctest.h"

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
sai::node_type_descriptor bounded_type(sai::value initial,
                                       sai::numeric_range range,
                                       sai::access_type access =
                                           sai::access_type::input_output) {
  sai::node_type_descriptor type;
  type.name = "Bounded";
  sai::field_descriptor field;
  field.name = "value";
  field.kind = std::visit([]<class T>(const T &) {
    if constexpr (std::is_same_v<T, std::monostate>)
      return sai::value_kind::sf_string;
    else
      return sai::value_traits<T>::kind;
  }, initial);
  field.access = access;
  field.default_value = std::move(initial);
  field.range = std::move(range);
  type.fields.push_back(std::move(field));
  sai::field_descriptor guard;
  guard.name = "guard";
  guard.kind = sai::value_kind::sf_float;
  guard.default_value = 7.f;
  type.fields.push_back(std::move(guard));
  return type;
}

enum class write_path { authoring, event_input, runtime_output };
void check_writes(write_path path) {
  for (bool minimum_inclusive : {false, true}) {
    for (bool maximum_inclusive : {false, true}) {
      CAPTURE(minimum_inclusive);
      CAPTURE(maximum_inclusive);
      const sai::numeric_range range{.minimum = 0., .maximum = 1.,
                                     .minimum_inclusive = minimum_inclusive,
                                     .maximum_inclusive = maximum_inclusive};
      auto descriptor = bounded_type(.5f, range);
      descriptor.fields[0].unit_category = "length";
      sai::type_registry registry;
      REQUIRE(registry.define(std::move(descriptor)));
      sai::browser host{std::move(registry)};
      auto scene = host.current_scene();
      auto setup = scene.edit();
      REQUIRE(setup.declare_unit(
          {.category = "length", .name = "double_metre", .conversion_factor = 2.}));
      const auto node = setup.create_node("Bounded").value();
      const auto field = setup.field(node, "value").value();
      const auto guard = setup.field(node, "guard").value();
      REQUIRE(setup.append_root(node));
      REQUIRE(setup.commit());
      const auto before = scene.snapshot();
      int notifications = 0;
      int deliveries = 0;
      auto observer = scene.observe([&](const auto &) { ++notifications; });
      auto field_observer = scene.observe(guard, [&](const auto &) { ++deliveries; });
      REQUIRE(field_observer);
      std::vector<float> invalid{
          std::nextafter(0.f, -1.f), std::nextafter(1.f, 2.f), .75f,
          std::numeric_limits<float>::quiet_NaN(),
          std::numeric_limits<float>::infinity(),
          -std::numeric_limits<float>::infinity()};
      if (!minimum_inclusive)
        invalid.push_back(0.f);
      if (!maximum_inclusive)
        invalid.push_back(1.f);
      for (float candidate : invalid) {
        CAPTURE(candidate);
        for (bool typed : {false, true}) {
          for (auto space : {sai::value_space::canonical,
                             sai::value_space::authored}) {
            // .75 authored metres becomes 1.5 canonical metres.
            if (candidate == .75f && space == sai::value_space::canonical)
              continue;
            CAPTURE(typed);
            CAPTURE(space);
            sai::result<void> submitted;
            std::optional<sai::sai_error> error;
            if (path == write_path::authoring) {
              auto edit = scene.edit();
              REQUIRE(edit.set(guard, sai::value{9.f}));
              submitted = typed ? edit.set(field.as<float>().value(), candidate, space)
                  : edit.set(field, sai::value{candidate}, space);
              REQUIRE_FALSE(submitted);
              CHECK_FALSE(edit.validate());
              const auto committed = edit.commit();
              REQUIRE_FALSE(committed);
              error = committed.error();
            } else if (path == write_path::event_input) {
              auto batch = scene.events({1});
              REQUIRE(batch.send(guard, sai::value{9.f}));
              submitted = typed ? batch.send(field.as<float>().value(), candidate, space)
                  : batch.send(field, sai::value{candidate}, space);
              REQUIRE_FALSE(submitted);
              const auto committed = batch.commit();
              REQUIRE_FALSE(committed);
              error = committed.error();
            } else {
              REQUIRE(host.set_runtime_processor(
                  scene, [&](sai::runtime_step &step) -> sai::result<void> {
                    auto staged = step.emit(guard, sai::value{9.f});
                    if (!staged)
                      return staged;
                    submitted = typed ? step.emit(field.as<float>().value(), candidate, space)
                        : step.emit(field, sai::value{candidate}, space);
                    return {};
                  }));
              const auto committed = scene.events({1}).commit();
              REQUIRE_FALSE(submitted);
              REQUIRE_FALSE(committed);
              error = committed.error();
              REQUIRE(host.set_runtime_processor(scene, {}));
            }
            CHECK(submitted.error().code == sai::error_code::invalid_value);
            CHECK(submitted.error().node == node.id());
            CHECK(submitted.error().field == "value");
            REQUIRE(error);
            CHECK(error->code == submitted.error().code);
            CHECK(error->node == submitted.error().node);
            CHECK(error->field == submitted.error().field);
            CHECK(scene.snapshot().revision() == before.revision());
            CHECK(scene.snapshot().read(field).value() == sai::value{.5f});
            CHECK(scene.snapshot().read(guard).value() == sai::value{7.f});
            CHECK(scene.drain().delivered == 0);
            CHECK(notifications == 0);
            CHECK(deliveries == 0);
          }
        }
      }
      // Authored assignment is checked after conversion into canonical units.
      auto retry = scene.events({1});
      REQUIRE(retry.send(field.as<float>().value(), .25f, sai::value_space::authored));
      REQUIRE(retry.send(guard, sai::value{8.f}));
      REQUIRE(retry.commit());
      CHECK(scene.snapshot().read(field).value() == sai::value{.5f});
      CHECK(scene.drain().delivered == 2);
      auto endpoints = scene.edit();
      REQUIRE(endpoints.set(field, sai::value{minimum_inclusive ? 0.f
                                       : std::nextafter(0.f, 1.f)}));
      REQUIRE(endpoints.set(field, sai::value{maximum_inclusive ? 1.f
                                       : std::nextafter(1.f, 0.f)}));
      REQUIRE(endpoints.commit());
    }
  }
}
} // namespace

TEST_CASE("numeric range authoring rejects invalid components atomically") {
  check_writes(write_path::authoring);
}
TEST_CASE("numeric range event input rejects invalid components atomically") {
  check_writes(write_path::event_input);
}
TEST_CASE("numeric range runtime output rejects invalid components atomically") {
  check_writes(write_path::runtime_output);
}
TEST_CASE("numeric ranges validate descriptor shape and defaults") {
  for (const auto &range : std::vector<sai::numeric_range>{
           {.minimum = 2., .maximum = 1.},
           {.minimum = 1., .maximum = 1., .minimum_inclusive = false},
           {.minimum = 1., .maximum = 1., .maximum_inclusive = false},
           {.minimum = std::numeric_limits<double>::quiet_NaN()},
           {.maximum = std::numeric_limits<double>::infinity()},
           {.minimum = 2.}}) {
    sai::type_registry registry;
    const auto rejected = registry.define(bounded_type(1.f, range));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
    CHECK(registry.node_types().empty());
  }
  for (const auto &initial : std::vector<sai::value>{
           true, std::string{"text"}, sai::enum_value{"choice"}, sai::image{},
           sai::node_id{}, sai::bool_list{}, sai::string_list{}, sai::node_list{}}) {
    sai::type_registry registry;
    CHECK_FALSE(registry.define(bounded_type(initial, {.minimum = 0.})));
  }
  for (const auto &type : std::vector<sai::node_type_descriptor>{
           bounded_type(std::int32_t{0}, {.minimum = 1.1, .maximum = 1.9},
                        sai::access_type::output_only),
           bounded_type(0.f, {.minimum = std::numeric_limits<float>::max(),
                             .minimum_inclusive = false},
                        sai::access_type::output_only),
           bounded_type(sai::float_list{}, {.minimum = 1., .maximum = 1.000000001,
                                            .minimum_inclusive = false})}) {
    sai::type_registry registry;
    CHECK_FALSE(registry.define(type));
  }
  sai::type_registry exact;
  REQUIRE(exact.define(bounded_type(1.f, {.minimum = 1., .maximum = 1.})));
  for (auto access : {sai::access_type::input_only, sai::access_type::output_only}) {
    sai::type_registry registry;
    REQUIRE(registry.define(bounded_type(0.f, {.minimum = 0.,
                                               .minimum_inclusive = false}, access)));
    auto wrong = bounded_type(-1.f, {.minimum = 0.}, access);
    wrong.name = "Wrong";
    CHECK_FALSE(registry.define(std::move(wrong)));
    auto declared = bounded_type(0.f, {.minimum = 0.,
                                      .minimum_inclusive = false}, access);
    declared.name = "Declared";
    declared.fields[0].default_origin = sai::default_source::schema;
    CHECK_FALSE(registry.define(std::move(declared)));
  }
}
TEST_CASE("numeric ranges reach every scalar and sequence component") {
  auto matrix = sai::matrix4d{};
  matrix.at(2, 3) = 2.;
  const std::vector<std::pair<sai::value, sai::value>> cases{
      {std::int32_t{0}, std::int32_t{2}}, {0., 2.},
      {sai::time_value{}, sai::time_value{2.}},
      {sai::vec2f{}, sai::vec2f{0, 2}},
      {sai::vec3f{}, sai::vec3f{0, 0, 2}},
      {sai::vec4f{}, sai::vec4f{0, 0, 0, 2}},
      {sai::vec2d{}, sai::vec2d{0, 2}},
      {sai::vec3d{}, sai::vec3d{0, 0, 2}},
      {sai::vec4d{}, sai::vec4d{0, 0, 0, 2}},
      {sai::rotation{}, sai::rotation{0, 0, 1, 2}},
      {sai::matrix4d{}, matrix},
      {sai::int32_list{0}, sai::int32_list{0, 2}},
      {sai::double_list{0}, sai::double_list{0, 2}},
      {sai::time_list{{0}}, sai::time_list{{0}, {2}}},
      {sai::vec3f_list{{}}, sai::vec3f_list{{}, {0, 0, 2}}},
      {sai::matrix4d_list{{}}, sai::matrix4d_list{{}, matrix}}};
  for (const auto &[initial, invalid] : cases) {
    CAPTURE(initial.index());
    sai::type_registry registry;
    REQUIRE(registry.define(bounded_type(initial, {.minimum = -1., .maximum = 1.})));
    sai::browser host{std::move(registry)};
    auto scene = host.current_scene();
    auto edit = scene.edit();
    const auto node = edit.create_node("Bounded").value();
    const auto field = edit.field(node, "value").value();
    const auto rejected = edit.set(field, invalid);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_value);
    CHECK_FALSE(edit.commit());
    CHECK(scene.snapshot().revision() == 0);
  }
}
TEST_CASE("numeric range MF editors validate the entire mutation") {
  sai::type_registry registry;
  REQUIRE(registry.define(bounded_type(sai::float_list{.5f},
                                      {.minimum = 0., .maximum = 1.})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Bounded").value();
  const auto field = setup.field(node, "value").value();
  const auto guard = setup.field(node, "guard").value();
  REQUIRE(setup.commit());
  const auto revision = scene.snapshot().revision();
  for (float invalid : {-.1f, 1.1f, std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::infinity()}) {
    for (bool typed : {false, true}) {
      for (int operation = 0; operation < 4; ++operation) {
        auto edit = scene.edit();
        REQUIRE(edit.set(guard, sai::value{9.f}));
        const auto mutate = [&](auto &range, auto item, auto list) {
          switch (operation) {
          case 0: return range.set(0, item);
          case 1: return range.insert(0, item);
          case 2: return range.append(item);
          default: return range.replace(list);
          }
        };
        sai::result<void> rejected;
        if (typed) {
          auto range = edit.multi(field.as<sai::float_list>().value()).value();
          rejected = mutate(range, invalid, sai::float_list{.5f, invalid});
        } else {
          auto range = edit.multi(field).value();
          rejected = mutate(range, sai::value{invalid},
                            sai::value{sai::float_list{.5f, invalid}});
        }
        REQUIRE_FALSE(rejected);
        CHECK(rejected.error().code == sai::error_code::invalid_value);
        CHECK_FALSE(edit.validate());
        CHECK_FALSE(edit.commit());
        CHECK(scene.snapshot().revision() == revision);
        CHECK(scene.snapshot().read(guard).value() == sai::value{7.f});
        CHECK(scene.snapshot().read(field).value() == sai::value{sai::float_list{.5f}});
        CHECK(scene.drain().delivered == 0);
      }
    }
  }
  auto accepted = scene.edit();
  auto range = accepted.multi(field.as<sai::float_list>().value()).value();
  REQUIRE(range.set(0, -0.f));
  REQUIRE(range.insert(0, 1.f));
  REQUIRE(range.append(0.f));
  REQUIRE(range.replace(sai::float_list{1.f, -0.f}));
  REQUIRE(accepted.commit());
  CHECK(sai::same_representation(scene.snapshot().read(field).value(),
                                sai::value{sai::float_list{1.f, -0.f}}));
}
TEST_CASE("numeric range ROUTE admission rolls back sources and destinations") {
  sai::type_registry registry;
  auto source_type = bounded_type(.5f, {});
  source_type.name = "Source";
  REQUIRE(registry.define(std::move(source_type)));
  REQUIRE(registry.define(bounded_type(.5f, {.minimum = 0., .maximum = 1.})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto source = setup.create_node("Source").value();
  const auto sink = setup.create_node("Bounded").value();
  const auto from = setup.field(source, "value").value();
  const auto to = setup.field(sink, "value").value();
  REQUIRE(setup.add_route(from, to));
  REQUIRE(setup.commit());
  const auto revision = scene.snapshot().revision();
  int notifications = 0;
  auto observer = scene.observe([&](const auto &) { ++notifications; });
  for (bool runtime : {false, true}) {
    if (runtime) {
      REQUIRE(host.set_runtime_processor(scene, [&](sai::runtime_step &step) {
        if (step.input())
          return sai::result<void>{};
        return step.emit(from, sai::value{2.f});
      }));
    }
    auto batch = scene.events({1});
    if (!runtime)
      REQUIRE(batch.send(from, sai::value{2.f}));
    const auto rejected = batch.commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_value);
    CHECK(rejected.error().node == sink.id());
    CHECK(rejected.error().field == "value");
    CHECK(scene.snapshot().revision() == revision);
    CHECK(scene.snapshot().read(from).value() == sai::value{.5f});
    CHECK(scene.snapshot().read(to).value() == sai::value{.5f});
    CHECK(scene.drain().delivered == 0);
    CHECK(notifications == 0);
    REQUIRE(host.set_runtime_processor(scene, {}));
  }
  auto retry = scene.events({1});
  REQUIRE(retry.send(from, sai::value{1.f}));
  REQUIRE(retry.commit());
  CHECK(scene.snapshot().read(to).value() == sai::value{1.f});
}
TEST_CASE("numeric ranges are discoverable from generated UOM fields") {
  const std::array<std::string_view, 4> selected{
      "Sphere", "Material", "Arc2D", "TextureProjector"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);
  const auto &radius = registry->find("Sphere")->fields;
  const auto lookup = [](const auto &fields, std::string_view name) {
    for (const auto &field : fields)
      if (field.name == name)
        return field.range;
    FAIL("missing generated field");
    return sai::numeric_range{};
  };
  CHECK(lookup(radius, "radius") == sai::numeric_range{
      .minimum = 0., .minimum_inclusive = false});
  CHECK(lookup(registry->find("Material")->fields, "transparency") ==
        sai::numeric_range{.minimum = 0., .maximum = 1.});
  CHECK(lookup(registry->find("Arc2D")->fields, "endAngle") ==
        sai::numeric_range{.minimum = -6.2832, .maximum = 6.2832,
                            .minimum_inclusive = false, .maximum_inclusive = false});
  sai::browser host{std::move(*registry)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto projector = edit.create_node("TextureProjector").value();
  const auto ratio = edit.field(projector, "aspectRatio").value();
  REQUIRE(edit.commit());
  CHECK(scene.snapshot().read(ratio).value() == sai::value{0.f});
  CHECK(lookup(scene.snapshot().node_types()[3].fields, "aspectRatio").minimum == 0.);
  REQUIRE(host.set_runtime_processor(scene, [&](sai::runtime_step &step) {
    return step.emit(ratio, sai::value{0.f});
  }));
  const auto rejected = scene.events({1}).commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_value);
  REQUIRE(host.set_runtime_processor(scene, [&](sai::runtime_step &step) {
    return step.emit(ratio, sai::value{1.5f});
  }));
  REQUIRE(scene.events({1}).commit());
  CHECK(scene.snapshot().read(ratio).value() == sai::value{1.5f});
}

TEST_CASE("numeric range endpoints use the field component precision") {
  sai::type_registry registry;
  REQUIRE(registry.define(bounded_type(.6f, {.minimum = .6, .maximum = .6})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Bounded").value();
  const auto field = setup.field(node, "value").value();
  REQUIRE(setup.set(field.as<float>().value(), .6f));
  REQUIRE(setup.commit());
  for (float invalid : {std::nextafter(.6f, 0.f), std::nextafter(.6f, 1.f)}) {
    auto edit = scene.edit();
    CHECK_FALSE(edit.set(field, sai::value{invalid}));
    CHECK_FALSE(edit.commit());
  }
  sai::type_registry doubles;
  REQUIRE(doubles.define(bounded_type(.6, {.minimum = .6, .maximum = .6})));
  sai::browser double_host{std::move(doubles)};
  auto edit = double_host.current_scene().edit();
  const auto double_node = edit.create_node("Bounded").value();
  const auto double_field = edit.field(double_node, "value").value();
  CHECK_FALSE(edit.set(double_field, sai::value{static_cast<double>(.6f)}));
  CHECK_FALSE(edit.commit());
  sai::type_registry integers;
  CHECK_FALSE(integers.define(bounded_type(std::int32_t{1}, {.minimum = 1.1})));
  REQUIRE(integers.define(bounded_type(
      std::numeric_limits<std::int32_t>::lowest(),
      {.maximum = std::numeric_limits<std::int32_t>::lowest(),
       .minimum_inclusive = false})));
  sai::type_registry floats;
  REQUIRE(floats.define(bounded_type(std::numeric_limits<float>::lowest(),
      {.maximum = std::numeric_limits<float>::lowest(),
       .minimum_inclusive = false})));
  auto generated = sai::generated_type_registry(std::array<std::string_view, 1>{"SpotLight"});
  REQUIRE(generated);
}
TEST_CASE("numeric ranges constrain normalized scalar and multi colors") {
  for (const auto &initial : std::vector<sai::value>{
           sai::color3f{}, sai::color4f{}, sai::color3f_list{{}}, sai::color4f_list{{}}}) {
    sai::type_registry registry;
    REQUIRE(registry.define(bounded_type(initial, {.minimum = 0., .maximum = .5})));
    sai::browser host{std::move(registry)};
    auto edit = host.current_scene().edit();
    const auto node = edit.create_node("Bounded").value();
    const auto field = edit.field(node, "value").value();
    const sai::value invalid = std::visit([]<class T>(const T &) -> sai::value {
      if constexpr (std::is_same_v<T, sai::color3f>)
        return sai::color3f{0, .75f, 0};
      else if constexpr (std::is_same_v<T, sai::color4f>)
        return sai::color4f{0, 0, 0, .75f};
      else if constexpr (std::is_same_v<T, sai::color3f_list>)
        return sai::color3f_list{{}, {0, 0, .75f}};
      else
        return sai::color4f_list{{}, {0, 0, 0, .75f}};
    }, initial);
    const auto rejected = edit.set(field, invalid);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_value);
    CHECK_FALSE(edit.commit());
  }
}
