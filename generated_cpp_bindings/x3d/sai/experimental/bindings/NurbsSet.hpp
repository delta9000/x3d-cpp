#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsSet {
  static constexpr std::string_view x3d_name = "NurbsSet";
  inline static constexpr field_key<NurbsSet,
                                    ::x3d::sai::experimental::node_list>
      addGeometry{"addGeometry", access_type::input_only};
  inline static constexpr field_key<NurbsSet, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<NurbsSet, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<NurbsSet, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<NurbsSet,
                                    ::x3d::sai::experimental::node_list>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<NurbsSet, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsSet, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsSet,
                                    ::x3d::sai::experimental::node_list>
      removeGeometry{"removeGeometry", access_type::input_only};
  inline static constexpr field_key<NurbsSet, float> tessellationScale{
      "tessellationScale", access_type::input_output};
  inline static constexpr field_key<NurbsSet, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<NurbsSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsSet, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
