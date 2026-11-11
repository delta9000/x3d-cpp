#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct OpacityMapVolumeStyle {
  static constexpr std::string_view x3d_name = "OpacityMapVolumeStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<OpacityMapVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<OpacityMapVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      transferFunction{"transferFunction", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {transferFunction.name(), transferFunction.kind,
       transferFunction.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
