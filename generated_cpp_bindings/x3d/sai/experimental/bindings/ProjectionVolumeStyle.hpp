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
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle,
                                    ::x3d::sai::experimental::enum_value>
      type{"type", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ProjectionVolumeStyle, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {intensityThreshold.name(), intensityThreshold.kind,
       intensityThreshold.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {type.name(), type.kind, type.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
