#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PeriodicWave {
  static constexpr std::string_view x3d_name = "PeriodicWave";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PeriodicWave, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::float_list>
      optionsImag{"optionsImag", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::float_list>
      optionsReal{"optionsReal", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> type{
      "type", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {optionsImag.name(), optionsImag.kind, optionsImag.access()},
      {optionsReal.name(), optionsReal.kind, optionsReal.access()},
      {type.name(), type.kind, type.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
