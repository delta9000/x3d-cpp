#include "x3d/sai/experimental/metadata.hpp"

#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

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

std::vector<std::string_view> tokens(std::string_view lexical) {
  std::vector<std::string_view> result;
  std::size_t begin = 0;
  while (begin < lexical.size()) {
    while (begin < lexical.size() &&
           (std::isspace(static_cast<unsigned char>(lexical[begin])) ||
            lexical[begin] == ',' || lexical[begin] == '[' ||
            lexical[begin] == ']'))
      ++begin;
    if (begin == lexical.size())
      break;
    std::size_t end = begin;
    while (end < lexical.size() &&
           !std::isspace(static_cast<unsigned char>(lexical[end])) &&
           lexical[end] != ',' && lexical[end] != '[' && lexical[end] != ']')
      ++end;
    result.push_back(lexical.substr(begin, end - begin));
    begin = end;
  }
  return result;
}

template <class T>
std::optional<std::vector<T>> parse_numbers(std::string_view lexical) {
  std::vector<T> result;
  for (const auto token : tokens(lexical)) {
    const auto parsed = parse_number<T>(token);
    if (!parsed)
      return std::nullopt;
    result.push_back(*parsed);
  }
  return result;
}

std::optional<bool_list> parse_bools(std::string_view lexical) {
  bool_list result;
  for (const auto token : tokens(lexical)) {
    if (token == "true" || token == "TRUE" || token == "1")
      result.push_back(true);
    else if (token == "false" || token == "FALSE" || token == "0")
      result.push_back(false);
    else
      return std::nullopt;
  }
  return result;
}

std::optional<string_list> parse_strings(std::string_view lexical) {
  string_list result;
  std::size_t cursor = 0;
  while (cursor < lexical.size()) {
    while (cursor < lexical.size() &&
           (std::isspace(static_cast<unsigned char>(lexical[cursor])) ||
            lexical[cursor] == '[' || lexical[cursor] == ']' ||
            lexical[cursor] == ','))
      ++cursor;
    if (cursor == lexical.size())
      break;
    std::string item;
    if (lexical[cursor] == '"') {
      ++cursor;
      bool closed = false;
      while (cursor < lexical.size()) {
        const char current = lexical[cursor++];
        if (current == '"') {
          closed = true;
          break;
        }
        if (current == '\\' && cursor < lexical.size() &&
            (lexical[cursor] == '\\' || lexical[cursor] == '"')) {
          item.push_back(lexical[cursor++]);
        } else {
          item.push_back(current);
        }
      }
      if (!closed)
        return std::nullopt;
    } else {
      const auto begin = cursor;
      while (cursor < lexical.size() &&
             !std::isspace(static_cast<unsigned char>(lexical[cursor])) &&
             lexical[cursor] != ']' && lexical[cursor] != ',')
        ++cursor;
      item.assign(lexical.substr(begin, cursor - begin));
    }
    result.push_back(std::move(item));
  }
  return result;
}

template <class Scalar, std::size_t Width, class Factory>
auto parse_fixed(std::string_view lexical, Factory factory)
    -> std::optional<std::invoke_result_t<Factory, const Scalar *>> {
  const auto values = parse_numbers<Scalar>(lexical);
  if (!values || values->size() != Width)
    return std::nullopt;
  return factory(values->data());
}

template <class Scalar, std::size_t Width, class Factory>
auto parse_grouped(std::string_view lexical, Factory factory) -> std::optional<
    std::vector<std::invoke_result_t<Factory, const Scalar *>>> {
  using element = std::invoke_result_t<Factory, const Scalar *>;
  const auto values = parse_numbers<Scalar>(lexical);
  if (!values || values->size() % Width != 0)
    return std::nullopt;
  std::vector<element> result;
  result.reserve(values->size() / Width);
  for (std::size_t offset = 0; offset < values->size(); offset += Width)
    result.push_back(factory(values->data() + offset));
  return result;
}

template <class T, std::size_t N> matrix<T, N> make_matrix(const T *values) {
  std::array<T, N * N> elements;
  std::copy_n(values, N * N, elements.begin());
  return matrix<T, N>{elements};
}

std::optional<std::uint32_t> parse_pixel(std::string_view token) {
  int base = 10;
  if (token.size() > 2 && token[0] == '0' &&
      (token[1] == 'x' || token[1] == 'X')) {
    token.remove_prefix(2);
    base = 16;
  }
  std::uint32_t result = 0;
  const auto parsed =
      std::from_chars(token.data(), token.data() + token.size(), result, base);
  if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size())
    return std::nullopt;
  return result;
}

std::optional<image> parse_image_at(const std::vector<std::string_view> &values,
                                    std::size_t &cursor) {
  if (values.size() - cursor < 3)
    return std::nullopt;
  const auto width = parse_number<std::int32_t>(values[cursor++]);
  const auto height = parse_number<std::int32_t>(values[cursor++]);
  const auto components = parse_number<std::int32_t>(values[cursor++]);
  if (!width || !height || !components || *width < 0 || *height < 0 ||
      *components < 0 || *components > 4)
    return std::nullopt;
  const auto pixel_count =
      static_cast<std::uint64_t>(*width) * static_cast<std::uint64_t>(*height);
  if (pixel_count > values.size() - cursor ||
      pixel_count * static_cast<std::uint64_t>(*components) >
          std::numeric_limits<std::size_t>::max())
    return std::nullopt;
  image result{.width = *width,
               .height = *height,
               .components = *components,
               .data = {}};
  result.data.reserve(static_cast<std::size_t>(pixel_count) * *components);
  for (std::uint64_t pixel_index = 0; pixel_index < pixel_count;
       ++pixel_index) {
    const auto pixel = parse_pixel(values[cursor++]);
    if (!pixel)
      return std::nullopt;
    if (*components < 4 && *pixel >= (std::uint32_t{1} << (*components * 8)))
      return std::nullopt;
    for (std::int32_t component = *components; component > 0; --component)
      result.data.push_back(
          static_cast<std::uint8_t>((*pixel >> ((component - 1) * 8)) & 0xffU));
  }
  return result;
}

template <class T>
result<std::pair<value_kind, value>>
finish_value(const schema_field_descriptor &field, std::optional<T> parsed) {
  if (!parsed)
    return metadata_error(error_code::invalid_descriptor,
                          "generated default is not valid for its field type",
                          field.name);
  return std::pair{field.type, value{std::move(*parsed)}};
}

result<std::pair<value_kind, value>>
adapt_value(const schema_field_descriptor &field) {
  const std::string lexical = field.declared_default.value_or("");
  const bool declared = field.declared_default.has_value();
  const auto f2 = [](const float *v) { return vec2f{v[0], v[1]}; };
  const auto d2 = [](const double *v) { return vec2d{v[0], v[1]}; };
  const auto f3 = [](const float *v) { return vec3f{v[0], v[1], v[2]}; };
  const auto d3 = [](const double *v) { return vec3d{v[0], v[1], v[2]}; };
  const auto f4 = [](const float *v) { return vec4f{v[0], v[1], v[2], v[3]}; };
  const auto d4 = [](const double *v) { return vec4d{v[0], v[1], v[2], v[3]}; };
  const auto color3 = [](const float *v) { return color3f{v[0], v[1], v[2]}; };
  const auto color4 = [](const float *v) {
    return color4f{v[0], v[1], v[2], v[3]};
  };
  const auto rotate = [](const float *v) {
    return rotation{v[0], v[1], v[2], v[3]};
  };

  switch (field.type) {
  case value_kind::sf_bool: {
    if (!declared)
      return finish_value(field, std::optional{false});
    const auto values = parse_bools(lexical);
    return finish_value(field, values && values->size() == 1
                                   ? std::optional<bool>{(*values)[0]}
                                   : std::nullopt);
  }
  case value_kind::sf_color:
    return finish_value(field, declared ? parse_fixed<float, 3>(lexical, color3)
                                        : std::optional{color3f{}});
  case value_kind::sf_color_rgba:
    return finish_value(field, declared ? parse_fixed<float, 4>(lexical, color4)
                                        : std::optional{color4f{}});
  case value_kind::sf_double:
    return finish_value(field, declared ? parse_number<double>(lexical)
                                        : std::optional{0.0});
  case value_kind::sf_float:
    return finish_value(field, declared ? parse_number<float>(lexical)
                                        : std::optional{0.0F});
  case value_kind::sf_image: {
    if (!declared)
      return finish_value(field, std::optional{image{}});
    const auto values = tokens(lexical);
    std::size_t cursor = 0;
    auto parsed = parse_image_at(values, cursor);
    if (cursor != values.size())
      parsed.reset();
    return finish_value(field, std::move(parsed));
  }
  case value_kind::sf_int32:
    return finish_value(field, declared ? parse_number<std::int32_t>(lexical)
                                        : std::optional{std::int32_t{0}});
  case value_kind::sf_matrix3d:
    return finish_value(field, declared ? parse_fixed<double, 9>(
                                              lexical, make_matrix<double, 3>)
                                        : std::optional{matrix3d{}});
  case value_kind::sf_matrix3f:
    return finish_value(
        field, declared ? parse_fixed<float, 9>(lexical, make_matrix<float, 3>)
                        : std::optional{matrix3f{}});
  case value_kind::sf_matrix4d:
    return finish_value(field, declared ? parse_fixed<double, 16>(
                                              lexical, make_matrix<double, 4>)
                                        : std::optional{matrix4d{}});
  case value_kind::sf_matrix4f:
    return finish_value(
        field, declared ? parse_fixed<float, 16>(lexical, make_matrix<float, 4>)
                        : std::optional{matrix4f{}});
  case value_kind::sf_node:
    return lexical.empty() || lexical == "NULL"
               ? finish_value(field, std::optional{node_id{}})
               : finish_value<node_id>(field, std::nullopt);
  case value_kind::sf_rotation:
    return finish_value(field, declared ? parse_fixed<float, 4>(lexical, rotate)
                                        : std::optional{rotation{}});
  case value_kind::sf_string:
    return finish_value(field, std::optional{parse_string(lexical)});
  case value_kind::sf_time:
    return finish_value(field,
                        declared ? [&]() -> std::optional<time_value> {
                          const auto parsed = parse_number<double>(lexical);
                          return parsed ? std::optional{time_value{*parsed}}
                                        : std::nullopt;
                        }()
                                 : std::optional{time_value{}});
  case value_kind::sf_vec2d:
    return finish_value(field, declared ? parse_fixed<double, 2>(lexical, d2)
                                        : std::optional{vec2d{}});
  case value_kind::sf_vec2f:
    return finish_value(field, declared ? parse_fixed<float, 2>(lexical, f2)
                                        : std::optional{vec2f{}});
  case value_kind::sf_vec3d:
    return finish_value(field, declared ? parse_fixed<double, 3>(lexical, d3)
                                        : std::optional{vec3d{}});
  case value_kind::sf_vec3f:
    return finish_value(field, declared ? parse_fixed<float, 3>(lexical, f3)
                                        : std::optional{vec3f{}});
  case value_kind::sf_vec4d:
    return finish_value(field, declared ? parse_fixed<double, 4>(lexical, d4)
                                        : std::optional{vec4d{}});
  case value_kind::sf_vec4f:
    return finish_value(field, declared ? parse_fixed<float, 4>(lexical, f4)
                                        : std::optional{vec4f{}});
  case value_kind::mf_bool:
    return finish_value(field, declared ? parse_bools(lexical)
                                        : std::optional{bool_list{}});
  case value_kind::mf_color:
    return finish_value(field, declared
                                   ? parse_grouped<float, 3>(lexical, color3)
                                   : std::optional{color3f_list{}});
  case value_kind::mf_color_rgba:
    return finish_value(field, declared
                                   ? parse_grouped<float, 4>(lexical, color4)
                                   : std::optional{color4f_list{}});
  case value_kind::mf_double:
    return finish_value(field, declared ? parse_numbers<double>(lexical)
                                        : std::optional{double_list{}});
  case value_kind::mf_float:
    return finish_value(field, declared ? parse_numbers<float>(lexical)
                                        : std::optional{float_list{}});
  case value_kind::mf_image: {
    image_list result;
    if (declared) {
      const auto values = tokens(lexical);
      std::size_t cursor = 0;
      while (cursor < values.size()) {
        auto parsed = parse_image_at(values, cursor);
        if (!parsed)
          return finish_value<image_list>(field, std::nullopt);
        result.push_back(std::move(*parsed));
      }
    }
    return finish_value(field, std::optional{std::move(result)});
  }
  case value_kind::mf_int32:
    return finish_value(field, declared ? parse_numbers<std::int32_t>(lexical)
                                        : std::optional{int32_list{}});
  case value_kind::mf_matrix3d:
    return finish_value(field, declared ? parse_grouped<double, 9>(
                                              lexical, make_matrix<double, 3>)
                                        : std::optional{matrix3d_list{}});
  case value_kind::mf_matrix3f:
    return finish_value(field, declared ? parse_grouped<float, 9>(
                                              lexical, make_matrix<float, 3>)
                                        : std::optional{matrix3f_list{}});
  case value_kind::mf_matrix4d:
    return finish_value(field, declared ? parse_grouped<double, 16>(
                                              lexical, make_matrix<double, 4>)
                                        : std::optional{matrix4d_list{}});
  case value_kind::mf_matrix4f:
    return finish_value(field, declared ? parse_grouped<float, 16>(
                                              lexical, make_matrix<float, 4>)
                                        : std::optional{matrix4f_list{}});
  case value_kind::mf_node:
    return lexical.empty() ? finish_value(field, std::optional{node_list{}})
                           : finish_value<node_list>(field, std::nullopt);
  case value_kind::mf_rotation:
    return finish_value(field, declared
                                   ? parse_grouped<float, 4>(lexical, rotate)
                                   : std::optional{rotation_list{}});
  case value_kind::mf_string:
    return finish_value(field, declared ? parse_strings(lexical)
                                        : std::optional{string_list{}});
  case value_kind::mf_time: {
    const auto parsed = declared ? parse_numbers<double>(lexical)
                                 : std::optional{double_list{}};
    if (!parsed)
      return finish_value<time_list>(field, std::nullopt);
    time_list result;
    result.reserve(parsed->size());
    for (const double seconds : *parsed)
      result.push_back(time_value{seconds});
    return finish_value(field, std::optional{std::move(result)});
  }
  case value_kind::mf_vec2d:
    return finish_value(field, declared ? parse_grouped<double, 2>(lexical, d2)
                                        : std::optional{vec2d_list{}});
  case value_kind::mf_vec2f:
    return finish_value(field, declared ? parse_grouped<float, 2>(lexical, f2)
                                        : std::optional{vec2f_list{}});
  case value_kind::mf_vec3d:
    return finish_value(field, declared ? parse_grouped<double, 3>(lexical, d3)
                                        : std::optional{vec3d_list{}});
  case value_kind::mf_vec3f:
    return finish_value(field, declared ? parse_grouped<float, 3>(lexical, f3)
                                        : std::optional{vec3f_list{}});
  case value_kind::mf_vec4d:
    return finish_value(field, declared ? parse_grouped<double, 4>(lexical, d4)
                                        : std::optional{vec4d_list{}});
  case value_kind::mf_vec4f:
    return finish_value(field, declared ? parse_grouped<float, 4>(lexical, f4)
                                        : std::optional{vec4f_list{}});
  case value_kind::sf_enum:
    return finish_value(field,
                        std::optional{enum_value{parse_string(lexical)}});
  case value_kind::mf_enum: {
    const auto parsed =
        declared ? parse_strings(lexical) : std::optional{string_list{}};
    if (!parsed)
      return finish_value<enum_list>(field, std::nullopt);
    enum_list result;
    result.reserve(parsed->size());
    for (auto &token : *parsed)
      result.push_back(enum_value{std::move(token)});
    return finish_value(field, std::optional{std::move(result)});
  }
  }
  return metadata_error(error_code::unsupported_field_type,
                        "generated field type is not represented", field.name);
}

} // namespace

result<value> default_value_for(const schema_field_descriptor &field) {
  auto adapted = adapt_value(field);
  if (!adapted) {
    auto error = adapted.error();
    error.operation = "default_value_for";
    return error;
  }
  return std::move(adapted).value().second;
}

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
    const auto generated =
        std::find_if(catalog.nodes.begin(), catalog.nodes.end(),
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
      const auto adapted_value = default_value_for(field);
      if (!adapted_value) {
        auto error = adapted_value.error();
        error.operation = "generated_type_registry";
        return error;
      }
      adapted.fields.push_back(field_descriptor{
          .name = field.name,
          .kind = field.type,
          .access = field.access,
          .default_value = adapted_value.value(),
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
