#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct NurbsTrimmedSurface {
  static constexpr std::string_view x3d_name = "NurbsTrimmedSurface";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_list>
      addTrimmingContour{"addTrimmingContour", access_type::input_only};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_id>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_list>
      removeTrimmingContour{"removeTrimmingContour", access_type::input_only};
  inline static constexpr field_key<NurbsTrimmedSurface, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::node_list>
      trimmingContour{"trimmingContour", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, bool> uClosed{
      "uClosed", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t>
      uDimension{"uDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::double_list>
      uKnot{"uKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t> uOrder{
      "uOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t>
      uTessellation{"uTessellation", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, bool> vClosed{
      "vClosed", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t>
      vDimension{"vDimension", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::double_list>
      vKnot{"vKnot", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t> vOrder{
      "vOrder", access_type::initialize_only};
  inline static constexpr field_key<NurbsTrimmedSurface, std::int32_t>
      vTessellation{"vTessellation", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface,
                                    ::x3d::sai::experimental::double_list>
      weight{"weight", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<NurbsTrimmedSurface, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 24> field_keys{{
      {addTrimmingContour.name(), addTrimmingContour.kind,
       addTrimmingContour.access()},
      {controlPoint.name(), controlPoint.kind, controlPoint.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {removeTrimmingContour.name(), removeTrimmingContour.kind,
       removeTrimmingContour.access()},
      {solid.name(), solid.kind, solid.access()},
      {texCoord.name(), texCoord.kind, texCoord.access()},
      {trimmingContour.name(), trimmingContour.kind, trimmingContour.access()},
      {uClosed.name(), uClosed.kind, uClosed.access()},
      {uDimension.name(), uDimension.kind, uDimension.access()},
      {uKnot.name(), uKnot.kind, uKnot.access()},
      {uOrder.name(), uOrder.kind, uOrder.access()},
      {uTessellation.name(), uTessellation.kind, uTessellation.access()},
      {vClosed.name(), vClosed.kind, vClosed.access()},
      {vDimension.name(), vDimension.kind, vDimension.access()},
      {vKnot.name(), vKnot.kind, vKnot.access()},
      {vOrder.name(), vOrder.kind, vOrder.access()},
      {vTessellation.name(), vTessellation.kind, vTessellation.access()},
      {weight.name(), weight.kind, weight.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
