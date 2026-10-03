#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ComposedVolumeStyle {
  static constexpr std::string_view x3d_name = "ComposedVolumeStyle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ComposedVolumeStyle, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ComposedVolumeStyle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ComposedVolumeStyle,
                                    ::x3d::sai::experimental::node_list>
      renderStyle{"renderStyle", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {renderStyle.name(), renderStyle.kind, renderStyle.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
