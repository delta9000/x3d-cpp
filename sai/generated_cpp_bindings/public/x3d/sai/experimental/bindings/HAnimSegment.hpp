#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimSegment {
  static constexpr std::string_view x3d_name = "HAnimSegment";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<HAnimSegment, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      centerOfMass{"centerOfMass", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      displacers{"displacers", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::float_list>
      momentsOfInertia{"momentsOfInertia", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSegment, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {centerOfMass.name(), centerOfMass.kind, centerOfMass.access()},
      {children.name(), children.kind, children.access()},
      {coord.name(), coord.kind, coord.access()},
      {description.name(), description.kind, description.access()},
      {displacers.name(), displacers.kind, displacers.access()},
      {mass.name(), mass.kind, mass.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {momentsOfInertia.name(), momentsOfInertia.kind,
       momentsOfInertia.access()},
      {name.name(), name.kind, name.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
