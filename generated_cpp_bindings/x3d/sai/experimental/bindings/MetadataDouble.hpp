#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MetadataDouble {
  static constexpr std::string_view x3d_name = "MetadataDouble";
  inline static constexpr field_key<MetadataDouble,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<MetadataDouble,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> reference{
      "reference", access_type::input_output};
  inline static constexpr field_key<MetadataDouble,
                                    ::x3d::sai::experimental::double_list>
      value{"value", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<MetadataDouble, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
