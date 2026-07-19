#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Coordinate {
  static constexpr std::string_view x3d_name = "Coordinate";
  inline static constexpr field_key<Coordinate,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Coordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Coordinate,
                                    ::x3d::sai::experimental::vec3f_list>
      point{"point", access_type::input_output};
  inline static constexpr field_key<Coordinate, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Coordinate, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Coordinate, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Coordinate, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Coordinate, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
