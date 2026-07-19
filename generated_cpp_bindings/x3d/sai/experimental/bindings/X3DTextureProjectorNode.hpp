#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTextureProjectorNode {
  static constexpr std::string_view x3d_name = "X3DTextureProjectorNode";
  inline static constexpr field_key<X3DTextureProjectorNode, float>
      ambientIntensity{"ambientIntensity", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, float> aspectRatio{
      "aspectRatio", access_type::output_only};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::color3f>
      color{"color", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::vec3f>
      direction{"direction", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, bool> global{
      "global", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, float> intensity{
      "intensity", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::vec3f>
      location{"location", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, float>
      nearDistance{"nearDistance", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, float>
      shadowIntensity{"shadowIntensity", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, bool> shadows{
      "shadows", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode,
                                    ::x3d::sai::experimental::node_id>
      texture{"texture", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DTextureProjectorNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
