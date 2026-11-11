#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct Contour2D {
  static constexpr std::string_view x3d_name = "Contour2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Contour2D,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<Contour2D,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<Contour2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Contour2D,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {children.name(), children.kind, children.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
