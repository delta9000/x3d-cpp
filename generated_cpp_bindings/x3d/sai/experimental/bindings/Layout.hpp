#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Layout {
  static constexpr std::string_view x3d_name = "Layout";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      align{"align", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Layout,
                                    ::x3d::sai::experimental::float_list>
      offset{"offset", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      offsetUnits{"offsetUnits", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      scaleMode{"scaleMode", access_type::input_output};
  inline static constexpr field_key<Layout,
                                    ::x3d::sai::experimental::float_list>
      size{"size", access_type::input_output};
  inline static constexpr field_key<Layout, ::x3d::sai::experimental::enum_list>
      sizeUnits{"sizeUnits", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Layout, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {align.name(), align.kind, align.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {offset.name(), offset.kind, offset.access()},
      {offsetUnits.name(), offsetUnits.kind, offsetUnits.access()},
      {scaleMode.name(), scaleMode.kind, scaleMode.access()},
      {size.name(), size.kind, size.access()},
      {sizeUnits.name(), sizeUnits.kind, sizeUnits.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
