#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct VolumePickSensor {
  static constexpr std::string_view x3d_name = "VolumePickSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<VolumePickSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string>
      intersectionType{"intersectionType", access_type::initialize_only};
  inline static constexpr field_key<VolumePickSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<VolumePickSensor, std::string>
      matchCriterion{"matchCriterion", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickedGeometry{"pickedGeometry", access_type::output_only};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::node_id>
      pickingGeometry{"pickingGeometry", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickTarget{"pickTarget", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string> sortOrder{
      "sortOrder", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {intersectionType.name(), intersectionType.kind,
       intersectionType.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {matchCriterion.name(), matchCriterion.kind, matchCriterion.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {objectType.name(), objectType.kind, objectType.access()},
      {pickedGeometry.name(), pickedGeometry.kind, pickedGeometry.access()},
      {pickingGeometry.name(), pickingGeometry.kind, pickingGeometry.access()},
      {pickTarget.name(), pickTarget.kind, pickTarget.access()},
      {sortOrder.name(), sortOrder.kind, sortOrder.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
