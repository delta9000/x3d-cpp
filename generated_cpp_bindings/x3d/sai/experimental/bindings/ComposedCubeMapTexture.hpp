#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ComposedCubeMapTexture {
  static constexpr std::string_view x3d_name = "ComposedCubeMapTexture";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      backTexture{"backTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      bottomTexture{"bottomTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      frontTexture{"frontTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      leftTexture{"leftTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      rightTexture{"rightTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<ComposedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      topTexture{"topTexture", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ComposedCubeMapTexture, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {backTexture.name(), backTexture.kind, backTexture.access()},
      {bottomTexture.name(), bottomTexture.kind, bottomTexture.access()},
      {description.name(), description.kind, description.access()},
      {frontTexture.name(), frontTexture.kind, frontTexture.access()},
      {IS.name(), IS.kind, IS.access()},
      {leftTexture.name(), leftTexture.kind, leftTexture.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {rightTexture.name(), rightTexture.kind, rightTexture.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
      {topTexture.name(), topTexture.kind, topTexture.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
