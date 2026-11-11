#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct LayoutLayer {
  static constexpr std::string_view x3d_name = "LayoutLayer";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      layout{"layout", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::string_list>
      objectType{"objectType", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, bool> pickable{
      "pickable", access_type::input_output};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<LayoutLayer,
                                    ::x3d::sai::experimental::node_id>
      viewport{"viewport", access_type::input_output};
  inline static constexpr field_key<LayoutLayer, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {children.name(), children.kind, children.access()},
      {layout.name(), layout.kind, layout.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {objectType.name(), objectType.kind, objectType.access()},
      {pickable.name(), pickable.kind, pickable.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {viewport.name(), viewport.kind, viewport.access()},
      {visible.name(), visible.kind, visible.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
