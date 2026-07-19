#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LoadSensor {
  static constexpr std::string_view x3d_name = "LoadSensor";
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<LoadSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LoadSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<LoadSensor, bool> isLoaded{
      "isLoaded", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::time_value>
      loadTime{"loadTime", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LoadSensor, float> progress{
      "progress", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::time_value>
      timeOut{"timeOut", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
