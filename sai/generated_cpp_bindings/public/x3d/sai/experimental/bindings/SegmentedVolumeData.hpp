#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct SegmentedVolumeData {
  static constexpr std::string_view x3d_name = "SegmentedVolumeData";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {dimensions.name(), dimensions.kind, dimensions.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {renderStyle.name(), renderStyle.kind, renderStyle.access()},
      {segmentEnabled.name(), segmentEnabled.kind, segmentEnabled.access()},
      {segmentIdentifiers.name(), segmentIdentifiers.kind,
       segmentIdentifiers.access()},
      {visible.name(), visible.kind, visible.access()},
      {voxels.name(), voxels.kind, voxels.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
