#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TouchSensor {
  static constexpr std::string_view x3d_name = "TouchSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TouchSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<TouchSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::vec3f>
      hitNormal_changed{"hitNormal_changed", access_type::output_only};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::vec3f>
      hitPoint_changed{"hitPoint_changed", access_type::output_only};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::vec2f>
      hitTexCoord_changed{"hitTexCoord_changed", access_type::output_only};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TouchSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<TouchSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TouchSensor,
                                    ::x3d::sai::experimental::time_value>
      touchTime{"touchTime", access_type::output_only};
  inline static constexpr field_key<TouchSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TouchSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TouchSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TouchSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TouchSensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {hitNormal_changed.name(), hitNormal_changed.kind,
       hitNormal_changed.access()},
      {hitPoint_changed.name(), hitPoint_changed.kind,
       hitPoint_changed.access()},
      {hitTexCoord_changed.name(), hitTexCoord_changed.kind,
       hitTexCoord_changed.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isOver.name(), isOver.kind, isOver.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {touchTime.name(), touchTime.kind, touchTime.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
