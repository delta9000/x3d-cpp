#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ImageCubeMapTexture {
  static constexpr std::string_view x3d_name = "ImageCubeMapTexture";
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<ImageCubeMapTexture,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ImageCubeMapTexture, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
