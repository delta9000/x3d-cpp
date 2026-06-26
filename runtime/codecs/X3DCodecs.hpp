// X3DCodecs.hpp
// Umbrella header for the hand-written, node-agnostic X3D codecs.
//
// Pulls in the tiny XML library, the reflection-driven field value I/O, and the
// codecs:
//   * XmlWriter          — runtime model -> X3D-XML
//   * XmlReader          — X3D-XML -> runtime model (DEF/USE identity, ROUTEs, head)
//   * JsonWriter         — runtime model -> X3D-JSON (writer only)
//   * VrmlWriter         — runtime model -> ClassicVRML (writer only)
//   * CanonicalXmlWriter — runtime model -> X3D Canonical Form (X3DC14N)
//
// All codecs are driven purely by the generated reflection FieldTable and the
// X3DNodeFactory; none contain per-node code. They have no external
// dependencies (the bundled XmlLite is the only XML facility used).
#ifndef X3D_CODECS_HPP
#define X3D_CODECS_HPP

#include "CanonicalXmlWriter.hpp"
#include "FieldValueIO.hpp"
#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"
#include "XmlLite.hpp"
#include "XmlReader.hpp"
#include "XmlWriter.hpp"

#endif // X3D_CODECS_HPP
