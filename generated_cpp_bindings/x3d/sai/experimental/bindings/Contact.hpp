#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Contact {
  static constexpr std::string_view x3d_name = "Contact";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 23> field_keys{{
      {appliedParameters.name(), appliedParameters.kind,
       appliedParameters.access()},
      {body1.name(), body1.kind, body1.access()},
      {body2.name(), body2.kind, body2.access()},
      {bounce.name(), bounce.kind, bounce.access()},
      {contactNormal.name(), contactNormal.kind, contactNormal.access()},
      {depth.name(), depth.kind, depth.access()},
      {frictionCoefficients.name(), frictionCoefficients.kind,
       frictionCoefficients.access()},
      {frictionDirection.name(), frictionDirection.kind,
       frictionDirection.access()},
      {geometry1.name(), geometry1.kind, geometry1.access()},
      {geometry2.name(), geometry2.kind, geometry2.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minBounceSpeed.name(), minBounceSpeed.kind, minBounceSpeed.access()},
      {position.name(), position.kind, position.access()},
      {slipCoefficients.name(), slipCoefficients.kind,
       slipCoefficients.access()},
      {softnessConstantForceMix.name(), softnessConstantForceMix.kind,
       softnessConstantForceMix.access()},
      {softnessErrorCorrection.name(), softnessErrorCorrection.kind,
       softnessErrorCorrection.access()},
      {surfaceSpeed.name(), surfaceSpeed.kind, surfaceSpeed.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
