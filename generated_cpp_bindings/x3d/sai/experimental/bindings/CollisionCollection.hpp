#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CollisionCollection {
  static constexpr std::string_view x3d_name = "CollisionCollection";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::enum_list>
      appliedParameters{"appliedParameters", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<CollisionCollection, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<CollisionCollection, float> bounce{
      "bounce", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::node_list>
      collidables{"collidables", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::vec2f>
      frictionCoefficients{"frictionCoefficients", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, float> minBounceSpeed{
      "minBounceSpeed", access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::vec2f>
      slipFactors{"slipFactors", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, float>
      softnessConstantForceMix{"softnessConstantForceMix",
                               access_type::input_output};
  inline static constexpr field_key<CollisionCollection, float>
      softnessErrorCorrection{"softnessErrorCorrection",
                              access_type::input_output};
  inline static constexpr field_key<CollisionCollection,
                                    ::x3d::sai::experimental::vec2f>
      surfaceSpeed{"surfaceSpeed", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CollisionCollection, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 22> field_keys{{
      {appliedParameters.name(), appliedParameters.kind,
       appliedParameters.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {bounce.name(), bounce.kind, bounce.access()},
      {collidables.name(), collidables.kind, collidables.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {frictionCoefficients.name(), frictionCoefficients.kind,
       frictionCoefficients.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minBounceSpeed.name(), minBounceSpeed.kind, minBounceSpeed.access()},
      {slipFactors.name(), slipFactors.kind, slipFactors.access()},
      {softnessConstantForceMix.name(), softnessConstantForceMix.kind,
       softnessConstantForceMix.access()},
      {softnessErrorCorrection.name(), softnessErrorCorrection.kind,
       softnessErrorCorrection.access()},
      {surfaceSpeed.name(), surfaceSpeed.kind, surfaceSpeed.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
