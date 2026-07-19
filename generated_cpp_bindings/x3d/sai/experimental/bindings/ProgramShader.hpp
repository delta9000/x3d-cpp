#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ProgramShader {
  static constexpr std::string_view x3d_name = "ProgramShader";
  inline static constexpr field_key<ProgramShader, bool> activate{
      "activate", access_type::input_only};
  inline static constexpr field_key<ProgramShader,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ProgramShader, bool> isSelected{
      "isSelected", access_type::output_only};
  inline static constexpr field_key<ProgramShader, bool> isValid{
      "isValid", access_type::output_only};
  inline static constexpr field_key<ProgramShader, std::string> language{
      "language", access_type::initialize_only};
  inline static constexpr field_key<ProgramShader,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ProgramShader,
                                    ::x3d::sai::experimental::node_list>
      programs{"programs", access_type::input_output};
  inline static constexpr field_key<ProgramShader, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ProgramShader, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ProgramShader, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ProgramShader, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ProgramShader, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
