#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct Polyline2D {
  static constexpr std::string_view x3d_name = "Polyline2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Polyline2D,
                                    ::x3d::sai::experimental::vec2f_list>
      lineSegments{"lineSegments", access_type::initialize_only};
  inline static constexpr field_key<Polyline2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {lineSegments.name(), lineSegments.kind, lineSegments.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
