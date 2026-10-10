#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct VisibilitySensor {
  static constexpr std::string_view x3d_name = "VisibilitySensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<VisibilitySensor,
                                    ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<VisibilitySensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<VisibilitySensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<VisibilitySensor,
                                    ::x3d::sai::experimental::time_value>
      enterTime{"enterTime", access_type::output_only};
  inline static constexpr field_key<VisibilitySensor,
                                    ::x3d::sai::experimental::time_value>
      exitTime{"exitTime", access_type::output_only};
  inline static constexpr field_key<VisibilitySensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<VisibilitySensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<VisibilitySensor,
                                    ::x3d::sai::experimental::vec3f>
      size{"size", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {center.name(), center.kind, center.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {enterTime.name(), enterTime.kind, enterTime.access()},
      {exitTime.name(), exitTime.kind, exitTime.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {size.name(), size.kind, size.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
