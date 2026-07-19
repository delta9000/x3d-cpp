#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DirectionalLight {
  static constexpr std::string_view x3d_name = "DirectionalLight";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<DirectionalLight, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<DirectionalLight,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<DirectionalLight,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, bool> global{
      "global", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<DirectionalLight,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<DirectionalLight,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, float> shadowIntensity{
      "shadowIntensity", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<DirectionalLight, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {color.name(), color.kind, color.access()},
      {direction.name(), direction.kind, direction.access()},
      {global.name(), global.kind, global.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {shadowIntensity.name(), shadowIntensity.kind, shadowIntensity.access()},
      {shadows.name(), shadows.kind, shadows.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
