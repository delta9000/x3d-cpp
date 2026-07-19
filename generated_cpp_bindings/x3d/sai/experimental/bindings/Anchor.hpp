#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Anchor {
  static constexpr std::string_view x3d_name = "Anchor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 20> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {IS.name(), IS.kind, IS.access()},
      {load.name(), load.kind, load.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {parameter.name(), parameter.kind, parameter.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {url.name(), url.kind, url.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
