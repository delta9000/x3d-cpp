#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Coordinate {
  static constexpr std::string_view x3d_name = "Coordinate";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Coordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Coordinate,
                                    ::x3d::sai::experimental::vec3f_list>
      point{"point", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {point.name(), point.kind, point.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
