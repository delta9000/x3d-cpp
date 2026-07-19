#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LinePickSensor {
  static constexpr std::string_view x3d_name = "LinePickSensor";
  inline static constexpr field_key<LinePickSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string>
      intersectionType{"intersectionType", access_type::initialize_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::enum_value>
      matchCriterion{"matchCriterion", access_type::input_output};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickedGeometry{"pickedGeometry", access_type::output_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::vec3f_list>
      pickedNormal{"pickedNormal", access_type::output_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::vec3f_list>
      pickedPoint{"pickedPoint", access_type::output_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::vec3f_list>
      pickedTextureCoordinate{"pickedTextureCoordinate",
                              access_type::output_only};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::node_id>
      pickingGeometry{"pickingGeometry", access_type::input_output};
  inline static constexpr field_key<LinePickSensor,
                                    ::x3d::sai::experimental::node_list>
      pickTarget{"pickTarget", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string> sortOrder{
      "sortOrder", access_type::initialize_only};
  inline static constexpr field_key<LinePickSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LinePickSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
