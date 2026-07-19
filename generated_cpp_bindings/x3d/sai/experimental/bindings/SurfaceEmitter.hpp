#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SurfaceEmitter {
  static constexpr std::string_view x3d_name = "SurfaceEmitter";
  inline static constexpr field_key<SurfaceEmitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter,
                                    ::x3d::sai::experimental::node_id>
      surface{"surface", access_type::initialize_only};
  inline static constexpr field_key<SurfaceEmitter, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, float> variation{
      "variation", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SurfaceEmitter, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
