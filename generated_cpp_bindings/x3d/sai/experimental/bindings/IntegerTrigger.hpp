#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IntegerTrigger {
  static constexpr std::string_view x3d_name = "IntegerTrigger";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<IntegerTrigger, std::int32_t> integerKey{
      "integerKey", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, bool> set_boolean{
      "set_boolean", access_type::input_only};
  inline static constexpr field_key<IntegerTrigger, std::int32_t> triggerValue{
      "triggerValue", access_type::output_only};
  inline static constexpr field_key<IntegerTrigger, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IntegerTrigger, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 10> field_keys{{
      {integerKey.name(), integerKey.kind, integerKey.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {set_boolean.name(), set_boolean.kind, set_boolean.access()},
      {triggerValue.name(), triggerValue.kind, triggerValue.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
