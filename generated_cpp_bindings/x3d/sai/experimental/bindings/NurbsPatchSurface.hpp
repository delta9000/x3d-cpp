#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsPatchSurface {
  static constexpr std::string_view x3d_name = "NurbsPatchSurface";
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::node_id>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, bool> uClosed{
      "uClosed", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t> uDimension{
      "uDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::double_list>
      uKnot{"uKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t> uOrder{
      "uOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t>
      uTessellation{"uTessellation", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, bool> vClosed{
      "vClosed", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t> vDimension{
      "vDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::double_list>
      vKnot{"vKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t> vOrder{
      "vOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsPatchSurface, std::int32_t>
      vTessellation{"vTessellation", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsPatchSurface, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
