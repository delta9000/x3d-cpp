#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct FloatVertexAttribute {
  static constexpr std::string_view x3d_name = "FloatVertexAttribute";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<FloatVertexAttribute,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<FloatVertexAttribute, std::string> name{
      "name", access_type::initialize_only};
  inline static constexpr field_key<FloatVertexAttribute, std::int32_t>
      numComponents{"numComponents", access_type::initialize_only};
  inline static constexpr field_key<FloatVertexAttribute,
                                    ::x3d::sai::experimental::float_list>
      value{"value", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {numComponents.name(), numComponents.kind, numComponents.access()},
      {value.name(), value.kind, value.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
