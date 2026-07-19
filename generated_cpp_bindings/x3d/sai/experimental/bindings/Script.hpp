#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Script {
  static constexpr std::string_view x3d_name = "Script";
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
};

} // namespace x3d::sai::experimental::bindings
