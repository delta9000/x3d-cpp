#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DISEntityTypeMapping {
  static constexpr std::string_view x3d_name = "DISEntityTypeMapping";
  inline static constexpr field_key<DISEntityTypeMapping,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t>
      category{"category", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t> country{
      "country", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t> domain{
      "domain", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t> extra{
      "extra", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t> kind{
      "kind", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t>
      specific{"specific", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping, std::int32_t>
      subcategory{"subcategory", access_type::initialize_only};
  inline static constexpr field_key<DISEntityTypeMapping,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<DISEntityTypeMapping, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
