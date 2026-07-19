#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Contact {
  static constexpr std::string_view x3d_name = "Contact";
  inline static constexpr field_key<Contact,
                                    ::x3d::sai::experimental::enum_list>
      appliedParameters{"appliedParameters", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<Contact, float> bounce{
      "bounce", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec3f>
      contactNormal{"contactNormal", access_type::input_output};
  inline static constexpr field_key<Contact, float> depth{
      "depth", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec2f>
      frictionCoefficients{"frictionCoefficients", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec3f>
      frictionDirection{"frictionDirection", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      geometry1{"geometry1", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      geometry2{"geometry2", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Contact, float> minBounceSpeed{
      "minBounceSpeed", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec2f>
      slipCoefficients{"slipCoefficients", access_type::input_output};
  inline static constexpr field_key<Contact, float> softnessConstantForceMix{
      "softnessConstantForceMix", access_type::input_output};
  inline static constexpr field_key<Contact, float> softnessErrorCorrection{
      "softnessErrorCorrection", access_type::input_output};
  inline static constexpr field_key<Contact, ::x3d::sai::experimental::vec2f>
      surfaceSpeed{"surfaceSpeed", access_type::input_output};
  inline static constexpr field_key<Contact, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Contact, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Contact, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Contact, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Contact, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
