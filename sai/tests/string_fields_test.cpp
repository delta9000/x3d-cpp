#include "x3d/sai/experimental/bindings/ArcClose2D.hpp"
#include "x3d/sai/experimental/bindings/CollisionCollection.hpp"
#include "x3d/sai/experimental/bindings/Fog.hpp"
#include "x3d/sai/experimental/bindings/FontStyle.hpp"
#include "x3d/sai/experimental/bindings/Layout.hpp"
#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include "doctest/doctest.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;
namespace bindings = sai::bindings;

TEST_CASE("bounded string fields support ordinary typed and dynamic values") {
  static_assert(bindings::Fog::fogType.kind == sai::value_kind::sf_string);
  static_assert(bindings::FontStyle::justify.kind ==
                sai::value_kind::mf_string);
  static_assert(bindings::ArcClose2D::closureType.kind ==
                sai::value_kind::sf_string);
  static_assert(bindings::Layout::align.kind == sai::value_kind::mf_string);
  static_assert(bindings::CollisionCollection::appliedParameters.kind ==
                sai::value_kind::mf_string);
  auto registry = sai::generated_type_registry_for<
      bindings::Fog, bindings::FontStyle, bindings::Layout,
      bindings::CollisionCollection, bindings::ArcClose2D>();
  REQUIRE(registry);
  sai::browser host{std::move(*registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto fog = setup.create<bindings::Fog>().value();
  const auto font = setup.create<bindings::FontStyle>().value();
  const auto layout = setup.create<bindings::Layout>().value();
  const auto collision = setup.create<bindings::CollisionCollection>().value();
  const auto arc = setup.create<bindings::ArcClose2D>().value();
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  const auto fog_type = fog.field(bindings::Fog::fogType);
  const auto justify = font.field(bindings::FontStyle::justify);
  const auto align = layout.field(bindings::Layout::align);
  const auto parameters =
      collision.field(bindings::CollisionCollection::appliedParameters);
  const auto closure = arc.field(bindings::ArcClose2D::closureType);
  CHECK(before.read(fog_type).value() == "LINEAR");
  CHECK(before.read(justify).value() == sai::string_list{"BEGIN"});
  CHECK(before.read(align).value() == sai::string_list{"CENTER", "CENTER"});
  CHECK(before.read(parameters).value() == sai::string_list{"BOUNCE"});
  CHECK(before.read(closure).value() == "PIE");
  const auto dynamic_fog = before.field(fog.dynamic(), "fogType").value();
  const auto dynamic_font = before.field(font.dynamic(), "justify").value();
  REQUIRE(dynamic_fog.as<std::string>());
  REQUIRE(dynamic_font.as<sai::string_list>());
  CHECK(dynamic_fog.kind() == sai::value_kind::sf_string);
  CHECK(dynamic_font.kind() == sai::value_kind::mf_string);
  REQUIRE_FALSE(dynamic_fog.as<float>());
  REQUIRE_FALSE(dynamic_font.as<std::string>());

  auto edit = scene.edit();
  REQUIRE(edit.set(fog_type, std::string{"EXPONENTIAL"}));
  REQUIRE(edit.set(closure, std::string{"CHORD"}));
  auto typed_multi = edit.multi(justify).value();
  REQUIRE(typed_multi.set(0, "END"));
  REQUIRE(typed_multi.append("MIDDLE"));
  auto dynamic_multi = edit.multi(dynamic_font).value();
  CHECK(std::get<std::string>(dynamic_multi.at(0).value()) == "END");
  REQUIRE(dynamic_multi.erase(1));
  REQUIRE(dynamic_multi.insert(1, sai::value{std::string{"FIRST"}}));
  // Normative values omitted from UOM choice lists must remain representable.
  REQUIRE(edit.set(align, sai::string_list{"CENTER"}));
  REQUIRE(edit.set(font.field(bindings::FontStyle::style), std::string{}));
  REQUIRE(edit.set(parameters, sai::string_list{"BOUNCE", "SLIP_1"}));
  REQUIRE(edit.validate());
  REQUIRE(edit.commit());
  const auto after = scene.snapshot();
  CHECK(after.read(fog_type).value() == "EXPONENTIAL");
  CHECK(std::get<std::string>(after.read(dynamic_fog).value()) ==
        "EXPONENTIAL");
  CHECK(after.read(justify).value() == sai::string_list{"END", "FIRST"});
  CHECK(std::get<sai::string_list>(after.read(dynamic_font).value()) ==
        sai::string_list{"END", "FIRST"});
  CHECK(after.read(align).value() == sai::string_list{"CENTER"});
  CHECK(after.read(font.field(bindings::FontStyle::style))->empty());
  CHECK(after.read(parameters).value() == sai::string_list{"BOUNCE", "SLIP_1"});
  CHECK(after.read(closure).value() == "CHORD");
  CHECK(before.read(fog_type).value() == "LINEAR");
  CHECK(before.read(justify).value() == sai::string_list{"BEGIN"});
}

TEST_CASE(
    "bounded string fields exchange ISO payloads through events and routes") {
  constexpr std::array<std::string_view, 2> names{"Fog", "FontStyle"};
  auto generated = sai::generated_type_registry(names);
  REQUIRE(generated);
  sai::type_registry registry;
  for (const auto &descriptor : generated->node_types())
    REQUIRE(registry.define(descriptor));
  sai::node_type_descriptor source_type;
  source_type.name = "StringSource";
  sai::field_descriptor scalar;
  scalar.name = "text";
  scalar.kind = sai::value_kind::sf_string;
  scalar.default_value = std::string{"LINEAR"};
  source_type.fields.push_back(std::move(scalar));
  sai::field_descriptor sequence;
  sequence.name = "texts";
  sequence.kind = sai::value_kind::mf_string;
  sequence.default_value = sai::string_list{"BEGIN"};
  source_type.fields.push_back(std::move(sequence));
  REQUIRE(registry.define(std::move(source_type)));
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto source = setup.create_node("StringSource").value();
  const auto fog = setup.create_node("Fog").value();
  const auto font = setup.create_node("FontStyle").value();
  const auto text = setup.field(source, "text").value();
  const auto texts = setup.field(source, "texts").value();
  const auto fog_type = setup.field(fog, "fogType").value();
  const auto justify = setup.field(font, "justify").value();
  REQUIRE(setup.add_route(text, fog_type));
  REQUIRE(setup.add_route(texts, justify));
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  std::vector<sai::value> observed;
  auto scalar_observer =
      scene.observe(fog_type, [&](const sai::event_delivery &event) {
        CHECK(event.origin == sai::event_origin::route);
        observed.push_back(event.payload);
        CHECK(std::get<std::string>(scene.snapshot().read(fog_type).value()) ==
              std::get<std::string>(event.payload));
      });
  auto sequence_observer =
      scene.observe(justify, [&](const sai::event_delivery &event) {
        CHECK(event.origin == sai::event_origin::route);
        observed.push_back(event.payload);
        CHECK(std::get<sai::string_list>(
                  scene.snapshot().read(justify).value()) ==
              std::get<sai::string_list>(event.payload));
      });
  REQUIRE(scalar_observer);
  REQUIRE(sequence_observer);
  auto events = scene.events({1});
  REQUIRE(
      events.send(text.as<std::string>().value(), std::string{"EXPONENTIAL"}));
  REQUIRE(events.send(texts, sai::value{sai::string_list{"MIDDLE", "BEGIN"}}));
  const auto routed = events.commit();
  REQUIRE(routed);
  REQUIRE(routed->deliveries.size() == 4);
  CHECK(observed.empty());
  CHECK(std::get<std::string>(scene.snapshot().read(fog_type).value()) ==
        "EXPONENTIAL");
  CHECK(std::get<sai::string_list>(scene.snapshot().read(justify).value()) ==
        sai::string_list{"MIDDLE", "BEGIN"});
  CHECK(scene.drain().delivered == 2);
  CHECK(observed == std::vector<sai::value>{
                        sai::value{std::string{"EXPONENTIAL"}},
                        sai::value{sai::string_list{"MIDDLE", "BEGIN"}}});
  observed.clear();

  REQUIRE(host.set_runtime_processor(
      scene, [&](sai::runtime_step &step) -> sai::result<void> {
        if (step.input())
          return {};
        CHECK(step.read(fog_type.as<std::string>().value()).value() ==
              "EXPONENTIAL");
        CHECK(step.read(justify.as<sai::string_list>().value()).value() ==
              sai::string_list{"MIDDLE", "BEGIN"});
        auto emitted = step.emit(text, sai::value{std::string{"LINEAR"}});
        if (!emitted)
          return emitted;
        return step.emit(texts.as<sai::string_list>().value(),
                         sai::string_list{"END", "FIRST"});
      }));
  const auto native = scene.events({2}).commit();
  REQUIRE(native);
  REQUIRE(native->deliveries.size() == 4);
  CHECK(std::get<std::string>(scene.snapshot().read(fog_type).value()) ==
        "LINEAR");
  CHECK(std::get<sai::string_list>(scene.snapshot().read(justify).value()) ==
        sai::string_list{"END", "FIRST"});
  CHECK(scene.drain().delivered == 2);
  CHECK(observed ==
        std::vector<sai::value>{sai::value{std::string{"LINEAR"}},
                                sai::value{sai::string_list{"END", "FIRST"}}});
  CHECK(std::get<std::string>(before.read(fog_type).value()) == "LINEAR");
  CHECK(std::get<sai::string_list>(before.read(justify).value()) ==
        sai::string_list{"BEGIN"});
}
