#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Sound {
  static constexpr std::string_view x3d_name = "Sound";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Sound, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Sound, ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<Sound, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<Sound, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<Sound, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Sound, ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<Sound, float> maxBack{
      "maxBack", access_type::input_output};
  inline static constexpr field_key<Sound, float> maxFront{
      "maxFront", access_type::input_output};
  inline static constexpr field_key<Sound, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Sound, float> minBack{
      "minBack", access_type::input_output};
  inline static constexpr field_key<Sound, float> minFront{
      "minFront", access_type::input_output};
  inline static constexpr field_key<Sound, float> priority{
      "priority", access_type::input_output};
  inline static constexpr field_key<Sound, ::x3d::sai::experimental::node_id>
      source{"source", access_type::input_output};
  inline static constexpr field_key<Sound, bool> spatialize{
      "spatialize", access_type::initialize_only};
  inline static constexpr field_key<Sound, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Sound, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Sound, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Sound, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Sound, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {description.name(), description.kind, description.access()},
      {direction.name(), direction.kind, direction.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {intensity.name(), intensity.kind, intensity.access()},
      {IS.name(), IS.kind, IS.access()},
      {location.name(), location.kind, location.access()},
      {maxBack.name(), maxBack.kind, maxBack.access()},
      {maxFront.name(), maxFront.kind, maxFront.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minBack.name(), minBack.kind, minBack.access()},
      {minFront.name(), minFront.kind, minFront.access()},
      {priority.name(), priority.kind, priority.access()},
      {source.name(), source.kind, source.access()},
      {spatialize.name(), spatialize.kind, spatialize.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
