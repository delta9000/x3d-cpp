#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PrimitivePickSensor {
  static constexpr std::string_view x3d_name = "PrimitivePickSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PrimitivePickSensor, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string>
      intersectionType{"intersectionType", access_type::initialize_only};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::enum_value>
      matchCriterion{"matchCriterion", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickedGeometry{"pickedGeometry", access_type::output_only};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::node_id>
      pickingGeometry{"pickingGeometry", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickTarget{"pickTarget", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string> sortOrder{
      "sortOrder", access_type::initialize_only};
  inline static constexpr field_key<PrimitivePickSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PrimitivePickSensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 17> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {intersectionType.name(), intersectionType.kind,
       intersectionType.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {matchCriterion.name(), matchCriterion.kind, matchCriterion.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {objectType.name(), objectType.kind, objectType.access()},
      {pickedGeometry.name(), pickedGeometry.kind, pickedGeometry.access()},
      {pickingGeometry.name(), pickingGeometry.kind, pickingGeometry.access()},
      {pickTarget.name(), pickTarget.kind, pickTarget.access()},
      {sortOrder.name(), sortOrder.kind, sortOrder.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
