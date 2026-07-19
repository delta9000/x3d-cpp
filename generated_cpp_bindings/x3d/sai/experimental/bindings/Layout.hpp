#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Layout {
  static constexpr std::string_view x3d_name = "Layout";
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      align{"align", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Layout,
                                    ::x3d::sai::experimental::float_list>
      offset{"offset", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      offsetUnits{"offsetUnits", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      scaleMode{"scaleMode", access_type::input_output};
  inline static constexpr field_key<Layout,
                                    ::x3d::sai::experimental::float_list>
      size{"size", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      sizeUnits{"sizeUnits", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
