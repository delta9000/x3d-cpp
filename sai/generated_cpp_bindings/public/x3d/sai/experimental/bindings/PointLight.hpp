#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct PointLight {
  static constexpr std::string_view x3d_name = "PointLight";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PointLight, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<PointLight, ::x3d::sai::experimental::vec3f>
      attenuation{"attenuation", access_type::input_output};
  inline static constexpr field_key<PointLight,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<PointLight, bool> global{
      "global", access_type::input_output};
  inline static constexpr field_key<PointLight, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<PointLight, ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<PointLight,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PointLight, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<PointLight, float> radius{
      "radius", access_type::input_output};
  inline static constexpr field_key<PointLight, float> shadowIntensity{
      "shadowIntensity", access_type::input_output};
  inline static constexpr field_key<PointLight, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 11> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {attenuation.name(), attenuation.kind, attenuation.access()},
      {color.name(), color.kind, color.access()},
      {global.name(), global.kind, global.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {location.name(), location.kind, location.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {radius.name(), radius.kind, radius.access()},
      {shadowIntensity.name(), shadowIntensity.kind, shadowIntensity.access()},
      {shadows.name(), shadows.kind, shadows.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
