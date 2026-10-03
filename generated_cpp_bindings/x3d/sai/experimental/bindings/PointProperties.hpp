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
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PointProperties, float> pointSizeMaxValue{
      "pointSizeMaxValue", access_type::input_output};
  inline static constexpr field_key<PointProperties, float> pointSizeMinValue{
      "pointSizeMinValue", access_type::input_output};
  inline static constexpr field_key<PointProperties, float>
      pointSizeScaleFactor{"pointSizeScaleFactor", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {attenuation.name(), attenuation.kind, attenuation.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pointSizeMaxValue.name(), pointSizeMaxValue.kind,
       pointSizeMaxValue.access()},
      {pointSizeMinValue.name(), pointSizeMinValue.kind,
       pointSizeMinValue.access()},
      {pointSizeScaleFactor.name(), pointSizeScaleFactor.kind,
       pointSizeScaleFactor.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
