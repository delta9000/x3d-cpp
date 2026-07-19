#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DUrlObject {
  static constexpr std::string_view x3d_name = "X3DUrlObject";
  inline static constexpr field_key<X3DUrlObject,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<X3DUrlObject,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<X3DUrlObject, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DUrlObject, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<X3DUrlObject,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
