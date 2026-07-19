#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ProtoInstance {
  static constexpr std::string_view x3d_name = "ProtoInstance";
  inline static constexpr field_key<ProtoInstance,
                                    ::x3d::sai::experimental::node_list>
      fieldValue{"fieldValue", access_type::input_output};
  inline static constexpr field_key<ProtoInstance,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ProtoInstance,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ProtoInstance, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
