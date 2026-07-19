#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LineProperties {
  static constexpr std::string_view x3d_name = "LineProperties";
  inline static constexpr field_key<LineProperties, bool> applied{
      "applied", access_type::input_output};
  inline static constexpr field_key<LineProperties,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::int32_t> linetype{
      "linetype", access_type::input_output};
  inline static constexpr field_key<LineProperties, float> linewidthScaleFactor{
      "linewidthScaleFactor", access_type::input_output};
  inline static constexpr field_key<LineProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
