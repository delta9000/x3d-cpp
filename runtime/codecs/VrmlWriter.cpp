#include "VrmlWriter.hpp"
#include "X3DProtoFieldOrder.hpp"

#include "DynamicField.hpp"
#include "FieldValueIO.hpp"
#include "ProtoNameMaps.hpp"
#include "VersionHeader.hpp"
#include "X3DRuntime.hpp"
#include "parse/NodeBuilder.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <algorithm>
#include <any>
#include <sstream>
#include <utility>

namespace x3d::codec {
using x3d::nodes::X3DNodeFactory;

namespace {

std::string defaultVrmlValue(X3DFieldType type) {
  if (type == X3DFieldType::SFNode)
    return "NULL";
  if (type == X3DFieldType::MFNode || type == X3DFieldType::MFEnum)
    return "";
  if (type == X3DFieldType::SFEnum)
    return "\"\"";
  return formatValue(type, parseValue(type, ""));
}

} // namespace

void VrmlWriter::writeInterfaceValue(std::ostringstream &os,
                                     X3DFieldType type,
                                     const std::any &value) {
  const std::string val = value.has_value()
                              ? formatValue(type, value)
                              : defaultVrmlValue(type);
  if (type == X3DFieldType::SFString)
    os << " \"" << vrmlEscapeString(val) << "\"";
  else if (type == X3DFieldType::SFEnum)
    os << " " << (value.has_value() ? val : "\"\"");
  else if (type == X3DFieldType::MFEnum || isMultiField(type))
    os << " [ " << vrmlBoolCase(type, val) << " ]";
  else
    os << " " << vrmlBoolCase(type, val);
}

std::string VrmlWriter::writeDocument(const runtime::X3DDocument &doc) {
  seen_.clear();
  defaults_.clear();
  scene_ = &doc.scene;
  authorFields_ = doc.scene.authorFields.get();
  std::ostringstream os;
  os << "#X3D V" << headerVersion(doc.version) << " utf8\n";
  os << "PROFILE " << doc.profileToken() << "\n";
  for (const auto &c : doc.head.components)
    os << "COMPONENT " << c.name << ":" << c.level << "\n";
  for (const auto &u : doc.head.units)
    os << "UNIT " << u.category << " " << u.name << " "
       << fmtDouble(u.conversionFactor) << "\n";
  for (const auto &m : doc.head.meta)
    os << "META \"" << vrmlEscapeString(m.name) << "\" \""
       << vrmlEscapeString(m.content) << "\"\n";
  os << "\n";

  // Emit PROTO/ExternProto declarations before root nodes.
  for (const auto &e : doc.scene.externProtoDeclarations)
    if (e)
      writeVrmlExternProtoDeclare(os, *e);
  for (const auto &d : doc.scene.protoDeclarations)
    if (d)
      writeVrmlProtoDeclare(os, *d);

  for (const auto &n : doc.scene.rootNodes) {
    if (n) {
      writeNode(os, n, 0);
      os << "\n";
    }
  }
  // Emit programmatic root instances without an authored graph slot.
  for (const auto &inst : doc.scene.protoInstances)
    if (!inst.expanded && !inst.hasPlacementTemplate() && inst.parent.expired())
      writeVrmlProtoInstance(os, inst, 0);
  for (const auto &r : doc.scene.routes) {
    os << "ROUTE " << r.fromNode << "." << r.fromField << " TO " << r.toNode
       << "." << r.toField << "\n";
  }
  return os.str();
}

std::string VrmlWriter::vrmlBoolCase(X3DFieldType type, const std::string &in) {
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

bool VrmlWriter::isMultiField(X3DFieldType t) {
  switch (t) {
  case X3DFieldType::MFBool:
  case X3DFieldType::MFInt32:
  case X3DFieldType::MFFloat:
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFTime:
  case X3DFieldType::MFString:
  case X3DFieldType::MFVec2f:
  case X3DFieldType::MFVec2d:
  case X3DFieldType::MFVec3f:
  case X3DFieldType::MFVec3d:
  case X3DFieldType::MFVec4f:
  case X3DFieldType::MFVec4d:
  case X3DFieldType::MFColor:
  case X3DFieldType::MFColorRGBA:
  case X3DFieldType::MFRotation:
  case X3DFieldType::MFMatrix3f:
  case X3DFieldType::MFMatrix3d:
  case X3DFieldType::MFMatrix4f:
  case X3DFieldType::MFMatrix4d:
  case X3DFieldType::MFImage:
  case X3DFieldType::MFNode:
  case X3DFieldType::MFEnum:
    return true;
  default:
    return false;
  }
}

std::string VrmlWriter::vrmlEscapeString(const std::string &in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    if (c == '\\' || c == '"')
      out += '\\';
    out += c;
  }
  return out;
}

void VrmlWriter::pad(std::ostringstream &os, int depth) {
  for (int i = 0; i < depth; ++i)
    os << "  ";
}

X3DNode *VrmlWriter::defaultFor(const std::string &typeName) {
  auto it = defaults_.find(typeName);
  if (it != defaults_.end())
    return it->second.get();
  auto fresh = X3DNodeFactory::create(typeName);
  X3DNode *raw = fresh.get();
  defaults_[typeName] = std::move(fresh);
  return raw;
}

const runtime::ProtoField *
VrmlWriter::interfaceField(const runtime::ProtoInstance &src,
                           const std::string &name) {
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

void VrmlWriter::writeVrmlProtoInstance(std::ostringstream &os,
                                        const runtime::ProtoInstance &src,
                                        int depth) {
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
          if (!child)
            continue;
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

void VrmlWriter::writeVrmlNestedFor(std::ostringstream &os,
                                    const std::shared_ptr<X3DNode> &node,
                                    int depth) {
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

void VrmlWriter::writeVrmlProtoDeclare(std::ostringstream &os,
                                       const runtime::ProtoDeclaration &d) {
  os << "PROTO " << d.name << " [\n";
  // One local DEF scope spans interface defaults and body nodes.
  VrmlWriter bodyWriter;
  bodyWriter.authorFields_ = d.authorFields.get();
  bodyWriter.bodyIsc_ = &d.body.isConnections;
  bodyWriter.bodyOrder_ = &d.body;
  bodyWriter.bodyNested_ = &d.body.nestedInstances;
  for (const auto &f : d.interface) {
    os << "  " << accessTypeName(f.access) << " " << fieldTypeName(f.type)
       << " " << f.name;
    if (!f.nodeDefault.empty()) {
      if (f.type == X3DFieldType::MFNode) {
        os << " [\n";
      } else {
        os << " ";
      }
      for (const auto &n : f.nodeDefault) {
        if (!n)
          continue;
        std::ostringstream nos;
        bodyWriter.writeNode(nos, n, f.type == X3DFieldType::MFNode ? 2 : 1);
        os << nos.str() << "\n";
      }
      if (f.type == X3DFieldType::MFNode)
        os << "  ]";
    } else if (f.access == AccessType::InitializeOnly ||
               f.access == AccessType::InputOutput) {
      writeInterfaceValue(os, f.type, f.value);
    }
    os << "\n";
  }
  os << "] {\n";

  // Body uses the interface's local scope; scene_=null prevents redirect.
  // PRF-1: hand the body's IS list to the body writer so writeNode emits
  // `field IS protoField` lines inside the node braces at every depth.
  // PRF-3: thread the nested-instance list so writeNode injects each Case-A
  // instance INSIDE its parent body node's body (in its slot), not after it.
  for (const auto &entry : d.body.orderedStatements()) {
    switch (entry.kind) {
    case runtime::ProtoBodyStatement::Kind::Node: {
      std::ostringstream nos;
      bodyWriter.writeNode(nos, entry.node, 1);
      os << "  " << nos.str() << "\n";
      break;
    }
    case runtime::ProtoBodyStatement::Kind::Instance:
      bodyWriter.writeVrmlProtoInstance(
          os, d.body.nestedInstances[entry.instanceIndex], 1);
      os << "\n";
      break;
    case runtime::ProtoBodyStatement::Kind::Proto:
      if (entry.proto) bodyWriter.writeVrmlProtoDeclare(os, *entry.proto);
      break;
    case runtime::ProtoBodyStatement::Kind::ExternProto:
      if (entry.externProto) bodyWriter.writeVrmlExternProtoDeclare(os, *entry.externProto);
      break;
    }
  }
  for (const auto &r : d.body.routes) {
    os << "  ROUTE " << r.fromNode << "." << r.fromField << " TO " << r.toNode
       << "." << r.toField << "\n";
  }
  os << "}\n\n";
}

void VrmlWriter::writeVrmlExternProtoDeclare(
    std::ostringstream &os, const runtime::ExternProtoDeclaration &d) {
  os << "EXTERNPROTO " << d.name << " [\n";
  for (const auto &f : d.interface) {
    os << "  " << accessTypeName(f.access) << " " << fieldTypeName(f.type)
       << " " << f.name << "\n";
  }
  os << "] [";
  for (std::size_t i = 0; i < d.url.size(); ++i) {
    if (i)
      os << ", ";
    os << "\"" << d.url[i] << "\"";
  }
  os << "]\n\n";
}

void VrmlWriter::writeNode(std::ostringstream &os,
                           const std::shared_ptr<X3DNode> &node, int depth) {
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

  // Re-emit a captured instance with DEF/USE identity after materialization.
  const runtime::ProtoInstance *source = nullptr;
  if (scene_) {
    auto it = scene_->expandedSources.find(node.get());
    if (it != scene_->expandedSources.end()) source = &it->second;
  }
  auto wrapper = std::dynamic_pointer_cast<runtime::ProtoInstanceTemplate>(node);
  const auto *placed = scene_ && !source && wrapper
                           ? scene_->instanceAtPlacement(node.get()) : nullptr;
  if (!source) source = placed;
  if (!source && wrapper) source = &wrapper->instance;
  if (source) {
    const std::string def = placed ? placed->DEF : node->getDEF();
    if (seen_.count(node.get())) {
      os << "USE " << def;
      return;
    }
    if (!def.empty()) seen_.insert(node.get());
    auto inst = *source;
    inst.DEF = def;
    writeVrmlProtoInstance(os, inst, depth);
    return;
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
    if (f.isNode())
      continue; // node-child fields emitted in authored order below
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

  // A PROTO body can declare a prototype between complete field assignments.
  // Classic's MFNode grammar does not admit declarations inside its brackets.
  const bool hasOrder = bodyOrder_ &&
      bodyOrder_->nodeStatements.contains(std::weak_ptr<X3DNode>(node));
  if (hasOrder) {
    auto entries = runtime::orderedNodeStatements(*bodyOrder_, node);
    // A declaration splitting one field can precede its literal prefix. Only
    // cross ordinary node graphs: moving it across a prototype use/declaration
    // could change lexical lookup (including in descendants or defaults).
    using Kind = runtime::ProtoBodyStatement::Kind;
    const auto isDeclaration = [](const auto &entry) {
      return entry.kind == Kind::Proto || entry.kind == Kind::ExternProto;
    };
    std::unordered_set<const X3DNode *> visited;
    const auto literal = [&](const auto &self, const auto &entry) -> bool {
      if (entry.kind != Kind::Node ||
          std::dynamic_pointer_cast<runtime::ProtoInstanceTemplate>(entry.node) ||
          (entry.node && authorFields_ && authorFields_->hasAuthorFields(*entry.node)))
        return false;
      if (!visited.insert(entry.node.get()).second) return true;
      for (const auto &child : runtime::orderedNodeStatements(*bodyOrder_, entry.node))
        if (!self(self, child)) return false;
      return true;
    };
    for (std::size_t i = 1; i < entries.size(); ++i) {
      if (!isDeclaration(entries[i]) || isDeclaration(entries[i - 1])) continue;
      std::size_t end = i + 1;
      while (end < entries.size() && isDeclaration(entries[end])) ++end;
      const auto &slot = entries[i - 1].field;
      if (end == entries.size() || entries[end].field != slot) continue;
      const auto fields = node->fields();
      if (std::none_of(fields.begin(), fields.end(), [&](const FieldInfo &field) {
            return field.x3dName == slot && field.type == X3DFieldType::MFNode;
          })) continue;
      std::size_t begin = i - 1;
      while (begin > 0 && entries[begin - 1].field == slot) --begin;
      visited.clear();
      bool safe = true;
      for (std::size_t j = begin; j < i && safe; ++j)
        safe = literal(literal, entries[j]);
      if (safe)
        std::rotate(entries.begin() + begin, entries.begin() + i, entries.begin() + end);
      i = end - 1;
    }
    std::unordered_set<std::string> emittedFields;
    for (std::size_t i = 0; i < entries.size();) {
      const auto &entry = entries[i];
      if (entry.kind == runtime::ProtoBodyStatement::Kind::Proto) {
        writeVrmlProtoDeclare(os, *entry.proto);
        ++i;
        continue;
      }
      if (entry.kind == runtime::ProtoBodyStatement::Kind::ExternProto) {
        writeVrmlExternProtoDeclare(os, *entry.externProto);
        ++i;
        continue;
      }
      const std::string &slot = entry.field;
      if (!emittedFields.insert(slot).second)
        throw std::runtime_error(
            "cannot currently preserve PROTO body child order in Classic "
            "within one node field: " + slot);
      const FieldInfo *field = nullptr;
      for (const auto &f : node->fields())
        if (f.isNode() && f.x3dName == slot) { field = &f; break; }
      if (!field)
        throw std::runtime_error("unknown PROTO body child field: " + slot);
      std::size_t end = i + 1;
      while (end < entries.size() &&
             (entries[end].kind == runtime::ProtoBodyStatement::Kind::Node ||
              entries[end].kind == runtime::ProtoBodyStatement::Kind::Instance) &&
             entries[end].field == slot)
        ++end;
      pad(os, depth + 1);
      os << slot << " ";
      if (field->type == X3DFieldType::MFNode) os << "[\n";
      for (std::size_t j = i; j < end; ++j) {
        if (field->type == X3DFieldType::MFNode) pad(os, depth + 2);
        if (entries[j].kind == runtime::ProtoBodyStatement::Kind::Node)
          writeNode(os, entries[j].node, depth + 2);
        else {
          std::ostringstream instanceText;
          writeVrmlProtoInstance(instanceText,
              bodyOrder_->nestedInstances[entries[j].instanceIndex], depth + 2);
          const std::string indented = instanceText.str();
          os << indented.substr(static_cast<std::size_t>((depth + 2) * 2));
        }
        os << "\n";
      }
      if (field->type == X3DFieldType::MFNode) {
        pad(os, depth + 1);
        os << "]\n";
      }
      i = end;
    }
  } else {
    for (const FieldInfo *cf : build::orderedChildFields(*node, scene_))
      writeNodeField(os, node, *cf, depth + 1);
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
  if (!hasOrder) writeVrmlNestedFor(os, node, depth + 1);

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
      if (inst.expanded || inst.hasPlacementTemplate())
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
          if (inst.expanded || inst.hasPlacementTemplate())
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
          if (firstNonSpace == std::string::npos)
            firstNonSpace = 0;
          pad(os, depth + 1);
          os << slot << " " << body.substr(firstNonSpace) << "\n";
          break; // SFNode: only one child
        }
      } else {
        pad(os, depth + 1);
        os << slot << " [\n";
        for (const auto &inst : scene_->protoInstances) {
          if (inst.expanded || inst.hasPlacementTemplate())
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

void VrmlWriter::writeAuthorFields(std::ostringstream &os, const X3DNode &node,
                                   int depth) {
  if (!authorFields_ || !authorFields_->hasAuthorFields(node))
    return;
  for (const FieldInfo &f : authorFields_->authorFields(node)) {
    pad(os, depth);
    os << accessTypeName(f.access) << " " << fieldTypeName(f.type) << " "
       << f.x3dName;
    const bool valued = f.access == AccessType::InitializeOnly ||
                        f.access == AccessType::InputOutput;
    if (valued)
      writeInterfaceValue(os, f.type, f.get ? f.get(node) : std::any{});
    os << "\n";
  }
}

void VrmlWriter::writeNodeField(std::ostringstream &os,
                                const std::shared_ptr<X3DNode> &node,
                                const FieldInfo &f, int depth) {
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

} // namespace x3d::codec
