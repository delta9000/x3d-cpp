#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct VolumePickSensor {
  static constexpr std::string_view x3d_name = "VolumePickSensor";
  inline static constexpr field_key<VolumePickSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string>
      intersectionType{"intersectionType", access_type::initialize_only};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<VolumePickSensor,
                                    ::x3d::sai::experimental::enum_value>
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
  inline static constexpr field_key<VolumePickSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<VolumePickSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
