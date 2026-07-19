#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Script {
  static constexpr std::string_view x3d_name = "Script";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Script,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<Script,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<Script, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Script, bool> directOutput{
      "directOutput", access_type::initialize_only};
  inline static constexpr field_key<Script, ::x3d::sai::experimental::node_list>
      field{"field", access_type::input_output};
  inline static constexpr field_key<Script, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Script, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<Script, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Script, bool> mustEvaluate{
      "mustEvaluate", access_type::initialize_only};
  inline static constexpr field_key<Script, std::string> sourceCode{
      "sourceCode", access_type::input_output};
  inline static constexpr field_key<Script,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<Script, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Script, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Script, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Script, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Script, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 16> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {description.name(), description.kind, description.access()},
      {directOutput.name(), directOutput.kind, directOutput.access()},
      {field.name(), field.kind, field.access()},
      {IS.name(), IS.kind, IS.access()},
      {load.name(), load.kind, load.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {mustEvaluate.name(), mustEvaluate.kind, mustEvaluate.access()},
      {sourceCode.name(), sourceCode.kind, sourceCode.access()},
      {url.name(), url.kind, url.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
