#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DNurbsControlCurveNode {
  static constexpr std::string_view x3d_name = "X3DNurbsControlCurveNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DNurbsControlCurveNode,
                                    ::x3d::sai::experimental::vec2d_list>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {controlPoint.name(), controlPoint.kind, controlPoint.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
