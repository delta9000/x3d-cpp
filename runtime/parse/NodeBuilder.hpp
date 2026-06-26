// NodeBuilder.hpp
// Shared node-agnostic build steps for every encoding reader. This is the
// reusable middle layer the four readers funnel through, so they differ only in
// their syntax front-end (XML / ClassicVRML / VRML97 / JSON), not in how they
// build the runtime model.
//
// These are exactly the operations XmlReader performs, lifted out of XmlReader
// into free functions keyed on already-parsed name/value strings (so they are
// encoding-independent):
//   * beginNode(typeName)            — X3DNodeFactory::create; null => skip.
//   * applyField(node, x3dName, wire)— find FieldInfo by x3dName; enum =>
//                                      setEnumString, else parseValue + set;
//                                      outputOnly/inputOnly value-writes are skipped
//                                      (initializeOnly fields are writable via their
//                                      reflection set thunk).
//   * attachChild(parent, slot, child) — the containerField resolution chain
//                                      from XmlReader::attachChild.
//   * findNodeField / findField      — FieldInfo lookup helpers.
//   * collectFieldValue(tok, type)   — gather the right run of VRML tokens for
//                                      a field and join them into the wire
//                                      string FieldValueIO::parseValue expects
//                                      (the one piece XmlReader did not need,
//                                      because XML delimits values by attribute
//                                      quotes).
//
// Header-only, namespace x3d::codec. Mirrors XmlReader's tolerance: unknown
// types/fields are skipped, never throwing.
#ifndef X3D_PARSE_NODE_BUILDER_HPP
#define X3D_PARSE_NODE_BUILDER_HPP

#include "FieldValueIO.hpp"   // parseValue, tokenize (x3d::codec)
#include "VrmlTokenizer.hpp"  // VrmlTokenizer, VrmlToken
#include "X3DNodeFactory.hpp" // X3DNodeFactory::create
#include "X3DRuntime.hpp"     // runtime::Scene

#include <memory>
#include <string>
#include <string_view>

namespace x3d::codec::build {

// ---------------------------------------------------------------------------
// FieldInfo lookup helpers (lifted verbatim from XmlReader).
// ---------------------------------------------------------------------------

/// Find a field by its X3D name, or null if the node has no such field.
inline const FieldInfo *findField(const FieldTable &table,
                                  std::string_view name) {
  for (const FieldInfo &f : table) {
    if (f.x3dName == name)
      return &f;
  }
  return nullptr;
}

/// True if `name` names an SFNode/MFNode field on this node (so a reader can
/// disambiguate `fieldName Type{…}` child syntax from a value field).
inline bool isNodeField(const FieldTable &table, std::string_view name) {
  const FieldInfo *f = findField(table, name);
  return f && f->isNode();
}

// ---------------------------------------------------------------------------
// Node construction.
// ---------------------------------------------------------------------------

/// Instantiate a node by type name. Returns null for an unknown type so the
/// caller can skip it (consistent with XmlReader).
inline std::shared_ptr<X3DNode> beginNode(std::string_view typeName) {
  return X3DNodeFactory::create(std::string(typeName));
}

// ---------------------------------------------------------------------------
// Value-field application (lifted from XmlReader::applyAttribute).
// ---------------------------------------------------------------------------

/// Set one value field on a node from its wire string, via the FieldInfo
/// thunks. Enum fields route through setEnumString; everything else through
/// parseValue + set. Read-only (outputOnly/initializeOnly) fields and unknown
/// names are silently skipped. DEF is handled here too (it is a normal SFString
/// field on every node) so callers need no special case for it.
inline void applyField(X3DNode &node, std::string_view x3dName,
                       const std::string &wire) {
  const FieldInfo *f = findField(node.fields(), x3dName);
  if (!f)
    return; // unknown field: ignore
  if (f->isEnum()) {
    if (f->setEnumString)
      f->setEnumString(node, stripEnumQuotes(wire)); // AUD-D
    return;
  }
  if (!f->isWritable())
    return; // outputOnly/inputOnly: skip (initializeOnly now writable)
  std::any v = parseValue(f->type, wire);
  if (v.has_value())
    f->set(node, v);
}

// ---------------------------------------------------------------------------
// Child attachment (lifted verbatim from XmlReader::attachChild).
// ---------------------------------------------------------------------------

/// Attach a child node into the correct SF/MFNode field of `parent`.
/// Resolution order (same as XmlReader):
///   1) the node field whose reflected containerField == `slot`;
///   2) failing that, the node field whose x3dName == `slot`;
///   3) failing that, the "children" node field if present;
///   4) failing that, the first writable node field (best-effort).
/// `slot` is the explicit containerField/field-name the syntax supplied (may be
/// empty). SFNode sets; MFNode appends. When `scene` is supplied, the resolved
/// field is recorded in authored order so round-trip writers preserve it.
inline void attachChild(X3DNode &parent, std::string_view slot,
                        const std::shared_ptr<X3DNode> &child,
                        runtime::Scene *scene = nullptr) {
  const FieldTable &table = parent.fields();
  const FieldInfo *target = nullptr;
  if (!slot.empty()) {
    for (const FieldInfo &f : table) {
      if (f.isNode() && f.containerField == slot) {
        target = &f;
        break;
      }
    }
    if (!target) {
      for (const FieldInfo &f : table) {
        if (f.isNode() && f.x3dName == slot) {
          target = &f;
          break;
        }
      }
    }
  }
  if (!target) {
    for (const FieldInfo &f : table) {
      if (f.isNode() && f.containerField == "children") {
        target = &f;
        break;
      }
    }
    if (!target) {
      for (const FieldInfo &f : table) {
        if (f.isNode() && f.isWritable()) {
          target = &f;
          break;
        }
      }
    }
  }
  if (!target || !target->isWritable())
    return;

  if (target->type == X3DFieldType::SFNode) {
    target->set(parent, std::any(child));
  } else { // MFNode: append to the existing vector
    std::any cur = target->get(parent);
    auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(cur);
    vec.push_back(child);
    target->set(parent, std::any(vec));
  }

  if (scene)
    scene->recordChildField(&parent, target->x3dName);
}

/// Node-child fields of `node`, ordered by the authored child-field order
/// recorded in `scene` (when present), then any remaining node-child fields in
/// static declaration order. Round-trip writers iterate this instead of the raw
/// field table so a node shared across fields keeps its authored DEF placement
/// (e.g. HAnimHumanoid.skeleton holds the hierarchy, joints holds the USE).
/// With no scene / no recorded order this is exactly declaration order.
inline std::vector<const FieldInfo *>
orderedChildFields(const X3DNode &node, const runtime::Scene *scene) {
  const FieldTable &table = node.fields();
  std::vector<const FieldInfo *> out;
  if (scene) {
    auto it = scene->childFieldOrder.find(&node);
    if (it != scene->childFieldOrder.end()) {
      for (const std::string &fname : it->second)
        for (const FieldInfo &f : table)
          if (f.isNode() && f.isReadable() && f.x3dName == fname) {
            out.push_back(&f);
            break;
          }
    }
  }
  for (const FieldInfo &f : table) {
    if (!f.isNode() || !f.isReadable())
      continue; // skip set-only InputOnly sinks (addChildren/removeChildren)
    bool have = false;
    for (const FieldInfo *p : out)
      if (p == &f) {
        have = true;
        break;
      }
    if (!have)
      out.push_back(&f);
  }
  return out;
}

// ---------------------------------------------------------------------------
// DEF/USE.
// ---------------------------------------------------------------------------

/// Register a node under a DEF name so a later USE shares its identity. Must be
/// called BEFORE recursing into the node's children, so a USE nested inside the
/// subtree resolves to this same shared_ptr.
inline void defineDef(runtime::Scene &scene, std::string_view def,
                      const std::shared_ptr<X3DNode> &node) {
  if (!def.empty())
    scene.define(std::string(def), node);
}

/// Resolve a USE name to the shared node previously registered under that DEF,
/// or null if unknown (tolerated; the caller skips).
inline std::shared_ptr<X3DNode> resolveUse(runtime::Scene &scene,
                                           std::string_view name) {
  return scene.resolve(std::string(name));
}

// ---------------------------------------------------------------------------
// collectFieldValue: gather the right run of VRML tokens for a field.
// ---------------------------------------------------------------------------

/// How many whitespace-separated scalar components one SF value of `type` has.
/// (SFString/enum are handled specially by the caller; MF* is bracketed.)
inline int sfComponentCount(X3DFieldType type) {
  switch (type) {
  case X3DFieldType::SFBool:
  case X3DFieldType::SFInt32:
  case X3DFieldType::SFFloat:
  case X3DFieldType::SFDouble:
  case X3DFieldType::SFTime:
    return 1;
  case X3DFieldType::SFVec2f:
  case X3DFieldType::SFVec2d:
    return 2;
  case X3DFieldType::SFColor:
  case X3DFieldType::SFVec3f:
  case X3DFieldType::SFVec3d:
    return 3;
  case X3DFieldType::SFColorRGBA:
  case X3DFieldType::SFVec4f:
  case X3DFieldType::SFVec4d:
  case X3DFieldType::SFRotation:
    return 4;
  case X3DFieldType::SFMatrix3f:
  case X3DFieldType::SFMatrix3d:
    return 9;
  case X3DFieldType::SFMatrix4f:
  case X3DFieldType::SFMatrix4d:
    return 16;
  default:
    return 1;
  }
}

/// How many whitespace-separated scalar components ONE element of an MF value
/// of `type` has (e.g. MFVec3f -> 3, MFRotation -> 4, MFInt32 -> 1). Used to
/// size a bracket-less single-element MF run.
inline int mfElementComponents(X3DFieldType type) {
  switch (type) {
  case X3DFieldType::MFBool:
  case X3DFieldType::MFInt32:
  case X3DFieldType::MFFloat:
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFTime:
    return 1;
  case X3DFieldType::MFVec2f:
  case X3DFieldType::MFVec2d:
    return 2;
  case X3DFieldType::MFColor:
  case X3DFieldType::MFVec3f:
  case X3DFieldType::MFVec3d:
    return 3;
  case X3DFieldType::MFColorRGBA:
  case X3DFieldType::MFVec4f:
  case X3DFieldType::MFVec4d:
  case X3DFieldType::MFRotation:
    return 4;
  case X3DFieldType::MFMatrix3f:
  case X3DFieldType::MFMatrix3d:
    return 9;
  case X3DFieldType::MFMatrix4f:
  case X3DFieldType::MFMatrix4d:
    return 16;
  default:
    return 1;
  }
}

inline bool isMF(X3DFieldType type) {
  switch (type) {
  case X3DFieldType::MFBool:
  case X3DFieldType::MFColor:
  case X3DFieldType::MFColorRGBA:
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFFloat:
  case X3DFieldType::MFImage:
  case X3DFieldType::MFInt32:
  case X3DFieldType::MFMatrix3d:
  case X3DFieldType::MFMatrix3f:
  case X3DFieldType::MFMatrix4d:
  case X3DFieldType::MFMatrix4f:
  case X3DFieldType::MFRotation:
  case X3DFieldType::MFString:
  case X3DFieldType::MFTime:
  case X3DFieldType::MFVec2d:
  case X3DFieldType::MFVec2f:
  case X3DFieldType::MFVec3d:
  case X3DFieldType::MFVec3f:
  case X3DFieldType::MFVec4d:
  case X3DFieldType::MFVec4f:
  case X3DFieldType::MFEnum:
    return true;
  default:
    return false;
  }
}

inline bool isStringType(X3DFieldType type) {
  return type == X3DFieldType::SFString || type == X3DFieldType::MFString;
}

/// Re-quote a string token for handing back into FieldValueIO::parseMFString,
/// escaping embedded quote/backslash (the inverse of the tokenizer's unescape).
inline std::string requote(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\')
      out += '\\';
    out += c;
  }
  out += '"';
  return out;
}

/// Consume the run of tokens that make up one field's value from a tokenizer
/// and return the joined wire string that FieldValueIO::parseValue (or, for
/// MFString, parseMFString within parseValue) expects.
///
///   * SF scalar / struct: `sfComponentCount(type)` plain tokens, space-joined.
///   * SFString: one quoted string token (re-quoted for parseValue).
///   * MFString: a `[ "a" "b" ]` run (or a single bare string), each element
///     re-quoted and space-joined so quotes are preserved.
///   * other MF*: a bracketed `[ … ]` run, OR a single bare SF-element run
///     (VRML permits `field 0 0 0` as a one-element MF) — every inner token
///     space-joined.
///
/// Enum fields are NOT routed here; the caller reads one token and calls
/// setEnumString directly (per the spec).
inline std::string collectFieldValue(VrmlTokenizer &tok, X3DFieldType type) {
  // ----- SFString -----
  // parseValue(SFString) takes the raw (unquoted) string verbatim, so we hand
  // back the token's already-unescaped contents — no surrounding quotes. (Only
  // MFString needs per-element quotes preserved, handled below.)
  if (type == X3DFieldType::SFString)
    return tok.next().text;

  // ----- MF (any) -----
  if (isMF(type)) {
    std::string out;
    bool stringMF = (type == X3DFieldType::MFString);
    auto appendTok = [&](const VrmlToken &t) {
      if (!out.empty())
        out += ' ';
      if (stringMF && t.kind == VrmlToken::Kind::String)
        out += requote(t.text);
      else
        out += t.text;
    };

    if (tok.peek().isPunct('[')) {
      tok.next(); // consume '['
      while (!tok.atEnd() && !tok.peek().isPunct(']'))
        appendTok(tok.next());
      if (tok.peek().isPunct(']'))
        tok.next(); // consume ']'
      return out;
    }

    // Bare single element (VRML permits a one-element MF without brackets).
    if (stringMF) {
      appendTok(tok.next());
      return out;
    }
    int n = mfElementComponents(type);
    for (int i = 0; i < n && !tok.atEnd() && !tok.peek().isPunct('}') &&
                    !tok.peek().isPunct(']');
         ++i)
      appendTok(tok.next());
    return out;
  }

  // ----- SF scalar / struct -----
  std::string out;
  int n = sfComponentCount(type);
  for (int i = 0; i < n && !tok.atEnd() && !tok.peek().isPunct('}') &&
                  !tok.peek().isPunct(']');
       ++i) {
    if (!out.empty())
      out += ' ';
    out += tok.next().text;
  }
  return out;
}

} // namespace x3d::codec::build

#endif // X3D_PARSE_NODE_BUILDER_HPP
