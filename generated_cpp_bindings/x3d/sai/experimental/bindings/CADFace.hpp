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
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CADFace, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<CADFace, ::x3d::sai::experimental::node_id>
      shape{"shape", access_type::input_output};
  inline static constexpr field_key<CADFace, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 7> field_keys{{
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {shape.name(), shape.kind, shape.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
