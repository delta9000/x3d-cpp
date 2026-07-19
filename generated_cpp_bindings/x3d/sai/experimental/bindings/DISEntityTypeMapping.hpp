#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DISEntityTypeMapping {
  static constexpr std::string_view x3d_name = "DISEntityTypeMapping";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {category.name(), category.kind, category.access()},
      {country.name(), country.kind, country.access()},
      {description.name(), description.kind, description.access()},
      {domain.name(), domain.kind, domain.access()},
      {extra.name(), extra.kind, extra.access()},
      {IS.name(), IS.kind, IS.access()},
      {kind.name(), kind.kind, kind.access()},
      {load.name(), load.kind, load.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {specific.name(), specific.kind, specific.access()},
      {subcategory.name(), subcategory.kind, subcategory.access()},
      {url.name(), url.kind, url.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
