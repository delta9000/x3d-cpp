#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LayoutLayer {
  static constexpr std::string_view x3d_name = "LayoutLayer";
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      layout{"layout", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, bool> pickable{
      "pickable", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      viewport{"viewport", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
