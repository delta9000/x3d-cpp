#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MultiTextureCoordinate {
  static constexpr std::string_view x3d_name = "MultiTextureCoordinate";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<MultiTextureCoordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MultiTextureCoordinate,
                                    ::x3d::sai::experimental::node_list>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {texCoord.name(), texCoord.kind, texCoord.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
