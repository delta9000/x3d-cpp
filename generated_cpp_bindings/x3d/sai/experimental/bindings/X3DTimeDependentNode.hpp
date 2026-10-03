#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTimeDependentNode {
  static constexpr std::string_view x3d_name = "X3DTimeDependentNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DTimeDependentNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DTimeDependentNode,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
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
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {description.name(), description.kind, description.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
