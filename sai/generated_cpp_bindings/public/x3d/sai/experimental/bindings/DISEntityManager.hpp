#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

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
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DISEntityManager, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<DISEntityManager,
                                    ::x3d::sai::experimental::node_list>
      removedEntities{"removedEntities", access_type::output_only};
  inline static constexpr field_key<DISEntityManager, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 8> field_keys{{
      {addedEntities.name(), addedEntities.kind, addedEntities.access()},
      {address.name(), address.kind, address.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {children.name(), children.kind, children.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {port.name(), port.kind, port.access()},
      {removedEntities.name(), removedEntities.kind, removedEntities.access()},
      {siteID.name(), siteID.kind, siteID.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
