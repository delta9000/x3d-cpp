#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Background {
  static constexpr std::string_view x3d_name = "Background";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      backUrl{"backUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      bottomUrl{"bottomUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      frontUrl{"frontUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::float_list>
      groundAngle{"groundAngle", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::color3f_list>
      groundColor{"groundColor", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Background, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      leftUrl{"leftUrl", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      rightUrl{"rightUrl", access_type::input_output};
  inline static constexpr field_key<Background, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::float_list>
      skyAngle{"skyAngle", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::color3f_list>
      skyColor{"skyColor", access_type::input_output};
  inline static constexpr field_key<Background,
                                    ::x3d::sai::experimental::string_list>
      topUrl{"topUrl", access_type::input_output};
  inline static constexpr field_key<Background, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<Background, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Background, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Background, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Background, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Background, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 21> field_keys{{
      {backUrl.name(), backUrl.kind, backUrl.access()},
      {bindTime.name(), bindTime.kind, bindTime.access()},
      {bottomUrl.name(), bottomUrl.kind, bottomUrl.access()},
      {frontUrl.name(), frontUrl.kind, frontUrl.access()},
      {groundAngle.name(), groundAngle.kind, groundAngle.access()},
      {groundColor.name(), groundColor.kind, groundColor.access()},
      {IS.name(), IS.kind, IS.access()},
      {isBound.name(), isBound.kind, isBound.access()},
      {leftUrl.name(), leftUrl.kind, leftUrl.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {rightUrl.name(), rightUrl.kind, rightUrl.access()},
      {set_bind.name(), set_bind.kind, set_bind.access()},
      {skyAngle.name(), skyAngle.kind, skyAngle.access()},
      {skyColor.name(), skyColor.kind, skyColor.access()},
      {topUrl.name(), topUrl.kind, topUrl.access()},
      {transparency.name(), transparency.kind, transparency.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
