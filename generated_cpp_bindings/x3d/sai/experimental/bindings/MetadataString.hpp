#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MetadataString {
  static constexpr std::string_view x3d_name = "MetadataString";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<MetadataString,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MetadataString, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<MetadataString, std::string> reference{
      "reference", access_type::input_output};
  inline static constexpr field_key<MetadataString,
                                    ::x3d::sai::experimental::string_list>
      value{"value", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {reference.name(), reference.kind, reference.access()},
      {value.name(), value.kind, value.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
