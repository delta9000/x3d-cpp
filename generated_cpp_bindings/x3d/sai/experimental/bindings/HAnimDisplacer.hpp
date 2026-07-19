#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimDisplacer {
  static constexpr std::string_view x3d_name = "HAnimDisplacer";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {coordIndex.name(), coordIndex.kind, coordIndex.access()},
      {description.name(), description.kind, description.access()},
      {displacements.name(), displacements.kind, displacements.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {weight.name(), weight.kind, weight.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
