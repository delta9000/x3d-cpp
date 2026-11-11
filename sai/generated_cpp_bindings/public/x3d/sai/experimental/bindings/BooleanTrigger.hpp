#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct BooleanTrigger {
  static constexpr std::string_view x3d_name = "BooleanTrigger";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<BooleanTrigger,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BooleanTrigger,
                                    ::x3d::sai::experimental::time_value>
      set_triggerTime{"set_triggerTime", access_type::input_only};
  inline static constexpr field_key<BooleanTrigger, bool> triggerTrue{
      "triggerTrue", access_type::output_only};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {set_triggerTime.name(), set_triggerTime.kind, set_triggerTime.access()},
      {triggerTrue.name(), triggerTrue.kind, triggerTrue.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
