#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ImageTexture3D {
  static constexpr std::string_view x3d_name = "ImageTexture3D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, bool> repeatR{
      "repeatR", access_type::initialize_only};
  inline static constexpr field_key<ImageTexture3D, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<ImageTexture3D, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<ImageTexture3D,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ImageTexture3D, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 16> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {description.name(), description.kind, description.access()},
      {IS.name(), IS.kind, IS.access()},
      {load.name(), load.kind, load.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {repeatR.name(), repeatR.kind, repeatR.access()},
      {repeatS.name(), repeatS.kind, repeatS.access()},
      {repeatT.name(), repeatT.kind, repeatT.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
      {url.name(), url.kind, url.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
