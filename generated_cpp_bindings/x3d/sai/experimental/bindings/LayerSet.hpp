#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LayerSet {
  static constexpr std::string_view x3d_name = "LayerSet";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LayerSet, std::int32_t> activeLayer{
      "activeLayer", access_type::input_output};
  inline static constexpr field_key<LayerSet, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LayerSet,
                                    ::x3d::sai::experimental::node_list>
      layers{"layers", access_type::input_output};
  inline static constexpr field_key<LayerSet, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LayerSet,
                                    ::x3d::sai::experimental::int32_list>
      order{"order", access_type::input_output};
  inline static constexpr field_key<LayerSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LayerSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LayerSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LayerSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LayerSet, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {activeLayer.name(), activeLayer.kind, activeLayer.access()},
      {IS.name(), IS.kind, IS.access()},
      {layers.name(), layers.kind, layers.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {order.name(), order.kind, order.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
