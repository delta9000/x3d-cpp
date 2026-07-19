#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LayerSet {
  static constexpr std::string_view x3d_name = "LayerSet";
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
};

} // namespace x3d::sai::experimental::bindings
