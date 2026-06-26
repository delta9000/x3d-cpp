// X3DFieldValue.hpp — discriminated union for X3D scalar/matrix SF field values.
// Used by MaterialExtensionField (RenderItem.hpp) and ShaderFieldBinding
// (Phase 3 shader descriptors).
//
// DESIGN NOTE — circular-dependency avoidance:
//   This header is included BY RenderItem.hpp (which defines TextureRef), so
//   including RenderItem.hpp here would create a circular dependency.
//   TextureRef is therefore intentionally omitted from the variant; texture
//   bindings are handled at the ShaderFieldBinding level by the consumer.
//
// Types covered: every non-node, non-double SF type from X3D Annex I Table I.3.
#ifndef X3D_RUNTIME_EXTRACT_X3D_FIELD_VALUE_HPP
#define X3D_RUNTIME_EXTRACT_X3D_FIELD_VALUE_HPP

#include "X3DReflection.hpp"  // X3DFieldType
#include "X3Dtypes.hpp"       // SF* value types
#include <variant>

namespace x3d::runtime::extract {

// Discriminated union covering the scalar + matrix SF field types used in
// author-shader <field> declarations (ComposedShader, Script, etc.).
// TextureRef is intentionally omitted to preserve the leaf-header invariant:
// X3DFieldValue.hpp is included BY RenderItem.hpp (which defines TextureRef),
// so including RenderItem.hpp here would create a circular dependency.
// Texture bindings are handled at the ShaderFieldBinding level by the consumer.
struct X3DFieldValue {
  X3DFieldType type = X3DFieldType::SFString;
  std::variant<std::monostate, float, int, bool, SFColor, SFColorRGBA,
               SFVec2f, SFVec3f, SFVec4f, SFMatrix3f, SFMatrix4f,
               SFString> value;
};

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_X3D_FIELD_VALUE_HPP
