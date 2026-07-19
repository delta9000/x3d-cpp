#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimDisplacer {
  static constexpr std::string_view x3d_name = "HAnimDisplacer";
  inline static constexpr field_key<HAnimDisplacer,
                                    ::x3d::sai::experimental::int32_list>
      coordIndex{"coordIndex", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer,
                                    ::x3d::sai::experimental::vec3f_list>
      displacements{"displacements", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, float> weight{
      "weight", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimDisplacer, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
