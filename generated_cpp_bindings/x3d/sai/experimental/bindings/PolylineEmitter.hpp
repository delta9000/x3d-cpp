#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PolylineEmitter {
  static constexpr std::string_view x3d_name = "PolylineEmitter";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<PolylineEmitter,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter,
                                    ::x3d::sai::experimental::int32_list>
      coordIndex{"coordIndex", access_type::initialize_only};
  inline static constexpr field_key<PolylineEmitter,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter,
                                    ::x3d::sai::experimental::int32_list>
      set_coordIndex{"set_coordIndex", access_type::input_only};
  inline static constexpr field_key<PolylineEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<PolylineEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {coord.name(), coord.kind, coord.access()},
      {coordIndex.name(), coordIndex.kind, coordIndex.access()},
      {direction.name(), direction.kind, direction.access()},
      {mass.name(), mass.kind, mass.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {set_coordIndex.name(), set_coordIndex.kind, set_coordIndex.access()},
      {speed.name(), speed.kind, speed.access()},
      {surfaceArea.name(), surfaceArea.kind, surfaceArea.access()},
      {variation.name(), variation.kind, variation.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
