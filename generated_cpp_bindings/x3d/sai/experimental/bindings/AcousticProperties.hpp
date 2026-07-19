#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct AcousticProperties {
  static constexpr std::string_view x3d_name = "AcousticProperties";
  inline static constexpr field_key<AcousticProperties, float> absorption{
      "absorption", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, float> diffuse{
      "diffuse", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<AcousticProperties,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<AcousticProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, float> refraction{
      "refraction", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, float> specular{
      "specular", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<AcousticProperties, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
