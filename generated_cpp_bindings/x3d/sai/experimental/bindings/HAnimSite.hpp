#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimSite {
  static constexpr std::string_view x3d_name = "HAnimSite";
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<HAnimSite, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::rotation>
      scaleOrientation{"scaleOrientation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
