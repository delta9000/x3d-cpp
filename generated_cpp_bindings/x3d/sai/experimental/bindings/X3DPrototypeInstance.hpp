#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DPrototypeInstance {
  static constexpr std::string_view x3d_name = "X3DPrototypeInstance";
  inline static constexpr field_key<X3DPrototypeInstance,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DPrototypeInstance,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
