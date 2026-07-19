#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSingleTextureCoordinateNode {
  static constexpr std::string_view x3d_name = "X3DSingleTextureCoordinateNode";
  inline static constexpr field_key<X3DSingleTextureCoordinateNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      mapping{"mapping", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      DEF{"DEF", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      USE{"USE", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      id{"id", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
