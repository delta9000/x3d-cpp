#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DLightNode {
  static constexpr std::string_view x3d_name = "X3DLightNode";
  inline static constexpr field_key<X3DLightNode, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<X3DLightNode,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<X3DLightNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DLightNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, float> shadowIntensity{
      "shadowIntensity", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DLightNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
