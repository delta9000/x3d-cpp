#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ShaderPart {
  static constexpr std::string_view x3d_name = "ShaderPart";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ShaderPart,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<ShaderPart,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<ShaderPart, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ShaderPart, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<ShaderPart,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ShaderPart, std::string> sourceCode{
      "sourceCode", access_type::input_output};
  inline static constexpr field_key<ShaderPart, std::string> type{
      "type", access_type::initialize_only};
  inline static constexpr field_key<ShaderPart,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {description.name(), description.kind, description.access()},
      {load.name(), load.kind, load.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {sourceCode.name(), sourceCode.kind, sourceCode.access()},
      {type.name(), type.kind, type.access()},
      {url.name(), url.kind, url.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
