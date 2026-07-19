#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeneratedCubeMapTexture {
  static constexpr std::string_view x3d_name = "GeneratedCubeMapTexture";
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::int32_t> size{
      "size", access_type::initialize_only};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::enum_value>
      update{"update", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
