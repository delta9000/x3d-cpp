#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CylinderSensor {
  static constexpr std::string_view x3d_name = "CylinderSensor";
  inline static constexpr field_key<CylinderSensor, bool> autoOffset{
      "autoOffset", access_type::input_output};
  inline static constexpr field_key<CylinderSensor,
                                    ::x3d::sai::experimental::rotation>
      axisRotation{"axisRotation", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, float> diskAngle{
      "diskAngle", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CylinderSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<CylinderSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<CylinderSensor, float> maxAngle{
      "maxAngle", access_type::input_output};
  inline static constexpr field_key<CylinderSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, float> minAngle{
      "minAngle", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, float> offset{
      "offset", access_type::input_output};
  inline static constexpr field_key<CylinderSensor,
                                    ::x3d::sai::experimental::rotation>
      rotation_changed{"rotation_changed", access_type::output_only};
  inline static constexpr field_key<CylinderSensor,
                                    ::x3d::sai::experimental::vec3f>
      trackPoint_changed{"trackPoint_changed", access_type::output_only};
  inline static constexpr field_key<CylinderSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CylinderSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
