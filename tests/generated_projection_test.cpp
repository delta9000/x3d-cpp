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
