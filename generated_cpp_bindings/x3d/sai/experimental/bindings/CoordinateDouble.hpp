#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CoordinateDouble {
  static constexpr std::string_view x3d_name = "CoordinateDouble";
  inline static constexpr field_key<CoordinateDouble,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble,
                                    ::x3d::sai::experimental::vec3d_list>
      point{"point", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CoordinateDouble, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
