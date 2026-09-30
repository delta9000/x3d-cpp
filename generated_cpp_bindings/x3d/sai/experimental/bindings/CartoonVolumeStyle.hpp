#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CartoonVolumeStyle {
  static constexpr std::string_view x3d_name = "CartoonVolumeStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<CartoonVolumeStyle, std::int32_t>
      colorSteps{"colorSteps", access_type::input_output};
  inline static constexpr field_key<CartoonVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CartoonVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CartoonVolumeStyle,
                                    ::x3d::sai::experimental::color4f>
      orthogonalColor{"orthogonalColor", access_type::input_output};
  inline static constexpr field_key<CartoonVolumeStyle,
                                    ::x3d::sai::experimental::color4f>
      parallelColor{"parallelColor", access_type::input_output};
  inline static constexpr field_key<CartoonVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      surfaceNormals{"surfaceNormals", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {colorSteps.name(), colorSteps.kind, colorSteps.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {orthogonalColor.name(), orthogonalColor.kind, orthogonalColor.access()},
      {parallelColor.name(), parallelColor.kind, parallelColor.access()},
      {surfaceNormals.name(), surfaceNormals.kind, surfaceNormals.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
