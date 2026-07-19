#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ExplosionEmitter {
  static constexpr std::string_view x3d_name = "ExplosionEmitter";
  inline static constexpr field_key<ExplosionEmitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter,
                                    ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ExplosionEmitter, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
