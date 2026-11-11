#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct PointEmitter {
  static constexpr std::string_view x3d_name = "PointEmitter";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PointEmitter,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<PointEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<PointEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PointEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<PointEmitter,
                                    ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<PointEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<PointEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<PointEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {direction.name(), direction.kind, direction.access()},
      {mass.name(), mass.kind, mass.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {position.name(), position.kind, position.access()},
      {speed.name(), speed.kind, speed.access()},
      {surfaceArea.name(), surfaceArea.kind, surfaceArea.access()},
      {variation.name(), variation.kind, variation.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
