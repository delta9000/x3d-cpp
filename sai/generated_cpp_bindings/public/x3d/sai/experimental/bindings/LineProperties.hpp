#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct LineProperties {
  static constexpr std::string_view x3d_name = "LineProperties";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LineProperties, bool> applied{
      "applied", access_type::input_output};
  inline static constexpr field_key<LineProperties, std::int32_t> linetype{
      "linetype", access_type::input_output};
  inline static constexpr field_key<LineProperties, float> linewidthScaleFactor{
      "linewidthScaleFactor", access_type::input_output};
  inline static constexpr field_key<LineProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {applied.name(), applied.kind, applied.access()},
      {linetype.name(), linetype.kind, linetype.access()},
      {linewidthScaleFactor.name(), linewidthScaleFactor.kind,
       linewidthScaleFactor.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
