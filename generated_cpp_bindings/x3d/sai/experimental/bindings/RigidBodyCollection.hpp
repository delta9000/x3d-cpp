#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct RigidBodyCollection {
  static constexpr std::string_view x3d_name = "RigidBodyCollection";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<RigidBodyCollection, bool> autoDisable{
      "autoDisable", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<RigidBodyCollection, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_list>
      bodies{"bodies", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_id>
      collider{"collider", access_type::initialize_only};
  inline static constexpr field_key<RigidBodyCollection, float>
      constantForceMix{"constantForceMix", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, float>
      contactSurfaceThickness{"contactSurfaceThickness",
                              access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, float>
      disableAngularSpeed{"disableAngularSpeed", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, float>
      disableLinearSpeed{"disableLinearSpeed", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::time_value>
      disableTime{"disableTime", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, float> errorCorrection{
      "errorCorrection", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::vec3f>
      gravity{"gravity", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::int32_t>
      iterations{"iterations", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_list>
      joints{"joints", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, float>
      maxCorrectionSpeed{"maxCorrectionSpeed", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, bool> preferAccuracy{
      "preferAccuracy", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection,
                                    ::x3d::sai::experimental::node_list>
      set_contacts{"set_contacts", access_type::input_only};
  inline static constexpr field_key<RigidBodyCollection, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<RigidBodyCollection, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 27> field_keys{{
      {autoDisable.name(), autoDisable.kind, autoDisable.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {bodies.name(), bodies.kind, bodies.access()},
      {collider.name(), collider.kind, collider.access()},
      {constantForceMix.name(), constantForceMix.kind,
       constantForceMix.access()},
      {contactSurfaceThickness.name(), contactSurfaceThickness.kind,
       contactSurfaceThickness.access()},
      {disableAngularSpeed.name(), disableAngularSpeed.kind,
       disableAngularSpeed.access()},
      {disableLinearSpeed.name(), disableLinearSpeed.kind,
       disableLinearSpeed.access()},
      {disableTime.name(), disableTime.kind, disableTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {errorCorrection.name(), errorCorrection.kind, errorCorrection.access()},
      {gravity.name(), gravity.kind, gravity.access()},
      {IS.name(), IS.kind, IS.access()},
      {iterations.name(), iterations.kind, iterations.access()},
      {joints.name(), joints.kind, joints.access()},
      {maxCorrectionSpeed.name(), maxCorrectionSpeed.kind,
       maxCorrectionSpeed.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {preferAccuracy.name(), preferAccuracy.kind, preferAccuracy.access()},
      {set_contacts.name(), set_contacts.kind, set_contacts.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
