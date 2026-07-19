#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimHumanoid {
  static constexpr std::string_view x3d_name = "HAnimHumanoid";
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<HAnimHumanoid, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::string_list>
      info{"info", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f_list>
      jointBindingPositions{"jointBindingPositions", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::rotation_list>
      jointBindingRotations{"jointBindingRotations", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f_list>
      jointBindingScales{"jointBindingScales", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      joints{"joints", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::int32_t> loa{
      "loa", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      motions{"motions", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::bool_list>
      motionsEnabled{"motionsEnabled", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::rotation>
      scaleOrientation{"scaleOrientation", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      segments{"segments", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      sites{"sites", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string>
      skeletalConfiguration{"skeletalConfiguration", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      skeleton{"skeleton", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      skin{"skin", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      skinBindingCoords{"skinBindingCoords", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      skinBindingNormals{"skinBindingNormals", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      skinCoord{"skinCoord", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_id>
      skinNormal{"skinNormal", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::enum_value>
      version{"version", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid,
                                    ::x3d::sai::experimental::node_list>
      viewpoints{"viewpoints", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimHumanoid, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
