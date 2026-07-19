#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct RigidBody {
  static constexpr std::string_view x3d_name = "RigidBody";
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
};

} // namespace x3d::sai::experimental::bindings
