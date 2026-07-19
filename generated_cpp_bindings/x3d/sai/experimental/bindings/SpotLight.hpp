#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SpotLight {
  static constexpr std::string_view x3d_name = "SpotLight";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SpotLight, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<SpotLight, ::x3d::sai::experimental::vec3f>
      attenuation{"attenuation", access_type::input_output};
  inline static constexpr field_key<SpotLight, float> beamWidth{
      "beamWidth", access_type::input_output};
  inline static constexpr field_key<SpotLight,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<SpotLight, float> cutOffAngle{
      "cutOffAngle", access_type::input_output};
  inline static constexpr field_key<SpotLight, ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<SpotLight, bool> global{
      "global", access_type::input_output};
  inline static constexpr field_key<SpotLight, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<SpotLight,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SpotLight, ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<SpotLight,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SpotLight, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<SpotLight, float> radius{
      "radius", access_type::input_output};
  inline static constexpr field_key<SpotLight, float> shadowIntensity{
      "shadowIntensity", access_type::input_output};
  inline static constexpr field_key<SpotLight, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<SpotLight, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SpotLight, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SpotLight, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SpotLight, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SpotLight, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 20> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {attenuation.name(), attenuation.kind, attenuation.access()},
      {beamWidth.name(), beamWidth.kind, beamWidth.access()},
      {color.name(), color.kind, color.access()},
      {cutOffAngle.name(), cutOffAngle.kind, cutOffAngle.access()},
      {direction.name(), direction.kind, direction.access()},
      {global.name(), global.kind, global.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {IS.name(), IS.kind, IS.access()},
      {location.name(), location.kind, location.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {radius.name(), radius.kind, radius.access()},
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
