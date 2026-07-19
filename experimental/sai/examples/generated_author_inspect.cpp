#include "x3d/sai/experimental/metadata.hpp"

#include "x3d/sai/experimental/bindings/Appearance.hpp"
#include "x3d/sai/experimental/bindings/Coordinate.hpp"
#include "x3d/sai/experimental/bindings/PixelTexture.hpp"
#include "x3d/sai/experimental/bindings/PointSet.hpp"
#include "x3d/sai/experimental/bindings/Shape.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include "x3d/sai/experimental/bindings/WorldInfo.hpp"

#include <array>
#include <cstdlib>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;
namespace bindings = x3d::sai::experimental::bindings;

template <class T> bool holds(sai::result<T> actual, const T &expected) {
  return actual && *actual == expected;
}

int main() {
  const std::array<std::string_view, 7> selected{
      "Appearance", "Coordinate", "PixelTexture", "PointSet",
      "Shape",      "Transform",  "WorldInfo"};
  auto registry = sai::generated_type_registry(selected);
  if (!registry)
    return EXIT_FAILURE;

  sai::browser browser{std::move(registry).value()};
  auto scene = browser.current_scene();
  auto edit = scene.edit();
  if (!edit.declare_unit(sai::unit_declaration{.category = "length",
                                               .name = "centimetre",
                                               .conversion_factor = 0.01}))
    return EXIT_FAILURE;

  const sai::vec3f authored_translation{1, 2, 3};
  auto transform = edit.create<bindings::Transform>().and_then(
      [&](sai::typed_node<bindings::Transform> created)
          -> sai::result<sai::typed_node<bindings::Transform>> {
        return edit
            .set(created.field(bindings::Transform::translation),
                 authored_translation)
            .transform([created = std::move(created)]() mutable {
              return std::move(created);
            });
      });
  auto shape = edit.create<bindings::Shape>();
  auto point_set = edit.create<bindings::PointSet>();
  auto coordinate = edit.create<bindings::Coordinate>();
  auto appearance = edit.create<bindings::Appearance>();
  auto texture = edit.create<bindings::PixelTexture>();
  auto world_info = edit.create<bindings::WorldInfo>();
  if (!transform || !shape || !point_set || !coordinate || !appearance ||
      !texture || !world_info)
    return EXIT_FAILURE;

  const sai::vec3f_list authored_points{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  const sai::image authored_image{1, 1, 4, {0x20, 0x40, 0x80, 0xff}};
  const sai::string_list authored_info{"generated bindings",
                                       "one semantic model"};
  const auto translation = transform->field(bindings::Transform::translation);
  const auto children = transform->field(bindings::Transform::children);
  const auto geometry = shape->field(bindings::Shape::geometry);
  const auto shape_appearance = shape->field(bindings::Shape::appearance);
  const auto coord = point_set->field(bindings::PointSet::coord);
  const auto point = coordinate->field(bindings::Coordinate::point);
  const auto appearance_texture =
      appearance->field(bindings::Appearance::texture);
  const auto image = texture->field(bindings::PixelTexture::image);
  const auto title = world_info->field(bindings::WorldInfo::title);
  const auto info = world_info->field(bindings::WorldInfo::info);
  const std::array<sai::node, 1> child_nodes{shape->dynamic()};

  if (!edit.set(children, std::span<const sai::node>{child_nodes}) ||
      !edit.set(geometry, point_set->dynamic()) ||
      !edit.set(shape_appearance, appearance->dynamic()) ||
      !edit.set(coord, coordinate->dynamic()) ||
      !edit.set(point, authored_points) ||
      !edit.set(appearance_texture, texture->dynamic()) ||
      !edit.set(image, authored_image) ||
      !edit.set(title, std::string{"Generated authoring"}) ||
      !edit.set(info, authored_info) ||
      !edit.append_root(transform->dynamic()) ||
      !edit.append_root(world_info->dynamic()) || !edit.commit())
    return EXIT_FAILURE;

  const auto snapshot = scene.snapshot();
  const std::vector<sai::unit_declaration> expected_units{
      {.category = "length", .name = "centimetre", .conversion_factor = 0.01}};
  if (snapshot.units() != expected_units ||
      !holds(snapshot.read(translation), authored_translation) ||
      !holds(snapshot.read(children), sai::node_list{shape->id()}) ||
      !holds(snapshot.read(geometry), point_set->id()) ||
      !holds(snapshot.read(point), authored_points) ||
      !holds(snapshot.read(image), authored_image) ||
      !holds(snapshot.read(title), std::string{"Generated authoring"}) ||
      !holds(snapshot.read(info), authored_info))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
