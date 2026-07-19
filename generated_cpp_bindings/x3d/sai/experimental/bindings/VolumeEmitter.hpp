#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct VolumeEmitter {
  static constexpr std::string_view x3d_name = "VolumeEmitter";
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::int32_list>
      coordIndex{"coordIndex", access_type::initialize_only};
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, bool> internal{
      "internal", access_type::initialize_only};
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter,
                                    ::x3d::sai::experimental::int32_list>
      set_coordIndex{"set_coordIndex", access_type::input_only};
  inline static constexpr field_key<VolumeEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<VolumeEmitter, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
