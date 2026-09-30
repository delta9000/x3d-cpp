#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoOrigin {
  static constexpr std::string_view x3d_name = "GeoOrigin";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<GeoOrigin, ::x3d::sai::experimental::vec3d>
      geoCoords{"geoCoords", access_type::input_output};
  inline static constexpr field_key<GeoOrigin,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoOrigin,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoOrigin, bool> rotateYUp{
      "rotateYUp", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {geoCoords.name(), geoCoords.kind, geoCoords.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {rotateYUp.name(), rotateYUp.kind, rotateYUp.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
