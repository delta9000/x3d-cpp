#include "x3d/sai/experimental/image.hpp"
#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace sai = x3d::sai::experimental;

namespace {
sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "Images";
  sai::field_descriptor field;
  field.name = "single";
  field.kind = sai::value_kind::sf_image;
  field.access = sai::access_type::input_output;
  field.default_value = sai::image{};
  type.fields.push_back(field);
  field.name = "many";
  field.kind = sai::value_kind::mf_image;
  field.default_value = sai::image_list{{}};
  type.fields.push_back(field);
  field.name = "guard";
  field.kind = sai::value_kind::sf_int32;
  field.default_value = std::int32_t{};
  type.fields.push_back(field);
  REQUIRE(types.define(std::move(type)));
  return types;
}
} // namespace

TEST_CASE("nonempty images with zero components cannot become node descriptor "
          "defaults") {
  for (bool many : {false, true}) {
    sai::type_registry types;
    sai::node_type_descriptor type;
    type.name = "InvalidImage";
    sai::field_descriptor field;
    field.name = "value";
    field.kind = many ? sai::value_kind::mf_image : sai::value_kind::sf_image;
    field.default_value = many ? sai::value{sai::image_list{{}, {1, 1, 0, {}}}}
                               : sai::value{sai::image{1, 1, 0, {}}};
    type.fields.push_back(std::move(field));
    const auto invalid = types.define(std::move(type));
    REQUIRE_FALSE(invalid);
    CHECK(invalid.error().code == sai::error_code::invalid_descriptor);
    CHECK(types.find("InvalidImage") == nullptr);
  }
}

TEST_CASE("zero-component image writes fail atomically for scalar MF and "
          "indexed paths") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto owner = setup.create_node("Images").value();
  REQUIRE(setup.append_root(owner));
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  const auto single = before.field(owner, "single").value();
  const auto many = before.field(owner, "many").value();
  const auto guard = before.field(owner, "guard").value();
  int calls = 0;
  auto observer = scene.observe([&](const sai::change_set &) { ++calls; });
  for (const auto invalid :
       {sai::image{1, 1, 0, {}}, sai::image{2, 3, 0, {}},
        sai::image{std::numeric_limits<std::int32_t>::max(),
                   std::numeric_limits<std::int32_t>::max(),
                   0,
                   {}}}) {
    CHECK_FALSE(sai::image_pixels(invalid));
    for (int path = 0; path < 4; ++path) {
      auto edit = scene.edit();
      REQUIRE(edit.set(guard, sai::value{std::int32_t{9}}));
      const auto rejected = [&]() -> sai::result<void> {
        if (path == 0)
          return edit.set(single, sai::value{invalid});
        if (path == 1)
          return edit.set(many, sai::value{sai::image_list{{}, invalid}});
        auto values = edit.multi(many).value();
        if (path == 2)
          return values.set(0, sai::value{invalid});
        return values.append(sai::value{invalid});
      }();
      REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code == sai::error_code::invalid_value);
      REQUIRE_FALSE(edit.commit());
      CHECK(scene.snapshot().revision() == before.revision());
      CHECK(scene.snapshot().read(guard).value() ==
            sai::value{std::int32_t{0}});
      CHECK(scene.snapshot().read(single).value() ==
            before.read(single).value());
      CHECK(scene.snapshot().read(many).value() == before.read(many).value());
    }
    for (bool sequence : {false, true}) {
      auto events = scene.events({2});
      REQUIRE(events.send(guard, sai::value{std::int32_t{9}}));
      const auto rejected =
          sequence ? events.send(many, sai::value{sai::image_list{{}, invalid}})
                   : events.send(single, sai::value{invalid});
      REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code == sai::error_code::invalid_value);
      REQUIRE_FALSE(events.commit());
      CHECK(scene.snapshot().revision() == before.revision());
      CHECK(scene.snapshot().read(guard).value() ==
            sai::value{std::int32_t{0}});
    }
  }
  CHECK(scene.drain().delivered == 0);
  CHECK(calls == 0);
}

TEST_CASE("empty image dimension conventions stay compatible across helpers "
          "and kernel") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto owner = setup.create_node("Images").value();
  REQUIRE(setup.commit());
  const auto single = scene.snapshot().field(owner, "single").value();
  const auto many = scene.snapshot().field(owner, "many").value();
  for (const auto &empty :
       {sai::image{}, sai::image{0, 3, 0, {}}, sai::image{2, 0, 0, {}},
        sai::image{0, 3, 4, {}}, sai::image{2, 0, 4, {}}}) {
    REQUIRE(sai::image_pixels(empty));
    CHECK(sai::image_pixels(empty)->empty());
    auto edit = scene.edit();
    REQUIRE(edit.set(single, sai::value{empty}));
    REQUIRE(edit.set(many, sai::value{sai::image_list{empty}}));
    REQUIRE(edit.commit());
    CHECK(scene.snapshot().read(single).value() == sai::value{empty});
    CHECK(scene.snapshot().read(many).value() ==
          sai::value{sai::image_list{empty}});
  }
  for (std::int32_t components = 1; components <= 4; ++components) {
    const sai::image pixel{1, 1, components,
                           std::vector<std::uint8_t>(components, 0x7f)};
    REQUIRE(sai::image_pixels(pixel));
    auto edit = scene.edit();
    REQUIRE(edit.set(single, sai::value{pixel}));
    REQUIRE(edit.commit());
    CHECK(scene.snapshot().read(single).value() == sai::value{pixel});
  }
}
