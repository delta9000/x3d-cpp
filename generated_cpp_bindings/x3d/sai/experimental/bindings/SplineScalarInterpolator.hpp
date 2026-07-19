#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SplineScalarInterpolator {
  static constexpr std::string_view x3d_name = "SplineScalarInterpolator";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SplineScalarInterpolator, bool> closed{
      "closed", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator,
                                    ::x3d::sai::experimental::float_list>
      keyValue{"keyValue", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator,
                                    ::x3d::sai::experimental::float_list>
      keyVelocity{"keyVelocity", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, bool>
      normalizeVelocity{"normalizeVelocity", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, float>
      set_fraction{"set_fraction", access_type::input_only};
  inline static constexpr field_key<SplineScalarInterpolator, float>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr field_key<SplineScalarInterpolator, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SplineScalarInterpolator, std::string>
      style{"style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {closed.name(), closed.kind, closed.access()},
      {IS.name(), IS.kind, IS.access()},
      {key.name(), key.kind, key.access()},
      {keyValue.name(), keyValue.kind, keyValue.access()},
      {keyVelocity.name(), keyVelocity.kind, keyVelocity.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normalizeVelocity.name(), normalizeVelocity.kind,
       normalizeVelocity.access()},
      {set_fraction.name(), set_fraction.kind, set_fraction.access()},
      {value_changed.name(), value_changed.kind, value_changed.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
