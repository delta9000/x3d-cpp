#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct RigidBodyCollection {
  static constexpr std::string_view x3d_name = "RigidBodyCollection";
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
};

} // namespace x3d::sai::experimental::bindings
