#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoViewpoint {
  static constexpr std::string_view x3d_name = "GeoViewpoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::vec3d>
      centerOfRotation{"centerOfRotation", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, float> fieldOfView{
      "fieldOfView", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<GeoViewpoint, bool> jump{
      "jump", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::node_id>
      navigationInfo{"navigationInfo", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, float> nearDistance{
      "nearDistance", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint,
                                    ::x3d::sai::experimental::vec3d>
      position{"position", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, bool> retainUserOffsets{
      "retainUserOffsets", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<GeoViewpoint, float> speedFactor{
      "speedFactor", access_type::initialize_only};
  inline static constexpr field_key<GeoViewpoint, bool> viewAll{
      "viewAll", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoViewpoint, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 24> field_keys{{
      {bindTime.name(), bindTime.kind, bindTime.access()},
      {centerOfRotation.name(), centerOfRotation.kind,
       centerOfRotation.access()},
      {description.name(), description.kind, description.access()},
      {farDistance.name(), farDistance.kind, farDistance.access()},
      {fieldOfView.name(), fieldOfView.kind, fieldOfView.access()},
      {geoOrigin.name(), geoOrigin.kind, geoOrigin.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {IS.name(), IS.kind, IS.access()},
      {isBound.name(), isBound.kind, isBound.access()},
      {jump.name(), jump.kind, jump.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {navigationInfo.name(), navigationInfo.kind, navigationInfo.access()},
      {nearDistance.name(), nearDistance.kind, nearDistance.access()},
      {orientation.name(), orientation.kind, orientation.access()},
      {position.name(), position.kind, position.access()},
      {retainUserOffsets.name(), retainUserOffsets.kind,
       retainUserOffsets.access()},
      {set_bind.name(), set_bind.kind, set_bind.access()},
      {speedFactor.name(), speedFactor.kind, speedFactor.access()},
      {viewAll.name(), viewAll.kind, viewAll.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
