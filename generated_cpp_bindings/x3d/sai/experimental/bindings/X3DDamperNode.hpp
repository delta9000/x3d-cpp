#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DDamperNode {
  static constexpr std::string_view x3d_name = "X3DDamperNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DDamperNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DDamperNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<X3DDamperNode,
                                    ::x3d::sai::experimental::time_value>
      tau{"tau", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, float> tolerance{
      "tolerance", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {order.name(), order.kind, order.access()},
      {tau.name(), tau.kind, tau.access()},
      {tolerance.name(), tolerance.kind, tolerance.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
