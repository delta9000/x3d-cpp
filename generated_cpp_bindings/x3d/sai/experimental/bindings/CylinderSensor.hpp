#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CylinderSensor {
  static constexpr std::string_view x3d_name = "CylinderSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {autoOffset.name(), autoOffset.kind, autoOffset.access()},
      {axisRotation.name(), axisRotation.kind, axisRotation.access()},
      {description.name(), description.kind, description.access()},
      {diskAngle.name(), diskAngle.kind, diskAngle.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isOver.name(), isOver.kind, isOver.access()},
      {maxAngle.name(), maxAngle.kind, maxAngle.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minAngle.name(), minAngle.kind, minAngle.access()},
      {offset.name(), offset.kind, offset.access()},
      {rotation_changed.name(), rotation_changed.kind,
       rotation_changed.access()},
      {trackPoint_changed.name(), trackPoint_changed.kind,
       trackPoint_changed.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
