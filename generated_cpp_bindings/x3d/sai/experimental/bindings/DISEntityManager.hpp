#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DISEntityManager {
  static constexpr std::string_view x3d_name = "DISEntityManager";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_list>
      addedEntities{"addedEntities", access_type::output_only};
  inline static constexpr field_key<DISEntityManager, std::string> address{
      "address", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::int32_t>
      applicationID{"applicationID", access_type::input_output};
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_list>
      removedEntities{"removedEntities", access_type::output_only};
  inline static constexpr field_key<DISEntityManager, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {addedEntities.name(), addedEntities.kind, addedEntities.access()},
      {address.name(), address.kind, address.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {children.name(), children.kind, children.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {port.name(), port.kind, port.access()},
      {removedEntities.name(), removedEntities.kind, removedEntities.access()},
      {siteID.name(), siteID.kind, siteID.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
