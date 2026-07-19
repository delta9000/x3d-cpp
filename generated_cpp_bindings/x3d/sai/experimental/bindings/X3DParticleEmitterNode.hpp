#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DParticleEmitterNode {
  static constexpr std::string_view x3d_name = "X3DParticleEmitterNode";
  inline static constexpr field_key<X3DParticleEmitterNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, bool> on{
      "on", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, float> surfaceArea{
      "surfaceArea", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, float> variation{
      "variation", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DParticleEmitterNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
