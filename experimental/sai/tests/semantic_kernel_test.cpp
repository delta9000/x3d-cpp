#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include "x3d/sai/experimental/X3DSAIBindingCatalog.hpp"
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/bindings/Coordinate.hpp"
#include "x3d/sai/experimental/bindings/Group.hpp"
#include "x3d/sai/experimental/bindings/PixelTexture.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include "x3d/sai/experimental/bindings/WorldInfo.hpp"

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"

#include "doctest/doctest.h"

#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

namespace sai = x3d::sai::experimental;

namespace {

const sai::graph_change &
require_graph_change(const sai::semantic_change &candidate) {
  const auto *change = sai::node_change(candidate);
  REQUIRE(change != nullptr);
  return *change;
}

struct copy_bomb_observer {
  std::shared_ptr<bool> armed;
  int *calls = nullptr;

  copy_bomb_observer(std::shared_ptr<bool> armed_value, int &call_count)
      : armed(std::move(armed_value)), calls(&call_count) {}
  copy_bomb_observer(const copy_bomb_observer &other)
      : armed(other.armed), calls(other.calls) {
    if (*armed)
      throw std::runtime_error("callback copied during dispatch");
  }
  copy_bomb_observer(copy_bomb_observer &&) noexcept = default;
  void operator()(const sai::change_set &) const { ++*calls; }
};

template <class Edit>
concept accepts_imported_containment = requires(
    Edit &edit, const sai::node &parent, const sai::imported_node &child) {
  edit.append(parent, "children", child);
};

template <class Edit>
concept accepts_imported_retained_write =
    requires(Edit &edit, const sai::dynamic_field &target,
             const sai::imported_node &value) { edit.set(target, value); };

template <class Batch>
concept accepts_imported_event_target =
    requires(Batch &batch, const sai::dynamic_imported_field &target) {
      batch.send(target, sai::value{sai::vec3f{}});
    };

sai::field_descriptor test_field(std::string name, sai::value_kind kind,
                                 sai::access_type access,
                                 sai::value default_value,
                                 bool containment = false) {
  return sai::field_descriptor{
      .name = std::move(name),
      .kind = kind,
      .access = access,
      .default_value = std::move(default_value),
      .default_origin = sai::default_source::field_type,
      .containment = containment,
      .accepted_node_types = {},
      .unit_category = std::nullopt,
  };
}

sai::interface_field_descriptor
test_interface_field(std::string name,
                     sai::value_kind kind = sai::value_kind::sf_string,
                     sai::access_type access = sai::access_type::input_output,
                     std::optional<sai::value> default_value = std::nullopt,
                     std::vector<std::string> accepted_node_types = {},
                     std::optional<std::string> unit_category = std::nullopt) {
  return sai::interface_field_descriptor{
      .name = std::move(name),
      .kind = kind,
      .access = access,
      .default_value = std::move(default_value),
      .accepted_node_types = std::move(accepted_node_types),
      .unit_category = std::move(unit_category),
  };
}

sai::local_declaration_descriptor test_local_declaration(
    std::string name,
    std::vector<sai::interface_field_descriptor> interface = {}) {
  return sai::local_declaration_descriptor{
      .name = std::move(name),
      .interface = std::move(interface),
      .body_roots = {},
      .appinfo = {},
      .documentation = {},
  };
}

sai::external_declaration_descriptor test_external_declaration(
    std::string name, std::vector<std::string> urls,
    std::vector<sai::interface_field_descriptor> interface = {}) {
  return sai::external_declaration_descriptor{
      .name = std::move(name),
      .interface = std::move(interface),
      .urls = std::move(urls),
      .load_state = sai::external_load_state::unresolved,
      .diagnostic = {},
      .resolved_declaration = std::nullopt,
      .appinfo = {},
      .documentation = {},
  };
}

sai::node_type_descriptor
test_node_type(std::string name, std::vector<sai::field_descriptor> fields) {
  return sai::node_type_descriptor{
      .name = std::move(name),
      .fields = std::move(fields),
      .component = {},
      .component_level = 0,
      .interfaces = {},
      .abstract = false,
  };
}

template <class T>
void check_owning_value_roundtrip(std::string type_name, sai::value_kind kind,
                                  T sample) {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      type_name,
      {test_field("value", kind, sai::access_type::input_output, T{})})));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto owner = edit.create_node(type_name);
  REQUIRE(owner);
  auto dynamic = edit.field(owner.value(), "value");
  REQUIRE(dynamic);
  auto typed = dynamic.value().template as<T>();
  REQUIRE(typed);
  REQUIRE(edit.set(typed.value(), sample));
  REQUIRE(edit.commit());

  auto snapshot = context.snapshot();
  auto typed_value = snapshot.read(typed.value());
  REQUIRE(typed_value);
  CHECK(typed_value.value() == sample);
  auto dynamic_value = snapshot.read(dynamic.value());
  REQUIRE(dynamic_value);
  CHECK(std::holds_alternative<T>(dynamic_value.value()));
  CHECK(std::get<T>(dynamic_value.value()) == sample);
}

sai::type_registry graph_registry() {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "Group",
      {test_field("children", sai::value_kind::mf_node,
                  sai::access_type::input_output, sai::node_list{}, true)})));
  REQUIRE(registry.define(test_node_type(
      "Transform",
      {
          test_field("translation", sai::value_kind::sf_vec3f,
                     sai::access_type::input_output, sai::vec3f{}),
          test_field("children", sai::value_kind::mf_node,
                     sai::access_type::input_output, sai::node_list{}, true),
          test_field("worldTranslation", sai::value_kind::sf_vec3f,
                     sai::access_type::output_only, sai::vec3f{}),
          test_field("initialLabel", sai::value_kind::sf_string,
                     sai::access_type::initialize_only, std::string{}),
          test_field("set_translation", sai::value_kind::sf_vec3f,
                     sai::access_type::input_only, sai::vec3f{}),
      })));
  REQUIRE(registry.define(test_node_type(
      "AccessNode",
      {
          test_field("initialChildren", sai::value_kind::mf_node,
                     sai::access_type::initialize_only, sai::node_list{}, true),
          test_field("receivedChildren", sai::value_kind::mf_node,
                     sai::access_type::input_only, sai::node_list{}, true),
          test_field("observedChildren", sai::value_kind::mf_node,
                     sai::access_type::output_only, sai::node_list{}, true),
      })));
  REQUIRE(registry.define(test_node_type(
      "Link", {
                  test_field("target", sai::value_kind::sf_node,
                             sai::access_type::input_output, sai::node_id{}),
                  test_field("targets", sai::value_kind::mf_node,
                             sai::access_type::input_output, sai::node_list{}),
              })));
  return registry;
}

struct BoundTransform {
  static constexpr std::string_view x3d_name = "Transform";
  inline static constexpr sai::field_key<BoundTransform, sai::vec3f>
      translation{"translation", sai::access_type::input_output};
};

struct BoundGroup {
  static constexpr std::string_view x3d_name = "Group";
  inline static constexpr sai::field_key<BoundGroup, sai::node_list> children{
      "children", sai::access_type::input_output};
};

template <class Node, class Key>
concept accepts_generated_key =
    requires(const Node &node, const Key &key) { node.field(key); };

} // namespace

TEST_CASE("experimental SAI starts as an empty descriptor-governed context") {
  sai::type_registry registry;
  auto defined = registry.define(test_node_type(
      "Group",
      {test_field("children", sai::value_kind::mf_node,
                  sai::access_type::input_output, sai::node_list{}, true)}));
  REQUIRE(defined);

  auto duplicate = registry.define(test_node_type("Group", {}));
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::duplicate_type);
  CHECK(duplicate.error().operation == "type_registry.define");

  sai::browser host{std::move(registry)};
  sai::execution_context context = host.current_scene();
  sai::scene_snapshot snapshot = context.snapshot();

  CHECK(context.generation() == 1);
  CHECK(snapshot.revision() == 0);
  CHECK(snapshot.roots().empty());
  CHECK(host.capabilities().authoring);
  CHECK(host.capabilities().inspection);
  CHECK_FALSE(host.capabilities().live);
  CHECK_FALSE(host.capabilities().async_loading);
  CHECK_FALSE(host.capabilities().rendering);
}

TEST_CASE("SAI results compose through expected-style monadic operations") {
  const auto composed =
      sai::result<int>{2}
          .and_then([](int value) -> sai::result<std::string> {
            return std::to_string(value * 3);
          })
          .transform([](const std::string &value) { return value + " units"; });
  REQUIRE(composed);
  CHECK(composed.value() == "6 units");

  int skipped = 0;
  const sai::sai_error failure{.code = sai::error_code::invalid_value,
                               .operation = "compose",
                               .message = "bad value",
                               .node = std::nullopt,
                               .declaration = std::nullopt,
                               .field = {}};
  const auto recovered =
      sai::result<int>{sai::unexpected{failure}}
          .and_then([&](int) -> sai::result<int> {
            ++skipped;
            return 0;
          })
          .or_else([](const sai::sai_error &error) -> sai::result<int> {
            CHECK(error.operation == "compose");
            return 7;
          });
  CHECK(skipped == 0);
  REQUIRE(recovered);
  CHECK(recovered.value() == 7);

  const auto translated =
      sai::result<int>{sai::unexpected{failure}}.transform_error(
          [](sai::sai_error error) {
            error.operation = "translated";
            return error;
          });
  REQUIRE_FALSE(translated);
  CHECK(translated.error().operation == "translated");

  const auto error_as_value = []() -> sai::result<sai::sai_error> {
    return sai::sai_error{.code = sai::error_code::cancelled,
                          .operation = "value",
                          .message = "an error-shaped success",
                          .node = std::nullopt,
                          .declaration = std::nullopt,
                          .field = {}};
  }();
  REQUIRE(error_as_value);
  CHECK(error_as_value.value().operation == "value");
  const sai::result<sai::sai_error> error_as_failure{sai::failure(failure)};
  REQUIRE_FALSE(error_as_failure);
  CHECK(error_as_failure.error().operation == "compose");

  const auto void_chain =
      sai::result<void>{}.and_then([]() -> sai::result<int> { return 11; });
  REQUIRE(void_chain);
  CHECK(void_chain.value() == 11);
}

TEST_CASE("generated field keys remove dynamic authoring lookup") {
  static_assert(accepts_generated_key<sai::typed_node<BoundTransform>,
                                      decltype(BoundTransform::translation)>);
  static_assert(!accepts_generated_key<sai::typed_node<BoundGroup>,
                                       decltype(BoundTransform::translation)>);

  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<BoundTransform>();
  REQUIRE(transform);
  auto translation = transform.value().field(BoundTransform::translation);
  static_assert(std::same_as<decltype(translation), sai::field<sai::vec3f>>);
  REQUIRE(edit.set(translation, sai::vec3f{1, 2, 3}));
  REQUIRE(edit.commit());

  const auto snapshot = context.snapshot();
  CHECK(snapshot.read(translation).value() == sai::vec3f{1, 2, 3});
  auto dynamic = snapshot.field(transform.value().dynamic(), "translation");
  REQUIRE(dynamic);
  CHECK(dynamic.value().node() == translation.node());
  CHECK(snapshot.read(dynamic.value()).value() ==
        sai::value{sai::vec3f{1, 2, 3}});

  auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  auto stale_edit = context.edit();
  REQUIRE(stale_edit.set(translation, sai::vec3f{4, 5, 6}));
  auto stale = stale_edit.commit();
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_handle);
}

TEST_CASE("generated typed keys and metadata are one schema view") {
  namespace bindings = sai::bindings;
  static_assert(bindings::Transform::schema_fingerprint ==
                bindings::model_fingerprint);
  static_assert(
      accepts_generated_key<sai::typed_node<bindings::Transform>,
                            decltype(bindings::Transform::translation)>);
  static_assert(
      !accepts_generated_key<sai::typed_node<bindings::Group>,
                             decltype(bindings::Transform::translation)>);

  const auto catalog = sai::generated_metadata_catalog();
  const auto check_key = [&](std::string_view node_name, const auto &key) {
    const auto node = std::ranges::find(catalog.nodes, node_name,
                                        &sai::schema_node_descriptor::name);
    REQUIRE(node != catalog.nodes.end());
    const auto field = std::ranges::find(node->fields, key.name(),
                                         &sai::schema_field_descriptor::name);
    REQUIRE(field != node->fields.end());
    using key_type = std::remove_cvref_t<decltype(key)>;
    CHECK(field->type == key_type::kind);
    CHECK(field->access == key.access());
  };
  check_key("Transform", bindings::Transform::translation);
  check_key("Group", bindings::Group::children);
  check_key("PixelTexture", bindings::PixelTexture::image);
  check_key("Coordinate", bindings::Coordinate::point);
  check_key("WorldInfo", bindings::WorldInfo::info);

  const std::array<std::string_view, 5> selected{
      "Transform", "Group", "PixelTexture", "Coordinate", "WorldInfo"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);
  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<bindings::Transform>();
  auto group = edit.create<bindings::Group>();
  auto texture = edit.create<bindings::PixelTexture>();
  auto coordinate = edit.create<bindings::Coordinate>();
  auto world_info = edit.create<bindings::WorldInfo>();
  REQUIRE(transform);
  REQUIRE(group);
  REQUIRE(texture);
  REQUIRE(coordinate);
  REQUIRE(world_info);

  const sai::vec3f translation_value{1, 2, 3};
  const sai::node_list children_value{transform.value().id()};
  const std::array<sai::node, 1> children_nodes{transform.value().dynamic()};
  const sai::image image_value{1, 1, 3, {10, 20, 30}};
  const sai::vec3f_list point_value{{1, 2, 3}, {4, 5, 6}};
  const sai::string_list info_value{"generated", "typed"};
  const auto translation =
      transform.value().field(bindings::Transform::translation);
  const auto children = group.value().field(bindings::Group::children);
  const auto image = texture.value().field(bindings::PixelTexture::image);
  const auto point = coordinate.value().field(bindings::Coordinate::point);
  const auto info = world_info.value().field(bindings::WorldInfo::info);
  REQUIRE(edit.set(translation, translation_value));
  REQUIRE(edit.set(children, std::span<const sai::node>{children_nodes}));
  REQUIRE(edit.set(image, image_value));
  REQUIRE(edit.set(point, point_value));
  REQUIRE(edit.set(info, info_value));
  REQUIRE(edit.commit());

  const auto snapshot = context.snapshot();
  CHECK(snapshot.read(translation).value() == translation_value);
  CHECK(snapshot.read(children).value() == children_value);
  CHECK(snapshot.read(image).value() == image_value);
  CHECK(snapshot.read(point).value() == point_value);
  CHECK(snapshot.read(info).value() == info_value);
  const auto check_dynamic = [&](const sai::node &owner, std::string_view name,
                                 const sai::value &expected) {
    const auto field = snapshot.field(owner, std::string{name});
    REQUIRE(field);
    const auto value = snapshot.read(field.value());
    REQUIRE(value);
    CHECK(value.value() == expected);
  };
  check_dynamic(transform.value().dynamic(), "translation", translation_value);
  check_dynamic(group.value().dynamic(), "children", children_value);
  check_dynamic(texture.value().dynamic(), "image", image_value);
  check_dynamic(coordinate.value().dynamic(), "point", point_value);
  check_dynamic(world_info.value().dynamic(), "info", info_value);
}

TEST_CASE("generated typed creation rejects mismatched registry provenance") {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "Transform",
      {test_field("translation", sai::value_kind::sf_string,
                  sai::access_type::input_output, std::string{})})));
  sai::browser host{std::move(registry)};
  auto edit = host.current_scene().edit();

  auto transform = edit.create<sai::bindings::Transform>();
  REQUIRE_FALSE(transform);
  CHECK(transform.error().code == sai::error_code::invalid_descriptor);
  CHECK(transform.error().operation == "scene_edit.create_generated");
}

TEST_CASE("mutating a generated registry revokes generated provenance") {
  const std::array generated_types{std::string_view{"Group"}};
  auto registry = sai::generated_type_registry(generated_types);
  REQUIRE(registry);
  REQUIRE(registry->schema_fingerprint());
  REQUIRE(registry->define(test_node_type(
      "Transform",
      {test_field("translation", sai::value_kind::sf_string,
                  sai::access_type::input_output, std::string{})})));
  CHECK_FALSE(registry->schema_fingerprint());

  sai::browser host{std::move(*registry)};
  auto edit = host.current_scene().edit();
  const auto transform = edit.create<sai::bindings::Transform>();
  REQUIRE_FALSE(transform);
  CHECK(transform.error().code == sai::error_code::invalid_descriptor);
  CHECK(transform.error().operation == "scene_edit.create_generated");
}

TEST_CASE("generated key catalog has exhaustive ordered metadata parity") {
  const auto metadata = sai::generated_metadata_catalog();
  const auto &keys = sai::bindings::node_keys;
  REQUIRE(keys.size() == metadata.nodes.size());
  CHECK(sai::bindings::model_fingerprint == metadata.model_fingerprint);

  for (std::size_t node_index = 0; node_index < keys.size(); ++node_index) {
    const auto &key_node = keys[node_index];
    const auto &metadata_node = metadata.nodes[node_index];
    CHECK(key_node.name == metadata_node.name);
    CHECK(key_node.schema_fingerprint == metadata.model_fingerprint);
    REQUIRE(key_node.fields.size() == metadata_node.fields.size());
    for (std::size_t field_index = 0; field_index < key_node.fields.size();
         ++field_index) {
      const auto &key_field = key_node.fields[field_index];
      const auto &metadata_field = metadata_node.fields[field_index];
      CHECK(key_field.name == metadata_field.name);
      CHECK(key_field.kind == metadata_field.type);
      CHECK(key_field.access == metadata_field.access);
    }
  }
}

TEST_CASE("generated typed and dynamic failures have one vocabulary") {
  const std::array<std::string_view, 1> selected{"Transform"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);
  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<sai::bindings::Transform>();
  REQUIRE(transform);
  REQUIRE(edit.commit());

  const auto snapshot = context.snapshot();
  const auto typed = transform->field(sai::bindings::Transform::addChildren);
  const auto dynamic = snapshot.field(transform->dynamic(), "addChildren");
  REQUIRE(dynamic);
  const auto typed_read = snapshot.read(typed);
  const auto dynamic_read = snapshot.read(dynamic.value());
  REQUIRE_FALSE(typed_read);
  REQUIRE_FALSE(dynamic_read);
  CHECK(typed_read.error().code == dynamic_read.error().code);
  CHECK(typed_read.error().operation == dynamic_read.error().operation);
  CHECK(typed_read.error().message == dynamic_read.error().message);
  CHECK(typed_read.error().node == dynamic_read.error().node);
  CHECK(typed_read.error().field == dynamic_read.error().field);
}

TEST_CASE("generated metadata authors without runtime node identity") {
  const std::array<std::string_view, 1> selected{"Box"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto box = edit.create_node("Box");
  REQUIRE(box);
  REQUIRE(edit.append_root(box.value()));
  REQUIRE(edit.commit());

  const auto described = context.snapshot().describe(box.value());
  REQUIRE(described);
  CHECK(described.value().name == "Box");
  CHECK(described.value().component == "Geometry3D");
  CHECK(described.value().component_level == 1);
  REQUIRE(described.value().fields.size() == 9);
  CHECK(described.value().fields[0].name == "IS");
  CHECK(described.value().fields[0].accepted_node_types ==
        std::vector<std::string>{"IS"});
  CHECK_FALSE(described.value().fields[0].unit_category.has_value());
  CHECK(described.value().fields[0].default_origin ==
        sai::default_source::schema);
  CHECK(described.value().fields[2].name == "size");
  CHECK(described.value().fields[2].default_origin ==
        sai::default_source::schema);
  CHECK(described.value().fields[2].default_value ==
        sai::value{sai::vec3f{2, 2, 2}});
  CHECK(described.value().fields[4].name == "DEF");
  CHECK(described.value().fields[4].default_origin ==
        sai::default_source::field_type);
}

TEST_CASE(
    "generated metadata adapter has exhaustive ordered descriptor parity") {
  const auto source = x3d::nodes::X3DSemanticMetadataRegistry::nodes();
  const auto adapted = sai::generated_metadata_catalog();
  CHECK(adapted.specification_version == "4.0");
  CHECK(adapted.model_fingerprint ==
        x3d::nodes::X3DSemanticMetadataRegistry::modelFingerprint());
  CHECK(adapted.model_fingerprint.size() == 64);
  CHECK(adapted.generator_version ==
        x3d::nodes::X3DSemanticMetadataRegistry::generatorVersion());
  CHECK_FALSE(adapted.generator_version.empty());
  CHECK_FALSE(adapted.unit_categories_complete);
  REQUIRE(adapted.nodes.size() == source.size());

  for (std::size_t node_index = 0; node_index < source.size(); ++node_index) {
    const auto &source_node = source[node_index];
    const auto &adapted_node = adapted.nodes[node_index];
    CHECK(adapted_node.name == source_node.name);
    CHECK(adapted_node.abstract == source_node.abstract);
    CHECK(adapted_node.component == source_node.component);
    CHECK(adapted_node.component_level == source_node.level);
    CHECK(adapted_node.interfaces == source_node.interfaces);
    REQUIRE(adapted_node.fields.size() == source_node.fields.size());

    for (std::size_t field_index = 0; field_index < source_node.fields.size();
         ++field_index) {
      const auto &source_field = source_node.fields[field_index];
      const auto &adapted_field = adapted_node.fields[field_index];
      CHECK(adapted_field.name == source_field.name);
      CHECK(static_cast<int>(adapted_field.type) ==
            static_cast<int>(source_field.type));
      CHECK(static_cast<int>(adapted_field.access) ==
            static_cast<int>(source_field.access));
      CHECK(adapted_field.declared_default == source_field.defaultValue);
      CHECK(adapted_field.accepted_node_types ==
            source_field.acceptableNodeTypes);
      CHECK(adapted_field.unit_category == source_field.unitCategory);
    }

    if (!source_node.abstract) {
      const auto live = x3d::nodes::X3DNodeFactory::create(source_node.name);
      REQUIRE(live);
      const auto &reflected = live->fields();
      REQUIRE(reflected.size() == source_node.fields.size());
      for (std::size_t field_index = 0; field_index < reflected.size();
           ++field_index) {
        CHECK(reflected[field_index].x3dName ==
              source_node.fields[field_index].name);
        CHECK(reflected[field_index].type ==
              source_node.fields[field_index].type);
        CHECK(reflected[field_index].access ==
              source_node.fields[field_index].access);
      }
    }
  }
}

TEST_CASE("the owning value vocabulary preserves every non-node ISO kind") {
  static_assert(std::variant_size_v<sai::value> == 45);
  static_assert(std::ranges::range<sai::bool_list>);
  static_assert(std::ranges::forward_range<sai::bool_list>);
  sai::bool_list booleans{true, false};
  CHECK(booleans.at(0));
  CHECK_FALSE(booleans.at(1));
  booleans.set(1, true);
  CHECK(booleans.at(1));
  CHECK_THROWS_AS(
      sai::matrix3f{}.at(std::numeric_limits<std::size_t>::max(), 1),
      std::out_of_range);
  check_owning_value_roundtrip("SFBoolValue", sai::value_kind::sf_bool, true);
  check_owning_value_roundtrip("SFColorValue", sai::value_kind::sf_color,
                               sai::color3f{0.25F, 0.5F, 0.75F});
  check_owning_value_roundtrip("SFColorRGBAValue",
                               sai::value_kind::sf_color_rgba,
                               sai::color4f{0.25F, 0.5F, 0.75F, 1.0F});
  check_owning_value_roundtrip("SFDoubleValue", sai::value_kind::sf_double,
                               1.25);
  check_owning_value_roundtrip("SFFloatValue", sai::value_kind::sf_float,
                               1.25F);
  check_owning_value_roundtrip("SFImageValue", sai::value_kind::sf_image,
                               sai::image{.width = 2,
                                          .height = 1,
                                          .components = 4,
                                          .data = {0, 1, 2, 3, 4, 5, 6, 7}});
  check_owning_value_roundtrip("SFInt32Value", sai::value_kind::sf_int32,
                               std::int32_t{-7});
  check_owning_value_roundtrip(
      "SFMatrix3dValue", sai::value_kind::sf_matrix3d,
      sai::matrix3d{std::array<double, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}});
  check_owning_value_roundtrip(
      "SFMatrix3fValue", sai::value_kind::sf_matrix3f,
      sai::matrix3f{std::array<float, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}});
  check_owning_value_roundtrip(
      "SFMatrix4dValue", sai::value_kind::sf_matrix4d,
      sai::matrix4d{std::array<double, 16>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                           12, 13, 14, 15, 16}});
  check_owning_value_roundtrip(
      "SFMatrix4fValue", sai::value_kind::sf_matrix4f,
      sai::matrix4f{std::array<float, 16>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                          13, 14, 15, 16}});
  check_owning_value_roundtrip("SFRotationValue", sai::value_kind::sf_rotation,
                               sai::rotation{0, 1, 0, 1.5F});
  check_owning_value_roundtrip("SFStringValue", sai::value_kind::sf_string,
                               std::string{"owned\0string", 12});
  check_owning_value_roundtrip("SFTimeValue", sai::value_kind::sf_time,
                               sai::time_value{12.5});
  check_owning_value_roundtrip("SFVec2dValue", sai::value_kind::sf_vec2d,
                               sai::vec2d{1, 2});
  check_owning_value_roundtrip("SFVec2fValue", sai::value_kind::sf_vec2f,
                               sai::vec2f{1, 2});
  check_owning_value_roundtrip("SFVec3dValue", sai::value_kind::sf_vec3d,
                               sai::vec3d{1, 2, 3});
  check_owning_value_roundtrip("SFVec3fValue", sai::value_kind::sf_vec3f,
                               sai::vec3f{1, 2, 3});
  check_owning_value_roundtrip("SFVec4dValue", sai::value_kind::sf_vec4d,
                               sai::vec4d{1, 2, 3, 4});
  check_owning_value_roundtrip("SFVec4fValue", sai::value_kind::sf_vec4f,
                               sai::vec4f{1, 2, 3, 4});
  check_owning_value_roundtrip("SFEnumValue", sai::value_kind::sf_enum,
                               sai::enum_value{"EXAMINE"});

  check_owning_value_roundtrip("MFBoolValue", sai::value_kind::mf_bool,
                               sai::bool_list{true, false});
  check_owning_value_roundtrip("MFColorValue", sai::value_kind::mf_color,
                               sai::color3f_list{{0.25F, 0.5F, 0.75F}});
  check_owning_value_roundtrip("MFColorRGBAValue",
                               sai::value_kind::mf_color_rgba,
                               sai::color4f_list{{0.25F, 0.5F, 0.75F, 1.0F}});
  check_owning_value_roundtrip("MFDoubleValue", sai::value_kind::mf_double,
                               sai::double_list{1.25, 2.5});
  check_owning_value_roundtrip("MFFloatValue", sai::value_kind::mf_float,
                               sai::float_list{1.25F, 2.5F});
  check_owning_value_roundtrip(
      "MFImageValue", sai::value_kind::mf_image,
      sai::image_list{
          {.width = 1, .height = 1, .components = 3, .data = {1, 2, 3}}});
  check_owning_value_roundtrip("MFInt32Value", sai::value_kind::mf_int32,
                               sai::int32_list{-1, 2});
  check_owning_value_roundtrip(
      "MFMatrix3dValue", sai::value_kind::mf_matrix3d,
      sai::matrix3d_list{
          sai::matrix3d{std::array<double, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}}});
  check_owning_value_roundtrip(
      "MFMatrix3fValue", sai::value_kind::mf_matrix3f,
      sai::matrix3f_list{
          sai::matrix3f{std::array<float, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}}});
  check_owning_value_roundtrip(
      "MFMatrix4dValue", sai::value_kind::mf_matrix4d,
      sai::matrix4d_list{sai::matrix4d{std::array<double, 16>{
          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}}});
  check_owning_value_roundtrip(
      "MFMatrix4fValue", sai::value_kind::mf_matrix4f,
      sai::matrix4f_list{sai::matrix4f{std::array<float, 16>{
          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}}});
  check_owning_value_roundtrip("MFRotationValue", sai::value_kind::mf_rotation,
                               sai::rotation_list{{0, 1, 0, 1.5F}});
  check_owning_value_roundtrip("MFStringValue", sai::value_kind::mf_string,
                               sai::string_list{"one", "two words"});
  check_owning_value_roundtrip("MFTimeValue", sai::value_kind::mf_time,
                               sai::time_list{{1.25}, {2.5}});
  check_owning_value_roundtrip("MFVec2dValue", sai::value_kind::mf_vec2d,
                               sai::vec2d_list{{1, 2}});
  check_owning_value_roundtrip("MFVec2fValue", sai::value_kind::mf_vec2f,
                               sai::vec2f_list{{1, 2}});
  check_owning_value_roundtrip("MFVec3dValue", sai::value_kind::mf_vec3d,
                               sai::vec3d_list{{1, 2, 3}});
  check_owning_value_roundtrip("MFVec3fValue", sai::value_kind::mf_vec3f,
                               sai::vec3f_list{{1, 2, 3}});
  check_owning_value_roundtrip("MFVec4dValue", sai::value_kind::mf_vec4d,
                               sai::vec4d_list{{1, 2, 3, 4}});
  check_owning_value_roundtrip("MFVec4fValue", sai::value_kind::mf_vec4f,
                               sai::vec4f_list{{1, 2, 3, 4}});
  check_owning_value_roundtrip("MFEnumValue", sai::value_kind::mf_enum,
                               sai::enum_list{{"EXAMINE"}, {"WALK"}});
}

TEST_CASE("every generated descriptor adapts into the executable vocabulary") {
  const auto catalog = sai::generated_metadata_catalog();
  std::vector<std::string_view> names;
  names.reserve(catalog.nodes.size());
  for (const auto &node : catalog.nodes)
    names.push_back(node.name);

  auto registry = sai::generated_type_registry(names);
  if (!registry)
    INFO(registry.error().field << ": " << registry.error().message);
  REQUIRE(registry);
  for (const auto &source_node : catalog.nodes) {
    const auto *adapted_node = registry.value().find(source_node.name);
    REQUIRE(adapted_node != nullptr);
    REQUIRE(adapted_node->fields.size() == source_node.fields.size());
    for (std::size_t index = 0; index < source_node.fields.size(); ++index) {
      CHECK(adapted_node->fields[index].name == source_node.fields[index].name);
      CHECK(adapted_node->fields[index].kind == source_node.fields[index].type);
    }
  }
}

TEST_CASE(
    "schema lexical defaults reject image truncation and preserve strings") {
  const sai::schema_field_descriptor overflowing_image{
      .name = "image",
      .type = sai::value_kind::sf_image,
      .access = sai::access_type::input_output,
      .declared_default = "1 1 1 0x1ff",
      .accepted_node_types = {},
      .unit_category = std::nullopt,
  };
  auto rejected = sai::default_value_for(overflowing_image);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  CHECK(rejected.error().operation == "default_value_for");

  const sai::schema_field_descriptor strings{
      .name = "strings",
      .type = sai::value_kind::mf_string,
      .access = sai::access_type::input_output,
      .declared_default = R"(["a", "b", "c:\new", "quote\"slash\\"])",
      .accepted_node_types = {},
      .unit_category = std::nullopt,
  };
  auto parsed = sai::default_value_for(strings);
  REQUIRE(parsed);
  REQUIRE(std::holds_alternative<sai::string_list>(parsed.value()));
  CHECK(std::get<sai::string_list>(parsed.value()) ==
        sai::string_list{"a", "b", R"(c:\new)", R"(quote"slash\)"});
}

TEST_CASE("a dynamic multi-field editor stages the complete indexed service") {
  sai::type_registry registry;
  REQUIRE(registry.define(
      test_node_type("Samples", {test_field("values", sai::value_kind::mf_int32,
                                            sai::access_type::input_output,
                                            sai::int32_list{10, 20, 30})})));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto owner = edit.create_node("Samples");
  REQUIRE(owner);
  auto field = edit.field(owner.value(), "values");
  REQUIRE(field);
  auto values = edit.multi(field.value());
  REQUIRE(values);
  CHECK(values.value().size().value() == 3);
  CHECK(values.value().at(1).value() == sai::value{std::int32_t{20}});
  REQUIRE(values.value().set(1, sai::value{std::int32_t{21}}));
  REQUIRE(values.value().insert(1, sai::value{std::int32_t{15}}));
  REQUIRE(values.value().erase(2));
  REQUIRE(values.value().append(sai::value{std::int32_t{40}}));
  REQUIRE(values.value().clear());
  REQUIRE(values.value().replace(sai::value{sai::int32_list{7, 8}}));
  CHECK(values.value().size().value() == 2);
  CHECK(values.value().at(0).value() == sai::value{std::int32_t{7}});
  auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 7);
  CHECK(require_graph_change(committed.value().changes[1]).kind ==
        sai::change_kind::multi_element_set);
  CHECK(require_graph_change(committed.value().changes[2]).kind ==
        sai::change_kind::multi_inserted);
  CHECK(require_graph_change(committed.value().changes[3]).kind ==
        sai::change_kind::multi_erased);
  CHECK(require_graph_change(committed.value().changes[4]).kind ==
        sai::change_kind::multi_inserted);
  CHECK(require_graph_change(committed.value().changes[5]).kind ==
        sai::change_kind::multi_cleared);
  CHECK(require_graph_change(committed.value().changes[6]).kind ==
        sai::change_kind::multi_replaced);
  CHECK(context.snapshot().read(field.value()).value() ==
        sai::value{sai::int32_list{7, 8}});
}

TEST_CASE("typed and dynamic multi-field editors publish equal traces") {
  const auto exercise = [](bool typed) -> sai::change_set {
    sai::type_registry registry;
    REQUIRE(registry.define(test_node_type(
        "Samples", {test_field("values", sai::value_kind::mf_int32,
                               sai::access_type::input_output,
                               sai::int32_list{10, 20})})));
    sai::browser host{std::move(registry)};
    auto context = host.current_scene();
    auto edit = context.edit();
    auto owner = edit.create_node("Samples");
    REQUIRE(owner);
    auto dynamic = edit.field(owner.value(), "values");
    REQUIRE(dynamic);
    if (typed) {
      auto field = dynamic.value().as<sai::int32_list>();
      REQUIRE(field);
      auto values = edit.multi(field.value());
      REQUIRE(values);
      CHECK(values.value().at(0).value() == 10);
      REQUIRE(values.value().set(0, 11));
      REQUIRE(values.value().insert(1, 12));
      REQUIRE(values.value().erase(2));
      REQUIRE(values.value().append(13));
      REQUIRE(values.value().clear());
      REQUIRE(values.value().replace(sai::int32_list{7, 8}));
    } else {
      auto values = edit.multi(dynamic.value());
      REQUIRE(values);
      CHECK(values.value().at(0).value() == sai::value{std::int32_t{10}});
      REQUIRE(values.value().set(0, sai::value{std::int32_t{11}}));
      REQUIRE(values.value().insert(1, sai::value{std::int32_t{12}}));
      REQUIRE(values.value().erase(2));
      REQUIRE(values.value().append(sai::value{std::int32_t{13}}));
      REQUIRE(values.value().clear());
      REQUIRE(values.value().replace(sai::value{sai::int32_list{7, 8}}));
    }
    auto committed = edit.commit();
    REQUIRE(committed);
    return committed.value();
  };

  const auto dynamic = exercise(false);
  const auto typed = exercise(true);
  REQUIRE(dynamic.changes.size() == typed.changes.size());
  for (std::size_t index = 0; index < dynamic.changes.size(); ++index) {
    const auto &dynamic_change = require_graph_change(dynamic.changes[index]);
    const auto &typed_change = require_graph_change(typed.changes[index]);
    CHECK(dynamic_change.kind == typed_change.kind);
    CHECK(dynamic_change.node == typed_change.node);
    CHECK(dynamic_change.field == typed_change.field);
    CHECK(sai::same_representation(dynamic_change.before, typed_change.before));
    CHECK(sai::same_representation(dynamic_change.after, typed_change.after));
    CHECK(dynamic_change.index == typed_change.index);
  }
}

TEST_CASE("multi-field editors are revision-bound capabilities") {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "Samples",
      {test_field("values", sai::value_kind::mf_int32,
                  sai::access_type::input_output, sai::int32_list{1})})));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  auto owner = author.create_node("Samples");
  REQUIRE(owner);
  REQUIRE(author.commit());
  auto field = context.snapshot().field(owner.value(), "values");
  REQUIRE(field);

  auto stale_edit = context.edit();
  auto stale_values = stale_edit.multi(field.value());
  REQUIRE(stale_values);
  auto winner = context.edit();
  REQUIRE(winner.set(field.value(), sai::value{sai::int32_list{2}}));
  REQUIRE(winner.commit());
  auto stale_read = stale_values.value().size();
  REQUIRE_FALSE(stale_read);
  CHECK(stale_read.error().code == sai::error_code::stale_revision);

  auto committed_edit = context.edit();
  auto committed_values = committed_edit.multi(field.value());
  REQUIRE(committed_values);
  REQUIRE(committed_values.value().append(sai::value{std::int32_t{3}}));
  REQUIRE(committed_edit.commit());
  auto after_commit = committed_values.value().at(0);
  REQUIRE_FALSE(after_commit);
  CHECK(after_commit.error().code == sai::error_code::stale_revision);
  auto second_commit = committed_edit.commit();
  REQUIRE_FALSE(second_commit);
  CHECK(second_commit.error().code == sai::error_code::poisoned_edit);

  auto retired_edit = context.edit();
  auto retired_values = retired_edit.multi(field.value());
  REQUIRE(retired_values);
  auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  auto retired_read = retired_values.value().size();
  REQUIRE_FALSE(retired_read);
  CHECK(retired_read.error().code == sai::error_code::stale_handle);
  auto retired_write =
      retired_values.value().append(sai::value{std::int32_t{4}});
  REQUIRE_FALSE(retired_write);
  CHECK(retired_write.error().code == sai::error_code::stale_handle);
}

TEST_CASE("MFNode editors validate the complete handle range before staging") {
  sai::type_registry registry;
  auto children =
      test_field("children", sai::value_kind::mf_node,
                 sai::access_type::input_output, sai::node_list{}, true);
  children.accepted_node_types = {"Box"};
  REQUIRE(registry.define(test_node_type("Parent", {std::move(children)})));
  REQUIRE(registry.define(test_node_type("Box", {})));
  REQUIRE(registry.define(test_node_type("Shape", {})));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  auto parent = author.create_node("Parent");
  auto box = author.create_node("Box");
  auto shape = author.create_node("Shape");
  REQUIRE(parent);
  REQUIRE(box);
  REQUIRE(shape);
  auto dynamic = author.field(parent.value(), "children");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::node_list>();
  REQUIRE(typed);
  auto values = author.multi(typed.value());
  REQUIRE(values);
  REQUIRE(values.value().append(box.value()));
  CHECK(values.value().at(0).value().id() == box.value().id());
  const std::array invalid_range{box.value(), shape.value()};
  auto rejected =
      values.value().replace(std::span<const sai::node>{invalid_range});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  REQUIRE_FALSE(author.commit());
  CHECK(context.snapshot().revision() == 0);
}

TEST_CASE(
    "numeric bits and large owning images survive the semantic boundary") {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "BoundaryValues",
      {
          test_field("number", sai::value_kind::sf_double,
                     sai::access_type::input_output, 0.0),
          test_field("time", sai::value_kind::sf_time,
                     sai::access_type::input_output, sai::time_value{}),
          test_field("image", sai::value_kind::sf_image,
                     sai::access_type::input_output, sai::image{}),
      })));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto owner = edit.create_node("BoundaryValues");
  REQUIRE(owner);
  auto number = edit.field(owner.value(), "number");
  auto time = edit.field(owner.value(), "time");
  auto pixels = edit.field(owner.value(), "image");
  REQUIRE(number);
  REQUIRE(time);
  REQUIRE(pixels);
  auto typed_number = number.value().as<double>();
  auto typed_time = time.value().as<sai::time_value>();
  auto typed_pixels = pixels.value().as<sai::image>();
  REQUIRE(typed_number);
  REQUIRE(typed_time);
  REQUIRE(typed_pixels);

  constexpr std::uint64_t nan_bits = 0x7ff8000000000042ULL;
  const double nan = std::bit_cast<double>(nan_bits);
  CHECK(sai::same_representation(sai::value{nan}, sai::value{nan}));
  CHECK_FALSE(sai::same_representation(sai::value{-0.0}, sai::value{0.0}));
  sai::image large{.width = 512,
                   .height = 512,
                   .components = 4,
                   .data = std::vector<std::uint8_t>(512U * 512U * 4U, 0xA5)};
  REQUIRE(edit.set(typed_number.value(), nan));
  REQUIRE(edit.set(typed_time.value(), sai::time_value{-0.0}));
  REQUIRE(edit.set(typed_pixels.value(), large));
  REQUIRE(edit.commit());

  const auto snapshot = context.snapshot();
  const auto stored_number = snapshot.read(typed_number.value());
  const auto stored_time = snapshot.read(typed_time.value());
  const auto stored_pixels = snapshot.read(typed_pixels.value());
  REQUIRE(stored_number);
  REQUIRE(stored_time);
  REQUIRE(stored_pixels);
  CHECK(std::bit_cast<std::uint64_t>(stored_number.value()) == nan_bits);
  CHECK(std::signbit(stored_time.value().seconds));
  CHECK(stored_pixels.value() == large);

  auto signed_zero = context.events(sai::event_time{1.0});
  REQUIRE(signed_zero.send(typed_time.value(), sai::time_value{0.0}));
  auto signed_zero_result = signed_zero.commit();
  REQUIRE(signed_zero_result);
  REQUIRE(signed_zero_result.value().state_changes);
  CHECK_FALSE(std::signbit(
      context.snapshot().read(typed_time.value()).value().seconds));

  const auto before_same_nan = context.snapshot().revision();
  auto same_nan = context.events(sai::event_time{2.0});
  REQUIRE(same_nan.send(typed_number.value(), nan));
  auto same_nan_result = same_nan.commit();
  REQUIRE(same_nan_result);
  CHECK_FALSE(same_nan_result.value().state_changes);
  CHECK(context.snapshot().revision() == before_same_nan);

  const auto before_invalid = context.snapshot().revision();
  auto invalid = context.edit();
  auto invalid_pixels = invalid.field(owner.value(), "image");
  REQUIRE(invalid_pixels);
  auto rejected = invalid.set(
      invalid_pixels.value(),
      sai::value{sai::image{.width = std::numeric_limits<std::int32_t>::max(),
                            .height = std::numeric_limits<std::int32_t>::max(),
                            .components = 4,
                            .data = {}}});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_value);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().revision() == before_invalid);
  CHECK(context.snapshot().read(typed_pixels.value()).value() == large);
}

TEST_CASE("generated abstract interfaces remain non-instantiable") {
  const std::array<std::string_view, 1> selected{"X3DNode"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto edit = host.current_scene().edit();
  auto abstract_node = edit.create_node("X3DNode");
  REQUIRE_FALSE(abstract_node);
  CHECK(abstract_node.error().code == sai::error_code::abstract_type);
}

TEST_CASE(
    "generated metadata preserves time instead of collapsing it to double") {
  const std::array<std::string_view, 1> selected{"TimeSensor"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);
  const auto *time_sensor = registry.value().find("TimeSensor");
  REQUIRE(time_sensor != nullptr);
  const auto cycle_interval = std::ranges::find_if(
      time_sensor->fields, [](const sai::field_descriptor &field) {
        return field.name == "cycleInterval";
      });
  REQUIRE(cycle_interval != time_sensor->fields.end());
  CHECK(cycle_interval->kind == sai::value_kind::sf_time);
  CHECK(std::holds_alternative<sai::time_value>(cycle_interval->default_value));
  CHECK(std::get<sai::time_value>(cycle_interval->default_value) ==
        sai::time_value{1.0});
}

TEST_CASE("generated node constraints govern containment authoring") {
  const std::array<std::string_view, 3> selected{"Group", "Shape", "Box"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto valid = context.edit();
  auto group = valid.create_node("Group");
  auto shape = valid.create_node("Shape");
  REQUIRE(group);
  REQUIRE(shape);
  REQUIRE(valid.append(group.value(), "children", shape.value()));
  REQUIRE(valid.commit());

  auto invalid = context.edit();
  auto box = invalid.create_node("Box");
  REQUIRE(box);
  auto rejected = invalid.append(group.value(), "children", box.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  REQUIRE_FALSE(invalid.commit());
}

TEST_CASE("registry rejects descriptors whose defaults or containment lie") {
  sai::type_registry registry;
  auto wrong_default = registry.define(test_node_type(
      "WrongDefault", {test_field("translation", sai::value_kind::sf_vec3f,
                                  sai::access_type::input_output,
                                  std::string{"not a vector"})}));
  REQUIRE_FALSE(wrong_default);
  CHECK(wrong_default.error().code == sai::error_code::invalid_descriptor);

  auto wrong_containment = registry.define(test_node_type(
      "WrongContainment",
      {test_field("translation", sai::value_kind::sf_vec3f,
                  sai::access_type::input_output, sai::vec3f{}, true)}));
  REQUIRE_FALSE(wrong_containment);
  CHECK(wrong_containment.error().code == sai::error_code::invalid_descriptor);
}

TEST_CASE("one shared node has one identity and two occurrences") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();

  auto root = edit.create_node("Group");
  auto shared = edit.create_node("Transform");
  REQUIRE(root);
  REQUIRE(shared);
  REQUIRE(edit.append_root(root.value()));
  REQUIRE(edit.append(root.value(), "children", shared.value()));
  REQUIRE(edit.append(root.value(), "children", shared.value()));

  auto committed = edit.commit();
  REQUIRE(committed);
  CHECK(committed.value().before_revision == 0);
  CHECK(committed.value().after_revision == 1);

  auto snapshot = context.snapshot();
  auto occurrences = snapshot.occurrences();
  REQUIRE(occurrences.size() == 3);
  CHECK(occurrences[0].node == root.value().id());
  CHECK(occurrences[1].node == shared.value().id());
  CHECK(occurrences[2].node == shared.value().id());
  CHECK(occurrences[1].path != occurrences[2].path);
  CHECK(occurrences[1].parent_occurrence == 0);
  CHECK(occurrences[2].parent_occurrence == 0);
}

TEST_CASE("a containment cycle rejects the complete edit") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto group = edit.create_node("Group");
  REQUIRE(group);
  REQUIRE(edit.append_root(group.value()));
  REQUIRE(edit.append(group.value(), "children", group.value()));

  auto rejected = edit.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::containment_cycle);
  CHECK(context.snapshot().revision() == 0);
  CHECK(context.snapshot().roots().empty());
}

TEST_CASE("dynamic discovery and typed fields are substitutable") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto before = context.snapshot();
  auto dynamic = before.field(transform.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  CHECK(before.read(dynamic.value()).value() == sai::value{sai::vec3f{}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});

  auto wrong_type = dynamic.value().as<std::string>();
  REQUIRE_FALSE(wrong_type);
  CHECK(wrong_type.error().code == sai::error_code::type_mismatch);
  CHECK(wrong_type.error().node == transform.value().id());
  CHECK(wrong_type.error().field == "translation");

  auto edit = context.edit();
  REQUIRE(edit.set(typed.value(), sai::vec3f{1, 2, 3}));
  auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 1);
  const auto &field_change = require_graph_change(committed.value().changes[0]);
  CHECK(field_change.kind == sai::change_kind::field_changed);
  CHECK(field_change.before == sai::value{sai::vec3f{}});
  CHECK(field_change.after == sai::value{sai::vec3f{1, 2, 3}});

  auto after = context.snapshot();
  CHECK(after.read(dynamic.value()).value() == sai::value{sai::vec3f{1, 2, 3}});
  CHECK(after.read(typed.value()).value() == sai::vec3f{1, 2, 3});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});
}

TEST_CASE("inspection discovers node types and ordered field definitions") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  REQUIRE(snapshot.roots().size() == 1);
  auto discovered_node = snapshot.lookup(snapshot.roots().front());
  REQUIRE(discovered_node);
  auto descriptor = snapshot.describe(discovered_node.value());
  REQUIRE(descriptor);
  CHECK(descriptor.value().name == "Transform");
  REQUIRE(descriptor.value().fields.size() == 5);
  CHECK(descriptor.value().fields[0].name == "translation");
  CHECK(descriptor.value().fields[0].kind == sai::value_kind::sf_vec3f);
  CHECK(descriptor.value().fields[0].access == sai::access_type::input_output);

  auto discovered_field = snapshot.field(discovered_node.value(),
                                         descriptor.value().fields[0].name);
  REQUIRE(discovered_field);
  CHECK(snapshot.read(discovered_field.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("declarations have stable identity and authored order") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto named_node = edit.create_node("Transform");
  REQUIRE(named_node);
  REQUIRE(edit.define_name("Widget", named_node.value()));

  auto local = edit.add_local_declaration(
      sai::local_declaration_descriptor{.name = "Widget",
                                        .interface = {},
                                        .body_roots = {},
                                        .appinfo = "local widget",
                                        .documentation = {}});
  auto external =
      edit.add_external_declaration(sai::external_declaration_descriptor{
          .name = "RemoteWidget",
          .interface = {},
          .urls = {"widgets.x3d#Widget", "fallback.x3d#Widget"},
          .load_state = sai::external_load_state::unresolved,
          .diagnostic = {},
          .resolved_declaration = std::nullopt,
          .appinfo = {},
          .documentation = {}});
  REQUIRE(local);
  REQUIRE(external);
  CHECK(local->id() != external->id());
  CHECK(local->id().value < external->id().value);
  REQUIRE(edit.commit());

  const auto snapshot = context.snapshot();
  REQUIRE(snapshot.declarations().size() == 2);
  CHECK(snapshot.declarations()[0] == local->id());
  CHECK(snapshot.declarations()[1] == external->id());
  CHECK(snapshot.named("Widget"));
  const auto found = snapshot.declaration_named("Widget");
  REQUIRE(found);
  CHECK(found->id() == local->id());

  const auto local_descriptor = snapshot.describe(local.value());
  const auto external_descriptor = snapshot.describe(external.value());
  REQUIRE(local_descriptor);
  REQUIRE(external_descriptor);
  CHECK(sai::kind(local_descriptor.value()) ==
        sai::declaration_kind::local_proto);
  CHECK(sai::name(local_descriptor.value()) == "Widget");
  CHECK(sai::kind(external_descriptor.value()) ==
        sai::declaration_kind::external_proto);
  CHECK(sai::name(external_descriptor.value()) == "RemoteWidget");
  const auto &external_payload = std::get<sai::external_declaration_descriptor>(
      external_descriptor->payload);
  CHECK(external_payload.urls ==
        std::vector<std::string>{"widgets.x3d#Widget", "fallback.x3d#Widget"});
  CHECK_FALSE(external_payload.resolved_declaration);
}

TEST_CASE("declaration interfaces validate exact field semantics") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();

  SUBCASE("authored interface order is preserved") {
    auto edit = context.edit();
    auto declaration =
        edit.add_local_declaration(sai::local_declaration_descriptor{
            .name = "Ordered",
            .interface =
                {
                    sai::interface_field_descriptor{
                        .name = "translation",
                        .kind = sai::value_kind::sf_vec3f,
                        .access = sai::access_type::input_output,
                        .default_value = sai::value{sai::vec3f{1, 2, 3}},
                        .accepted_node_types = {},
                        .unit_category = "length"},
                    sai::interface_field_descriptor{
                        .name = "trigger",
                        .kind = sai::value_kind::sf_bool,
                        .access = sai::access_type::input_only,
                        .default_value = std::nullopt,
                        .accepted_node_types = {},
                        .unit_category = std::nullopt},
                },
            .body_roots = {},
            .appinfo = {},
            .documentation = {}});
    REQUIRE(declaration);
    REQUIRE(edit.commit());
    const auto descriptor = context.snapshot().describe(declaration.value());
    REQUIRE(descriptor);
    const auto &local =
        std::get<sai::local_declaration_descriptor>(descriptor->payload);
    REQUIRE(local.interface.size() == 2);
    CHECK(local.interface[0].name == "translation");
    CHECK(local.interface[1].name == "trigger");
  }

  SUBCASE("interface names are unique") {
    auto edit = context.edit();
    const auto rejected = edit.add_local_declaration(test_local_declaration(
        "DuplicateField",
        {test_interface_field("value"), test_interface_field("value")}));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::duplicate_field);
  }

  SUBCASE("default representation matches the field kind") {
    auto edit = context.edit();
    const auto rejected = edit.add_local_declaration(test_local_declaration(
        "WrongDefault",
        {test_interface_field("translation", sai::value_kind::sf_vec3f,
                              sai::access_type::input_output,
                              sai::value{std::string{"wrong"}})}));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  }

  SUBCASE("event fields cannot declare defaults") {
    auto edit = context.edit();
    const auto rejected =
        edit.add_external_declaration(test_external_declaration(
            "EventDefault", {"event.x3d#EventDefault"},
            {test_interface_field("trigger", sai::value_kind::sf_bool,
                                  sai::access_type::input_only,
                                  sai::value{false})}));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  }

  SUBCASE("unit categories require scalable compatible kinds") {
    auto edit = context.edit();
    const auto rejected = edit.add_local_declaration(test_local_declaration(
        "WrongUnit", {test_interface_field("label", sai::value_kind::sf_string,
                                           sai::access_type::input_output,
                                           std::nullopt, {}, "length")}));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  }

  SUBCASE("accepted node types require a node-valued field") {
    auto edit = context.edit();
    const auto rejected = edit.add_local_declaration(test_local_declaration(
        "WrongConstraint",
        {test_interface_field("label", sai::value_kind::sf_string,
                              sai::access_type::input_output, std::nullopt,
                              {"Transform"})}));
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_descriptor);
  }
}

TEST_CASE("rejected declaration edits publish nothing") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  REQUIRE(edit.add_local_declaration(test_local_declaration("Shared")));
  const auto duplicate = edit.add_external_declaration(
      test_external_declaration("Shared", {"shared.x3d#Shared"}));
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::duplicate_name);
  const auto unpublished = edit.commit();
  REQUIRE_FALSE(unpublished);
  CHECK(context.snapshot().revision() == 0);
  CHECK(context.snapshot().declarations().empty());

  auto missing_url = context.edit();
  const auto invalid = missing_url.add_external_declaration(
      test_external_declaration("NoUrl", {}));
  REQUIRE_FALSE(invalid);
  CHECK(invalid.error().code == sai::error_code::invalid_descriptor);
  REQUIRE_FALSE(missing_url.commit());
  CHECK(context.snapshot().revision() == 0);
}

TEST_CASE("declaration query and mutation operations are identity duals") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto local = author.add_local_declaration(test_local_declaration("Widget"));
  auto external = author.add_external_declaration(
      test_external_declaration("Remote", {"first.x3d#Widget"}));
  REQUIRE(local);
  REQUIRE(external);
  REQUIRE(author.commit());
  const auto historical = context.snapshot();
  const auto original_order = historical.declarations();

  auto update = context.edit();
  REQUIRE(update.rename_declaration(local.value(), "RenamedWidget"));
  REQUIRE(update.update_declaration(
      external.value(),
      test_external_declaration("Remote", {"second.x3d#Widget"})));
  const auto updated = update.commit();
  REQUIRE(updated);
  REQUIRE(updated->changes.size() == 2);
  CHECK(sai::declaration_change_of(updated->changes[0])->kind ==
        sai::declaration_change_kind::renamed);
  const auto *update_change = sai::declaration_change_of(updated->changes[1]);
  REQUIRE(update_change);
  CHECK(update_change->kind == sai::declaration_change_kind::updated);
  const auto &before_external = std::get<sai::external_declaration_descriptor>(
      update_change->before->payload);
  const auto &after_external = std::get<sai::external_declaration_descriptor>(
      update_change->after->payload);
  CHECK(before_external.urls == std::vector<std::string>{"first.x3d#Widget"});
  CHECK(after_external.urls == std::vector<std::string>{"second.x3d#Widget"});

  const auto current = context.snapshot();
  CHECK(current.declarations() == original_order);
  CHECK_FALSE(current.declaration_named("Widget"));
  CHECK(current.declaration_named("RenamedWidget")->id() == local->id());
  const auto updated_external = current.describe(external.value());
  REQUIRE(updated_external);
  CHECK(
      std::get<sai::external_declaration_descriptor>(updated_external->payload)
          .urls == std::vector<std::string>{"second.x3d#Widget"});

  CHECK(historical.declaration_named("Widget")->id() == local->id());
  CHECK_FALSE(historical.declaration_named("RenamedWidget"));
  CHECK(std::get<sai::external_declaration_descriptor>(
            historical.describe(external.value())->payload)
            .urls == std::vector<std::string>{"first.x3d#Widget"});

  auto removal = context.edit();
  REQUIRE(removal.remove_declaration(local.value()));
  REQUIRE(removal.commit());
  const auto stale = context.snapshot().describe(local.value());
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_handle);
  CHECK(stale.error().declaration == local->id());
  CHECK(historical.describe(local.value()));

  auto replacement = context.edit();
  auto recreated = replacement.add_local_declaration(
      test_local_declaration("RenamedWidget"));
  REQUIRE(recreated);
  REQUIRE(replacement.commit());
  CHECK(recreated->id() != local->id());
  CHECK(recreated->id().value > local->id().value);
}

TEST_CASE("declaration updates preserve kind and separate rename intent") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto local = author.add_local_declaration(test_local_declaration("Local"));
  auto external = author.add_external_declaration(
      test_external_declaration("External", {"external.x3d#External"}));
  REQUIRE(local);
  REQUIRE(external);
  REQUIRE(author.commit());

  auto kind_change = context.edit();
  const auto wrong_kind = kind_change.update_declaration(
      local.value(), test_external_declaration("Local", {"local.x3d#Local"}));
  REQUIRE_FALSE(wrong_kind);
  CHECK(wrong_kind.error().code == sai::error_code::invalid_descriptor);
  REQUIRE_FALSE(kind_change.commit());

  auto hidden_rename = context.edit();
  const auto wrong_name = hidden_rename.update_declaration(
      external.value(),
      test_external_declaration("Different", {"external.x3d#External"}));
  REQUIRE_FALSE(wrong_name);
  CHECK(wrong_name.error().code == sai::error_code::invalid_name);
  REQUIRE_FALSE(hidden_rename.commit());
}

TEST_CASE("graph and declaration changes share one authored order") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create_node("Transform");
  auto declaration =
      edit.add_local_declaration(test_local_declaration("Original"));
  REQUIRE(transform);
  REQUIRE(declaration);
  REQUIRE(edit.rename_declaration(declaration.value(), "Renamed"));
  auto dynamic_translation = edit.field(transform.value(), "translation");
  REQUIRE(dynamic_translation);
  auto translation = dynamic_translation->as<sai::vec3f>();
  REQUIRE(translation);
  REQUIRE(edit.set(translation.value(), sai::vec3f{1, 2, 3}));
  REQUIRE(edit.remove_declaration(declaration.value()));

  const auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed->changes.size() == 5);

  const auto *created = sai::node_change(committed->changes[0]);
  REQUIRE(created);
  CHECK(created->kind == sai::change_kind::node_created);
  CHECK(created->node == transform->id());
  CHECK_FALSE(sai::declaration_change_of(committed->changes[0]));

  const auto *added = sai::declaration_change_of(committed->changes[1]);
  REQUIRE(added);
  CHECK(added->kind == sai::declaration_change_kind::added);
  CHECK(added->declaration == declaration->id());
  REQUIRE_FALSE(added->before);
  REQUIRE(added->after);
  CHECK(sai::name(added->after.value()) == "Original");

  const auto *renamed = sai::declaration_change_of(committed->changes[2]);
  REQUIRE(renamed);
  CHECK(renamed->kind == sai::declaration_change_kind::renamed);
  CHECK(sai::name(renamed->before.value()) == "Original");
  CHECK(sai::name(renamed->after.value()) == "Renamed");

  const auto *changed = sai::node_change(committed->changes[3]);
  REQUIRE(changed);
  CHECK(changed->kind == sai::change_kind::field_changed);
  CHECK(changed->before == sai::value{sai::vec3f{}});
  CHECK(changed->after == sai::value{sai::vec3f{1, 2, 3}});

  const auto *removed = sai::declaration_change_of(committed->changes[4]);
  REQUIRE(removed);
  CHECK(removed->kind == sai::declaration_change_kind::removed);
  REQUIRE(removed->before);
  REQUIRE_FALSE(removed->after);
  CHECK(sai::name(removed->before.value()) == "Renamed");
  CHECK(sai::change_kind_of(committed->changes[4]) ==
        sai::semantic_change_kind{sai::declaration_change_kind::removed});
}

TEST_CASE("local declarations exclusively claim their template closure") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto first_root = edit.create_node("Group");
  auto second_root = edit.create_node("Link");
  auto shared = edit.create_node("Transform");
  REQUIRE(first_root);
  REQUIRE(second_root);
  REQUIRE(shared);
  REQUIRE(edit.append(first_root.value(), "children", shared.value()));
  auto target = edit.field(second_root.value(), "target");
  REQUIRE(target);
  REQUIRE(edit.set(target.value(), shared.value()));
  const std::array roots{first_root.value(), second_root.value()};
  auto prototype = edit.add_local_declaration(
      test_local_declaration("Composite"), std::span<const sai::node>{roots});
  REQUIRE(prototype);
  REQUIRE(edit.commit());

  const auto descriptor = context.snapshot().describe(prototype.value());
  REQUIRE(descriptor);
  const auto &local =
      std::get<sai::local_declaration_descriptor>(descriptor->payload);
  CHECK(local.body_roots ==
        std::vector<sai::node_id>{first_root->id(), second_root->id()});

  auto root_leak = context.edit();
  const auto rooted = root_leak.append_root(first_root.value());
  REQUIRE_FALSE(rooted);
  CHECK(rooted.error().code == sai::error_code::invalid_context);

  auto scene_reference = context.edit();
  auto scene_link = scene_reference.create_node("Link");
  REQUIRE(scene_link);
  auto scene_target = scene_reference.field(scene_link.value(), "target");
  REQUIRE(scene_target);
  const auto retained =
      scene_reference.set(scene_target.value(), shared.value());
  REQUIRE_FALSE(retained);
  CHECK(retained.error().code == sai::error_code::invalid_context);

  auto inverse_reference = context.edit();
  auto ordinary = inverse_reference.create_node("Transform");
  REQUIRE(ordinary);
  const auto escaped = inverse_reference.set(target.value(), ordinary.value());
  REQUIRE_FALSE(escaped);
  CHECK(escaped.error().code == sai::error_code::invalid_context);

  auto second_owner = context.edit();
  const std::array already_owned{first_root.value()};
  const auto duplicate_owner = second_owner.add_local_declaration(
      test_local_declaration("Other"),
      std::span<const sai::node>{already_owned});
  REQUIRE_FALSE(duplicate_owner);
  CHECK(duplicate_owner.error().code == sai::error_code::node_in_use);

  sai::browser foreign_host{graph_registry()};
  auto foreign_context = foreign_host.current_scene();
  auto foreign_edit = foreign_context.edit();
  auto foreign_root = foreign_edit.create_node("Group");
  REQUIRE(foreign_root);
  const std::array foreign_roots{foreign_root.value()};
  auto cross_context = context.edit();
  const auto foreign_body = cross_context.add_local_declaration(
      test_local_declaration("Foreign"),
      std::span<const sai::node>{foreign_roots});
  REQUIRE_FALSE(foreign_body);
  CHECK(foreign_body.error().code == sai::error_code::invalid_context);
}

TEST_CASE("template claims reject live references and cycles atomically") {
  SUBCASE("a live scene root cannot be reclassified as template state") {
    sai::browser host{graph_registry()};
    auto context = host.current_scene();
    auto edit = context.edit();
    auto root = edit.create_node("Group");
    REQUIRE(root);
    REQUIRE(edit.append_root(root.value()));
    const std::array roots{root.value()};
    const auto rejected = edit.add_local_declaration(
        test_local_declaration("Rooted"), std::span<const sai::node>{roots});
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::node_in_use);
    REQUIRE_FALSE(edit.commit());
    CHECK(context.snapshot().revision() == 0);
  }

  SUBCASE("a containment cycle is rejected before ownership mutation") {
    sai::browser host{graph_registry()};
    auto context = host.current_scene();
    auto edit = context.edit();
    auto first = edit.create_node("Group");
    auto second = edit.create_node("Group");
    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(edit.append(first.value(), "children", second.value()));
    REQUIRE(edit.append(second.value(), "children", first.value()));
    const std::array roots{first.value()};
    const auto rejected = edit.add_local_declaration(
        test_local_declaration("Cyclic"), std::span<const sai::node>{roots});
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::containment_cycle);
    REQUIRE_FALSE(edit.commit());
    CHECK(context.snapshot().revision() == 0);
  }
}

TEST_CASE("template scope rejects public aliases routes and removal") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto root = author.create_node("Transform");
  REQUIRE(root);
  const std::array roots{root.value()};
  auto prototype = author.add_local_declaration(
      test_local_declaration("Private"), std::span<const sai::node>{roots});
  REQUIRE(prototype);
  REQUIRE(author.commit());

  auto named = context.edit();
  REQUIRE_FALSE(named.define_name("Leaked", root.value()));
  auto exported = context.edit();
  REQUIRE_FALSE(exported.export_node("Leaked", root.value()));
  auto removed = context.edit();
  const auto retained = removed.remove_node(root.value());
  REQUIRE_FALSE(retained);
  CHECK(retained.error().code == sai::error_code::node_in_use);

  auto routed = context.edit();
  auto sink = routed.create_node("Transform");
  REQUIRE(sink);
  auto source_field = routed.field(root.value(), "worldTranslation");
  auto sink_field = routed.field(sink.value(), "set_translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE_FALSE(routed.add_route(source_field.value(), sink_field.value()));
}

TEST_CASE(
    "removing a declaration releases rather than deletes template nodes") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto root = author.create_node("Group");
  REQUIRE(root);
  const std::array roots{root.value()};
  auto prototype = author.add_local_declaration(
      test_local_declaration("Reusable"), std::span<const sai::node>{roots});
  REQUIRE(prototype);
  REQUIRE(author.commit());

  auto release = context.edit();
  REQUIRE(release.remove_declaration(prototype.value()));
  REQUIRE(release.append_root(root.value()));
  REQUIRE(release.commit());
  CHECK(context.snapshot().roots() == std::vector<sai::node_id>{root->id()});
  CHECK(context.snapshot().lookup(root->id()));
}

TEST_CASE("external declarations expose honest explicit load states") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  const std::vector interface{test_interface_field(
      "translation", sai::value_kind::sf_vec3f, sai::access_type::input_output,
      sai::value{sai::vec3f{}}, {}, "length")};
  auto author = context.edit();
  auto local =
      author.add_local_declaration(test_local_declaration("Local", interface));
  auto external = author.add_external_declaration(test_external_declaration(
      "Remote", {"first.x3d#Local", "fallback.x3d#Local"}, interface));
  REQUIRE(local);
  REQUIRE(external);
  REQUIRE(author.commit());

  const auto transition = [&](sai::external_load_state state,
                              std::string diagnostic,
                              std::optional<sai::declaration_id> target) {
    auto edit = context.edit();
    auto replacement = test_external_declaration(
        "Remote", {"first.x3d#Local", "fallback.x3d#Local"}, interface);
    replacement.load_state = state;
    replacement.diagnostic = std::move(diagnostic);
    replacement.resolved_declaration = target;
    REQUIRE(edit.update_declaration(external.value(), replacement));
    REQUIRE(edit.commit());
    const auto inspected = context.snapshot().describe(external.value());
    REQUIRE(inspected);
    return std::get<sai::external_declaration_descriptor>(inspected->payload);
  };

  auto loading =
      transition(sai::external_load_state::loading, {}, std::nullopt);
  CHECK(loading.load_state == sai::external_load_state::loading);
  CHECK_FALSE(loading.resolved_declaration);
  auto failed = transition(sai::external_load_state::failed, "network failure",
                           std::nullopt);
  CHECK(failed.load_state == sai::external_load_state::failed);
  CHECK(failed.diagnostic == "network failure");
  CHECK_FALSE(failed.resolved_declaration);
  auto resolved =
      transition(sai::external_load_state::resolved, {}, local->id());
  CHECK(resolved.load_state == sai::external_load_state::resolved);
  CHECK(resolved.resolved_declaration == local->id());
  CHECK(resolved.urls ==
        std::vector<std::string>{"first.x3d#Local", "fallback.x3d#Local"});
  auto unresolved =
      transition(sai::external_load_state::unresolved, {}, std::nullopt);
  CHECK(unresolved.load_state == sai::external_load_state::unresolved);
  CHECK_FALSE(unresolved.resolved_declaration);
}

TEST_CASE("resolved externals require and retain compatible local identity") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  const std::vector interface{
      test_interface_field("first", sai::value_kind::sf_vec3f,
                           sai::access_type::input_output,
                           sai::value{sai::vec3f{}}, {}, "length"),
      test_interface_field("second", sai::value_kind::sf_node,
                           sai::access_type::initialize_only,
                           sai::value{sai::node_id{}}, {"Transform"})};
  auto author = context.edit();
  auto local =
      author.add_local_declaration(test_local_declaration("Local", interface));
  auto external = author.add_external_declaration(
      test_external_declaration("Remote", {"remote.x3d#Local"}, interface));
  REQUIRE(local);
  REQUIRE(external);
  REQUIRE(author.commit());

  std::vector<std::vector<sai::interface_field_descriptor>> incompatible;
  incompatible.push_back(interface);
  incompatible.back()[0].name = "different";
  incompatible.push_back(interface);
  incompatible.back()[0].kind = sai::value_kind::sf_vec3d;
  incompatible.back()[0].default_value = sai::value{sai::vec3d{}};
  incompatible.push_back(interface);
  incompatible.back()[0].access = sai::access_type::initialize_only;
  incompatible.push_back(interface);
  std::ranges::reverse(incompatible.back());
  incompatible.push_back(interface);
  incompatible.back()[1].accepted_node_types = {"Group"};
  incompatible.push_back(interface);
  incompatible.back()[0].default_value = sai::value{sai::vec3f{1, 0, 0}};
  incompatible.push_back(interface);
  incompatible.back()[0].unit_category = "angle";
  for (const auto &candidate : incompatible) {
    auto reject = context.edit();
    auto wrong =
        test_external_declaration("Remote", {"remote.x3d#Local"}, candidate);
    wrong.load_state = sai::external_load_state::resolved;
    wrong.resolved_declaration = local->id();
    const auto mismatch = reject.update_declaration(external.value(), wrong);
    REQUIRE_FALSE(mismatch);
    CHECK(mismatch.error().code == sai::error_code::invalid_descriptor);
    REQUIRE_FALSE(reject.commit());
  }

  auto resolve = context.edit();
  auto compatible =
      test_external_declaration("Remote", {"remote.x3d#Local"}, interface);
  compatible.load_state = sai::external_load_state::resolved;
  compatible.resolved_declaration = local->id();
  REQUIRE(resolve.update_declaration(external.value(), compatible));
  REQUIRE(resolve.commit());

  auto rename = context.edit();
  REQUIRE(rename.rename_declaration(local.value(), "RenamedLocal"));
  REQUIRE(rename.commit());
  CHECK(std::get<sai::external_declaration_descriptor>(
            context.snapshot().describe(external.value())->payload)
            .resolved_declaration == local->id());

  auto mutate = context.edit();
  auto changed_local = test_local_declaration("RenamedLocal", interface);
  changed_local.interface[0].kind = sai::value_kind::sf_vec3d;
  changed_local.interface[0].default_value = sai::value{sai::vec3d{}};
  const auto incompatible_target =
      mutate.update_declaration(local.value(), changed_local);
  REQUIRE_FALSE(incompatible_target);
  CHECK(incompatible_target.error().code ==
        sai::error_code::declaration_in_use);

  auto removal = context.edit();
  const auto retained = removal.remove_declaration(local.value());
  REQUIRE_FALSE(retained);
  CHECK(retained.error().code == sai::error_code::declaration_in_use);

  auto release = context.edit();
  auto failed = compatible;
  failed.load_state = sai::external_load_state::failed;
  failed.resolved_declaration.reset();
  failed.diagnostic = "not found";
  REQUIRE(release.update_declaration(external.value(), failed));
  REQUIRE(release.remove_declaration(local.value()));
  REQUIRE(release.commit());
}

TEST_CASE("node-valued writes require a context-bearing node handle") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto target = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(target);
  auto target_field = author.field(link.value(), "target");
  REQUIRE(target_field);
  REQUIRE(author.set(target_field.value(), target.value()));
  REQUIRE(author.append_root(link.value()));
  REQUIRE(author.commit());
  CHECK(context.snapshot().read(target_field.value()).value() ==
        sai::value{target.value().id()});

  auto bare_id = context.edit();
  auto denied_bare =
      bare_id.set(target_field.value(), sai::value{target.value().id()});
  REQUIRE_FALSE(denied_bare);
  CHECK(denied_bare.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(bare_id.commit());

  sai::browser foreign_host{graph_registry()};
  auto foreign_context = foreign_host.current_scene();
  auto foreign_author = foreign_context.edit();
  auto foreign_target = foreign_author.create_node("Transform");
  REQUIRE(foreign_target);
  REQUIRE(foreign_author.commit());

  auto cross_domain = context.edit();
  auto denied_foreign =
      cross_domain.set(target_field.value(), foreign_target.value());
  REQUIRE_FALSE(denied_foreign);
  CHECK(denied_foreign.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(cross_domain.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("node ranges validate every element before staging") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(first);
  REQUIRE(second);
  auto targets = author.field(link.value(), "targets");
  REQUIRE(targets);
  const std::array nodes{first.value(), second.value()};
  REQUIRE(author.set(targets.value(), std::span<const sai::node>{nodes}));
  REQUIRE(author.commit());
  CHECK(context.snapshot().read(targets.value()).value() ==
        sai::value{sai::node_list{first.value().id(), second.value().id()}});

  sai::browser foreign_host{graph_registry()};
  auto foreign_edit = foreign_host.current_scene().edit();
  auto foreign = foreign_edit.create_node("Transform");
  REQUIRE(foreign);
  REQUIRE(foreign_edit.commit());

  auto invalid = context.edit();
  const std::array mixed{first.value(), foreign.value()};
  auto denied = invalid.set(targets.value(), std::span<const sai::node>{mixed});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("one invalid write poisons the edit and publishes nothing") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto base = context.snapshot();
  auto translation = base.field(transform.value(), "translation");
  auto output = base.field(transform.value(), "worldTranslation");
  REQUIRE(translation);
  REQUIRE(output);

  auto edit = context.edit();
  REQUIRE(edit.set(translation.value(), sai::vec3f{4, 5, 6}));
  auto denied = edit.set(output.value(), sai::vec3f{7, 8, 9});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);
  CHECK(denied.error().operation == "scene_edit.set");

  auto rejected = edit.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::access_denied);
  CHECK(context.snapshot().revision() == base.revision());
  CHECK(context.snapshot().read(translation.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("field access is a lifecycle rule rather than a setter shape") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  auto initial = author.field(transform.value(), "initialLabel");
  REQUIRE(initial);
  REQUIRE(author.set(initial.value(), std::string{"authored"}));
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto snapshot = context.snapshot();
  auto input = snapshot.field(transform.value(), "set_translation");
  REQUIRE(input);
  auto unreadable = snapshot.read(input.value());
  REQUIRE_FALSE(unreadable);
  CHECK(unreadable.error().code == sai::error_code::access_denied);

  auto relabel = context.edit();
  auto denied_initial = relabel.set(initial.value(), std::string{"too late"});
  REQUIRE_FALSE(denied_initial);
  CHECK(denied_initial.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(relabel.commit());

  auto input_edit = context.edit();
  auto denied_input = input_edit.set(input.value(), sai::vec3f{1, 2, 3});
  REQUIRE_FALSE(denied_input);
  CHECK(denied_input.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(input_edit.commit());
  CHECK(context.snapshot().read(initial.value()).value() ==
        sai::value{std::string{"authored"}});
}

TEST_CASE("field capability queries predict every lifecycle intent") {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "AccessTable",
      {
          test_field("initial", sai::value_kind::sf_int32,
                     sai::access_type::initialize_only, std::int32_t{}),
          test_field("input", sai::value_kind::sf_int32,
                     sai::access_type::input_only, std::int32_t{}),
          test_field("output", sai::value_kind::sf_int32,
                     sai::access_type::output_only, std::int32_t{}),
          test_field("duplex", sai::value_kind::sf_int32,
                     sai::access_type::input_output, std::int32_t{}),
      })));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  auto owner = author.create_node("AccessTable");
  REQUIRE(owner);
  auto initial = author.field(owner.value(), "initial");
  auto input = author.field(owner.value(), "input");
  auto output = author.field(owner.value(), "output");
  auto duplex = author.field(owner.value(), "duplex");
  REQUIRE(initial);
  REQUIRE(input);
  REQUIRE(output);
  REQUIRE(duplex);

  CHECK(
      author.writable(initial.value(), sai::write_intent::initialize).value());
  CHECK_FALSE(
      author.writable(input.value(), sai::write_intent::initialize).value());
  CHECK_FALSE(
      author.writable(output.value(), sai::write_intent::initialize).value());
  CHECK(author.writable(duplex.value(), sai::write_intent::initialize).value());
  CHECK_FALSE(
      author.writable(initial.value(), sai::write_intent::retained).value());
  CHECK(author.writable(duplex.value(), sai::write_intent::retained).value());
  CHECK_FALSE(
      author.writable(input.value(), sai::write_intent::event_input).value());
  CHECK_FALSE(
      author.writable(duplex.value(), sai::write_intent::event_input).value());
  CHECK_FALSE(author.writable(output.value(), sai::write_intent::runtime_output)
                  .value());
  CHECK_FALSE(author.writable(duplex.value(), sai::write_intent::runtime_output)
                  .value());
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  CHECK(snapshot.readable(initial.value()).value());
  CHECK_FALSE(snapshot.readable(input.value()).value());
  CHECK(snapshot.readable(output.value()).value());
  CHECK(snapshot.readable(duplex.value()).value());
  auto events = context.events(sai::event_time{41.0});
  CHECK(events.writable(input.value(), sai::write_intent::event_input).value());
  CHECK(
      events.writable(duplex.value(), sai::write_intent::event_input).value());
  CHECK_FALSE(
      events.writable(output.value(), sai::write_intent::event_input).value());
  CHECK_FALSE(events.writable(output.value(), sai::write_intent::runtime_output)
                  .value());
  auto live = context.edit();
  CHECK_FALSE(
      live.writable(initial.value(), sai::write_intent::initialize).value());
  CHECK(live.writable(duplex.value(), sai::write_intent::retained).value());

  auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  auto stale_readability = snapshot.readable(duplex.value());
  REQUIRE(stale_readability);
  CHECK(stale_readability.value());
  auto stale_writability =
      live.writable(duplex.value(), sai::write_intent::retained);
  REQUIRE_FALSE(stale_writability);
  CHECK(stale_writability.error().code == sai::error_code::stale_handle);
}

TEST_CASE(
    "authored unit boundaries convert exactly once around canonical state") {
  sai::type_registry registry;
  auto distance = test_field("distance", sai::value_kind::sf_vec3f,
                             sai::access_type::input_output, sai::vec3f{});
  distance.unit_category = "length";
  auto invalid_units =
      test_field("label", sai::value_kind::sf_string,
                 sai::access_type::input_output, std::string{});
  invalid_units.unit_category = "length";
  auto invalid_type = registry.define(
      test_node_type("InvalidUnits", {std::move(invalid_units)}));
  REQUIRE_FALSE(invalid_type);
  CHECK(invalid_type.error().code == sai::error_code::invalid_descriptor);
  REQUIRE(registry.define(test_node_type("Measure", {std::move(distance)})));
  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  REQUIRE(author.declare_unit(sai::unit_declaration{
      .category = "length", .name = "centimetre", .conversion_factor = 0.01}));
  auto source = author.create_node("Measure");
  auto sink = author.create_node("Measure");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_distance = author.field(source.value(), "distance");
  auto sink_distance = author.field(sink.value(), "distance");
  REQUIRE(source_distance);
  REQUIRE(sink_distance);
  REQUIRE(author.set(source_distance.value(), sai::value{sai::vec3f{100, 0, 0}},
                     sai::value_space::authored));
  REQUIRE(author.add_route(source_distance.value(), sink_distance.value()));
  auto authored = author.commit();
  REQUIRE(authored);
  const auto &unit_change = require_graph_change(authored.value().changes[0]);
  CHECK(unit_change.kind == sai::change_kind::unit_declared);
  CHECK(unit_change.before == sai::value{std::string{"centimetre"}});
  CHECK(unit_change.after == sai::value{0.01});

  auto snapshot = context.snapshot();
  REQUIRE(snapshot.units().size() == 1);
  CHECK(snapshot.units()[0].name == "centimetre");
  CHECK(snapshot.read(source_distance.value()).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
  CHECK(snapshot.read(source_distance.value(), sai::value_space::authored)
            .value() == sai::value{sai::vec3f{100, 0, 0}});

  auto events = context.events(sai::event_time{70.0});
  REQUIRE(events.send(source_distance.value(),
                      sai::value{sai::vec3f{250, 0, 0}},
                      sai::value_space::authored));
  auto delivered = events.commit();
  REQUIRE(delivered);
  REQUIRE(delivered.value().deliveries.size() == 2);
  CHECK(delivered.value().deliveries[0].payload ==
        sai::value{sai::vec3f{2.5F, 0, 0}});
  CHECK(delivered.value().deliveries[1].payload ==
        sai::value{sai::vec3f{2.5F, 0, 0}});
  snapshot = context.snapshot();
  CHECK(snapshot.read(sink_distance.value()).value() ==
        sai::value{sai::vec3f{2.5F, 0, 0}});
  CHECK(snapshot.read(sink_distance.value(), sai::value_space::authored)
            .value() == sai::value{sai::vec3f{250, 0, 0}});

  auto expose = context.edit();
  REQUIRE(expose.export_node("Measured", sink.value()));
  REQUIRE(expose.commit());
  auto importer = host.create_scene();
  auto import_edit = importer.edit();
  REQUIRE(import_edit.import_node("ImportedMeasure", context, "Measured"));
  REQUIRE(import_edit.commit());
  auto imported_snapshot = importer.snapshot();
  auto imported_node = imported_snapshot.imported("ImportedMeasure");
  REQUIRE(imported_node);
  auto imported_distance =
      imported_snapshot.field(imported_node.value(), "distance");
  REQUIRE(imported_distance);
  CHECK(imported_snapshot
            .read(imported_distance.value(), sai::value_space::canonical)
            .value() == sai::value{sai::vec3f{2.5F, 0, 0}});
  CHECK(imported_snapshot
            .read(imported_distance.value(), sai::value_space::authored)
            .value() == sai::value{sai::vec3f{250, 0, 0}});

  sai::type_registry late_registry;
  REQUIRE(late_registry.define(test_node_type("Measure", {})));
  sai::browser late_host{std::move(late_registry)};
  auto late_context = late_host.current_scene();
  auto late_edit = late_context.edit();
  REQUIRE(late_edit.create_node("Measure"));
  auto late_declaration = late_edit.declare_unit(sai::unit_declaration{
      .category = "length", .name = "centimetre", .conversion_factor = 0.01});
  REQUIRE_FALSE(late_declaration);
  CHECK(late_declaration.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(late_edit.commit());
  CHECK(late_context.snapshot().revision() == 0);
}

TEST_CASE("inputOnly event intent is transient and access governed") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  auto input = snapshot.field(transform.value(), "set_translation");
  auto output = snapshot.field(transform.value(), "worldTranslation");
  auto initial = snapshot.field(transform.value(), "initialLabel");
  REQUIRE(input);
  REQUIRE(output);
  REQUIRE(initial);

  auto events = context.events(sai::event_time{42.0});
  REQUIRE(events.send(input.value(), sai::vec3f{1, 2, 3}));
  auto delivered = events.commit();
  REQUIRE(delivered);
  CHECK(delivered.value().time == sai::event_time{42.0});
  CHECK(delivered.value().before_revision == snapshot.revision());
  CHECK(delivered.value().after_revision == snapshot.revision());
  REQUIRE(delivered.value().deliveries.size() == 1);
  CHECK(delivered.value().deliveries[0].target == transform.value().id());
  CHECK(delivered.value().deliveries[0].field == "set_translation");
  CHECK(delivered.value().deliveries[0].payload ==
        sai::value{sai::vec3f{1, 2, 3}});
  CHECK(context.snapshot().revision() == snapshot.revision());
  CHECK_FALSE(context.snapshot().read(input.value()));

  auto denied_output = context.events(sai::event_time{43.0});
  auto output_result = denied_output.send(output.value(), sai::vec3f{4, 5, 6});
  REQUIRE_FALSE(output_result);
  CHECK(output_result.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(denied_output.commit());

  auto denied_initial = context.events(sai::event_time{43.0});
  auto initial_result =
      denied_initial.send(initial.value(), std::string{"late"});
  REQUIRE_FALSE(initial_result);
  CHECK(initial_result.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(denied_initial.commit());
  CHECK(context.snapshot().revision() == snapshot.revision());
}

TEST_CASE("inputOutput event intent publishes one coherent retained revision") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto before = context.snapshot();
  auto dynamic = before.field(transform.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  auto events = context.events(sai::event_time{50.0});
  REQUIRE(events.send(typed.value(), sai::vec3f{4, 5, 6}));
  auto delivered = events.commit();
  REQUIRE(delivered);
  CHECK(delivered.value().before_revision == before.revision());
  CHECK(delivered.value().after_revision == before.revision() + 1);
  REQUIRE(delivered.value().state_changes);
  REQUIRE(delivered.value().state_changes->changes.size() == 1);
  const auto &delivered_change =
      require_graph_change(delivered.value().state_changes->changes[0]);
  CHECK(delivered_change.kind == sai::change_kind::field_changed);
  CHECK(delivered_change.before == sai::value{sai::vec3f{}});
  CHECK(delivered_change.after == sai::value{sai::vec3f{4, 5, 6}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});
  CHECK(context.snapshot().read(dynamic.value()).value() ==
        sai::value{sai::vec3f{4, 5, 6}});
}

TEST_CASE("typed and dynamic event intent have equal traces and failures") {
  sai::browser dynamic_host{graph_registry()};
  auto dynamic_context = dynamic_host.current_scene();
  auto dynamic_author = dynamic_context.edit();
  auto dynamic_node = dynamic_author.create_node("Transform");
  REQUIRE(dynamic_node);
  REQUIRE(dynamic_author.commit());
  auto dynamic_field =
      dynamic_context.snapshot().field(dynamic_node.value(), "translation");
  auto dynamic_output = dynamic_context.snapshot().field(dynamic_node.value(),
                                                         "worldTranslation");
  REQUIRE(dynamic_field);
  REQUIRE(dynamic_output);

  sai::browser typed_host{graph_registry()};
  auto typed_context = typed_host.current_scene();
  auto typed_author = typed_context.edit();
  auto typed_node = typed_author.create_node("Transform");
  REQUIRE(typed_node);
  REQUIRE(typed_author.commit());
  auto typed_dynamic =
      typed_context.snapshot().field(typed_node.value(), "translation");
  auto typed_output_dynamic =
      typed_context.snapshot().field(typed_node.value(), "worldTranslation");
  REQUIRE(typed_dynamic);
  REQUIRE(typed_output_dynamic);
  auto typed_field = typed_dynamic.value().as<sai::vec3f>();
  auto typed_output = typed_output_dynamic.value().as<sai::vec3f>();
  REQUIRE(typed_field);
  REQUIRE(typed_output);

  auto dynamic_events = dynamic_context.events(sai::event_time{55.0});
  REQUIRE(dynamic_events.send(dynamic_field.value(),
                              sai::value{sai::vec3f{7, 8, 9}}));
  auto dynamic_result = dynamic_events.commit();
  REQUIRE(dynamic_result);
  auto typed_events = typed_context.events(sai::event_time{55.0});
  REQUIRE(typed_events.send(typed_field.value(), sai::vec3f{7, 8, 9}));
  auto typed_result = typed_events.commit();
  REQUIRE(typed_result);

  CHECK(dynamic_result.value().time == typed_result.value().time);
  CHECK(dynamic_result.value().before_revision ==
        typed_result.value().before_revision);
  CHECK(dynamic_result.value().after_revision ==
        typed_result.value().after_revision);
  REQUIRE(dynamic_result.value().deliveries.size() == 1);
  REQUIRE(typed_result.value().deliveries.size() == 1);
  const auto &dynamic_delivery = dynamic_result.value().deliveries.front();
  const auto &typed_delivery = typed_result.value().deliveries.front();
  CHECK(dynamic_delivery.target == typed_delivery.target);
  CHECK(dynamic_delivery.field == typed_delivery.field);
  CHECK(dynamic_delivery.payload == typed_delivery.payload);
  CHECK(dynamic_delivery.cause == typed_delivery.cause);
  CHECK(dynamic_delivery.route_index == typed_delivery.route_index);
  CHECK(dynamic_context.snapshot().read(dynamic_field.value()).value() ==
        sai::value{typed_context.snapshot().read(typed_field.value()).value()});

  auto denied_dynamic = dynamic_context.events(sai::event_time{56.0});
  auto dynamic_failure = denied_dynamic.send(dynamic_output.value(),
                                             sai::value{sai::vec3f{1, 1, 1}});
  auto denied_typed = typed_context.events(sai::event_time{56.0});
  auto typed_failure =
      denied_typed.send(typed_output.value(), sai::vec3f{1, 1, 1});
  REQUIRE_FALSE(dynamic_failure);
  REQUIRE_FALSE(typed_failure);
  CHECK(dynamic_failure.error().code == typed_failure.error().code);
  CHECK(dynamic_failure.error().operation == typed_failure.error().operation);
  CHECK(dynamic_failure.error().node == typed_failure.error().node);
  CHECK(dynamic_failure.error().field == typed_failure.error().field);
}

TEST_CASE("typed and dynamic MF events preserve the complete owning payload") {
  const auto exercise = [](bool typed) -> sai::event_result {
    sai::type_registry registry;
    REQUIRE(registry.define(test_node_type(
        "Samples",
        {test_field("values", sai::value_kind::mf_int32,
                    sai::access_type::input_output, sai::int32_list{})})));
    sai::browser host{std::move(registry)};
    auto context = host.current_scene();
    auto author = context.edit();
    auto owner = author.create_node("Samples");
    REQUIRE(owner);
    REQUIRE(author.commit());
    auto dynamic = context.snapshot().field(owner.value(), "values");
    REQUIRE(dynamic);
    auto events = context.events(sai::event_time{57.0});
    const sai::int32_list payload{3, 1, 4, 1, 5};
    if (typed) {
      auto field = dynamic.value().as<sai::int32_list>();
      REQUIRE(field);
      REQUIRE(events.send(field.value(), payload));
    } else {
      REQUIRE(events.send(dynamic.value(), sai::value{payload}));
    }
    auto result = events.commit();
    REQUIRE(result);
    return result.value();
  };

  const auto dynamic = exercise(false);
  const auto typed = exercise(true);
  REQUIRE(dynamic.deliveries.size() == 1);
  REQUIRE(typed.deliveries.size() == 1);
  CHECK(sai::same_representation(dynamic.deliveries[0].payload,
                                 typed.deliveries[0].payload));
  REQUIRE(dynamic.state_changes);
  REQUIRE(typed.state_changes);
  REQUIRE(dynamic.state_changes->changes.size() == 1);
  REQUIRE(typed.state_changes->changes.size() == 1);
  CHECK(sai::same_representation(
      require_graph_change(dynamic.state_changes->changes[0]).after,
      require_graph_change(typed.state_changes->changes[0]).after));
}

TEST_CASE("node event payloads require context-bearing handles and ranges") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(author.commit());
  auto snapshot = context.snapshot();
  auto target = snapshot.field(link.value(), "target");
  auto targets = snapshot.field(link.value(), "targets");
  REQUIRE(target);
  REQUIRE(targets);

  auto events = context.events(sai::event_time{60.0});
  REQUIRE(events.send(target.value(), first.value()));
  const std::array nodes{first.value(), second.value()};
  REQUIRE(events.send(targets.value(), std::span<const sai::node>{nodes}));
  REQUIRE(events.commit());
  CHECK(context.snapshot().read(target.value()).value() ==
        sai::value{first.value().id()});
  CHECK(context.snapshot().read(targets.value()).value() ==
        sai::value{sai::node_list{first.value().id(), second.value().id()}});

  sai::browser foreign_host{graph_registry()};
  auto foreign_edit = foreign_host.current_scene().edit();
  auto foreign = foreign_edit.create_node("Transform");
  REQUIRE(foreign);
  REQUIRE(foreign_edit.commit());
  auto invalid = context.events(sai::event_time{61.0});
  auto denied = invalid.send(target.value(), foreign.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().revision() == 2);
}

TEST_CASE("a multi-hop route cascade preserves time and causal identity") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  auto third = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(third);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  auto third_value = author.field(third.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(third_value);
  REQUIRE(author.add_route(first_value.value(), second_value.value()));
  REQUIRE(author.add_route(second_value.value(), third_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{70.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{7, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 3);
  CHECK(cascade.value().deliveries[0].time == sai::event_time{70.0});
  CHECK_FALSE(cascade.value().deliveries[0].cause);
  CHECK_FALSE(cascade.value().deliveries[0].route_index);
  CHECK(cascade.value().deliveries[1].target == second.value().id());
  CHECK(cascade.value().deliveries[1].cause == 0);
  CHECK(cascade.value().deliveries[1].route_index == 0);
  CHECK(cascade.value().deliveries[2].target == third.value().id());
  CHECK(cascade.value().deliveries[2].cause == 1);
  CHECK(cascade.value().deliveries[2].route_index == 1);
  REQUIRE(cascade.value().state_changes);
  CHECK(cascade.value().state_changes->changes.size() == 3);
  CHECK(context.snapshot().read(third_value.value()).value() ==
        sai::value{sai::vec3f{7, 0, 0}});
}

TEST_CASE("routed node events obey the destination accepted-node constraint") {
  auto source_value =
      test_field("value", sai::value_kind::sf_node,
                 sai::access_type::input_output, sai::node_id{});
  source_value.accepted_node_types = {"Box"};
  auto sink_value = test_field("value", sai::value_kind::sf_node,
                               sai::access_type::input_output, sai::node_id{});
  sink_value.accepted_node_types = {"Shape"};

  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type("Box", {})));
  REQUIRE(registry.define(test_node_type("Shape", {})));
  REQUIRE(
      registry.define(test_node_type("NodeSource", {std::move(source_value)})));
  REQUIRE(registry.define(test_node_type("NodeSink", {std::move(sink_value)})));

  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  auto box = author.create_node("Box");
  auto source = author.create_node("NodeSource");
  auto sink = author.create_node("NodeSink");
  REQUIRE(box);
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "value");
  auto input = author.field(sink.value(), "value");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{69.0});
  REQUIRE(events.send(output.value(), box.value()));
  auto rejected = events.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  CHECK(rejected.error().operation == "event_batch.commit");
  CHECK(rejected.error().node == sink.value().id());
  CHECK(rejected.error().field == "value");
  CHECK(context.snapshot().revision() == 1);
  CHECK(context.snapshot().read(input.value()).value() ==
        sai::value{sai::node_id{}});
}

TEST_CASE("a route cycle reaches quiescence by firing each route once") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(author.add_route(first_value.value(), second_value.value()));
  REQUIRE(author.add_route(second_value.value(), first_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{71.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{1, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 3);
  CHECK(cascade.value().deliveries[1].route_index == 0);
  CHECK(cascade.value().deliveries[2].route_index == 1);
  CHECK(cascade.value().deliveries[2].target == first.value().id());
  REQUIRE(cascade.value().state_changes);
  CHECK(cascade.value().state_changes->changes.size() == 2);
}

TEST_CASE("fan-in preserves the trace and exposes nonportable ordering") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(sink);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  auto sink_value = author.field(sink.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(sink_value);
  REQUIRE(author.add_route(first_value.value(), sink_value.value()));
  REQUIRE(author.add_route(second_value.value(), sink_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{80.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(events.send(second_value.value(), sai::vec3f{2, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 4);
  REQUIRE(cascade.value().portability_issues.size() == 1);
  CHECK(cascade.value().portability_issues[0].code ==
        sai::event_issue_code::nonportable_fan_in);
  CHECK(cascade.value().portability_issues[0].target == sink.value().id());
  CHECK(cascade.value().portability_issues[0].field == "translation");
  CHECK(cascade.value().portability_issues[0].delivery_indices ==
        std::vector<std::size_t>{2, 3});
  CHECK(context.snapshot().read(sink_value.value()).value() ==
        sai::value{sai::vec3f{2, 0, 0}});
}

TEST_CASE("duplicate event seeds poison the complete batch") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();
  const auto revision = context.snapshot().revision();

  auto events = context.events(sai::event_time{81.0});
  REQUIRE(events.send(translation, sai::vec3f{1, 0, 0}));
  auto duplicate = events.send(translation, sai::vec3f{2, 0, 0});
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::ambiguous_event_seed);
  REQUIRE_FALSE(events.commit());
  CHECK(context.snapshot().revision() == revision);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("specialized containment mutation obeys the field lifecycle") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto parent = author.create_node("AccessNode");
  auto child = author.create_node("Transform");
  REQUIRE(parent);
  REQUIRE(child);
  REQUIRE(author.append(parent.value(), "initialChildren", child.value()));
  REQUIRE(author.append_root(parent.value()));
  REQUIRE(author.commit());

  auto late_initial = context.edit();
  auto denied_initial =
      late_initial.append(parent.value(), "initialChildren", child.value());
  REQUIRE_FALSE(denied_initial);
  CHECK(denied_initial.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(late_initial.commit());

  auto input = context.edit();
  auto denied_input =
      input.append(parent.value(), "receivedChildren", child.value());
  REQUIRE_FALSE(denied_input);
  CHECK(denied_input.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(input.commit());

  auto output = context.edit();
  auto denied_output =
      output.append(parent.value(), "observedChildren", child.value());
  REQUIRE_FALSE(denied_output);
  CHECK(denied_output.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(output.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("routes connect output-capable ports to input-capable ports") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "worldTranslation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto invalid = context.edit();
  auto initial_source = invalid.field(source.value(), "initialLabel");
  auto initial_sink = invalid.field(sink.value(), "initialLabel");
  REQUIRE(initial_source);
  REQUIRE(initial_sink);
  auto denied = invalid.add_route(initial_source.value(), initial_sink.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_route);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().routes().size() == 1);
}

TEST_CASE("duplicate route identity is rejected without publication") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "worldTranslation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto duplicate = context.edit();
  auto denied = duplicate.add_route(output.value(), input.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_route);
  REQUIRE_FALSE(duplicate.commit());
  CHECK(context.snapshot().revision() == 1);
  CHECK(context.snapshot().routes().size() == 1);
}

TEST_CASE("change sets preserve authored operation order") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create_node("Transform");
  REQUIRE(transform);
  auto translation = edit.field(transform.value(), "translation");
  REQUIRE(translation);
  REQUIRE(edit.set(translation.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(edit.append_root(transform.value()));

  auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 3);
  CHECK(require_graph_change(committed.value().changes[0]).kind ==
        sai::change_kind::node_created);
  CHECK(require_graph_change(committed.value().changes[1]).kind ==
        sai::change_kind::field_changed);
  CHECK(require_graph_change(committed.value().changes[2]).kind ==
        sai::change_kind::root_inserted);
}

TEST_CASE(
    "public semantic ranges stay pure derived and authored-order stable") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto source_context = host.create_scene();
  auto source_edit = source_context.edit();
  auto imported_source = source_edit.create_node("Transform");
  REQUIRE(imported_source);
  REQUIRE(source_edit.export_node("Remote", imported_source.value()));
  REQUIRE(source_edit.commit());

  auto author = context.edit();
  REQUIRE(author.declare_unit(sai::unit_declaration{
      .category = "length", .name = "millimetre", .conversion_factor = 0.001}));
  REQUIRE(author.declare_unit(sai::unit_declaration{
      .category = "angle", .name = "degree", .conversion_factor = 0.017}));
  auto group = author.create_node("Group");
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(group);
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(author.append(group.value(), "children", second.value()));
  REQUIRE(author.append(group.value(), "children", first.value()));
  REQUIRE(author.append_root(group.value()));
  REQUIRE(author.append_root(first.value()));
  REQUIRE(author.define_name("Second", second.value()));
  REQUIRE(author.define_name("First", first.value()));
  REQUIRE(author.export_node("FirstPublic", first.value()));
  REQUIRE(author.export_node("SecondPublic", second.value()));
  REQUIRE(author.import_node("RemoteLocal", source_context, "Remote"));
  auto first_output = author.field(first.value(), "worldTranslation");
  auto second_input = author.field(second.value(), "set_translation");
  auto second_output = author.field(second.value(), "worldTranslation");
  auto first_input = author.field(first.value(), "set_translation");
  REQUIRE(first_output);
  REQUIRE(second_input);
  REQUIRE(second_output);
  REQUIRE(first_input);
  REQUIRE(author.add_route(first_output.value(), second_input.value()));
  REQUIRE(author.add_route(second_output.value(), first_input.value()));
  auto changes = author.commit();
  REQUIRE(changes);
  REQUIRE(changes.value().changes.size() == 16);
  CHECK(require_graph_change(changes.value().changes[0]).kind ==
        sai::change_kind::unit_declared);
  CHECK(require_graph_change(changes.value().changes[0]).field == "length");
  CHECK(require_graph_change(changes.value().changes[1]).kind ==
        sai::change_kind::unit_declared);
  CHECK(require_graph_change(changes.value().changes[1]).field == "angle");
  CHECK(require_graph_change(changes.value().changes[2]).kind ==
        sai::change_kind::node_created);
  CHECK(require_graph_change(changes.value().changes.back()).kind ==
        sai::change_kind::route_added);

  const auto snapshot = context.snapshot();
  const auto revision = snapshot.revision();
  const auto units = snapshot.units();
  const auto roots = snapshot.roots();
  const auto occurrences = snapshot.occurrences();
  const auto names = snapshot.names();
  const auto exports = snapshot.exports();
  const auto imports = snapshot.imports();
  const auto routes = snapshot.routes();
  const auto capabilities = host.capabilities();
  auto children = snapshot.field(group.value(), "children");
  REQUIRE(children);
  const auto child_ids = snapshot.read(children.value());
  REQUIRE(child_ids);
  auto observation = context.observe([](const sai::change_set &) {
    FAIL("pure inspection queued a change notification");
  });

  for (int pass = 0; pass < 3; ++pass) {
    CHECK(snapshot.revision() == revision);
    CHECK(snapshot.units() == units);
    CHECK(snapshot.roots() == roots);
    CHECK(snapshot.names() == names);
    CHECK(snapshot.exports() == exports);
    CHECK(snapshot.imports() == imports);
    CHECK(snapshot.routes() == routes);
    CHECK(snapshot.read(children.value()).value() == child_ids.value());
    CHECK(snapshot.readable(children.value()).value());
    const auto repeated_occurrences = snapshot.occurrences();
    REQUIRE(repeated_occurrences.size() == occurrences.size());
    for (std::size_t index = 0; index < occurrences.size(); ++index) {
      CHECK(repeated_occurrences[index].node == occurrences[index].node);
      CHECK(repeated_occurrences[index].parent_occurrence ==
            occurrences[index].parent_occurrence);
      CHECK(repeated_occurrences[index].container_field ==
            occurrences[index].container_field);
      CHECK(repeated_occurrences[index].index == occurrences[index].index);
      CHECK(repeated_occurrences[index].path == occurrences[index].path);
    }
    for (std::size_t index = 0; index < names.size(); ++index)
      CHECK(snapshot.named(names[index].name).value().id() ==
            names[index].node);
    for (std::size_t index = 0; index < exports.size(); ++index)
      CHECK(snapshot.exported(exports[index].name).value().id() ==
            exports[index].node);
  }
  CHECK(context.snapshot().revision() == revision);
  CHECK(context.drain().delivered == 0);
  CHECK(observation.active());
  CHECK(capabilities.authoring);
  CHECK(capabilities.inspection);
  CHECK_FALSE(capabilities.live);
  CHECK_FALSE(capabilities.async_loading);
  CHECK_FALSE(capabilities.rendering);

  REQUIRE(units.size() == 2);
  CHECK(units[0].category == "length");
  CHECK(units[1].category == "angle");
  REQUIRE(roots.size() == 2);
  CHECK(roots[0] == group.value().id());
  CHECK(roots[1] == first.value().id());
  CHECK(std::get<sai::node_list>(child_ids.value()) ==
        sai::node_list{second.value().id(), first.value().id()});
  REQUIRE(names.size() == 2);
  CHECK(names[0].name == "Second");
  CHECK(names[1].name == "First");
  REQUIRE(exports.size() == 2);
  CHECK(exports[0].name == "FirstPublic");
  CHECK(exports[1].name == "SecondPublic");
  REQUIRE(imports.size() == 1);
  CHECK(imports[0].local_name == "RemoteLocal");
  REQUIRE(routes.size() == 2);
  CHECK(routes[0].source == first.value().id());
  CHECK(routes[1].source == second.value().id());
  REQUIRE(occurrences.size() == 4);
  CHECK(occurrences[0].node == group.value().id());
  CHECK(occurrences[1].node == second.value().id());
  CHECK(occurrences[2].node == first.value().id());
  CHECK(occurrences[3].node == first.value().id());

  std::vector<sai::occurrence> rebuilt;
  for (std::size_t root_index = 0; root_index < roots.size(); ++root_index) {
    const auto parent = rebuilt.size();
    rebuilt.push_back(sai::occurrence{.node = roots[root_index],
                                      .parent_occurrence = std::nullopt,
                                      .container_field = {},
                                      .index = root_index,
                                      .path = {root_index}});
    if (roots[root_index] != group.value().id())
      continue;
    const auto &ids = std::get<sai::node_list>(child_ids.value());
    for (std::size_t child_index = 0; child_index < ids.size(); ++child_index) {
      rebuilt.push_back(sai::occurrence{.node = ids[child_index],
                                        .parent_occurrence = parent,
                                        .container_field = "children",
                                        .index = child_index,
                                        .path = {root_index, child_index}});
    }
  }
  REQUIRE(rebuilt.size() == occurrences.size());
  for (std::size_t index = 0; index < rebuilt.size(); ++index) {
    CHECK(rebuilt[index].node == occurrences[index].node);
    CHECK(rebuilt[index].parent_occurrence ==
          occurrences[index].parent_occurrence);
    CHECK(rebuilt[index].container_field == occurrences[index].container_field);
    CHECK(rebuilt[index].index == occurrences[index].index);
    CHECK(rebuilt[index].path == occurrences[index].path);
  }

  observation.cancel();
  auto first_error = context.observe([](const sai::change_set &) {
    throw std::runtime_error("first diagnostic");
  });
  auto second_error = context.observe([](const sai::change_set &) {
    throw std::runtime_error("second diagnostic");
  });
  auto trigger = context.edit();
  auto translation = trigger.field(first.value(), "translation");
  REQUIRE(translation);
  REQUIRE(trigger.set(translation.value(), sai::vec3f{1, 2, 3}));
  REQUIRE(trigger.commit());
  const auto diagnostics = context.drain();
  REQUIRE(diagnostics.errors.size() == 2);
  CHECK(diagnostics.errors[0].message == "first diagnostic");
  CHECK(diagnostics.errors[1].message == "second diagnostic");
  CHECK(first_error.active());
  CHECK(second_error.active());
}

TEST_CASE("names and routes publish at the same revision as their nodes") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto source = edit.create_node("Transform");
  auto sink = edit.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_field = edit.field(source.value(), "translation");
  auto sink_field = edit.field(sink.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE(edit.define_name("Source", source.value()));
  REQUIRE(edit.define_name("Sink", sink.value()));
  REQUIRE(edit.add_route(source_field.value(), sink_field.value()));
  REQUIRE(edit.append_root(source.value()));
  REQUIRE(edit.append_root(sink.value()));
  REQUIRE(edit.commit());

  auto snapshot = context.snapshot();
  REQUIRE(snapshot.named("Source"));
  REQUIRE(snapshot.named("Sink"));
  CHECK(snapshot.named("Source").value().id() == source.value().id());
  REQUIRE(snapshot.routes().size() == 1);
  CHECK(snapshot.routes()[0].source == source.value().id());
  CHECK(snapshot.routes()[0].source_field == "translation");
  CHECK(snapshot.routes()[0].sink == sink.value().id());
  CHECK(snapshot.routes()[0].sink_field == "translation");
}

TEST_CASE(
    "context exports are ordered aliases while ordinary lookup stays local") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto sibling = host.create_scene();

  auto source_edit = source.edit();
  auto source_node = source_edit.create_node("Transform");
  REQUIRE(source_node);
  REQUIRE(source_edit.define_name("Local", source_node.value()));
  REQUIRE(source_edit.export_node("PublicTransform", source_node.value()));
  REQUIRE(source_edit.commit());

  auto sibling_edit = sibling.edit();
  auto sibling_node = sibling_edit.create_node("Transform");
  REQUIRE(sibling_node);
  REQUIRE(sibling_edit.define_name("Local", sibling_node.value()));
  REQUIRE(sibling_edit.commit());

  const auto source_snapshot = source.snapshot();
  const auto sibling_snapshot = sibling.snapshot();
  CHECK(source_snapshot.named("Local").value().id() ==
        source_node.value().id());
  CHECK(sibling_snapshot.named("Local").value().id() ==
        sibling_node.value().id());
  REQUIRE_FALSE(sibling_snapshot.exported("PublicTransform"));
  REQUIRE(source_snapshot.exports().size() == 1);
  CHECK(source_snapshot.exports()[0].name == "PublicTransform");
  CHECK(source_snapshot.exports()[0].node == source_node.value().id());
  CHECK(source_snapshot.exported("PublicTransform").value().id() ==
        source_node.value().id());

  auto remove = source.edit();
  REQUIRE(remove.remove_export(source_snapshot.exports().front()));
  REQUIRE(remove.commit());
  CHECK(source.snapshot().exports().empty());
  REQUIRE_FALSE(source.snapshot().exported("PublicTransform"));
}

TEST_CASE("imports cross only explicit same-browser apertures") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());

  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());
  const auto snapshot = parent.snapshot();
  REQUIRE(snapshot.imports().size() == 1);
  CHECK(snapshot.imports()[0].local_name == "Remote");
  CHECK(snapshot.imports()[0].source_generation == source.generation());
  CHECK(snapshot.imports()[0].exported_name == "Shared");
  CHECK(snapshot.imports()[0].target ==
        sai::semantic_node_id{source.generation(), shared.value().id()});
  auto imported = snapshot.imported("Remote");
  REQUIRE(imported);
  CHECK(imported.value().identity() == snapshot.imports()[0].target);
  CHECK(imported.value().local_name() == "Remote");

  sai::browser foreign_host{graph_registry()};
  auto foreign_source = foreign_host.current_scene();
  auto foreign_edit = foreign_source.edit();
  auto foreign_node = foreign_edit.create_node("Transform");
  REQUIRE(foreign_node);
  REQUIRE(foreign_edit.export_node("Foreign", foreign_node.value()));
  REQUIRE(foreign_edit.commit());
  auto denied_edit = parent.edit();
  auto denied = denied_edit.import_node("Denied", foreign_source, "Foreign");
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(denied_edit.commit());
  CHECK(parent.snapshot().imports().size() == 1);
}

TEST_CASE("DEF and import names share one atomic local collision domain") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());

  auto named_parent = host.create_scene();
  auto named_edit = named_parent.edit();
  auto local = named_edit.create_node("Transform");
  REQUIRE(local);
  REQUIRE(named_edit.define_name("Collision", local.value()));
  auto import_collision = named_edit.import_node("Collision", source, "Shared");
  REQUIRE_FALSE(import_collision);
  CHECK(import_collision.error().code == sai::error_code::duplicate_name);
  REQUIRE_FALSE(named_edit.commit());
  CHECK(named_parent.snapshot().revision() == 0);

  auto imported_parent = host.create_scene();
  auto imported_edit = imported_parent.edit();
  auto another_local = imported_edit.create_node("Transform");
  REQUIRE(another_local);
  REQUIRE(imported_edit.import_node("Collision", source, "Shared"));
  auto name_collision =
      imported_edit.define_name("Collision", another_local.value());
  REQUIRE_FALSE(name_collision);
  CHECK(name_collision.error().code == sai::error_code::duplicate_name);
  REQUIRE_FALSE(imported_edit.commit());
  CHECK(imported_parent.snapshot().revision() == 0);
}

TEST_CASE("source export remapping invalidates a staged import atomically") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto first = source_edit.create_node("Transform");
  auto second = source_edit.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(source_edit.export_node("Shared", first.value()));
  REQUIRE(source_edit.commit());

  auto staged = parent.edit();
  REQUIRE(staged.import_node("Remote", source, "Shared"));
  int callbacks = 0;
  auto observed = parent.observe([&](const sai::change_set &) { ++callbacks; });

  const auto source_before = source.snapshot();
  auto remap = source.edit();
  REQUIRE(remap.remove_export(source_before.exports().front()));
  REQUIRE(remap.export_node("Shared", second.value()));
  REQUIRE(remap.commit());

  auto rejected = staged.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::stale_aperture);
  CHECK(parent.snapshot().revision() == 0);
  CHECK(parent.snapshot().imports().empty());
  CHECK(parent.drain().delivered == 0);
  CHECK(callbacks == 0);
  CHECK(observed.active());
}

TEST_CASE("ordered imports are their own removal tokens") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto first = source_edit.create_node("Transform");
  auto second = source_edit.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(source_edit.export_node("First", first.value()));
  REQUIRE(source_edit.export_node("Second", second.value()));
  REQUIRE(source_edit.commit());

  auto imports = parent.edit();
  REQUIRE(imports.import_node("A", source, "First"));
  REQUIRE(imports.import_node("B", source, "Second"));
  REQUIRE(imports.commit());
  const auto before = parent.snapshot();
  REQUIRE(before.imports().size() == 2);
  CHECK(before.imports()[0].local_name == "A");
  CHECK(before.imports()[1].local_name == "B");

  auto remove = parent.edit();
  REQUIRE(remove.remove_import(before.imports().front()));
  auto committed = remove.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 1);
  CHECK(require_graph_change(committed.value().changes[0]).kind ==
        sai::change_kind::import_removed);
  const auto after = parent.snapshot();
  REQUIRE(after.imports().size() == 1);
  CHECK(after.imports()[0].local_name == "B");
}

TEST_CASE("committed imports retain source nodes across contexts") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto importer = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());

  auto import_edit = importer.edit();
  REQUIRE(import_edit.import_node("Remote", source, "Shared"));
  REQUIRE(import_edit.commit());

  auto blocked = source.edit();
  REQUIRE(blocked.remove_export(source.snapshot().exports().front()));
  const auto rejected = blocked.remove_node(shared.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::node_in_use);
  CHECK(rejected.error().operation == "scene_edit.remove_node");
  REQUIRE_FALSE(blocked.commit());
  CHECK(source.snapshot().lookup(shared->id()));

  auto detach = importer.edit();
  REQUIRE(detach.remove_import(importer.snapshot().imports().front()));
  REQUIRE(detach.commit());
  auto removal = source.edit();
  REQUIRE(removal.remove_export(source.snapshot().exports().front()));
  REQUIRE(removal.remove_node(shared.value()));
  REQUIRE(removal.commit());
  CHECK_FALSE(source.snapshot().lookup(shared->id()));

  auto staged_source = host.create_scene();
  auto staged_importer = host.create_scene();
  auto staged_author = staged_source.edit();
  auto staged_node = staged_author.create_node("Transform");
  REQUIRE(staged_node);
  REQUIRE(staged_author.export_node("Staged", staged_node.value()));
  REQUIRE(staged_author.commit());
  auto abandoned = staged_importer.edit();
  REQUIRE(abandoned.import_node("Remote", staged_source, "Staged"));

  auto unblocked = staged_source.edit();
  REQUIRE(unblocked.remove_export(staged_source.snapshot().exports().front()));
  REQUIRE(unblocked.remove_node(staged_node.value()));
  REQUIRE(unblocked.commit());
}

TEST_CASE("a committed import wins over an already staged source removal") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto importer = host.create_scene();
  auto author = source.edit();
  auto shared = author.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(author.export_node("Shared", shared.value()));
  REQUIRE(author.commit());

  auto staged_removal = source.edit();
  REQUIRE(staged_removal.remove_export(source.snapshot().exports().front()));
  REQUIRE(staged_removal.remove_node(shared.value()));

  auto winner = importer.edit();
  REQUIRE(winner.import_node("Remote", source, "Shared"));
  REQUIRE(winner.commit());

  const auto rejected = staged_removal.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::node_in_use);
  CHECK(rejected.error().operation == "scene_edit.commit");
  CHECK(source.snapshot().lookup(shared->id()));
  CHECK(source.snapshot().exports().size() == 1);
  CHECK(importer.snapshot().imports().size() == 1);
}

TEST_CASE(
    "imported typed and dynamic reads capture immutable source revisions") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  auto translation = source_edit.field(shared.value(), "translation");
  REQUIRE(translation);
  REQUIRE(source_edit.set(translation.value(), sai::vec3f{1, 2, 3}));
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());
  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());

  const auto before = parent.snapshot();
  auto imported = before.imported("Remote");
  REQUIRE(imported);
  CHECK(imported.value().source_revision() == source.snapshot().revision());
  auto described = before.describe(imported.value());
  REQUIRE(described);
  CHECK(described.value().name == "Transform");
  auto dynamic = before.field(imported.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  CHECK(before.read(dynamic.value()).value() ==
        sai::value{sai::vec3f{1, 2, 3}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{1, 2, 3});

  auto source_update = source.edit();
  REQUIRE(source_update.set(translation.value(), sai::vec3f{4, 5, 6}));
  REQUIRE(source_update.commit());
  CHECK(before.read(typed.value()).value() == sai::vec3f{1, 2, 3});

  const auto after = parent.snapshot();
  auto current = after.imported("Remote");
  REQUIRE(current);
  CHECK(current.value().source_revision() == source.snapshot().revision());
  auto current_field = after.field(current.value(), "translation");
  REQUIRE(current_field);
  CHECK(after.read(current_field.value()).value() ==
        sai::value{sai::vec3f{4, 5, 6}});
}

TEST_CASE("imported authored reads retain their captured source authority") {
  sai::type_registry registry;
  auto distance = test_field("distance", sai::value_kind::sf_vec3f,
                             sai::access_type::input_output, sai::vec3f{});
  distance.unit_category = "length";
  REQUIRE(registry.define(test_node_type("Measure", {std::move(distance)})));
  sai::browser host{std::move(registry)};
  auto parent = host.current_scene();

  const auto [snapshot, imported_distance] = [&] {
    auto source = host.create_scene();
    auto source_edit = source.edit();
    REQUIRE(source_edit.declare_unit(
        sai::unit_declaration{.category = "length",
                              .name = "centimetre",
                              .conversion_factor = 0.01}));
    auto measured = source_edit.create_node("Measure");
    REQUIRE(measured);
    auto source_distance = source_edit.field(measured.value(), "distance");
    REQUIRE(source_distance);
    REQUIRE(source_edit.set(source_distance.value(),
                            sai::value{sai::vec3f{100, 0, 0}},
                            sai::value_space::authored));
    REQUIRE(source_edit.export_node("Measured", measured.value()));
    REQUIRE(source_edit.commit());

    auto import_edit = parent.edit();
    REQUIRE(import_edit.import_node("Remote", source, "Measured"));
    REQUIRE(import_edit.commit());
    auto captured = parent.snapshot();
    auto imported = captured.imported("Remote");
    REQUIRE(imported);
    auto field = captured.field(imported.value(), "distance");
    REQUIRE(field);
    return std::pair{std::move(captured), std::move(field).value()};
  }();

  CHECK(snapshot.read(imported_distance, sai::value_space::canonical).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
  CHECK(snapshot.read(imported_distance, sai::value_space::authored).value() ==
        sai::value{sai::vec3f{100, 0, 0}});
}

TEST_CASE("imported authority is inspection-only and context checked") {
  static_assert(!accepts_imported_containment<sai::scene_edit>);
  static_assert(!accepts_imported_retained_write<sai::scene_edit>);
  static_assert(!accepts_imported_event_target<sai::event_batch>);
  static_assert(!std::convertible_to<sai::imported_node, sai::node>);

  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto unrelated = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());
  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());

  const auto old_parent_snapshot = parent.snapshot();
  auto imported = old_parent_snapshot.imported("Remote");
  REQUIRE(imported);
  auto field = old_parent_snapshot.field(imported.value(), "translation");
  REQUIRE(field);
  auto wrong_context = unrelated.snapshot().describe(imported.value());
  REQUIRE_FALSE(wrong_context);
  CHECK(wrong_context.error().code == sai::error_code::invalid_context);

  int parent_callbacks = 0;
  auto observed =
      parent.observe([&](const sai::change_set &) { ++parent_callbacks; });
  auto source_update = source.edit();
  auto source_field = source_update.field(shared.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(source_update.set(source_field.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(source_update.commit());
  CHECK(parent.drain().delivered == 0);
  CHECK(parent_callbacks == 0);
  CHECK(observed.active());
  CHECK(old_parent_snapshot.read(field.value()).value() ==
        sai::value{sai::vec3f{}});

  REQUIRE(host.replace_world(parent));
  auto current_parent_snapshot = parent.snapshot();
  REQUIRE_FALSE(current_parent_snapshot.imported("Remote"));
  auto stale = current_parent_snapshot.describe(imported.value());
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_handle);
  CHECK(old_parent_snapshot.read(field.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("enumerated roots names and routes are their own removal tokens") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_field = author.field(source.value(), "translation");
  auto sink_field = author.field(sink.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE(author.append_root(source.value()));
  REQUIRE(author.append_root(sink.value()));
  REQUIRE(author.define_name("Source", source.value()));
  REQUIRE(author.add_route(source_field.value(), sink_field.value()));
  REQUIRE(author.commit());

  const auto before = context.snapshot();
  REQUIRE(before.roots().size() == 2);
  REQUIRE(before.routes().size() == 1);
  REQUIRE(before.lookup(before.roots().front()));
  CHECK(before.lookup(before.roots().front()).value().id() ==
        source.value().id());
  REQUIRE(before.names().size() == 1);
  CHECK(before.names().front().name == "Source");
  CHECK(before.names().front().node == source.value().id());

  auto missing = before.lookup(sai::node_id{999999});
  REQUIRE_FALSE(missing);
  CHECK(missing.error().code == sai::error_code::unknown_node);

  auto remove = context.edit();
  REQUIRE(remove.remove_root(0));
  REQUIRE(remove.undefine_name(before.names().front().name));
  REQUIRE(remove.remove_route(before.routes().front()));
  auto committed = remove.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 3);
  const auto &root_removed = require_graph_change(committed.value().changes[0]);
  CHECK(root_removed.kind == sai::change_kind::root_removed);
  CHECK(root_removed.before == sai::value{source.value().id()});
  CHECK(root_removed.index == 0);
  CHECK(require_graph_change(committed.value().changes[1]).kind ==
        sai::change_kind::name_removed);
  CHECK(require_graph_change(committed.value().changes[1]).field == "Source");
  CHECK(require_graph_change(committed.value().changes[2]).kind ==
        sai::change_kind::route_removed);

  const auto after = context.snapshot();
  REQUIRE(after.roots().size() == 1);
  CHECK(after.roots().front() == sink.value().id());
  CHECK_FALSE(after.named("Source"));
  CHECK(after.routes().empty());
}

TEST_CASE("optimistic edits conflict while old snapshots remain immutable") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto old_snapshot = context.snapshot();
  auto translation = old_snapshot.field(transform.value(), "translation");
  REQUIRE(translation);
  auto first = context.edit();
  auto competing = context.edit();
  REQUIRE(first.set(translation.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(competing.set(translation.value(), sai::vec3f{2, 0, 0}));
  REQUIRE(first.commit());

  auto conflict = competing.commit();
  REQUIRE_FALSE(conflict);
  CHECK(conflict.error().code == sai::error_code::stale_revision);
  CHECK(conflict.error().base_revision == old_snapshot.revision());
  CHECK(conflict.error().current_revision == context.snapshot().revision());
  CHECK(old_snapshot.read(translation.value()).value() ==
        sai::value{sai::vec3f{}});
  CHECK(context.snapshot().read(translation.value()).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
}

TEST_CASE("unreferenced node removal preserves historical snapshots") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(author.commit());

  const auto historical = context.snapshot();
  const auto historical_field = historical.field(first.value(), "translation");
  REQUIRE(historical_field);

  auto removal = context.edit();
  REQUIRE(removal.remove_node(first.value()));
  const auto committed = removal.commit();
  REQUIRE(committed);
  REQUIRE(committed->changes.size() == 1);
  const auto &removed = require_graph_change(committed->changes.front());
  CHECK(removed.kind == sai::change_kind::node_removed);
  CHECK(removed.node == first->id());

  const auto current = context.snapshot();
  CHECK(historical.read(historical_field.value()).value() ==
        sai::value{sai::vec3f{}});
  CHECK_FALSE(current.lookup(first->id()));
  CHECK(current.lookup(second->id()));
  CHECK(second->id().value > first->id().value);
}

TEST_CASE("node removal rejects retained semantic references") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  sai::node_id target_id;

  SUBCASE("root") {
    auto author = context.edit();
    auto target = author.create_node("Transform");
    REQUIRE(target);
    REQUIRE(author.append_root(target.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("shared containment occurrences") {
    auto author = context.edit();
    auto parent = author.create_node("Group");
    auto target = author.create_node("Transform");
    REQUIRE(parent);
    REQUIRE(target);
    REQUIRE(author.append(parent.value(), "children", target.value()));
    REQUIRE(author.append(parent.value(), "children", target.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("SFNode reference") {
    auto author = context.edit();
    auto owner = author.create_node("Link");
    auto target = author.create_node("Transform");
    REQUIRE(owner);
    REQUIRE(target);
    auto field = author.field(owner.value(), "target");
    REQUIRE(field);
    REQUIRE(author.set(field.value(), target.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("MFNode reference") {
    auto author = context.edit();
    auto owner = author.create_node("Link");
    auto target = author.create_node("Transform");
    REQUIRE(owner);
    REQUIRE(target);
    auto field = author.field(owner.value(), "targets");
    REQUIRE(field);
    const std::array targets{target.value()};
    REQUIRE(author.set(field.value(), std::span<const sai::node>{targets}));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("name") {
    auto author = context.edit();
    auto target = author.create_node("Transform");
    REQUIRE(target);
    REQUIRE(author.define_name("Target", target.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("export") {
    auto author = context.edit();
    auto target = author.create_node("Transform");
    REQUIRE(target);
    REQUIRE(author.export_node("Target", target.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  SUBCASE("route endpoint") {
    auto author = context.edit();
    auto target = author.create_node("Transform");
    auto sink = author.create_node("Transform");
    REQUIRE(target);
    REQUIRE(sink);
    auto source_field = author.field(target.value(), "worldTranslation");
    auto sink_field = author.field(sink.value(), "translation");
    REQUIRE(source_field);
    REQUIRE(sink_field);
    REQUIRE(author.add_route(source_field.value(), sink_field.value()));
    REQUIRE(author.commit());
    target_id = target->id();
  }

  const auto before = context.snapshot();
  const auto target = before.lookup(target_id);
  REQUIRE(target);
  auto removal = context.edit();
  const auto rejected = removal.remove_node(target.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::node_in_use);
  CHECK(rejected.error().operation == "scene_edit.remove_node");
  CHECK(rejected.error().node == target_id);
  const auto unpublished = removal.commit();
  REQUIRE_FALSE(unpublished);
  CHECK(unpublished.error().code == sai::error_code::node_in_use);
  CHECK(context.snapshot().revision() == before.revision());
  CHECK(context.snapshot().lookup(target_id));
}

TEST_CASE("explicit detachment and node removal publish atomically") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto target = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(target);
  REQUIRE(sink);
  auto source_field = author.field(target.value(), "worldTranslation");
  auto sink_field = author.field(sink.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE(author.append_root(target.value()));
  REQUIRE(author.define_name("Target", target.value()));
  REQUIRE(author.add_route(source_field.value(), sink_field.value()));
  REQUIRE(author.commit());

  const auto before = context.snapshot();
  auto removal = context.edit();
  REQUIRE(removal.remove_root(0));
  REQUIRE(removal.undefine_name("Target"));
  REQUIRE(removal.remove_route(before.routes().front()));
  REQUIRE(removal.remove_node(target.value()));
  const auto committed = removal.commit();
  REQUIRE(committed);
  REQUIRE(committed->changes.size() == 4);
  CHECK(require_graph_change(committed->changes[0]).kind ==
        sai::change_kind::root_removed);
  CHECK(require_graph_change(committed->changes[1]).kind ==
        sai::change_kind::name_removed);
  CHECK(require_graph_change(committed->changes[2]).kind ==
        sai::change_kind::route_removed);
  CHECK(require_graph_change(committed->changes[3]).kind ==
        sai::change_kind::node_removed);
  CHECK_FALSE(context.snapshot().lookup(target->id()));
}

static_assert(noexcept(std::declval<sai::node &>().dispose()));
static_assert(
    noexcept(std::declval<sai::typed_node<BoundTransform> &>().dispose()));

TEST_CASE("wrapper disposal is local idempotent and callback free") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto typed = author.create<BoundTransform>();
  REQUIRE(typed);
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  const auto revision = snapshot.revision();
  int callbacks = 0;
  auto observer =
      context.observe([&](const sai::change_set &) { ++callbacks; });

  auto typed_copy = typed.value();
  const auto typed_id = typed->id();
  typed->dispose();
  typed->dispose();
  CHECK(typed->disposed());
  CHECK(typed->id() == typed_id);
  CHECK_FALSE(typed_copy.disposed());
  CHECK(snapshot.describe(typed_copy.dynamic()));
  CHECK_FALSE(snapshot.describe(typed->dynamic()));

  auto dynamic = typed_copy.dynamic();
  auto dynamic_copy = dynamic;
  const auto dynamic_id = dynamic.id();
  dynamic.dispose();
  dynamic.dispose();
  CHECK(dynamic.disposed());
  CHECK(dynamic.id() == dynamic_id);
  CHECK_FALSE(dynamic_copy.disposed());
  CHECK(snapshot.describe(dynamic_copy));
  const auto disposed = snapshot.describe(dynamic);
  REQUIRE_FALSE(disposed);
  CHECK(disposed.error().code == sai::error_code::stale_handle);

  CHECK(context.snapshot().revision() == revision);
  CHECK(context.drain().delivered == 0);
  CHECK(callbacks == 0);
  CHECK(observer.active());
}

TEST_CASE("removed live handles become stale and node IDs are not recycled") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto removed = author.create_node("Transform");
  REQUIRE(removed);
  REQUIRE(author.commit());
  const auto removed_id = removed->id();

  auto removal = context.edit();
  REQUIRE(removal.remove_node(removed.value()));
  REQUIRE(removal.commit());
  const auto stale = context.snapshot().describe(removed.value());
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_handle);

  auto replacement = context.edit();
  auto created = replacement.create_node("Transform");
  REQUIRE(created);
  REQUIRE(replacement.commit());
  CHECK(created->id() != removed_id);
  CHECK(created->id().value > removed_id.value);
}

TEST_CASE("node removal makes an older event batch stale") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto target = author.create_node("Transform");
  REQUIRE(target);
  REQUIRE(author.commit());
  const auto input =
      context.snapshot().field(target.value(), "set_translation");
  REQUIRE(input);
  auto queued = context.events(sai::event_time{1.0});
  REQUIRE(queued.send(input.value(), sai::value{sai::vec3f{1, 2, 3}}));

  auto removal = context.edit();
  REQUIRE(removal.remove_node(target.value()));
  REQUIRE(removal.commit());

  const auto stale = queued.commit();
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_revision);
  CHECK(context.snapshot().revision() == 2);
  CHECK(context.drain().delivered == 0);
}

TEST_CASE("field observation retains its node until cancellation") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto target = author.create_node("Transform");
  REQUIRE(target);
  REQUIRE(author.commit());
  const auto field = context.snapshot().field(target.value(), "translation");
  REQUIRE(field);
  auto observer =
      context.observe(field.value(), [](const sai::event_delivery &) {});
  REQUIRE(observer);

  auto blocked = context.edit();
  const auto rejected = blocked.remove_node(target.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::node_in_use);
  CHECK(rejected.error().field == "translation");

  observer->cancel();
  auto removal = context.edit();
  REQUIRE(removal.remove_node(target.value()));
  REQUIRE(removal.commit());
}

TEST_CASE("undrained event notification retains its target node") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto target = author.create_node("Transform");
  REQUIRE(target);
  REQUIRE(author.commit());
  const auto field = context.snapshot().field(target.value(), "translation");
  REQUIRE(field);
  int callbacks = 0;
  auto observer = context.observe(
      field.value(), [&](const sai::event_delivery &) { ++callbacks; });
  REQUIRE(observer);
  auto events = context.events(sai::event_time{1.0});
  REQUIRE(events.send(field.value(), sai::value{sai::vec3f{1, 2, 3}}));
  REQUIRE(events.commit());
  observer->cancel();

  auto blocked = context.edit();
  const auto rejected = blocked.remove_node(target.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::node_in_use);
  CHECK(rejected.error().field == "translation");

  CHECK(context.drain().delivered == 0);
  CHECK(callbacks == 0);
  auto removal = context.edit();
  REQUIRE(removal.remove_node(target.value()));
  REQUIRE(removal.commit());
}

TEST_CASE(
    "world replacement invalidates handles without invalidating snapshots") {
  sai::browser host{graph_registry()};
  auto old_context = host.current_scene();
  auto author = old_context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto historical = old_context.snapshot();
  auto old_field = historical.field(transform.value(), "translation");
  REQUIRE(old_field);
  int retired_calls = 0;
  auto retired =
      old_context.observe([&](const sai::change_set &) { ++retired_calls; });
  CHECK(retired.active());

  auto replacement = host.create_scene();
  CHECK(replacement.generation() != old_context.generation());
  REQUIRE(host.replace_world(replacement));
  CHECK(host.current_scene().generation() == replacement.generation());
  CHECK_FALSE(retired.active());
  CHECK(old_context.drain().delivered == 0);
  CHECK(retired_calls == 0);

  auto foreign = replacement.snapshot().field(transform.value(), "translation");
  REQUIRE_FALSE(foreign);
  CHECK(foreign.error().code == sai::error_code::stale_handle);
  CHECK(historical.read(old_field.value()).value() == sai::value{sai::vec3f{}});

  auto stale_edit = old_context.edit();
  REQUIRE(stale_edit.set(old_field.value(), sai::vec3f{9, 9, 9}));
  auto stale_commit = stale_edit.commit();
  REQUIRE_FALSE(stale_commit);
  CHECK(stale_commit.error().code == sai::error_code::stale_handle);
}

TEST_CASE("a browser rejects an execution context from another browser") {
  sai::browser first{graph_registry()};
  sai::browser second{graph_registry()};
  auto foreign = second.create_scene();
  const auto original = first.current_scene().generation();

  auto rejected = first.replace_world(foreign);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_context);
  CHECK(first.current_scene().generation() == original);
}

static_assert(std::movable<sai::subscription>);
static_assert(!std::copy_constructible<sai::subscription>);

TEST_CASE("commit queues observation and drain permits a reentrant commit") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  std::vector<sai::revision_id> observed;
  auto subscription = context.observe([&](const sai::change_set &changes) {
    observed.push_back(changes.after_revision);
    if (observed.size() == 1) {
      auto nested = context.edit();
      REQUIRE(nested.set(translation, sai::vec3f{2, 0, 0}));
      REQUIRE(nested.commit());
    }
  });
  CHECK(subscription.active());

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());
  CHECK(observed.empty());

  auto report = context.drain();
  CHECK(report.delivered == 2);
  CHECK(report.errors.empty());
  REQUIRE(observed.size() == 2);
  CHECK(observed[0] + 1 == observed[1]);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{2, 0, 0}});
}

TEST_CASE("subscription cancellation and destruction are callback-free") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  int calls = 0;
  auto cancelled = context.observe([&](const sai::change_set &) { ++calls; });
  cancelled.cancel();
  CHECK_FALSE(cancelled.active());
  {
    auto destroyed = context.observe([&](const sai::change_set &) { ++calls; });
    CHECK(destroyed.active());
  }

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{3, 0, 0}));
  REQUIRE(edit.commit());
  CHECK(context.drain().delivered == 0);
  CHECK(calls == 0);
}

TEST_CASE("one throwing observer does not starve later observers") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  auto throwing = context.observe([](const sai::change_set &) {
    throw std::runtime_error("user callback");
  });
  int later_calls = 0;
  auto later = context.observe([&](const sai::change_set &) { ++later_calls; });
  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());

  auto report = context.drain();
  CHECK(report.delivered == 1);
  REQUIRE(report.errors.size() == 1);
  CHECK(report.errors[0].code == sai::error_code::callback_failed);
  CHECK(report.errors[0].operation == "execution_context.drain");
  CHECK(later_calls == 1);
  CHECK(throwing.active());
  CHECK(later.active());
}

TEST_CASE("dispatch does not copy user callables") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  auto armed = std::make_shared<bool>(false);
  int guarded_calls = 0;
  sai::execution_context::observer guarded{
      copy_bomb_observer{armed, guarded_calls}};
  auto first = context.observe(std::move(guarded));
  int later_calls = 0;
  auto later = context.observe([&](const sai::change_set &) { ++later_calls; });
  *armed = true;

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());
  sai::dispatch_report report;
  CHECK_NOTHROW(report = context.drain());
  CHECK(report.delivered == 2);
  CHECK(report.errors.empty());
  CHECK(guarded_calls == 1);
  CHECK(later_calls == 1);
  CHECK(first.active());
  CHECK(later.active());
}

TEST_CASE("field event observation is queued cancellable and access checked") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_value = author.field(source.value(), "translation");
  auto sink_value = author.field(sink.value(), "translation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(source_value);
  REQUIRE(sink_value);
  REQUIRE(input);
  REQUIRE(author.add_route(source_value.value(), sink_value.value()));
  REQUIRE(author.commit());

  std::vector<sai::event_delivery> observed;
  auto registered = context.observe(sink_value.value(),
                                    [&](const sai::event_delivery &delivery) {
                                      observed.push_back(delivery);
                                    });
  REQUIRE(registered);
  auto subscription = std::move(registered).value();
  auto denied =
      context.observe(input.value(), [](const sai::event_delivery &) {});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);

  auto events = context.events(sai::event_time{90.0});
  REQUIRE(events.send(source_value.value(), sai::vec3f{9, 0, 0}));
  REQUIRE(events.commit());
  CHECK(observed.empty());
  auto report = context.drain();
  CHECK(report.delivered == 1);
  CHECK(report.errors.empty());
  REQUIRE(observed.size() == 1);
  CHECK(observed[0].target == sink.value().id());
  CHECK(observed[0].time == sai::event_time{90.0});

  auto cancelled = context.observe(sink_value.value(),
                                   [&](const sai::event_delivery &delivery) {
                                     observed.push_back(delivery);
                                   });
  REQUIRE(cancelled);
  auto cancelled_subscription = std::move(cancelled).value();
  auto later = context.events(sai::event_time{91.0});
  REQUIRE(later.send(source_value.value(), sai::vec3f{10, 0, 0}));
  REQUIRE(later.commit());
  subscription.cancel();
  cancelled_subscription.cancel();
  CHECK(context.drain().delivered == 0);
  CHECK(observed.size() == 1);
}

TEST_CASE(
    "field observers isolate exceptions and permit a later reentrant batch") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_value = author.field(source.value(), "translation").value();
  auto sink_value = author.field(sink.value(), "translation").value();
  REQUIRE(author.add_route(source_value, sink_value));
  REQUIRE(author.commit());

  auto throwing = context.observe(sink_value, [](const sai::event_delivery &) {
    throw std::runtime_error("field observer");
  });
  REQUIRE(throwing);
  std::vector<sai::event_time> observed;
  auto reentrant =
      context.observe(sink_value, [&](const sai::event_delivery &delivery) {
        observed.push_back(delivery.time);
        if (observed.size() == 1) {
          auto nested = context.events(sai::event_time{101.0});
          REQUIRE(nested.send(source_value, sai::vec3f{2, 0, 0}));
          REQUIRE(nested.commit());
        }
      });
  REQUIRE(reentrant);

  auto initial = context.events(sai::event_time{100.0});
  REQUIRE(initial.send(source_value, sai::vec3f{1, 0, 0}));
  REQUIRE(initial.commit());
  auto report = context.drain();
  CHECK(report.delivered == 2);
  CHECK(report.errors.size() == 2);
  REQUIRE(observed.size() == 2);
  CHECK(observed[0] == sai::event_time{100.0});
  CHECK(observed[1] == sai::event_time{101.0});
}

TEST_CASE("event publication rejects timestamps that do not move forward") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();
  auto first = context.events(sai::event_time{110.0});
  REQUIRE(first.send(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(first.commit());
  const auto revision = context.snapshot().revision();

  auto same_time = context.events(sai::event_time{110.0});
  REQUIRE(same_time.send(translation, sai::vec3f{2, 0, 0}));
  auto rejected = same_time.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::event_time_regression);
  CHECK(context.snapshot().revision() == revision);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
}

TEST_CASE("cancelled and stale load completions are inert") {
  sai::browser host{graph_registry()};
  const auto original_generation = host.current_scene().generation();

  auto cancelled_ticket = host.begin_load();
  auto cancelled_candidate = host.create_scene();
  REQUIRE(host.cancel(cancelled_ticket));
  auto cancelled = host.complete_load(cancelled_ticket, cancelled_candidate);
  REQUIRE_FALSE(cancelled);
  CHECK(cancelled.error().code == sai::error_code::cancelled);
  CHECK(host.current_scene().generation() == original_generation);

  auto stale_ticket = host.begin_load();
  auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  auto stale_candidate = host.create_scene();
  auto stale = host.complete_load(stale_ticket, stale_candidate);
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_completion);
  CHECK(host.current_scene().generation() == replacement.generation());

  auto valid_ticket = host.begin_load();
  auto valid_candidate = host.create_scene();
  REQUIRE(host.complete_load(valid_ticket, valid_candidate));
  CHECK(host.current_scene().generation() == valid_candidate.generation());
}
