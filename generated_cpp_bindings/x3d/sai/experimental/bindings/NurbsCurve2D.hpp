#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsCurve2D {
  static constexpr std::string_view x3d_name = "NurbsCurve2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<NurbsCurve2D, bool> closed{
      "closed", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::vec2d_list>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::double_list>
      knot{"knot", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<NurbsCurve2D, std::int32_t> tessellation{
      "tessellation", access_type::input_output};
  inline static constexpr field_key<NurbsCurve2D,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 7> field_keys{{
      {closed.name(), closed.kind, closed.access()},
      {controlPoint.name(), controlPoint.kind, controlPoint.access()},
      {knot.name(), knot.kind, knot.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {order.name(), order.kind, order.access()},
      {tessellation.name(), tessellation.kind, tessellation.access()},
      {weight.name(), weight.kind, weight.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
