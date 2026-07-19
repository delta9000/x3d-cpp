#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Background {
  static constexpr std::string_view x3d_name = "Background";
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      backUrl{"backUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      bottomUrl{"bottomUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      frontUrl{"frontUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::float_list>
      groundAngle{"groundAngle", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::color3f_list>
      groundColor{"groundColor", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Background, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      leftUrl{"leftUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      rightUrl{"rightUrl", access_type::input_output};
  inline static constexpr field_key<Background, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::float_list>
      skyAngle{"skyAngle", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::color3f_list>
      skyColor{"skyColor", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      topUrl{"topUrl", access_type::input_output};
  inline static constexpr field_key<Background, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<Background, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Background, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Background, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Background, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Background, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
