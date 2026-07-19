#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DDragSensorNode {
  static constexpr std::string_view x3d_name = "X3DDragSensorNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DDragSensorNode, bool> autoOffset{
      "autoOffset", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DDragSensorNode, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<X3DDragSensorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode,
                                    ::x3d::sai::experimental::vec3f>
      trackPoint_changed{"trackPoint_changed", access_type::output_only};
  inline static constexpr field_key<X3DDragSensorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DDragSensorNode, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {autoOffset.name(), autoOffset.kind, autoOffset.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isOver.name(), isOver.kind, isOver.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {trackPoint_changed.name(), trackPoint_changed.kind,
       trackPoint_changed.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
