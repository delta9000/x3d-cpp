#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTimeDependentNode {
  static constexpr std::string_view x3d_name = "X3DTimeDependentNode";
  inline static constexpr field_key<X3DTimeDependentNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DTimeDependentNode, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
