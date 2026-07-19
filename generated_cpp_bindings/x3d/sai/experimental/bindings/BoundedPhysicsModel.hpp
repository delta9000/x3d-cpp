#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BoundedPhysicsModel {
  static constexpr std::string_view x3d_name = "BoundedPhysicsModel";
  inline static constexpr field_key<BoundedPhysicsModel, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<BoundedPhysicsModel, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
