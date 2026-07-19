#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoLOD {
  static constexpr std::string_view x3d_name = "GeoLOD";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3d>
      center{"center", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child1Url{"child1Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child2Url{"child2Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child3Url{"child3Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child4Url{"child4Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_list>
      children{"children", access_type::output_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::int32_t> level_changed{
      "level_changed", access_type::output_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoLOD, float> range{
      "range", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_list>
      rootNode{"rootNode", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      rootUrl{"rootUrl", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 23> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {center.name(), center.kind, center.access()},
      {child1Url.name(), child1Url.kind, child1Url.access()},
      {child2Url.name(), child2Url.kind, child2Url.access()},
      {child3Url.name(), child3Url.kind, child3Url.access()},
      {child4Url.name(), child4Url.kind, child4Url.access()},
      {children.name(), children.kind, children.access()},
      {geoOrigin.name(), geoOrigin.kind, geoOrigin.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {IS.name(), IS.kind, IS.access()},
      {level_changed.name(), level_changed.kind, level_changed.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {range.name(), range.kind, range.access()},
      {rootNode.name(), rootNode.kind, rootNode.access()},
      {rootUrl.name(), rootUrl.kind, rootUrl.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
