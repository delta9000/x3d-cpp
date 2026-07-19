#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Extrusion {
  static constexpr std::string_view x3d_name = "Extrusion";
  inline static constexpr field_key<Extrusion, bool> beginCap{
      "beginCap", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> convex{
      "convex", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, float> creaseAngle{
      "creaseAngle", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      crossSection{"crossSection", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, bool> endCap{
      "endCap", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::rotation_list>
      orientation{"orientation", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      scale{"scale", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      set_crossSection{"set_crossSection", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::rotation_list>
      set_orientation{"set_orientation", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec2f_list>
      set_scale{"set_scale", access_type::input_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec3f_list>
      set_spine{"set_spine", access_type::input_only};
  inline static constexpr field_key<Extrusion, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Extrusion,
                                    ::x3d::sai::experimental::vec3f_list>
      spine{"spine", access_type::initialize_only};
  inline static constexpr field_key<Extrusion, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Extrusion, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
