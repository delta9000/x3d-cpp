#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SegmentedVolumeData {
  static constexpr std::string_view x3d_name = "SegmentedVolumeData";
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<SegmentedVolumeData, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      dimensions{"dimensions", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::node_list>
      renderStyle{"renderStyle", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::bool_list>
      segmentEnabled{"segmentEnabled", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::node_id>
      segmentIdentifiers{"segmentIdentifiers", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData,
                                    ::x3d::sai::experimental::node_id>
      voxels{"voxels", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SegmentedVolumeData, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
