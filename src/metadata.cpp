#include "x3d/sai/experimental/metadata.hpp"

#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>

namespace x3d::sai::experimental {
namespace {

// This translation unit is the only bridge from generated schema vocabulary
// into the runtime-independent experimental SAI descriptor vocabulary.
using x3d::core::AccessType;
using x3d::core::X3DFieldType;

sai_error metadata_error(error_code code, std::string message,
                         std::string field = {}) {
  sai_error error;
  error.code = code;
  error.operation = "generated_type_registry";
  error.message = std::move(message);
  error.field = std::move(field);
  return error;
}

std::optional<access_type> adapt_access(AccessType access) {
  switch (access) {
  case AccessType::InitializeOnly:
    return access_type::initialize_only;
  case AccessType::InputOnly:
    return access_type::input_only;
  case AccessType::OutputOnly:
    return access_type::output_only;
  case AccessType::InputOutput:
    return access_type::input_output;
  }
  return std::nullopt;
}

std::optional<schema_field_type> adapt_type(X3DFieldType type) {
  switch (type) {
  case X3DFieldType::SFBool:
    return schema_field_type::sf_bool;
  case X3DFieldType::SFColor:
    return schema_field_type::sf_color;
  case X3DFieldType::SFColorRGBA:
    return schema_field_type::sf_color_rgba;
  case X3DFieldType::SFDouble:
    return schema_field_type::sf_double;
  case X3DFieldType::SFFloat:
    return schema_field_type::sf_float;
  case X3DFieldType::SFImage:
    return schema_field_type::sf_image;
  case X3DFieldType::SFInt32:
    return schema_field_type::sf_int32;
  case X3DFieldType::SFMatrix3d:
    return schema_field_type::sf_matrix3d;
  case X3DFieldType::SFMatrix3f:
    return schema_field_type::sf_matrix3f;
  case X3DFieldType::SFMatrix4d:
    return schema_field_type::sf_matrix4d;
  case X3DFieldType::SFMatrix4f:
    return schema_field_type::sf_matrix4f;
  case X3DFieldType::SFNode:
    return schema_field_type::sf_node;
  case X3DFieldType::SFRotation:
    return schema_field_type::sf_rotation;
  case X3DFieldType::SFString:
    return schema_field_type::sf_string;
  case X3DFieldType::SFTime:
    return schema_field_type::sf_time;
  case X3DFieldType::SFVec2d:
    return schema_field_type::sf_vec2d;
  case X3DFieldType::SFVec2f:
    return schema_field_type::sf_vec2f;
  case X3DFieldType::SFVec3d:
    return schema_field_type::sf_vec3d;
  case X3DFieldType::SFVec3f:
    return schema_field_type::sf_vec3f;
  case X3DFieldType::SFVec4d:
    return schema_field_type::sf_vec4d;
  case X3DFieldType::SFVec4f:
    return schema_field_type::sf_vec4f;
  case X3DFieldType::MFBool:
    return schema_field_type::mf_bool;
  case X3DFieldType::MFColor:
    return schema_field_type::mf_color;
  case X3DFieldType::MFColorRGBA:
    return schema_field_type::mf_color_rgba;
  case X3DFieldType::MFDouble:
    return schema_field_type::mf_double;
  case X3DFieldType::MFFloat:
    return schema_field_type::mf_float;
  case X3DFieldType::MFImage:
    return schema_field_type::mf_image;
  case X3DFieldType::MFInt32:
    return schema_field_type::mf_int32;
  case X3DFieldType::MFMatrix3d:
    return schema_field_type::mf_matrix3d;
  case X3DFieldType::MFMatrix3f:
    return schema_field_type::mf_matrix3f;
  case X3DFieldType::MFMatrix4d:
    return schema_field_type::mf_matrix4d;
  case X3DFieldType::MFMatrix4f:
    return schema_field_type::mf_matrix4f;
  case X3DFieldType::MFNode:
    return schema_field_type::mf_node;
  case X3DFieldType::MFRotation:
    return schema_field_type::mf_rotation;
  case X3DFieldType::MFString:
    return schema_field_type::mf_string;
  case X3DFieldType::MFTime:
    return schema_field_type::mf_time;
  case X3DFieldType::MFVec2d:
    return schema_field_type::mf_vec2d;
  case X3DFieldType::MFVec2f:
    return schema_field_type::mf_vec2f;
  case X3DFieldType::MFVec3d:
    return schema_field_type::mf_vec3d;
  case X3DFieldType::MFVec3f:
    return schema_field_type::mf_vec3f;
  case X3DFieldType::MFVec4d:
    return schema_field_type::mf_vec4d;
  case X3DFieldType::MFVec4f:
    return schema_field_type::mf_vec4f;
  case X3DFieldType::SFEnum:
    return schema_field_type::sf_enum;
  case X3DFieldType::MFEnum:
    return schema_field_type::mf_enum;
  }
  return std::nullopt;
}

template <class T> std::optional<T> parse_number(std::string_view lexical) {
  T parsed{};
  const auto *begin = lexical.data();
  const auto *end = begin + lexical.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end)
    return std::nullopt;
  return parsed;
}

std::string parse_string(std::string lexical) {
  if (lexical.size() >= 2 && lexical.front() == '"' && lexical.back() == '"')
    return lexical.substr(1, lexical.size() - 2);
  return lexical;
}

result<std::pair<value_kind, value>>
adapt_value(const schema_field_descriptor &field) {
  const std::string lexical = field.declared_default.value_or("");
  switch (field.type) {
  case schema_field_type::sf_bool:
    if (!field.declared_default)
      return std::pair{value_kind::boolean, value{false}};
    if (lexical == "true" || lexical == "false")
      return std::pair{value_kind::boolean, value{lexical == "true"}};
    break;
  case schema_field_type::sf_int32: {
    if (!field.declared_default)
      return std::pair{value_kind::int32, value{std::int32_t{0}}};
    if (const auto parsed = parse_number<std::int32_t>(lexical))
      return std::pair{value_kind::int32, value{*parsed}};
    break;
  }
  case schema_field_type::sf_double: {
    if (!field.declared_default)
      return std::pair{value_kind::number, value{0.0}};
    if (const auto parsed = parse_number<double>(lexical))
      return std::pair{value_kind::number, value{*parsed}};
    break;
  }
  case schema_field_type::sf_string:
    return std::pair{value_kind::string,
                     value{parse_string(field.declared_default.value_or(""))}};
  case schema_field_type::sf_vec3f: {
    if (!field.declared_default)
      return std::pair{value_kind::vec3f, value{vec3f{}}};
    vec3f parsed;
    const auto first_space = lexical.find(' ');
    const auto second_space = lexical.find(' ', first_space + 1);
    if (first_space != std::string::npos && second_space != std::string::npos) {
      const auto x =
          parse_number<float>(std::string_view{lexical}.substr(0, first_space));
      const auto y = parse_number<float>(std::string_view{lexical}.substr(
          first_space + 1, second_space - first_space - 1));
      const auto z = parse_number<float>(
          std::string_view{lexical}.substr(second_space + 1));
      if (x && y && z) {
        parsed = {*x, *y, *z};
        return std::pair{value_kind::vec3f, value{parsed}};
      }
    }
    break;
  }
  case schema_field_type::sf_node:
    if (!field.declared_default || lexical == "NULL")
      return std::pair{value_kind::node, value{node_id{}}};
    break;
  case schema_field_type::mf_node:
    if (!field.declared_default || lexical.empty())
      return std::pair{value_kind::node_list, value{node_list{}}};
    break;
  default:
    return metadata_error(error_code::unsupported_field_type,
                          "generated field type is not yet represented",
                          field.name);
  }
  return metadata_error(error_code::invalid_descriptor,
                        "generated default is not valid for its field type",
                        field.name);
}

} // namespace

metadata_catalog generated_metadata_catalog() {
  metadata_catalog adapted;
  adapted.specification_version =
      x3d::nodes::X3DSemanticMetadataRegistry::specificationVersion();
  adapted.model_fingerprint =
      x3d::nodes::X3DSemanticMetadataRegistry::modelFingerprint();
  adapted.generator_version =
      x3d::nodes::X3DSemanticMetadataRegistry::generatorVersion();
  adapted.unit_categories_complete =
      x3d::nodes::X3DSemanticMetadataRegistry::unitCategoriesComplete();
  const auto generated = x3d::nodes::X3DSemanticMetadataRegistry::nodes();
  adapted.nodes.reserve(generated.size());
  for (const auto &node : generated) {
    schema_node_descriptor adapted_node{
        .name = node.name,
        .abstract = node.abstract,
        .component = node.component,
        .component_level = node.level,
        .interfaces = node.interfaces,
        .fields = {},
    };
    adapted_node.fields.reserve(node.fields.size());
    for (const auto &field : node.fields) {
      const auto type = adapt_type(field.type);
      const auto access = adapt_access(field.access);
      if (!type || !access)
        throw std::logic_error(
            "generated metadata contains an unmapped field type or access");
      adapted_node.fields.push_back(schema_field_descriptor{
          .name = field.name,
          .type = *type,
          .access = *access,
          .declared_default = field.defaultValue,
          .accepted_node_types = field.acceptableNodeTypes,
          .unit_category = field.unitCategory,
      });
    }
    adapted.nodes.push_back(std::move(adapted_node));
  }
  return adapted;
}

result<type_registry>
generated_type_registry(std::span<const std::string_view> node_types) {
  type_registry registry;
  const auto catalog = generated_metadata_catalog();
  for (const auto requested : node_types) {
    const auto generated = std::find_if(
        catalog.nodes.begin(), catalog.nodes.end(),
        [&](const schema_node_descriptor &candidate) {
          return candidate.name == requested;
        });
    if (generated == catalog.nodes.end())
      return metadata_error(error_code::unknown_type,
                            "generated node type is unknown: " +
                                std::string{requested});

    node_type_descriptor adapted;
    adapted.name = generated->name;
    adapted.component = generated->component;
    adapted.component_level = generated->component_level;
    adapted.interfaces = generated->interfaces;
    adapted.abstract = generated->abstract;
    adapted.fields.reserve(generated->fields.size());
    for (const auto &field : generated->fields) {
      const auto adapted_value = adapt_value(field);
      if (!adapted_value)
        return adapted_value.error();
      adapted.fields.push_back(field_descriptor{
          .name = field.name,
          .kind = adapted_value.value().first,
          .access = field.access,
          .default_value = adapted_value.value().second,
          .default_origin = field.declared_default ? default_source::schema
                                                   : default_source::field_type,
          .containment = field.type == schema_field_type::sf_node ||
                         field.type == schema_field_type::mf_node,
          .accepted_node_types = field.accepted_node_types,
          .unit_category = field.unit_category,
      });
    }
    if (auto defined = registry.define(std::move(adapted)); !defined)
      return defined.error();
  }
  return registry;
}

} // namespace x3d::sai::experimental
