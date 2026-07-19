#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Collision {
  static constexpr std::string_view x3d_name = "Collision";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<Collision, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<Collision, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<Collision, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::time_value>
      collideTime{"collideTime", access_type::output_only};
  inline static constexpr field_key<Collision, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Collision, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Collision, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_id>
      proxy{"proxy", access_type::initialize_only};
  inline static constexpr field_key<Collision,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<Collision, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<Collision, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Collision, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Collision, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Collision, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Collision, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {children.name(), children.kind, children.access()},
      {collideTime.name(), collideTime.kind, collideTime.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {proxy.name(), proxy.kind, proxy.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
