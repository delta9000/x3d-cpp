#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSequencerNode {
  static constexpr std::string_view x3d_name = "X3DSequencerNode";
  inline static constexpr field_key<X3DSequencerNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode, bool> next{
      "next", access_type::input_only};
  inline static constexpr field_key<X3DSequencerNode, bool> previous{
      "previous", access_type::input_only};
  inline static constexpr field_key<X3DSequencerNode, float> set_fraction{
      "set_fraction", access_type::input_only};
  inline static constexpr field_key<X3DSequencerNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DSequencerNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
