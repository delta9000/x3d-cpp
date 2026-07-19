#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Cylinder {
  static constexpr std::string_view x3d_name = "Cylinder";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Cylinder, bool> bottom{
      "bottom", access_type::input_output};
  inline static constexpr field_key<Cylinder, float> height{
      "height", access_type::initialize_only};
  inline static constexpr field_key<Cylinder, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Cylinder, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Cylinder, float> radius{
      "radius", access_type::initialize_only};
  inline static constexpr field_key<Cylinder, bool> side{
      "side", access_type::input_output};
  inline static constexpr field_key<Cylinder, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Cylinder, bool> top{
      "top", access_type::input_output};
  inline static constexpr field_key<Cylinder, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Cylinder, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Cylinder, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Cylinder, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Cylinder, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {bottom.name(), bottom.kind, bottom.access()},
      {height.name(), height.kind, height.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {radius.name(), radius.kind, radius.access()},
      {side.name(), side.kind, side.access()},
      {solid.name(), solid.kind, solid.access()},
      {top.name(), top.kind, top.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
