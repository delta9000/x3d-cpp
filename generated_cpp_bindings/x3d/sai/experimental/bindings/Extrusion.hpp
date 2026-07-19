#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Extrusion {
  static constexpr std::string_view x3d_name = "Extrusion";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Extrusion, bool> beginCap{
      "beginCap", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> convex{
      "convex", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, float> creaseAngle{
      "creaseAngle", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      crossSection{"crossSection", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> endCap{
      "endCap", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::rotation_list>
      orientation{"orientation", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      scale{"scale", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      set_crossSection{"set_crossSection", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::rotation_list>
      set_orientation{"set_orientation", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      set_scale{"set_scale", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec3f_list>
      set_spine{"set_spine", access_type::input_only};
  inline static constexpr field_key<Extrusion, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec3f_list>
      spine{"spine", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 21> field_keys{{
      {beginCap.name(), beginCap.kind, beginCap.access()},
      {ccw.name(), ccw.kind, ccw.access()},
      {convex.name(), convex.kind, convex.access()},
      {creaseAngle.name(), creaseAngle.kind, creaseAngle.access()},
      {crossSection.name(), crossSection.kind, crossSection.access()},
      {endCap.name(), endCap.kind, endCap.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {orientation.name(), orientation.kind, orientation.access()},
      {scale.name(), scale.kind, scale.access()},
      {set_crossSection.name(), set_crossSection.kind,
       set_crossSection.access()},
      {set_orientation.name(), set_orientation.kind, set_orientation.access()},
      {set_scale.name(), set_scale.kind, set_scale.access()},
      {set_spine.name(), set_spine.kind, set_spine.access()},
      {solid.name(), solid.kind, solid.access()},
      {spine.name(), spine.kind, spine.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
