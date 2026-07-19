#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureProjector {
  static constexpr std::string_view x3d_name = "TextureProjector";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TextureProjector, float> ambientIntensity{
      "ambientIntensity", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> aspectRatio{
      "aspectRatio", access_type::output_only};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> fieldOfView{
      "fieldOfView", access_type::input_output};
  inline static constexpr field_key<TextureProjector, bool> global{
      "global", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> nearDistance{
      "nearDistance", access_type::input_output};
  inline static constexpr field_key<TextureProjector, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<TextureProjector, float> shadowIntensity{
      "shadowIntensity", access_type::input_output};
  inline static constexpr field_key<TextureProjector, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::node_id>
      texture{"texture", access_type::input_output};
  inline static constexpr field_key<TextureProjector,
                                    ::x3d::sai::experimental::vec3f>
      upVector{"upVector", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TextureProjector, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 23> field_keys{{
      {ambientIntensity.name(), ambientIntensity.kind,
       ambientIntensity.access()},
      {aspectRatio.name(), aspectRatio.kind, aspectRatio.access()},
      {color.name(), color.kind, color.access()},
      {description.name(), description.kind, description.access()},
      {direction.name(), direction.kind, direction.access()},
      {farDistance.name(), farDistance.kind, farDistance.access()},
      {fieldOfView.name(), fieldOfView.kind, fieldOfView.access()},
      {global.name(), global.kind, global.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {IS.name(), IS.kind, IS.access()},
      {location.name(), location.kind, location.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {nearDistance.name(), nearDistance.kind, nearDistance.access()},
      {on.name(), on.kind, on.access()},
      {shadowIntensity.name(), shadowIntensity.kind, shadowIntensity.access()},
      {shadows.name(), shadows.kind, shadows.access()},
      {texture.name(), texture.kind, texture.access()},
      {upVector.name(), upVector.kind, upVector.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
