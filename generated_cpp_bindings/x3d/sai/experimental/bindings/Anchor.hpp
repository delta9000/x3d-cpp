#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Anchor {
  static constexpr std::string_view x3d_name = "Anchor";
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<Anchor,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<Anchor,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<Anchor, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Anchor, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Anchor,
                                    ::x3d::sai::experimental::string_list>
      parameter{"parameter", access_type::input_output};
  inline static constexpr field_key<Anchor, ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<Anchor,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<Anchor, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Anchor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
