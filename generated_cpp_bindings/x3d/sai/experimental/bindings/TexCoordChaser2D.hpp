#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TexCoordChaser2D {
  static constexpr std::string_view x3d_name = "TexCoordChaser2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::time_value>
      duration{"duration", access_type::initialize_only};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::vec2f_list>
      initialDestination{"initialDestination", access_type::initialize_only};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::vec2f_list>
      initialValue{"initialValue", access_type::initialize_only};
  inline static constexpr field_key<TexCoordChaser2D, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::vec2f_list>
      set_destination{"set_destination", access_type::input_only};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::vec2f_list>
      set_value{"set_value", access_type::input_only};
  inline static constexpr field_key<TexCoordChaser2D,
                                    ::x3d::sai::experimental::vec2f_list>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {duration.name(), duration.kind, duration.access()},
      {initialDestination.name(), initialDestination.kind,
       initialDestination.access()},
      {initialValue.name(), initialValue.kind, initialValue.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_destination.name(), set_destination.kind, set_destination.access()},
      {set_value.name(), set_value.kind, set_value.access()},
      {value_changed.name(), value_changed.kind, value_changed.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
