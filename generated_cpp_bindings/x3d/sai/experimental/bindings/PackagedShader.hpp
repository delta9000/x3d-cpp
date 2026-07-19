#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PackagedShader {
  static constexpr std::string_view x3d_name = "PackagedShader";
  inline static constexpr field_key<PackagedShader, bool> activate{
      "activate", access_type::input_only};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::node_list>
      field{"field", access_type::input_output};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PackagedShader, bool> isSelected{
      "isSelected", access_type::output_only};
  inline static constexpr field_key<PackagedShader, bool> isValid{
      "isValid", access_type::output_only};
  inline static constexpr field_key<PackagedShader, std::string> language{
      "language", access_type::initialize_only};
  inline static constexpr field_key<PackagedShader, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PackagedShader,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PackagedShader, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
