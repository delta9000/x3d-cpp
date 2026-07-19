#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PlaneSensor {
  static constexpr std::string_view x3d_name = "PlaneSensor";
  inline static constexpr field_key<PlaneSensor, bool> autoOffset{
      "autoOffset", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::rotation>
      axisRotation{"axisRotation", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<PlaneSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::vec2f>
      maxPosition{"maxPosition", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::vec2f>
      minPosition{"minPosition", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::vec3f>
      offset{"offset", access_type::input_output};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::vec3f>
      trackPoint_changed{"trackPoint_changed", access_type::output_only};
  inline static constexpr field_key<PlaneSensor,
                                    ::x3d::sai::experimental::vec3f>
      translation_changed{"translation_changed", access_type::output_only};
  inline static constexpr field_key<PlaneSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PlaneSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
