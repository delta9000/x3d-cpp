#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsTextureCoordinate {
  static constexpr std::string_view x3d_name = "NurbsTextureCoordinate";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::vec2f_list>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::int32_t>
      uDimension{"uDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::double_list>
      uKnot{"uKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate, std::int32_t>
      uOrder{"uOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate, std::int32_t>
      vDimension{"vDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::double_list>
      vKnot{"vKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate, std::int32_t>
      vOrder{"vOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsTextureCoordinate,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsTextureCoordinate, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {controlPoint.name(), controlPoint.kind, controlPoint.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {uDimension.name(), uDimension.kind, uDimension.access()},
      {uKnot.name(), uKnot.kind, uKnot.access()},
      {uOrder.name(), uOrder.kind, uOrder.access()},
      {vDimension.name(), vDimension.kind, vDimension.access()},
      {vKnot.name(), vKnot.kind, vKnot.access()},
      {vOrder.name(), vOrder.kind, vOrder.access()},
      {weight.name(), weight.kind, weight.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
