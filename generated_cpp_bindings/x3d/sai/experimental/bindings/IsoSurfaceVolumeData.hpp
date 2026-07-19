#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IsoSurfaceVolumeData {
  static constexpr std::string_view x3d_name = "IsoSurfaceVolumeData";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<IsoSurfaceVolumeData, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<IsoSurfaceVolumeData, float>
      contourStepSize{"contourStepSize", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::vec3f>
      dimensions{"dimensions", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::node_id>
      gradients{"gradients", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::node_list>
      renderStyle{"renderStyle", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, float>
      surfaceTolerance{"surfaceTolerance", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::float_list>
      surfaceValues{"surfaceValues", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData,
                                    ::x3d::sai::experimental::node_id>
      voxels{"voxels", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IsoSurfaceVolumeData, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 18> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {contourStepSize.name(), contourStepSize.kind, contourStepSize.access()},
      {dimensions.name(), dimensions.kind, dimensions.access()},
      {gradients.name(), gradients.kind, gradients.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {renderStyle.name(), renderStyle.kind, renderStyle.access()},
      {surfaceTolerance.name(), surfaceTolerance.kind,
       surfaceTolerance.access()},
      {surfaceValues.name(), surfaceValues.kind, surfaceValues.access()},
      {visible.name(), visible.kind, visible.access()},
      {voxels.name(), voxels.kind, voxels.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
