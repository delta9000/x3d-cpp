#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct SphereSensor {
  static constexpr std::string_view x3d_name = "SphereSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SphereSensor, bool> autoOffset{
      "autoOffset", access_type::input_output};
  inline static constexpr field_key<SphereSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<SphereSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<SphereSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<SphereSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::rotation>
      offset{"offset", access_type::input_output};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::rotation>
      rotation_changed{"rotation_changed", access_type::output_only};
  inline static constexpr field_key<SphereSensor,
                                    ::x3d::sai::experimental::vec3f>
      trackPoint_changed{"trackPoint_changed", access_type::output_only};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {autoOffset.name(), autoOffset.kind, autoOffset.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isOver.name(), isOver.kind, isOver.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {offset.name(), offset.kind, offset.access()},
      {rotation_changed.name(), rotation_changed.kind,
       rotation_changed.access()},
      {trackPoint_changed.name(), trackPoint_changed.kind,
       trackPoint_changed.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
