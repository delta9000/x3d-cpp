#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SpatialSound {
  static constexpr std::string_view x3d_name = "SpatialSound";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> coneInnerAngle{
      "coneInnerAngle", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> coneOuterAngle{
      "coneOuterAngle", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> coneOuterGain{
      "coneOuterGain", access_type::input_output};
  inline static constexpr field_key<SpatialSound, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::enum_value>
      distanceModel{"distanceModel", access_type::input_output};
  inline static constexpr field_key<SpatialSound, bool> dopplerEnabled{
      "dopplerEnabled", access_type::input_output};
  inline static constexpr field_key<SpatialSound, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<SpatialSound, bool> enableHRTF{
      "enableHRTF", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> maxDistance{
      "maxDistance", access_type::input_output};
  inline static constexpr field_key<SpatialSound,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> priority{
      "priority", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> referenceDistance{
      "referenceDistance", access_type::input_output};
  inline static constexpr field_key<SpatialSound, float> rolloffFactor{
      "rolloffFactor", access_type::input_output};
  inline static constexpr field_key<SpatialSound, bool> spatialize{
      "spatialize", access_type::initialize_only};
  inline static constexpr field_key<SpatialSound, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SpatialSound, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SpatialSound, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SpatialSound, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SpatialSound, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 25> field_keys{{
      {children.name(), children.kind, children.access()},
      {coneInnerAngle.name(), coneInnerAngle.kind, coneInnerAngle.access()},
      {coneOuterAngle.name(), coneOuterAngle.kind, coneOuterAngle.access()},
      {coneOuterGain.name(), coneOuterGain.kind, coneOuterGain.access()},
      {description.name(), description.kind, description.access()},
      {direction.name(), direction.kind, direction.access()},
      {distanceModel.name(), distanceModel.kind, distanceModel.access()},
      {dopplerEnabled.name(), dopplerEnabled.kind, dopplerEnabled.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {enableHRTF.name(), enableHRTF.kind, enableHRTF.access()},
      {gain.name(), gain.kind, gain.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {IS.name(), IS.kind, IS.access()},
      {location.name(), location.kind, location.access()},
      {maxDistance.name(), maxDistance.kind, maxDistance.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {priority.name(), priority.kind, priority.access()},
      {referenceDistance.name(), referenceDistance.kind,
       referenceDistance.access()},
      {rolloffFactor.name(), rolloffFactor.kind, rolloffFactor.access()},
      {spatialize.name(), spatialize.kind, spatialize.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
