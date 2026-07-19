#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Fog {
  static constexpr std::string_view x3d_name = "Fog";
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::enum_value>
      fogType{"fogType", access_type::input_output};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::node_id> IS{
      "IS", access_type::input_output};
  inline static constexpr field_key<Fog, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<Fog, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Fog, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<Fog, float> visibilityRange{
      "visibilityRange", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Fog, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
