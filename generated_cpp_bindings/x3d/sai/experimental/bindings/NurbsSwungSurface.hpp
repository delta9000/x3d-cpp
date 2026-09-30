#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsSwungSurface {
  static constexpr std::string_view x3d_name = "NurbsSwungSurface";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<NurbsSwungSurface, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<NurbsSwungSurface,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsSwungSurface,
                                    ::x3d::sai::experimental::node_id>
      profileCurve{"profileCurve", access_type::input_output};
  inline static constexpr field_key<NurbsSwungSurface, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<NurbsSwungSurface,
                                    ::x3d::sai::experimental::node_id>
      trajectoryCurve{"trajectoryCurve", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {ccw.name(), ccw.kind, ccw.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {profileCurve.name(), profileCurve.kind, profileCurve.access()},
      {solid.name(), solid.kind, solid.access()},
      {trajectoryCurve.name(), trajectoryCurve.kind, trajectoryCurve.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
