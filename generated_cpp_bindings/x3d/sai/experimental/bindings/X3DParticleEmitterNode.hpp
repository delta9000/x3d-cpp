#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DParticleEmitterNode {
  static constexpr std::string_view x3d_name = "X3DParticleEmitterNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {IS.name(), IS.kind, IS.access()},
      {mass.name(), mass.kind, mass.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {on.name(), on.kind, on.access()},
      {speed.name(), speed.kind, speed.access()},
      {surfaceArea.name(), surfaceArea.kind, surfaceArea.access()},
      {variation.name(), variation.kind, variation.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
