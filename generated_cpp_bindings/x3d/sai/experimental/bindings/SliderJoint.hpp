#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SliderJoint {
  static constexpr std::string_view x3d_name = "SliderJoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::vec3f>
      axis{"axis", access_type::input_output};
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::string_list>
      forceOutput{"forceOutput", access_type::input_output};
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SliderJoint, float> maxSeparation{
      "maxSeparation", access_type::input_output};
  inline static constexpr field_key<SliderJoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SliderJoint, float> minSeparation{
      "minSeparation", access_type::input_output};
  inline static constexpr field_key<SliderJoint, float> separation{
      "separation", access_type::output_only};
  inline static constexpr field_key<SliderJoint, float> separationRate{
      "separationRate", access_type::output_only};
  inline static constexpr field_key<SliderJoint, float> sliderForce{
      "sliderForce", access_type::input_output};
  inline static constexpr field_key<SliderJoint, float> stopBounce{
      "stopBounce", access_type::input_output};
  inline static constexpr field_key<SliderJoint, float> stopErrorCorrection{
      "stopErrorCorrection", access_type::input_output};
  inline static constexpr field_key<SliderJoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SliderJoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SliderJoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SliderJoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SliderJoint, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 18> field_keys{{
      {axis.name(), axis.kind, axis.access()},
      {body1.name(), body1.kind, body1.access()},
      {body2.name(), body2.kind, body2.access()},
      {forceOutput.name(), forceOutput.kind, forceOutput.access()},
      {IS.name(), IS.kind, IS.access()},
      {maxSeparation.name(), maxSeparation.kind, maxSeparation.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minSeparation.name(), minSeparation.kind, minSeparation.access()},
      {separation.name(), separation.kind, separation.access()},
      {separationRate.name(), separationRate.kind, separationRate.access()},
      {sliderForce.name(), sliderForce.kind, sliderForce.access()},
      {stopBounce.name(), stopBounce.kind, stopBounce.access()},
      {stopErrorCorrection.name(), stopErrorCorrection.kind,
       stopErrorCorrection.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
