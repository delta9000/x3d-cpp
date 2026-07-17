#include "NodeBuilder.hpp"

#include "FieldValueIO.hpp"
#include "VrmlTokenizer.hpp"
#include "X3DRuntime.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include <cstdlib>

namespace x3d::codec::build {

const FieldInfo *findField(const FieldTable &table, std::string_view name) {
  for (const FieldInfo &f : table) {
    if (f.x3dName == name)
      return &f;
  }
  return nullptr;
}

bool isNodeField(const FieldTable &table, std::string_view name) {
  const FieldInfo *f = findField(table, name);
  return f && f->isNode();
}

std::shared_ptr<x3d::nodes::X3DNode> beginNode(std::string_view typeName) {
  return x3d::nodes::X3DNodeFactory::create(std::string(typeName));
}

void applyField(x3d::nodes::X3DNode &node, std::string_view x3dName,
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

void attachChild(x3d::nodes::X3DNode &parent, std::string_view slot,
                 const std::shared_ptr<x3d::nodes::X3DNode> &child,
                 runtime::Scene *scene) {
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
    auto vec =
        std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(cur);
    vec.push_back(child);
    target->set(parent, std::any(vec));
  }

  if (scene)
    scene->recordChildField(&parent, target->x3dName);
}

void defineDef(runtime::Scene &scene, std::string_view def,
               const std::shared_ptr<x3d::nodes::X3DNode> &node) {
  if (!def.empty())
    scene.define(std::string(def), node);
}

std::shared_ptr<x3d::nodes::X3DNode> resolveUse(runtime::Scene &scene,
                                                std::string_view name) {
  return scene.resolve(std::string(name));
}

int sfComponentCount(X3DFieldType type) {
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

int mfElementComponents(X3DFieldType type) {
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

bool isMF(X3DFieldType type) {
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

bool isStringType(X3DFieldType type) {
  return type == X3DFieldType::SFString || type == X3DFieldType::MFString;
}

std::string requote(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\')
      out += '\\';
    out += c;
  }
  out += '"';
  return out;
}

std::string collectFieldValue(VrmlTokenizer &tok, X3DFieldType type) {
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

  // ----- SFImage -----
  // Wire form: `width height numComponents` then width*height pixel words
  // (ENC-VRML-SFIMAGE: a fixed component count of 1 consumed only `width`,
  // destroying every inline PixelTexture on the ClassicVRML hop).
  if (type == X3DFieldType::SFImage) {
    std::string out;
    long w = 0, h = 0;
    for (int i = 0; i < 3 && !tok.atEnd() && !tok.peek().isPunct('}') &&
                    !tok.peek().isPunct(']');
         ++i) {
      const std::string t = tok.next().text;
      if (i == 0)
        w = std::strtol(t.c_str(), nullptr, 0);
      else if (i == 1)
        h = std::strtol(t.c_str(), nullptr, 0);
      if (!out.empty())
        out += ' ';
      out += t;
    }
    // Malformed headers (negative / absurd sizes) collect nothing further;
    // the atEnd/punct guards stop early on truncated pixel runs. The 2^20
    // per-axis cap keeps w*h from overflowing on hostile headers.
    constexpr long kMaxAxis = 1L << 20;
    const long pixels =
        (w > 0 && h > 0 && w <= kMaxAxis && h <= kMaxAxis) ? w * h : 0;
    for (long p = 0; p < pixels && !tok.atEnd() && !tok.peek().isPunct('}') &&
                     !tok.peek().isPunct(']');
         ++p) {
      out += ' ';
      out += tok.next().text;
    }
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
