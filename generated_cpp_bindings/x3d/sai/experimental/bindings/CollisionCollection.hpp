#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CollisionCollection {
  static constexpr std::string_view x3d_name = "CollisionCollection";
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
};

} // namespace x3d::sai::experimental::bindings
