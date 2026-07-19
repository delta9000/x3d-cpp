#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTexture3DNode {
  static constexpr std::string_view x3d_name = "X3DTexture3DNode";
  inline static constexpr field_key<X3DTexture3DNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode, bool> repeatR{
      "repeatR", access_type::initialize_only};
  inline static constexpr field_key<X3DTexture3DNode, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<X3DTexture3DNode, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<X3DTexture3DNode,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<X3DTexture3DNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DTexture3DNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
