#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ConeEmitter {
  static constexpr std::string_view x3d_name = "ConeEmitter";
  inline static constexpr field_key<ConeEmitter, float> angle{
      "angle", access_type::input_output};
  inline static constexpr field_key<ConeEmitter,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<ConeEmitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<ConeEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<ConeEmitter,
                                    ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ConeEmitter, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
