#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsSweptSurface {
  static constexpr std::string_view x3d_name = "NurbsSweptSurface";
  inline static constexpr field_key<NurbsSweptSurface, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<NurbsSweptSurface,
                                    ::x3d::sai::experimental::node_id>
      crossSectionCurve{"crossSectionCurve", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<NurbsSweptSurface,
                                    ::x3d::sai::experimental::node_id>
      trajectoryCurve{"trajectoryCurve", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsSweptSurface, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
