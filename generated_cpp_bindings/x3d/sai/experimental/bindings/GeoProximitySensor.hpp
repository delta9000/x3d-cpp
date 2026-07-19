#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoProximitySensor {
  static constexpr std::string_view x3d_name = "GeoProximitySensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      center{"center", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      centerOfRotation_changed{"centerOfRotation_changed",
                               access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::time_value>
      enterTime{"enterTime", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::time_value>
      exitTime{"exitTime", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      geoCenter{"geoCenter", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      geoCoord_changed{"geoCoord_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::rotation>
      orientation_changed{"orientation_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      position_changed{"position_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      size{"size", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 21> field_keys{{
      {center.name(), center.kind, center.access()},
      {centerOfRotation_changed.name(), centerOfRotation_changed.kind,
       centerOfRotation_changed.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {enterTime.name(), enterTime.kind, enterTime.access()},
      {exitTime.name(), exitTime.kind, exitTime.access()},
      {geoCenter.name(), geoCenter.kind, geoCenter.access()},
      {geoCoord_changed.name(), geoCoord_changed.kind,
       geoCoord_changed.access()},
      {geoOrigin.name(), geoOrigin.kind, geoOrigin.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {orientation_changed.name(), orientation_changed.kind,
       orientation_changed.access()},
      {position_changed.name(), position_changed.kind,
       position_changed.access()},
      {size.name(), size.kind, size.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
