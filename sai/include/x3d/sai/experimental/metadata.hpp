#ifndef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#define X3D_SAI_EXPERIMENTAL_METADATA_HPP

#include "types.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::sai::experimental {

using schema_field_type = value_kind;

struct schema_field_descriptor {
  std::string name;
  schema_field_type type = schema_field_type::sf_string;
  access_type access = access_type::input_output;
  std::optional<std::string> declared_default;
  std::vector<std::string> accepted_node_types;
  std::optional<std::string> unit_category;
  numeric_range range{};
  bool unique_non_null_nodes = false;
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

// Returns owning runtime-field descriptors, not a lossless UOM/document model.
// DEF/USE/IS and host class/id/CSS style are intentionally excluded. Text-style
// fields on FontStyle/ScreenFontStyle remain. See
// docs/wiki/sai/reference/generated-fields.md.
// Bounds, enum-membership and reference/containment classification are not a
// complete constraint model; unit_categories_complete also remains false.
metadata_catalog generated_metadata_catalog();

// Converts a schema lexical default (or the ISO field-type default when
// absent) into the exact owning SAI alternative.
result<value> default_value_for(const schema_field_descriptor &field);

// Copies selected descriptors from the generated UOM catalog. The returned
// registry owns every string and value; generated runtime nodes never become
// part of SAI identity. Unsupported field vocabularies fail closed until the
// owning SAI value vocabulary represents them.
result<type_registry>
generated_type_registry(std::span<const std::string_view> node_types);

template <class... Tags>
  requires(sizeof...(Tags) > 0 &&
           (requires { std::string_view{Tags::schema_fingerprint}; } && ...))
result<type_registry> generated_type_registry_for() {
  constexpr std::array names{std::string_view{Tags::x3d_name}...};
  return generated_type_registry(names);
}

} // namespace x3d::sai::experimental

#endif
