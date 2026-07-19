#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PointProperties {
  static constexpr std::string_view x3d_name = "PointProperties";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PointProperties,
                                    ::x3d::sai::experimental::vec3f>
      attenuation{"attenuation", access_type::input_output};
  inline static constexpr field_key<PointProperties,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PointProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PointProperties, float> pointSizeMaxValue{
      "pointSizeMaxValue", access_type::input_output};
  inline static constexpr field_key<PointProperties, float> pointSizeMinValue{
      "pointSizeMinValue", access_type::input_output};
  inline static constexpr field_key<PointProperties, float>
      pointSizeScaleFactor{"pointSizeScaleFactor", access_type::input_output};
  inline static constexpr field_key<PointProperties, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PointProperties, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PointProperties, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PointProperties, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PointProperties, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {attenuation.name(), attenuation.kind, attenuation.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pointSizeMaxValue.name(), pointSizeMaxValue.kind,
       pointSizeMaxValue.access()},
      {pointSizeMinValue.name(), pointSizeMinValue.kind,
       pointSizeMinValue.access()},
      {pointSizeScaleFactor.name(), pointSizeScaleFactor.kind,
       pointSizeScaleFactor.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
