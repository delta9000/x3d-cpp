#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DLayerNode {
  static constexpr std::string_view x3d_name = "X3DLayerNode";
  inline static constexpr field_key<X3DLayerNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, bool> pickable{
      "pickable", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode,
                                    ::x3d::sai::experimental::node_id>
      viewport{"viewport", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DLayerNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
