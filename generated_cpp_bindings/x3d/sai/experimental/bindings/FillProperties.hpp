#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct FillProperties {
  static constexpr std::string_view x3d_name = "FillProperties";
  inline static constexpr field_key<FillProperties, bool> filled{
      "filled", access_type::input_output};
  inline static constexpr field_key<FillProperties,
                                    ::x3d::sai::experimental::color3f>
      hatchColor{"hatchColor", access_type::input_output};
  inline static constexpr field_key<FillProperties, bool> hatched{
      "hatched", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::int32_t> hatchStyle{
      "hatchStyle", access_type::input_output};
  inline static constexpr field_key<FillProperties,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<FillProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<FillProperties, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
