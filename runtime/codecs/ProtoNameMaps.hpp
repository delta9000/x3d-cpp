// ProtoNameMaps.hpp
// Inverse name maps for X3DFieldType and AccessType — the string->enum
// direction lives in XmlReader; these free functions go the other way.
//
// Reused by XmlWriter, JsonWriter, VrmlWriter. Do NOT depend on any writer.
// Header-only, namespace x3d::codec.
#ifndef X3D_PROTO_NAME_MAPS_HPP
#define X3D_PROTO_NAME_MAPS_HPP

#include "X3DReflection.hpp"

namespace x3d::codec {

/// Returns the X3D type-name string for a field type (e.g. SFVec3f -> "SFVec3f").
/// SFEnum -> "SFString", MFEnum -> "MFString" (X3D has no enum field type;
/// proto interfaces declare them as strings).
inline const char *fieldTypeName(X3DFieldType t) {
  switch (t) {
  case X3DFieldType::SFBool:       return "SFBool";
  case X3DFieldType::SFColor:      return "SFColor";
  case X3DFieldType::SFColorRGBA:  return "SFColorRGBA";
  case X3DFieldType::SFDouble:     return "SFDouble";
  case X3DFieldType::SFFloat:      return "SFFloat";
  case X3DFieldType::SFImage:      return "SFImage";
  case X3DFieldType::SFInt32:      return "SFInt32";
  case X3DFieldType::SFMatrix3d:   return "SFMatrix3d";
  case X3DFieldType::SFMatrix3f:   return "SFMatrix3f";
  case X3DFieldType::SFMatrix4d:   return "SFMatrix4d";
  case X3DFieldType::SFMatrix4f:   return "SFMatrix4f";
  case X3DFieldType::SFNode:       return "SFNode";
  case X3DFieldType::SFRotation:   return "SFRotation";
  case X3DFieldType::SFString:     return "SFString";
  case X3DFieldType::SFTime:       return "SFTime";
  case X3DFieldType::SFVec2d:      return "SFVec2d";
  case X3DFieldType::SFVec2f:      return "SFVec2f";
  case X3DFieldType::SFVec3d:      return "SFVec3d";
  case X3DFieldType::SFVec3f:      return "SFVec3f";
  case X3DFieldType::SFVec4d:      return "SFVec4d";
  case X3DFieldType::SFVec4f:      return "SFVec4f";
  case X3DFieldType::MFBool:       return "MFBool";
  case X3DFieldType::MFColor:      return "MFColor";
  case X3DFieldType::MFColorRGBA:  return "MFColorRGBA";
  case X3DFieldType::MFDouble:     return "MFDouble";
  case X3DFieldType::MFFloat:      return "MFFloat";
  case X3DFieldType::MFImage:      return "MFImage";
  case X3DFieldType::MFInt32:      return "MFInt32";
  case X3DFieldType::MFMatrix3d:   return "MFMatrix3d";
  case X3DFieldType::MFMatrix3f:   return "MFMatrix3f";
  case X3DFieldType::MFMatrix4d:   return "MFMatrix4d";
  case X3DFieldType::MFMatrix4f:   return "MFMatrix4f";
  case X3DFieldType::MFNode:       return "MFNode";
  case X3DFieldType::MFRotation:   return "MFRotation";
  case X3DFieldType::MFString:     return "MFString";
  case X3DFieldType::MFTime:       return "MFTime";
  case X3DFieldType::MFVec2d:      return "MFVec2d";
  case X3DFieldType::MFVec2f:      return "MFVec2f";
  case X3DFieldType::MFVec3d:      return "MFVec3d";
  case X3DFieldType::MFVec3f:      return "MFVec3f";
  case X3DFieldType::MFVec4d:      return "MFVec4d";
  case X3DFieldType::MFVec4f:      return "MFVec4f";
  case X3DFieldType::SFEnum:       return "SFString";  // X3D has no enum field type
  case X3DFieldType::MFEnum:       return "MFString";  // ditto
  }
  return "SFString"; // unreachable; silence -Wreturn-type
}

/// Returns the X3D accessType string for an AccessType enumerator.
inline const char *accessTypeName(AccessType a) {
  switch (a) {
  case AccessType::InitializeOnly: return "initializeOnly";
  case AccessType::InputOnly:      return "inputOnly";
  case AccessType::OutputOnly:     return "outputOnly";
  case AccessType::InputOutput:    return "inputOutput";
  }
  return "inputOutput"; // unreachable; silence -Wreturn-type
}

} // namespace x3d::codec

#endif // X3D_PROTO_NAME_MAPS_HPP
