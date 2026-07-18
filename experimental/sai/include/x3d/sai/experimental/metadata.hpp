#ifndef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#define X3D_SAI_EXPERIMENTAL_METADATA_HPP

#include "types.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::sai::experimental {

enum class schema_field_type {
  sf_bool,
  sf_color,
  sf_color_rgba,
  sf_double,
  sf_float,
  sf_image,
  sf_int32,
  sf_matrix3d,
  sf_matrix3f,
  sf_matrix4d,
  sf_matrix4f,
  sf_node,
  sf_rotation,
  sf_string,
  sf_time,
  sf_vec2d,
  sf_vec2f,
  sf_vec3d,
  sf_vec3f,
  sf_vec4d,
  sf_vec4f,
  mf_bool,
  mf_color,
  mf_color_rgba,
  mf_double,
  mf_float,
  mf_image,
  mf_int32,
  mf_matrix3d,
  mf_matrix3f,
  mf_matrix4d,
  mf_matrix4f,
  mf_node,
  mf_rotation,
  mf_string,
  mf_time,
  mf_vec2d,
  mf_vec2f,
  mf_vec3d,
  mf_vec3f,
  mf_vec4d,
  mf_vec4f,
  sf_enum,
  mf_enum,
};

struct schema_field_descriptor {
  std::string name;
  schema_field_type type = schema_field_type::sf_string;
  access_type access = access_type::input_output;
  std::optional<std::string> declared_default;
  std::vector<std::string> accepted_node_types;
  std::optional<std::string> unit_category;
};

struct schema_node_descriptor {
  std::string name;
  bool abstract = false;
  std::string component;
  int component_level = 0;
  std::vector<std::string> interfaces;
  std::vector<schema_field_descriptor> fields;
};

struct metadata_catalog {
  std::string specification_version;
  std::string model_fingerprint;
  std::string generator_version;
  bool unit_categories_complete = false;
  std::vector<schema_node_descriptor> nodes;
};

// Returns an owning copy of every generated descriptor, including field kinds
// that the current executable value vocabulary cannot yet author.
metadata_catalog generated_metadata_catalog();

// Copies selected descriptors from the generated UOM catalog. The returned
// registry owns every string and value; generated runtime nodes never become
// part of SAI identity. Unsupported field vocabularies fail closed until the
// owning SAI value vocabulary represents them.
result<type_registry>
generated_type_registry(std::span<const std::string_view> node_types);

} // namespace x3d::sai::experimental

#endif
