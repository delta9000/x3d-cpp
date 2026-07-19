#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct RigidBody {
  static constexpr std::string_view x3d_name = "RigidBody";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<RigidBody, float> angularDampingFactor{
      "angularDampingFactor", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      angularVelocity{"angularVelocity", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> autoDamp{
      "autoDamp", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> autoDisable{
      "autoDisable", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<RigidBody, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      centerOfMass{"centerOfMass", access_type::input_output};
  inline static constexpr field_key<RigidBody, float> disableAngularSpeed{
      "disableAngularSpeed", access_type::input_output};
  inline static constexpr field_key<RigidBody, float> disableLinearSpeed{
      "disableLinearSpeed", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::time_value>
      disableTime{"disableTime", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      finiteRotationAxis{"finiteRotationAxis", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> fixed{
      "fixed", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::vec3f_list>
      forces{"forces", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::node_list>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::matrix3f>
      inertia{"inertia", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<RigidBody, float> linearDampingFactor{
      "linearDampingFactor", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      linearVelocity{"linearVelocity", access_type::input_output};
  inline static constexpr field_key<RigidBody, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::node_id>
      massDensityModel{"massDensityModel", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<RigidBody, ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<RigidBody,
                                    ::x3d::sai::experimental::vec3f_list>
      torques{"torques", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> useFiniteRotation{
      "useFiniteRotation", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> useGlobalGravity{
      "useGlobalGravity", access_type::input_output};
  inline static constexpr field_key<RigidBody, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<RigidBody, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<RigidBody, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<RigidBody, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<RigidBody, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<RigidBody, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 34> field_keys{{
      {angularDampingFactor.name(), angularDampingFactor.kind,
       angularDampingFactor.access()},
      {angularVelocity.name(), angularVelocity.kind, angularVelocity.access()},
      {autoDamp.name(), autoDamp.kind, autoDamp.access()},
      {autoDisable.name(), autoDisable.kind, autoDisable.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {centerOfMass.name(), centerOfMass.kind, centerOfMass.access()},
      {disableAngularSpeed.name(), disableAngularSpeed.kind,
       disableAngularSpeed.access()},
      {disableLinearSpeed.name(), disableLinearSpeed.kind,
       disableLinearSpeed.access()},
      {disableTime.name(), disableTime.kind, disableTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {finiteRotationAxis.name(), finiteRotationAxis.kind,
       finiteRotationAxis.access()},
      {fixed.name(), fixed.kind, fixed.access()},
      {forces.name(), forces.kind, forces.access()},
      {geometry.name(), geometry.kind, geometry.access()},
      {inertia.name(), inertia.kind, inertia.access()},
      {IS.name(), IS.kind, IS.access()},
      {linearDampingFactor.name(), linearDampingFactor.kind,
       linearDampingFactor.access()},
      {linearVelocity.name(), linearVelocity.kind, linearVelocity.access()},
      {mass.name(), mass.kind, mass.access()},
      {massDensityModel.name(), massDensityModel.kind,
       massDensityModel.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {orientation.name(), orientation.kind, orientation.access()},
      {position.name(), position.kind, position.access()},
      {torques.name(), torques.kind, torques.access()},
      {useFiniteRotation.name(), useFiniteRotation.kind,
       useFiniteRotation.access()},
      {useGlobalGravity.name(), useGlobalGravity.kind,
       useGlobalGravity.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
