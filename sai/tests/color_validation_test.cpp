#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
enum class write_path { authoring, event_input, runtime_output };

template <class Color> std::vector<Color> invalid_colors() {
  const std::array invalid_channels{
      std::nextafter(0.f, -1.f), std::nextafter(1.f, 2.f),
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity()};
  std::vector<Color> colors;
  const auto add = [&](float Color::*member) {
    for (const float channel : invalid_channels) {
      Color color{};
      color.*member = channel;
      colors.push_back(color);
    }
  };
  add(&Color::r);
  add(&Color::g);
  add(&Color::b);
  if constexpr (std::is_same_v<Color, sai::color4f>)
    add(&Color::a);
  return colors;
}

template <class T> sai::node_type_descriptor color_type(T initial) {
  sai::node_type_descriptor type;
  type.name = "Colors";
  sai::field_descriptor color;
  color.name = "color";
  color.kind = sai::value_traits<T>::kind;
  color.default_value = std::move(initial);
  type.fields.push_back(std::move(color));
  sai::field_descriptor guard;
  guard.name = "guard";
  guard.kind = sai::value_kind::sf_float;
  guard.default_value = 7.f;
  type.fields.push_back(std::move(guard));
  return type;
}

template <class T> void check_writes(const std::vector<T> &invalid,
                                     write_path path) {
  sai::type_registry registry;
  REQUIRE(registry.define(color_type(T{})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Colors").value();
  const auto color = setup.field(node, "color").value();
  const auto guard = setup.field(node, "guard").value();
  REQUIRE(setup.append_root(node));
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  int changes = 0;
  int deliveries = 0;
  auto change_observer = scene.observe([&](const auto &) { ++changes; });
  auto field_observer = scene.observe(guard, [&](const auto &) { ++deliveries; });
  REQUIRE(field_observer);
  const char *operation = path == write_path::authoring ? "scene_edit.set"
                         : path == write_path::event_input ? "event_batch.send"
                         : "runtime_step.emit";
  for (std::size_t index = 0; index < invalid.size(); ++index) {
    CAPTURE(sai::value_traits<T>::kind);
    CAPTURE(index);
    for (bool typed : {false, true}) {
      CAPTURE(typed);
      for (const auto space : {sai::value_space::canonical,
                               sai::value_space::authored}) {
        CAPTURE(space);
        const auto submit = [&](auto &writer) -> sai::result<void> {
          if (typed)
            return writer.set(color.template as<T>().value(), invalid[index],
                              space);
          return writer.set(color, sai::value{invalid[index]}, space);
        };
        const auto send = [&](auto &writer) -> sai::result<void> {
          if (typed)
            return writer.send(color.template as<T>().value(), invalid[index],
                               space);
          return writer.send(color, sai::value{invalid[index]}, space);
        };
        sai::result<void> submitted;
        std::optional<sai::sai_error> commit_error;
        if (path == write_path::authoring) {
          auto edit = scene.edit();
          REQUIRE(edit.set(guard, sai::value{9.f}));
          submitted = submit(edit);
          REQUIRE_FALSE(submitted);
          const auto validated = edit.validate();
          REQUIRE_FALSE(validated);
          CHECK(validated.error().code == sai::error_code::invalid_value);
          const auto committed = edit.commit();
          REQUIRE_FALSE(committed);
          commit_error = committed.error();
        } else if (path == write_path::event_input) {
          auto batch = scene.events({1});
          REQUIRE(batch.send(guard, sai::value{9.f}));
          submitted = send(batch);
          REQUIRE_FALSE(submitted);
          const auto committed = batch.commit();
          REQUIRE_FALSE(committed);
          commit_error = committed.error();
        } else {
          REQUIRE(host.set_runtime_processor(
              scene, [&](sai::runtime_step &step) -> sai::result<void> {
                auto staged = step.emit(guard, sai::value{9.f});
                if (!staged)
                  return staged;
                if (typed)
                  submitted = step.emit(color.template as<T>().value(),
                                        invalid[index], space);
                else
                  submitted = step.emit(color, sai::value{invalid[index]}, space);
                // Ignoring the rejected emit must still poison the frame.
                return {};
              }));
          const auto committed = scene.events({1}).commit();
          REQUIRE_FALSE(submitted);
          REQUIRE_FALSE(committed);
          commit_error = committed.error();
          REQUIRE(host.set_runtime_processor(scene, {}));
        }
        CHECK(submitted.error().code == sai::error_code::invalid_value);
        CHECK(submitted.error().operation == operation);
        CHECK(submitted.error().generation == scene.generation());
        CHECK(submitted.error().base_revision == before.revision());
        CHECK(submitted.error().node == node.id());
        CHECK(submitted.error().field == "color");
        REQUIRE(commit_error);
        CHECK(commit_error->code == submitted.error().code);
        CHECK(commit_error->operation == submitted.error().operation);
        CHECK(commit_error->node == submitted.error().node);
        CHECK(commit_error->field == submitted.error().field);
        CHECK(scene.snapshot().revision() == before.revision());
        CHECK(sai::same_representation(scene.snapshot().read(color).value(),
                                      before.read(color).value()));
        CHECK(scene.snapshot().read(guard).value() == sai::value{7.f});
        CHECK(scene.drain().delivered == 0);
        CHECK(changes == 0);
        CHECK(deliveries == 0);
      }
    }
  }
  // A failed frame neither consumes the timestamp nor holds the reservation.
  auto retry = scene.events({1});
  REQUIRE(retry.send(guard, sai::value{8.f}));
  REQUIRE(retry.commit());
  CHECK(scene.drain().delivered == 2);
  CHECK(changes == 1);
  CHECK(deliveries == 1);
}

void check_color_writes(write_path path) {
  const auto rgb = invalid_colors<sai::color3f>();
  const auto rgba = invalid_colors<sai::color4f>();
  check_writes(rgb, path);
  check_writes(rgba, path);
  std::vector<sai::color3f_list> rgbs;
  std::vector<sai::color4f_list> rgbas;
  for (const auto &color : rgb)
    rgbs.push_back({sai::color3f{}, color});
  for (const auto &color : rgba)
    rgbas.push_back({sai::color4f{}, color});
  check_writes(rgbs, path);
  check_writes(rgbas, path);
}

template <class Element>
void check_multi_editors(const std::vector<Element> &invalid_values) {
  using Sequence = std::vector<Element>;
  sai::type_registry registry;
  REQUIRE(registry.define(color_type(Sequence{Element{}})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Colors").value();
  const auto field = setup.field(node, "color").value();
  const auto guard = setup.field(node, "guard").value();
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  int notifications = 0;
  auto observer = scene.observe([&](const auto &) { ++notifications; });
  for (const auto &invalid : invalid_values) {
    for (bool typed : {false, true}) {
      for (int operation = 0; operation < 4; ++operation) {
        CAPTURE(sai::value_traits<Sequence>::kind);
        CAPTURE(typed);
        CAPTURE(operation);
        auto edit = scene.edit();
        REQUIRE(edit.set(guard, sai::value{9.f}));
        const auto mutate = [&](auto &range, auto element, auto replacement) {
          switch (operation) {
          case 0: return range.set(0, std::move(element));
          case 1: return range.insert(0, std::move(element));
          case 2: return range.append(std::move(element));
          default: return range.replace(std::move(replacement));
          }
        };
        sai::result<void> rejected;
        if (typed) {
          auto range = edit.multi(field.template as<Sequence>().value()).value();
          rejected = mutate(range, invalid, Sequence{Element{}, invalid});
        } else {
          auto range = edit.multi(field).value();
          rejected = mutate(range, sai::value{invalid},
                            sai::value{Sequence{Element{}, invalid}});
        }
        REQUIRE_FALSE(rejected);
        CHECK(rejected.error().code == sai::error_code::invalid_value);
        CHECK(rejected.error().node == node.id());
        CHECK(rejected.error().field == "color");
        const auto validated = edit.validate();
        REQUIRE_FALSE(validated);
        CHECK(validated.error().code == rejected.error().code);
        const auto committed = edit.commit();
        REQUIRE_FALSE(committed);
        CHECK(committed.error().code == rejected.error().code);
        CHECK(scene.snapshot().revision() == before.revision());
        CHECK(sai::same_representation(scene.snapshot().read(field).value(),
                                      before.read(field).value()));
        CHECK(scene.snapshot().read(guard).value() == sai::value{7.f});
        CHECK(scene.drain().delivered == 0);
        CHECK(notifications == 0);
      }
    }
  }
  // A genuinely wrong element kind retains the distinct type error.
  for (int operation = 0; operation < 4; ++operation) {
    CAPTURE(operation);
    auto edit = scene.edit();
    auto range = edit.multi(field).value();
    sai::result<void> mismatch;
    switch (operation) {
    case 0: mismatch = range.set(0, sai::value{0.f}); break;
    case 1: mismatch = range.insert(0, sai::value{0.f}); break;
    case 2: mismatch = range.append(sai::value{0.f}); break;
    default: mismatch = range.replace(sai::value{sai::float_list{0.f}}); break;
    }
    REQUIRE_FALSE(mismatch);
    CHECK(mismatch.error().code == sai::error_code::type_mismatch);
  }
}

template <class T> void check_default(T invalid) {
  sai::type_registry registry;
  const auto rejected = registry.define(color_type(invalid));
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  CHECK(registry.node_types().empty());
  REQUIRE(registry.define(color_type(T{})));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  REQUIRE(edit.create_node("Colors"));
  sai::local_declaration_descriptor declaration;
  declaration.name = "Palette";
  sai::interface_field_descriptor field;
  field.name = "color";
  field.kind = sai::value_traits<T>::kind;
  field.default_value = std::move(invalid);
  declaration.interface.push_back(std::move(field));
  const auto rejected_default = edit.add_local_declaration(std::move(declaration));
  REQUIRE_FALSE(rejected_default);
  CHECK(rejected_default.error().code == sai::error_code::invalid_descriptor);
  CHECK_FALSE(edit.validate());
  CHECK_FALSE(edit.commit());
  CHECK(scene.snapshot().revision() == 0);
}
} // namespace

TEST_CASE("color authoring rejects invalid channels atomically") {
  check_color_writes(write_path::authoring);
}
TEST_CASE("color event input rejects invalid channels atomically") {
  check_color_writes(write_path::event_input);
}
TEST_CASE("color runtime output rejects invalid channels atomically") {
  check_color_writes(write_path::runtime_output);
}
TEST_CASE("color MF editors reject invalid elements atomically") {
  check_multi_editors(invalid_colors<sai::color3f>());
  check_multi_editors(invalid_colors<sai::color4f>());
}
TEST_CASE("color defaults reject invalid scalar and multi representations") {
  for (const auto &invalid : invalid_colors<sai::color3f>()) {
    check_default(invalid);
    check_default(sai::color3f_list{sai::color3f{}, invalid});
  }
  for (const auto &invalid : invalid_colors<sai::color4f>()) {
    check_default(invalid);
    check_default(sai::color4f_list{sai::color4f{}, invalid});
  }
}
TEST_CASE("color endpoints and signed zero survive every owning boundary") {
  const auto exercise = []<class T>(T valid) {
    sai::type_registry registry;
    REQUIRE(registry.define(color_type(T{})));
    sai::browser host{std::move(registry)};
    auto scene = host.current_scene();
    auto setup = scene.edit();
    const auto node = setup.create_node("Colors").value();
    const auto dynamic = setup.field(node, "color").value();
    const auto typed = dynamic.template as<T>().value();
    REQUIRE(setup.set(typed, valid, sai::value_space::authored));
    REQUIRE(setup.commit());
    CHECK(sai::same_representation(scene.snapshot().read(dynamic).value(),
                                  sai::value{valid}));
    auto input = scene.events({1});
    REQUIRE(input.send(dynamic, sai::value{valid}));
    REQUIRE(input.commit());
    REQUIRE(host.set_runtime_processor(
        scene, [&](sai::runtime_step &step) { return step.emit(typed, valid); }));
    REQUIRE(scene.events({2}).commit());
    CHECK(sai::same_representation(scene.snapshot().read(dynamic).value(),
                                  sai::value{valid}));
  };
  exercise(sai::color3f{-0.f, 1.f, 0.f});
  exercise(sai::color4f{1.f, -0.f, 0.f, 1.f});
  exercise(sai::color3f_list{{0.f, 1.f, -0.f}, {-0.f, 0.f, 1.f}});
  exercise(sai::color4f_list{{0.f, 1.f, -0.f, 0.f}, {1.f, 0.f, 1.f, -0.f}});
}

TEST_CASE("image MF editors distinguish malformed values from wrong kinds") {
  check_multi_editors(std::vector<sai::image>{
      {.width = -1, .height = 1, .components = 1, .data = {}},
      {.width = 1, .height = 1, .components = 5, .data = {}},
      {.width = 2, .height = 1, .components = 3, .data = {1, 2, 3}}});
}
