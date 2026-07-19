#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimSite {
  static constexpr std::string_view x3d_name = "HAnimSite";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<HAnimSite, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<HAnimSite,
                                    ::x3d::sai::experimental::rotation>
      scaleOrientation{"scaleOrientation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<HAnimSite, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimSite, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 21> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {center.name(), center.kind, center.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {rotation.name(), rotation.kind, rotation.access()},
      {scale.name(), scale.kind, scale.access()},
      {scaleOrientation.name(), scaleOrientation.kind,
       scaleOrientation.access()},
      {translation.name(), translation.kind, translation.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
