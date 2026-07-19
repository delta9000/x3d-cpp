#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DLayerNode {
  static constexpr std::string_view x3d_name = "X3DLayerNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {objectType.name(), objectType.kind, objectType.access()},
      {pickable.name(), pickable.kind, pickable.access()},
      {viewport.name(), viewport.kind, viewport.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
