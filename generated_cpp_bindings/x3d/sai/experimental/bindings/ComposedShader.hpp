#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ComposedShader {
  static constexpr std::string_view x3d_name = "ComposedShader";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ComposedShader, bool> activate{
      "activate", access_type::input_only};
  inline static constexpr field_key<ComposedShader,
                                    ::x3d::sai::experimental::node_list>
      field{"field", access_type::input_output};
  inline static constexpr field_key<ComposedShader,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ComposedShader, bool> isSelected{
      "isSelected", access_type::output_only};
  inline static constexpr field_key<ComposedShader, bool> isValid{
      "isValid", access_type::output_only};
  inline static constexpr field_key<ComposedShader, std::string> language{
      "language", access_type::initialize_only};
  inline static constexpr field_key<ComposedShader,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ComposedShader,
                                    ::x3d::sai::experimental::node_list>
      parts{"parts", access_type::input_output};
  inline static constexpr field_key<ComposedShader, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ComposedShader, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ComposedShader, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ComposedShader, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ComposedShader, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {activate.name(), activate.kind, activate.access()},
      {field.name(), field.kind, field.access()},
      {IS.name(), IS.kind, IS.access()},
      {isSelected.name(), isSelected.kind, isSelected.access()},
      {isValid.name(), isValid.kind, isValid.access()},
      {language.name(), language.kind, language.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {parts.name(), parts.kind, parts.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
