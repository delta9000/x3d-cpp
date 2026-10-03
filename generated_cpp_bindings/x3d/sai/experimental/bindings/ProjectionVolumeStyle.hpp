#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ProjectionVolumeStyle {
  static constexpr std::string_view x3d_name = "ProjectionVolumeStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ProjectionVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, float>
      intensityThreshold{"intensityThreshold", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> type{
      "type", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {intensityThreshold.name(), intensityThreshold.kind,
       intensityThreshold.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {type.name(), type.kind, type.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
