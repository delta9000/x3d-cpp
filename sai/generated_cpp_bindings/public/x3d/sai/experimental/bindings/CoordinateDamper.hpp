#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct CoordinateDamper {
  static constexpr std::string_view x3d_name = "CoordinateDamper";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      initialDestination{"initialDestination", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      initialValue{"initialValue", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      set_destination{"set_destination", access_type::input_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      set_value{"set_value", access_type::input_only};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::time_value>
      tau{"tau", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper, float> tolerance{
      "tolerance", access_type::input_output};
  inline static constexpr field_key<CoordinateDamper,
                                    ::x3d::sai::experimental::vec3f_list>
      value_changed{"value_changed", access_type::output_only};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {initialDestination.name(), initialDestination.kind,
       initialDestination.access()},
      {initialValue.name(), initialValue.kind, initialValue.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {order.name(), order.kind, order.access()},
      {set_destination.name(), set_destination.kind, set_destination.access()},
      {set_value.name(), set_value.kind, set_value.access()},
      {tau.name(), tau.kind, tau.access()},
      {tolerance.name(), tolerance.kind, tolerance.access()},
      {value_changed.name(), value_changed.kind, value_changed.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
