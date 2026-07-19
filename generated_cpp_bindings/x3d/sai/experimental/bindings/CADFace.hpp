#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CADFace {
  static constexpr std::string_view x3d_name = "CADFace";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<CADFace, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::node_id>
      shape{"shape", access_type::input_output};
  inline static constexpr field_key<CADFace, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {shape.name(), shape.kind, shape.access()},
      {visible.name(), visible.kind, visible.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
