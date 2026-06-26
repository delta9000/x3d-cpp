// VrmlWriter.hpp
// Node-agnostic ClassicVRML (X3D ClassicVRML / .x3dv) serializer (writer only).
//
// Emits the ClassicVRML encoding:
//   #X3D V4.0 utf8
//   PROFILE Interchange
//   <statements...>
//   <root nodes...>
// where each node is:
//   [DEF Name] TypeName { field value  field value  childField Node { ... } }
// USE references emit `USE Name`. SF/MF value fields emit `name value`; node
// fields emit `name <node>` (SFNode) or `name [ <node> ... ]` (MFNode). MF/SF
// structured values reuse the X3D wire formatting (space-separated). MFString
// keeps its quoted-list form. Enum fields emit their token via the FieldInfo
// string thunk. ROUTEs emit `ROUTE A.f TO B.g`.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_VRML_WRITER_HPP
#define X3D_VRML_WRITER_HPP

#include "DynamicField.hpp"  // S1: author-field decls to re-emit (Task B)
#include "FieldValueIO.hpp"
#include "ProtoNameMaps.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"

#include <algorithm>
#include <any>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::codec {

/// Serializes the runtime document model to ClassicVRML.
class VrmlWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc) {
    seen_.clear();
    defaults_.clear();
    scene_ = &doc.scene;
    std::ostringstream os;
    os << "#X3D V" << headerVersion(doc.version) << " utf8\n";
    os << "PROFILE " << doc.profileName() << "\n";
    for (const auto &c : doc.head.components)
      os << "COMPONENT " << c.name << ":" << c.level << "\n";
    for (const auto &u : doc.head.units)
      os << "UNIT " << u.category << " " << u.name << " "
         << fmtDouble(u.conversionFactor) << "\n";
    for (const auto &m : doc.head.meta)
      os << "META \"" << m.name << "\" \"" << m.content << "\"\n";
    os << "\n";

    // Emit PROTO/ExternProto declarations before root nodes.
    for (const auto &d : doc.scene.protoDeclarations)
      if (d) writeVrmlProtoDeclare(os, *d);
    for (const auto &e : doc.scene.externProtoDeclarations)
      if (e) writeVrmlExternProtoDeclare(os, *e);

    for (const auto &n : doc.scene.rootNodes) {
      if (n)
        writeNode(os, n, 0);
    }
    // AUD-B: re-emit scene-root ProtoInstances that did NOT expand (no graph
    // node, so not covered by the expandedSources re-emit in writeNode).
    for (const auto &inst : doc.scene.protoInstances)
      if (!inst.expanded && inst.parent.expired())
        writeVrmlProtoInstance(os, inst, 0);
    for (const auto &r : doc.scene.routes) {
      os << "ROUTE " << r.fromNode << "." << r.fromField << " TO " << r.toNode
         << "." << r.toField << "\n";
    }
    return os.str();
  }

private:
  /// X3D has no version below 3.0; floor a sub-3.0/legacy token to "3.0" so the
  /// emitted header is always valid (VP-2 §8). >= 3.0 (incl. future) passes through.
  static std::string headerVersion(const std::string &v) {
    int major = 0;
    try { major = std::stoi(v.substr(0, v.find('.'))); } catch (...) { return "3.0"; }
    return (major < 3) ? std::string("3.0") : v;
  }

  // ClassicVRML (ISO 19776-2) writes booleans as TRUE/FALSE, not the XML/JSON
  // lowercase tokens. formatValue() yields lowercase; uppercase each token here
  // for SFBool and the space-separated MFBool list, leaving everything else as
  // emitted by the shared formatter.
  static std::string vrmlBoolCase(X3DFieldType type, const std::string &in) {
    if (type != X3DFieldType::SFBool && type != X3DFieldType::MFBool)
      return in;
    std::string out;
    out.reserve(in.size());
    std::size_t i = 0;
    while (i < in.size()) {
      if (in.compare(i, 4, "true") == 0 &&
          (i + 4 == in.size() || in[i + 4] == ' ')) {
        out += "TRUE";
        i += 4;
      } else if (in.compare(i, 5, "false") == 0 &&
                 (i + 5 == in.size() || in[i + 5] == ' ')) {
        out += "FALSE";
        i += 5;
      } else {
        out += in[i++];
      }
    }
    return out;
  }

  // True for every multiple-value (MF*) field type. ClassicVRML / VRML97 grammar
  // (ISO/IEC 19776-2, VRML97 Annex A) requires an MF value to be bracketed
  //   mfValue ::= '[' ']' | '[' sfValue+ ']' | sfValue
  // A bare run of >1 sfValue is NOT a valid mfValue (it truncates to the first
  // element on reparse), and an empty MF must be written '[ ]'. We always
  // bracket MF values (single/empty included) — always grammatical, never lossy.
  static bool isMultiField(X3DFieldType t) {
    switch (t) {
    case X3DFieldType::MFBool:   case X3DFieldType::MFInt32:
    case X3DFieldType::MFFloat:  case X3DFieldType::MFDouble:
    case X3DFieldType::MFTime:   case X3DFieldType::MFString:
    case X3DFieldType::MFVec2f:  case X3DFieldType::MFVec2d:
    case X3DFieldType::MFVec3f:  case X3DFieldType::MFVec3d:
    case X3DFieldType::MFVec4f:  case X3DFieldType::MFVec4d:
    case X3DFieldType::MFColor:  case X3DFieldType::MFColorRGBA:
    case X3DFieldType::MFRotation: case X3DFieldType::MFMatrix3f:
    case X3DFieldType::MFMatrix3d: case X3DFieldType::MFMatrix4f:
    case X3DFieldType::MFMatrix4d: case X3DFieldType::MFImage:
    case X3DFieldType::MFNode:   case X3DFieldType::MFEnum:
      return true;
    default:
      return false;
    }
  }

  // ClassicVRML SFString literals are double-quoted; embedded backslash and
  // double-quote must be escaped (\\ and \"). MFString self-escapes via
  // fmtMFString; this is only for the bare SFString branches.
  static std::string vrmlEscapeString(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
      if (c == '\\' || c == '"')
        out += '\\';
      out += c;
    }
    return out;
  }

  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup. Null for body writers.
  const runtime::Scene *scene_ = nullptr;
  // PRF-1: while re-emitting a ProtoBody, the body's IsConnection list so that
  // writeNode can emit `nodeField IS protoField` lines inside EVERY emitted body
  // node at any depth (mirrors the XML writer's bodyIsc_). Null outside a body.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  // PRF-3: while re-emitting a ProtoBody, the body's nestedInstances list so
  // writeNode can inject each Case-A instance (parent == the node being
  // written) INSIDE that node's body, in its parentField slot — mirroring
  // XmlWriter pushing them onto the parent element's children — rather than
  // appending them after the parent node. Null outside a body.
  const std::vector<runtime::ProtoInstance> *bodyNested_ = nullptr;

  static void pad(std::ostringstream &os, int depth) {
    for (int i = 0; i < depth; ++i)
      os << "  ";
  }

  X3DNode *defaultFor(const std::string &typeName) {
    auto it = defaults_.find(typeName);
    if (it != defaults_.end())
      return it->second.get();
    auto fresh = X3DNodeFactory::create(typeName);
    X3DNode *raw = fresh.get();
    defaults_[typeName] = std::move(fresh);
    return raw;
  }

  // ---- PROTO emit helpers ----

  /// Look up a ProtoInstance's interface field by name (for type information).
  static const runtime::ProtoField *
  interfaceField(const runtime::ProtoInstance &src, const std::string &name) {
    auto scan = [&](const std::vector<runtime::ProtoField> &iface)
        -> const runtime::ProtoField * {
      for (const runtime::ProtoField &f : iface)
        if (f.name == name)
          return &f;
      return nullptr;
    };
    if (src.declaration)
      return scan(src.declaration->interface);
    if (src.externDeclaration)
      return scan(src.externDeclaration->interface);
    return nullptr;
  }

  /// Emit a proto instance: `[DEF d ]Name {\n  field value\n}\n`
  void writeVrmlProtoInstance(std::ostringstream &os,
                              const runtime::ProtoInstance &src, int depth) {
    pad(os, depth);
    if (!src.DEF.empty())
      os << "DEF " << src.DEF << " ";
    os << src.name << " {\n";
    for (const runtime::ProtoFieldValue &fv : src.fieldValues) {
      if (!fv.nodeValue.empty()) {
        // Node-valued field.
        pad(os, depth + 1);
        os << fv.name << " ";
        if (fv.nodeValue.size() == 1) {
          writeNode(os, fv.nodeValue[0], depth + 1);
          os << "\n";
        } else {
          os << "[\n";
          for (const auto &child : fv.nodeValue) {
            if (!child) continue;
            pad(os, depth + 2);
            writeNode(os, child, depth + 2);
            os << "\n";
          }
          pad(os, depth + 1);
          os << "]\n";
        }
      } else if (fv.value.has_value()) {
        const runtime::ProtoField *pf = interfaceField(src, fv.name);
        if (pf) {
          pad(os, depth + 1);
          std::string valStr = formatValue(pf->type, fv.value);
          if (pf->type == X3DFieldType::SFString)
            os << fv.name << " \"" << vrmlEscapeString(valStr) << "\"\n";
          else if (isMultiField(pf->type))
            os << fv.name << " [ " << vrmlBoolCase(pf->type, valStr) << " ]\n";
          else
            os << fv.name << " " << vrmlBoolCase(pf->type, valStr) << "\n";
        }
      }
    }
    pad(os, depth);
    os << "}";
    if (depth == 0)
      os << "\n";
  }

  /// PRF-3: emit `slot [ <ProtoInstance> ... ]` blocks for every Case-A nested
  /// instance whose parent is `node`, grouped by parentField slot, so the
  /// ClassicVrmlReader recovers parent == node and the same slot. No-op when no
  /// body nested list is threaded in. `depth` is the slot line's indent level.
  void writeVrmlNestedFor(std::ostringstream &os,
                          const std::shared_ptr<X3DNode> &node, int depth) {
    if (!bodyNested_)
      return;
    std::vector<std::string> slots;
    for (const auto &ni : *bodyNested_) {
      if (ni.parent.lock().get() != node.get())
        continue;
      const std::string slot =
          ni.parentField.empty() ? ni.containerField : ni.parentField;
      if (std::find(slots.begin(), slots.end(), slot) == slots.end())
        slots.push_back(slot);
    }
    for (const std::string &slot : slots) {
      pad(os, depth);
      os << slot << " [\n";
      for (const auto &ni : *bodyNested_) {
        if (ni.parent.lock().get() != node.get())
          continue;
        const std::string s =
            ni.parentField.empty() ? ni.containerField : ni.parentField;
        if (s != slot)
          continue;
        writeVrmlProtoInstance(os, ni, depth + 1);
        os << "\n";
      }
      pad(os, depth);
      os << "]\n";
    }
  }

  /// Emit a PROTO declaration: `PROTO Name [ interface ] {\n body \n}\n`
  void writeVrmlProtoDeclare(std::ostringstream &os,
                             const runtime::ProtoDeclaration &d) {
    os << "PROTO " << d.name << " [\n";
    for (const auto &f : d.interface) {
      os << "  " << accessTypeName(f.access) << " " << fieldTypeName(f.type)
         << " " << f.name;
      if (!f.nodeDefault.empty()) {
        os << " [\n";
        VrmlWriter bodyWriter;
        for (const auto &n : f.nodeDefault) {
          if (!n) continue;
          std::ostringstream nos;
          bodyWriter.writeNode(nos, n, 2);
          os << nos.str() << "\n";
        }
        os << "  ]";
      } else if (f.value.has_value() &&
                 (f.access == AccessType::InitializeOnly ||
                  f.access == AccessType::InputOutput)) {
        std::string valStr = formatValue(f.type, f.value);
        if (f.type == X3DFieldType::SFString)
          os << " \"" << vrmlEscapeString(valStr) << "\"";
        else if (isMultiField(f.type))
          os << " [ " << vrmlBoolCase(f.type, valStr) << " ]";
        else
          os << " " << vrmlBoolCase(f.type, valStr);
      }
      os << "\n";
    }
    os << "] {\n";

    // Body — fresh writer so DEF/USE is isolated; scene_=null means no proto redirect.
    VrmlWriter bodyWriter;
    // PRF-1: hand the body's IS list to the body writer so writeNode emits
    // `field IS protoField` lines inside the node braces at every depth.
    bodyWriter.bodyIsc_ = &d.body.isConnections;
    // PRF-3: thread the nested-instance list so writeNode injects each Case-A
    // instance INSIDE its parent body node's body (in its slot), not after it.
    bodyWriter.bodyNested_ = &d.body.nestedInstances;
    for (const auto &n : d.body.nodes) {
      if (!n) continue;
      std::ostringstream nos;
      bodyWriter.writeNode(nos, n, 1);
      os << "  " << nos.str() << "\n";
    }
    // Body ROUTEs.
    for (const auto &r : d.body.routes) {
      os << "  ROUTE " << r.fromNode << "." << r.fromField << " TO "
         << r.toNode << "." << r.toField << "\n";
    }
    // Case B: direct ProtoBody children.
    for (const auto &ni : d.body.nestedInstances) {
      if (!ni.parent.lock()) {
        writeVrmlProtoInstance(os, ni, 1);
        os << "\n";
      }
    }
    os << "}\n\n";
  }

  /// Emit an ExternProto declaration.
  void writeVrmlExternProtoDeclare(std::ostringstream &os,
                                   const runtime::ExternProtoDeclaration &d) {
    os << "EXTERNPROTO " << d.name << " [\n";
    for (const auto &f : d.interface) {
      os << "  " << accessTypeName(f.access) << " " << fieldTypeName(f.type)
         << " " << f.name << "\n";
    }
    os << "] [";
    for (std::size_t i = 0; i < d.url.size(); ++i) {
      if (i) os << ", ";
      os << "\"" << d.url[i] << "\"";
    }
    os << "]\n\n";
  }

  void writeNode(std::ostringstream &os, const std::shared_ptr<X3DNode> &node,
                 int depth) {
    // Inline round-trip: if this node is the synthetic Group of an expanded
    // <Inline>, re-emit the original Inline node and do NOT descend into the
    // Group's child content (which the reader will re-fetch on load).
    if (scene_) {
      auto il = scene_->expandedInlines.find(node.get());
      if (il != scene_->expandedInlines.end()) {
        writeNode(os, il->second, depth);
        return;
      }
    }

    // PROTO round-trip: if this node is the expanded primary of a captured
    // <ProtoInstance>, re-emit the original instance and do NOT descend.
    if (scene_) {
      auto it = scene_->expandedSources.find(node.get());
      if (it != scene_->expandedSources.end()) {
        writeVrmlProtoInstance(os, it->second, depth);
        return;
      }
    }

    const std::string def = node->getDEF();
    if (seen_.count(node.get())) {
      os << "USE " << def;
      return;
    }
    if (!def.empty()) {
      os << "DEF " << def << " ";
      seen_.insert(node.get());
    }
    os << node->nodeTypeName() << " {\n";

    X3DNode *defaults = defaultFor(node->nodeTypeName());

    for (const FieldInfo &f : node->fields()) {
      if (!f.isReadable())
        continue;
      if (f.x3dName == "DEF" || f.x3dName == "USE" || f.x3dName == "IS")
        continue;
      if (f.isNode()) {
        writeNodeField(os, node, f, depth + 1);
        continue;
      }
      // Value field.
      std::string text;
      std::string defText;
      if (f.isEnum()) {
        if (!f.getEnumString)
          continue;
        text = f.getEnumString(*node);
        if (defaults && f.getEnumString)
          defText = f.getEnumString(*defaults);
      } else {
        text = formatValue(f.type, f.get(*node));
        if (defaults) {
          for (const FieldInfo &df : defaults->fields()) {
            if (df.x3dName == f.x3dName && df.isReadable()) {
              defText = formatValue(df.type, df.get(*defaults));
              break;
            }
          }
        }
      }
      if (defaults && text == defText)
        continue;
      // ClassicVRML wraps SFString in quotes; MFString already self-quotes.
      pad(os, depth + 1);
      if (f.type == X3DFieldType::SFString)
        os << f.x3dName << " \"" << vrmlEscapeString(text) << "\"\n";
      else if (isMultiField(f.type))
        os << f.x3dName << " [ " << vrmlBoolCase(f.type, text) << " ]\n";
      else
        os << f.x3dName << " " << vrmlBoolCase(f.type, text) << "\n";
    }

    // Task B: re-emit author (Script) field declarations captured by the reader
    // into the S1 store as `accessType FieldType name [default]` interface lines
    // inside the node body, so a re-parse recovers them (round-trip).
    writeAuthorFields(os, *node, depth + 1);

    // PRF-1: re-emit IS connections bound to THIS node (at any depth) as
    // `nodeField IS protoField` lines inside the node body, matching what the
    // ClassicVrmlReader parses (`name IS protoField` in parseNodeBody).
    if (bodyIsc_) {
      for (const auto &c : *bodyIsc_) {
        if (c.node.get() != node.get())
          continue;
        pad(os, depth + 1);
        os << c.nodeField << " IS " << c.protoField << "\n";
      }
    }

    // PRF-3: inject Case-A nested ProtoInstances whose parent is THIS node,
    // grouped by parentField, as `slot [ <instance> ... ]` blocks inside the
    // node body — mirroring XmlWriter pushing them onto the parent element's
    // children so the ClassicVrmlReader recovers parent == this node (the
    // reader's applyNodeField threads parentShared+slot into parseNode).
    writeVrmlNestedFor(os, node, depth + 1);

    // Scene-level nested ProtoInstances: any un-expanded ProtoInstance in
    // scene.protoInstances whose parent is THIS node. These are NOT in the node
    // graph (expansion failed) and NOT scene-root (parent is live), so without
    // this injection they are lost. Emit per-slot, respecting SFNode vs MFNode:
    // SFNode slots emit a bare `slot TypeName { ... }` (no brackets); MFNode
    // slots emit `slot [ TypeName { ... } ... ]` — the ClassicVrmlReader's
    // applyNodeField skips a `[` prefix for SFNode fields.
    if (scene_) {
      // Collect unique slots for instances parented to this node.
      std::vector<std::string> slots;
      for (const auto &inst : scene_->protoInstances) {
        if (inst.expanded)
          continue;
        auto p = inst.parent.lock();
        if (!p || p.get() != node.get())
          continue;
        const std::string slot =
            inst.parentField.empty() ? inst.containerField : inst.parentField;
        if (std::find(slots.begin(), slots.end(), slot) == slots.end())
          slots.push_back(slot);
      }
      for (const std::string &slot : slots) {
        // Determine SFNode vs MFNode by looking up the slot in the parent's
        // field table. Default to MFNode (bracket form) if unknown.
        bool isSFNode = false;
        for (const FieldInfo &f : node->fields()) {
          if ((f.containerField == slot || f.x3dName == slot) && f.isNode()) {
            isSFNode = (f.type == X3DFieldType::SFNode);
            break;
          }
        }
        if (isSFNode) {
          // Emit the first (and typically only) instance bare: `slot Inst {...}`
          // writeVrmlProtoInstance emits its own leading pad, so capture to a
          // temp stream and strip the leading whitespace before appending.
          for (const auto &inst : scene_->protoInstances) {
            if (inst.expanded)
              continue;
            auto p = inst.parent.lock();
            if (!p || p.get() != node.get())
              continue;
            const std::string s =
                inst.parentField.empty() ? inst.containerField : inst.parentField;
            if (s != slot)
              continue;
            std::ostringstream tmp;
            writeVrmlProtoInstance(tmp, inst, depth + 1);
            // Strip the leading spaces written by pad(os, depth+1).
            std::string body = tmp.str();
            std::size_t firstNonSpace = body.find_first_not_of(' ');
            if (firstNonSpace == std::string::npos) firstNonSpace = 0;
            pad(os, depth + 1);
            os << slot << " " << body.substr(firstNonSpace) << "\n";
            break; // SFNode: only one child
          }
        } else {
          pad(os, depth + 1);
          os << slot << " [\n";
          for (const auto &inst : scene_->protoInstances) {
            if (inst.expanded)
              continue;
            auto p = inst.parent.lock();
            if (!p || p.get() != node.get())
              continue;
            const std::string s =
                inst.parentField.empty() ? inst.containerField : inst.parentField;
            if (s != slot)
              continue;
            writeVrmlProtoInstance(os, inst, depth + 2);
            os << "\n";
          }
          pad(os, depth + 1);
          os << "]\n";
        }
      }
    }

    pad(os, depth);
    os << "}";
    if (depth == 0)
      os << "\n";
  }

  // Task B: emit author field interface declarations from the S1 store as
  //   accessType FieldType name [default]
  // (the inverse of ClassicVrmlReader::consumeInterfaceDeclaration). A readable
  // (initializeOnly/inputOutput) field whose boxed value is present emits its
  // default; event fields (inputOnly/outputOnly) and value-less fields emit no
  // default. SFString defaults are quoted, booleans cased TRUE/FALSE — matching
  // the rest of the writer.
  void writeAuthorFields(std::ostringstream &os, const X3DNode &node,
                         int depth) {
    if (!runtime::dynamicFieldStore().hasAuthorFields(node))
      return;
    for (const FieldInfo &f : runtime::dynamicFieldStore().authorFields(node)) {
      pad(os, depth);
      os << accessTypeName(f.access) << " " << fieldTypeName(f.type) << " "
         << f.x3dName;
      const bool valued = f.access == AccessType::InitializeOnly ||
                          f.access == AccessType::InputOutput;
      if (valued && f.get) {
        std::any v = f.get(node);
        if (v.has_value()) {
          std::string valStr = formatValue(f.type, v);
          if (f.type == X3DFieldType::SFString)
            os << " \"" << vrmlEscapeString(valStr) << "\"";
          else if (isMultiField(f.type))
            os << " [ " << vrmlBoolCase(f.type, valStr) << " ]";
          else
            os << " " << vrmlBoolCase(f.type, valStr);
        }
      }
      os << "\n";
    }
  }

  void writeNodeField(std::ostringstream &os,
                      const std::shared_ptr<X3DNode> &node, const FieldInfo &f,
                      int depth) {
    std::any v = f.get(*node);
    if (f.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
      if (!child)
        return;
      pad(os, depth);
      os << f.containerField << " ";
      writeNode(os, child, depth);
      os << "\n";
    } else {
      auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
      bool any = false;
      for (const auto &c : vec)
        if (c) {
          any = true;
          break;
        }
      if (!any)
        return;
      pad(os, depth);
      os << f.containerField << " [\n";
      for (const auto &c : vec) {
        if (!c)
          continue;
        pad(os, depth + 1);
        writeNode(os, c, depth + 1);
        os << "\n";
      }
      pad(os, depth);
      os << "]\n";
    }
  }
};

} // namespace x3d::codec

#endif // X3D_VRML_WRITER_HPP
