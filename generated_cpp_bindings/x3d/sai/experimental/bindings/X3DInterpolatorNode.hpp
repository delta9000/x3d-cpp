#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DInterpolatorNode {
  static constexpr std::string_view x3d_name = "X3DInterpolatorNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, float> set_fraction{
      "set_fraction", access_type::input_only};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {IS.name(), IS.kind, IS.access()},
      {key.name(), key.kind, key.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_fraction.name(), set_fraction.kind, set_fraction.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
