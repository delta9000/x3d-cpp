#include "doctest/doctest.h"
#include "x3d/sai/experimental/X3DSAIBindingCatalog.hpp"
#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
template <class T>
concept has_def_key = requires { T::DEF; };
template <class T>
concept has_use_key = requires { T::USE; };
template <class T>
concept has_is_key = requires { T::IS; };
template <class T>
concept has_class_key = requires { T::class_; };
template <class T>
concept has_id_key = requires { T::id; };
template <class T>
concept has_style_key = requires { T::style; };

// A user-supplied key describing the removed legacy surface. It must still
// go through registry validation rather than recreating an encoding field.
struct LegacyTransform {
  static constexpr std::string_view x3d_name = "Transform";
  static constexpr std::string_view schema_fingerprint =
      sai::bindings::model_fingerprint;
  inline static constexpr sai::field_key<LegacyTransform, std::string> DEF{
      "DEF", sai::access_type::input_output};
};
} // namespace

TEST_CASE("generated runtime fields exclude encoding binding and host syntax") {
  // These are semantic expectations, not a comparison of two projections.
  CHECK_FALSE(has_def_key<sai::bindings::Transform>);
  CHECK_FALSE(has_use_key<sai::bindings::Transform>);
  CHECK_FALSE(has_is_key<sai::bindings::Transform>);
  CHECK_FALSE(has_class_key<sai::bindings::Transform>);
  CHECK_FALSE(has_id_key<sai::bindings::Transform>);
  CHECK_FALSE(has_style_key<sai::bindings::Transform>);

  const auto check_fields = [](const auto &nodes) {
    for (const auto &node : nodes) {
      CAPTURE(node.name);
      for (const auto &field : node.fields) {
        CAPTURE(field.name);
        CHECK(field.name != "DEF");
        CHECK(field.name != "USE");
        CHECK(field.name != "IS");
        CHECK(field.name != "class");
        CHECK(field.name != "id");
        if (node.name != "FontStyle" && node.name != "ScreenFontStyle")
          CHECK(field.name != "style");
      }
    }
  };
  check_fields(sai::generated_metadata_catalog().nodes);
  check_fields(sai::bindings::node_keys);
}

TEST_CASE("generated nodes use semantic names and shared occurrences") {
  auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<sai::bindings::Transform>();
  REQUIRE(transform);
  REQUIRE(edit.define_name("Original", transform->dynamic()));
  REQUIRE(edit.append_root(transform->dynamic()));
  REQUIRE(edit.append_root(transform->dynamic()));
  REQUIRE(edit.commit());

  const auto before = context.snapshot();
  const auto description = before.describe(transform->dynamic());
  REQUIRE(description);
  std::vector<std::string> fields;
  for (const auto &field : description->fields)
    fields.push_back(field.name);
  CHECK(fields ==
        std::vector<std::string>{"addChildren", "bboxCenter", "bboxDisplay",
                                 "bboxSize", "center", "children", "metadata",
                                 "removeChildren", "rotation", "scale",
                                 "scaleOrientation", "translation", "visible"});
  for (const auto name : {"DEF", "USE", "IS", "class", "id", "style"}) {
    CAPTURE(name);
    auto field = before.field(transform->dynamic(), name);
    CHECK_FALSE(field);
    if (!field)
      CHECK(field.error().code == sai::error_code::unknown_field);
  }
  CHECK(before.named("Original")->id() == transform->id());
  REQUIRE(before.occurrences().size() == 2);
  CHECK(before.roots()[0] == before.roots()[1]);

  auto rename = context.edit();
  REQUIRE(rename.undefine_name("Original"));
  REQUIRE(rename.define_name("Renamed", transform->dynamic()));
  REQUIRE(rename.commit());
  CHECK_FALSE(context.snapshot().named("Original"));
  CHECK(context.snapshot().named("Renamed")->id() == transform->id());
  CHECK(before.named("Original")->id() == transform->id());
}

TEST_CASE("legacy encoding keys cannot create retained values or events") {
  auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<LegacyTransform>();
  REQUIRE(transform);
  REQUIRE(edit.commit());
  const auto before = context.snapshot();
  // Even a handcrafted typed key cannot bypass the descriptor into a second
  // identity store, retained value, or event stream.
  const auto fake_def = transform->field(LegacyTransform::DEF);
  auto rejected_edit = context.edit();
  const auto written = rejected_edit.set(fake_def, std::string{"Other"});
  CHECK_FALSE(written);
  if (!written)
    CHECK(written.error().code == sai::error_code::unknown_field);
  CHECK_FALSE(rejected_edit.commit());
  auto events = context.events(sai::event_time{1.0});
  const auto sent = events.send(fake_def, std::string{"Other"});
  CHECK_FALSE(sent);
  if (!sent)
    CHECK(sent.error().code == sai::error_code::unknown_field);
  CHECK_FALSE(events.commit());
  CHECK(context.snapshot().revision() == before.revision());
}

TEST_CASE("font style remains a semantic field rather than CSS") {
  CHECK(has_style_key<sai::bindings::FontStyle>);
  CHECK(has_style_key<sai::bindings::ScreenFontStyle>);
  auto registry =
      sai::generated_type_registry_for<sai::bindings::FontStyle,
                                       sai::bindings::ScreenFontStyle>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto font = edit.create<sai::bindings::FontStyle>();
  auto screen = edit.create<sai::bindings::ScreenFontStyle>();
  REQUIRE(font);
  REQUIRE(screen);
  const auto style = font->field(sai::bindings::FontStyle::style);
  const auto screen_style =
      screen->field(sai::bindings::ScreenFontStyle::style);
  REQUIRE(edit.set(style, std::string{"BOLD"}));
  REQUIRE(edit.commit());
  CHECK(context.snapshot().read(style).value() == std::string{"BOLD"});
  CHECK(context.snapshot().read(screen_style).value() ==
        std::string{"PLAIN"});
}

TEST_CASE("generated grouping children expose their explicit uniqueness constraint") {
  const auto catalog = sai::generated_metadata_catalog();
  for (const auto name : {"X3DGroupingNode", "X3DViewportNode", "Transform",
                          "Group", "CADPart", "LayoutGroup", "Viewport"}) {
    CAPTURE(name);
    const auto type = std::ranges::find(catalog.nodes, name,
                                        &sai::schema_node_descriptor::name);
    REQUIRE(type != catalog.nodes.end());
    for (const auto &field : type->fields)
      CHECK(field.unique_non_null_nodes == (field.name == "children"));
  }
  for (const auto name : {"Contour2D", "MetadataSet", "Layer", "Analyser"}) {
    const auto type = std::ranges::find(catalog.nodes, name,
                                        &sai::schema_node_descriptor::name);
    REQUIRE(type != catalog.nodes.end());
    for (const auto &field : type->fields)
      CHECK_FALSE(field.unique_non_null_nodes);
  }
  auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto edit = host.current_scene().edit();
  const auto node = edit.create<sai::bindings::Transform>().value();
  REQUIRE(edit.commit());
  const auto described = host.current_scene().snapshot().describe(node.dynamic());
  REQUIRE(described);
  for (const auto &field : described->fields)
    CHECK(field.unique_non_null_nodes == (field.name == "children"));
}

TEST_CASE("generated grouping duplicate children reject whole values and MF edits") {
  for (bool typed : {false, true}) {
    for (int operation = 0; operation < 6; ++operation) {
      CAPTURE(typed);
      CAPTURE(operation);
      auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
      REQUIRE(registry);
      sai::browser host{std::move(*registry)};
      auto context = host.current_scene();
      auto setup = context.edit();
      const auto parent = setup.create<sai::bindings::Transform>().value();
      const auto first = setup.create<sai::bindings::Transform>().value();
      const auto second = setup.create<sai::bindings::Transform>().value();
      const auto children = parent.field(sai::bindings::Transform::children);
      const auto guard = parent.field(sai::bindings::Transform::translation);
      const std::array initial{first.dynamic(), second.dynamic()};
      REQUIRE(setup.set(children, std::span<const sai::node>{initial}));
      REQUIRE(setup.commit());
      const auto before = context.snapshot();
      int notifications = 0;
      auto observer = context.observe([&](const auto &) { ++notifications; });
      auto edit = context.edit();
      REQUIRE(edit.set(guard, sai::vec3f{7, 8, 9}));
      const std::array duplicate{first.dynamic(), sai::node{}, first.dynamic()};
      if (operation == 0) {
        if (typed)
          REQUIRE(edit.set(children, std::span<const sai::node>{duplicate}));
        else
          REQUIRE(edit.set(children.dynamic(), std::span<const sai::node>{duplicate}));
      } else if (operation == 5) {
        REQUIRE(edit.append(parent.dynamic(), "children", first.dynamic()));
      } else {
        const auto mutate = [&](auto range) {
          switch (operation) {
          case 1: return range.set(1, first.dynamic());
          case 2: return range.insert(1, first.dynamic());
          case 3: return range.append(first.dynamic());
          default: return range.replace(std::span<const sai::node>{duplicate});
          }
        };
        if (typed)
          REQUIRE(mutate(edit.multi(children).value()));
        else
          REQUIRE(mutate(edit.multi(children.dynamic()).value()));
      }
      const auto validation = edit.validate();
      REQUIRE_FALSE(validation);
      CHECK(validation.error().code == sai::error_code::invalid_value);
      CHECK(validation.error().node == parent.id());
      CHECK(validation.error().field == "children");
      const auto committed = edit.commit();
      REQUIRE_FALSE(committed);
      CHECK(committed.error().code == sai::error_code::invalid_value);
      CHECK(context.snapshot().revision() == before.revision());
      CHECK(context.snapshot().read(children).value() == before.read(children).value());
      CHECK(context.snapshot().read(guard).value() == sai::vec3f{});
      CHECK(context.drain().delivered == 0);
      CHECK(notifications == 0);
    }
  }
}

TEST_CASE("generated grouping children preserve NULL positions and cross-parent sharing") {
  auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  const auto first = edit.create<sai::bindings::Transform>().value();
  const auto second = edit.create<sai::bindings::Transform>().value();
  const auto child = edit.create<sai::bindings::Transform>().value();
  const auto first_children = first.field(sai::bindings::Transform::children);
  const auto second_children = second.field(sai::bindings::Transform::children);
  const std::array payload{sai::node{}, child.dynamic(), sai::node{}, sai::node{}};
  REQUIRE(edit.set(first_children, std::span<const sai::node>{payload}));
  REQUIRE(edit.set(second_children, std::span<const sai::node>{payload}));
  REQUIRE(edit.append_root(first.dynamic()));
  REQUIRE(edit.append_root(second.dynamic()));
  REQUIRE(edit.validate());
  REQUIRE(edit.commit());
  const sai::node_list expected{sai::node_id{}, child.id(), sai::node_id{}, sai::node_id{}};
  CHECK(context.snapshot().read(first_children).value() == expected);
  CHECK(context.snapshot().read(second_children).value() == expected);
  auto events = context.events({1});
  REQUIRE(events.send(first_children, std::span<const sai::node>{payload}));
  REQUIRE(events.commit());
  CHECK(context.snapshot().read(first_children).value() == expected);

  const auto before = context.snapshot();
  auto cycle = context.edit();
  REQUIRE(cycle.append(child.dynamic(), "children", first.dynamic()));
  const auto rejected = cycle.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::containment_cycle);
  CHECK(context.snapshot().revision() == before.revision());
}

TEST_CASE("generated grouping uniqueness rejects direct and runtime node events") {
  for (bool runtime : {false, true}) {
    for (bool typed : {false, true}) {
      CAPTURE(runtime);
      CAPTURE(typed);
      auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
      REQUIRE(registry);
      sai::browser host{std::move(*registry)};
      auto context = host.current_scene();
      auto setup = context.edit();
      const auto parent = setup.create<sai::bindings::Transform>().value();
      const auto child = setup.create<sai::bindings::Transform>().value();
      const auto children = parent.field(sai::bindings::Transform::children);
      const auto guard = parent.field(sai::bindings::Transform::translation);
      REQUIRE(setup.commit());
      const auto before = context.snapshot();
      const std::array duplicate{child.dynamic(), sai::node{}, child.dynamic()};
      int notifications = 0;
      auto observer = context.observe([&](const auto &) { ++notifications; });
      if (runtime) {
        REQUIRE(host.set_runtime_processor(context, [&](sai::runtime_step &step) {
          if (step.input())
            return sai::result<void>{};
          REQUIRE(step.emit(guard, sai::vec3f{1, 2, 3}));
          // The frame must still be poisoned even if a processor ignores it.
          const auto rejected = typed
              ? step.emit(children, std::span<const sai::node>{duplicate})
              : step.emit(children.dynamic(), std::span<const sai::node>{duplicate});
          REQUIRE_FALSE(rejected);
          CHECK(rejected.error().code == sai::error_code::invalid_value);
          return sai::result<void>{};
        }));
      }
      auto events = context.events({1});
      if (!runtime) {
        REQUIRE(events.send(guard, sai::vec3f{1, 2, 3}));
        const auto rejected = typed
            ? events.send(children, std::span<const sai::node>{duplicate})
            : events.send(children.dynamic(), std::span<const sai::node>{duplicate});
        REQUIRE_FALSE(rejected);
        CHECK(rejected.error().code == sai::error_code::invalid_value);
      }
      CHECK_FALSE(events.commit());
      CHECK(context.snapshot().revision() == before.revision());
      CHECK(context.snapshot().read(children).value().empty());
      CHECK(context.snapshot().read(guard).value() == sai::vec3f{});
      CHECK(context.drain().delivered == 0);
      CHECK(notifications == 0);
    }
  }
}

TEST_CASE("generated grouping uniqueness rejects a permissive routed source") {
  for (bool runtime : {false, true}) {
    auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
    REQUIRE(registry);
    sai::node_type_descriptor source_type;
    source_type.name = "Source";
    sai::field_descriptor source_field;
    source_field.name = "nodes";
    source_field.kind = sai::value_kind::mf_node;
    source_field.default_value = sai::node_list{};
    source_type.fields.push_back(source_field);
    REQUIRE(registry->define(source_type));
    sai::browser host{std::move(*registry)};
    auto context = host.current_scene();
    auto setup = context.edit();
    const auto source = setup.create_node("Source").value();
    const auto parent = setup.create_node("Transform").value();
    const auto child = setup.create_node("Transform").value();
    const auto output = setup.field(source, "nodes").value();
    const auto children = setup.field(parent, "children").value();
    REQUIRE(setup.add_route(output, children));
    REQUIRE(setup.commit());
    const auto before = context.snapshot();
    const std::array duplicate{child, sai::node{}, child};
    int notifications = 0;
    auto observer = context.observe([&](const auto &) { ++notifications; });
    if (runtime) {
      REQUIRE(host.set_runtime_processor(context, [&](sai::runtime_step &step) {
        if (step.input())
          return sai::result<void>{};
        return step.emit(output, std::span<const sai::node>{duplicate});
      }));
    }
    auto events = context.events({1});
    if (!runtime)
      REQUIRE(events.send(output, std::span<const sai::node>{duplicate}));
    const auto rejected = events.commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_value);
    CHECK(rejected.error().node == parent.id());
    CHECK(rejected.error().field == "children");
    CHECK(context.snapshot().revision() == before.revision());
    CHECK(context.snapshot().read(output).value() == sai::value{sai::node_list{}});
    CHECK(context.snapshot().read(children).value() == sai::value{sai::node_list{}});
    CHECK(context.drain().delivered == 0);
    CHECK(notifications == 0);
  }
}
