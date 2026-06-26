// JsonWriter.hpp
// Node-agnostic X3D-JSON serializer (writer only; round-trip not required).
//
// Emits the X3D-JSON encoding per the Web3D X3D JSON schema conventions:
//   { "X3D": { "@profile": "...", "@version": "...",
//              "head": { "meta": [ { "@name":..., "@content":... } ], ... },
//              "Scene": { "-children": [ <node>, ... ] } } }
// where each node is { "TypeName": { "@field": value, "-childField": [ ... ] }
// }. Value attributes are prefixed '@'; SFNode/MFNode child fields are prefixed
// '-' and always hold an array of child node objects. DEF/USE map to
// "@DEF"/"@USE". MF numeric values are emitted as JSON arrays; SF structured
// values (vectors, colors, rotations) are emitted as JSON arrays of their
// components; scalars use native JSON types where natural (numbers, booleans),
// strings stay strings.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_JSON_WRITER_HPP
#define X3D_JSON_WRITER_HPP

#include "DynamicField.hpp" // author-field re-emit (SCR-SAI-DYN S1)
#include "FieldValueIO.hpp"
#include "ProtoNameMaps.hpp"
#include "Script.hpp" // Script::getSourceCode (inline source re-emit)
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

/// Serializes the runtime document model to X3D-JSON.
class JsonWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc) {
    seen_.clear();
    defaults_.clear();
    scene_ = &doc.scene;
    std::ostringstream os;
    os << "{\n";
    os << "  \"X3D\": {\n";
    os << "    \"@profile\": " << jstr(doc.profileName()) << ",\n";
    os << "    \"@version\": " << jstr(doc.version) << ",\n";
    os << "    \"head\": {\n";
    writeHead(os, doc.head, 3);
    os << "    },\n";
    os << "    \"Scene\": {\n";
    os << "      \"-children\": [\n";
    writeSceneChildren(os, doc.scene, 4);
    os << "      ]";
    // ROUTE / IMPORT / EXPORT statements at scene scope, as sibling members of
    // "-children" so the JsonReader can recover them (read/write symmetry).
    writeSceneStatements(os, doc.scene, 3);
    os << "\n";
    os << "    }\n";
    os << "  }\n";
    os << "}\n";
    return os.str();
  }

private:
  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup. Null for body-internal
  // fresh writers so proto expansion redirect is off inside proto bodies.
  const runtime::Scene *scene_ = nullptr;
  // PRF-1: while re-emitting a ProtoBody, the body's IsConnection list so that
  // writeNode can attach an "IS":{"connect":[...]} member to EVERY emitted body
  // node at any depth (mirrors the XML writer's bodyIsc_). Null outside a body.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  // PRF-3: while re-emitting a ProtoBody, the body's nestedInstances list so
  // writeNode can inject each Case-A instance (parent == the node being written)
  // INSIDE that node's child slot — mirroring XmlWriter pushing the instance
  // onto the parent element's children — rather than appending it as a sibling.
  // Null outside a body.
  const std::vector<runtime::ProtoInstance> *bodyNested_ = nullptr;

  X3DNode *defaultFor(const std::string &typeName) {
    auto it = defaults_.find(typeName);
    if (it != defaults_.end())
      return it->second.get();
    auto fresh = X3DNodeFactory::create(typeName);
    X3DNode *raw = fresh.get();
    defaults_[typeName] = std::move(fresh);
    return raw;
  }

  static void pad(std::ostringstream &os, int depth) {
    for (int i = 0; i < depth; ++i)
      os << "  ";
  }

  static std::string jstr(const std::string &s) {
    std::string out = "\"";
    for (char c : s) {
      switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\t':
        out += "\\t";
        break;
      case '\r':
        out += "\\r";
        break;
      default:
        out += c;
        break;
      }
    }
    out += "\"";
    return out;
  }

  void writeHead(std::ostringstream &os, const runtime::Head &head, int depth) {
    bool first = true;
    if (!head.components.empty()) {
      pad(os, depth);
      os << "\"component\": [\n";
      for (std::size_t i = 0; i < head.components.size(); ++i) {
        pad(os, depth + 1);
        os << "{ \"@name\": " << jstr(head.components[i].name)
           << ", \"@level\": " << head.components[i].level << " }"
           << (i + 1 < head.components.size() ? "," : "") << "\n";
      }
      pad(os, depth);
      os << "]";
      first = false;
    }
    if (!head.meta.empty()) {
      if (!first)
        os << ",";
      os << "\n";
      pad(os, depth);
      os << "\"meta\": [\n";
      for (std::size_t i = 0; i < head.meta.size(); ++i) {
        pad(os, depth + 1);
        os << "{ \"@name\": " << jstr(head.meta[i].name)
           << ", \"@content\": " << jstr(head.meta[i].content) << " }"
           << (i + 1 < head.meta.size() ? "," : "") << "\n";
      }
      pad(os, depth);
      os << "]";
      first = false;
    }
    os << "\n";
  }

  /// Emit all scene -children: PROTO declarations first, then root nodes.
  void writeSceneChildren(std::ostringstream &os, const runtime::Scene &s,
                          int depth) {
    bool firstWritten = true;

    // PROTO declarations.
    for (const auto &d : s.protoDeclarations) {
      if (!d) continue;
      if (!firstWritten) os << ",\n";
      firstWritten = false;
      writeJsonProtoDeclare(os, *d, depth);
    }
    // ExternProto declarations.
    for (const auto &e : s.externProtoDeclarations) {
      if (!e) continue;
      if (!firstWritten) os << ",\n";
      firstWritten = false;
      writeJsonExternProtoDeclare(os, *e, depth);
    }
    // Root nodes.
    for (const auto &n : s.rootNodes) {
      if (!n) continue;
      if (!firstWritten) os << ",\n";
      firstWritten = false;
      writeNode(os, n, depth);
    }
    // AUD-B: re-emit scene-root ProtoInstances that did NOT expand (no graph
    // node, so not covered by the expandedSources re-emit in writeNode).
    for (const auto &inst : s.protoInstances) {
      if (inst.expanded || !inst.parent.expired()) continue;
      if (!firstWritten) os << ",\n";
      firstWritten = false;
      writeJsonProtoInstance(os, inst, depth);
    }
    os << "\n";
  }

  /// Emit ROUTE/IMPORT/EXPORT scene-scope statements as members of the "Scene"
  /// object (each an array of '@'-prefixed-attribute objects), matching the
  /// JsonReader's expectations so they survive a JSON round-trip. Each emitted
  /// member is preceded by ",\n" so it chains after the "-children" array.
  void writeSceneStatements(std::ostringstream &os, const runtime::Scene &scene,
                            int depth) {
    if (!scene.routes.empty()) {
      os << ",\n";
      pad(os, depth);
      os << "\"ROUTE\": [\n";
      for (std::size_t i = 0; i < scene.routes.size(); ++i) {
        const runtime::Route &r = scene.routes[i];
        pad(os, depth + 1);
        os << "{ \"@fromNode\": " << jstr(r.fromNode)
           << ", \"@fromField\": " << jstr(r.fromField)
           << ", \"@toNode\": " << jstr(r.toNode)
           << ", \"@toField\": " << jstr(r.toField) << " }"
           << (i + 1 < scene.routes.size() ? "," : "") << "\n";
      }
      pad(os, depth);
      os << "]";
    }
    if (!scene.imports.empty()) {
      os << ",\n";
      pad(os, depth);
      os << "\"IMPORT\": [\n";
      for (std::size_t i = 0; i < scene.imports.size(); ++i) {
        const runtime::Import &im = scene.imports[i];
        pad(os, depth + 1);
        os << "{ \"@inlineDEF\": " << jstr(im.inlineDEF)
           << ", \"@importedDEF\": " << jstr(im.importedDEF)
           << ", \"@AS\": " << jstr(im.as) << " }"
           << (i + 1 < scene.imports.size() ? "," : "") << "\n";
      }
      pad(os, depth);
      os << "]";
    }
    if (!scene.exports.empty()) {
      os << ",\n";
      pad(os, depth);
      os << "\"EXPORT\": [\n";
      for (std::size_t i = 0; i < scene.exports.size(); ++i) {
        const runtime::Export &ex = scene.exports[i];
        pad(os, depth + 1);
        os << "{ \"@localDEF\": " << jstr(ex.localDEF)
           << ", \"@AS\": " << jstr(ex.as) << " }"
           << (i + 1 < scene.exports.size() ? "," : "") << "\n";
      }
      pad(os, depth);
      os << "]";
    }
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

  /// Re-emit a captured <ProtoInstance> as a JSON object.
  void writeJsonProtoInstance(std::ostringstream &os,
                              const runtime::ProtoInstance &src, int depth) {
    pad(os, depth);
    os << "{ \"ProtoInstance\": {\n";

    std::vector<std::string> members;
    members.push_back("\"@name\": " + jstr(src.name));
    if (!src.DEF.empty())
      members.push_back("\"@DEF\": " + jstr(src.DEF));
    if (!src.containerField.empty() && src.containerField != "children")
      members.push_back("\"@containerField\": " + jstr(src.containerField));

    // Collect all fieldValues (scalar and node-valued) in original order into
    // a single "fieldValue" array to avoid duplicate JSON object keys.
    std::vector<std::string> fvElements;
    for (const runtime::ProtoFieldValue &fv : src.fieldValues) {
      if (!fv.nodeValue.empty()) {
        // Node-valued: emit as { "@name": <name>, "-children": [ ... ] }
        std::string elem = "{ \"@name\": " + jstr(fv.name) + ", \"-children\": [";
        bool fc = true;
        for (const auto &child : fv.nodeValue) {
          if (!child) continue;
          if (!fc) elem += ",";
          fc = false;
          std::ostringstream nos;
          writeNode(nos, child, 0);
          elem += nos.str();
        }
        elem += "] }";
        fvElements.push_back(std::move(elem));
      } else if (fv.value.has_value()) {
        // Scalar-valued: emit as { "@name": <name>, "@value": <formatted> }
        // or just { "@name": <name> } if the type is unknown.
        const runtime::ProtoField *pf = interfaceField(src, fv.name);
        if (pf) {
          std::string valStr = formatValue(pf->type, fv.value);
          fvElements.push_back("{ \"@name\": " + jstr(fv.name) +
                               ", \"@value\": " + jstr(valStr) + " }");
        } else {
          fvElements.push_back("{ \"@name\": " + jstr(fv.name) + " }");
        }
      }
    }

    if (!fvElements.empty()) {
      std::string fvArr = "\"fieldValue\": [\n";
      for (std::size_t i = 0; i < fvElements.size(); ++i) {
        fvArr += fvElements[i];
        if (i + 1 < fvElements.size()) fvArr += ",";
        fvArr += "\n";
      }
      fvArr += "]";
      members.push_back(std::move(fvArr));
    }

    for (std::size_t i = 0; i < members.size(); ++i) {
      pad(os, depth + 1);
      os << members[i] << (i + 1 < members.size() ? "," : "") << "\n";
    }
    pad(os, depth);
    os << "} }";
  }

  /// Emit one proto interface `field` as a JSON object.
  std::string jsonProtoField(const runtime::ProtoField &f, int depth) {
    std::ostringstream os;
    os << "{ \"@name\": " << jstr(f.name)
       << ", \"@type\": " << jstr(fieldTypeName(f.type))
       << ", \"@accessType\": " << jstr(accessTypeName(f.access));
    if (!f.nodeDefault.empty()) {
      // SFNode/MFNode default: emit as "-children".
      os << ", \"-children\": [";
      bool first = true;
      for (const auto &n : f.nodeDefault) {
        if (!n) continue;
        if (!first) os << ",";
        first = false;
        // Use a fresh writer (no scene context) for body nodes.
        JsonWriter bodyWriter;
        std::ostringstream nos;
        bodyWriter.writeNode(nos, n, 0);
        os << nos.str();
      }
      os << "]";
    } else if (f.value.has_value() &&
               (f.access == AccessType::InitializeOnly ||
                f.access == AccessType::InputOutput)) {
      os << ", \"@value\": " << jstr(formatValue(f.type, f.value));
    }
    os << " }";
    return os.str();
  }

  /// Emit a <ProtoDeclare> as a JSON object entry in the -children array.
  void writeJsonProtoDeclare(std::ostringstream &os,
                             const runtime::ProtoDeclaration &d, int depth) {
    pad(os, depth);
    os << "{ \"ProtoDeclare\": {\n";

    pad(os, depth + 1);
    os << "\"@name\": " << jstr(d.name);

    if (!d.appinfo.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"@appinfo\": " << jstr(d.appinfo);
    }
    if (!d.documentation.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"@documentation\": " << jstr(d.documentation);
    }

    // ProtoInterface (if any fields).
    if (!d.interface.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"ProtoInterface\": {\n";
      pad(os, depth + 2);
      os << "\"field\": [\n";
      for (std::size_t i = 0; i < d.interface.size(); ++i) {
        pad(os, depth + 3);
        os << jsonProtoField(d.interface[i], depth + 3);
        if (i + 1 < d.interface.size()) os << ",";
        os << "\n";
      }
      pad(os, depth + 2);
      os << "]\n";
      pad(os, depth + 1);
      os << "}";
    }

    // ProtoBody — use a FRESH writer so body DEF/USE is isolated.
    os << ",\n";
    pad(os, depth + 1);
    os << "\"ProtoBody\": {\n";
    pad(os, depth + 2);
    os << "\"-children\": [\n";

    bool firstBody = true;
    JsonWriter bodyWriter;
    // PRF-1: hand the body's IS list to the body writer so writeNode attaches an
    // "IS" member at every depth during the recursive descent.
    bodyWriter.bodyIsc_ = &d.body.isConnections;
    // PRF-3: thread the nested-instance list so writeNode injects each Case-A
    // instance INSIDE its parent body node's child slot (not as a sibling).
    bodyWriter.bodyNested_ = &d.body.nestedInstances;
    for (const auto &n : d.body.nodes) {
      if (!n) continue;
      if (!firstBody) os << ",\n";
      firstBody = false;
      // Emit the body node via the fresh bodyWriter; IS members AND any Case-A
      // nested instances rooted at this node (or deeper) are injected by
      // writeNode during the recursive descent.
      std::ostringstream nos;
      bodyWriter.writeNode(nos, n, depth + 3);
      os << nos.str();
    }
    // Case B: direct ProtoBody children (no parent node).
    for (const auto &ni : d.body.nestedInstances) {
      if (!ni.parent.lock()) {
        if (!firstBody) os << ",\n";
        firstBody = false;
        writeJsonProtoInstance(os, ni, depth + 3);
      }
    }
    os << "\n";
    pad(os, depth + 2);
    os << "]";

    // Body ROUTEs.
    if (!d.body.routes.empty()) {
      os << ",\n";
      pad(os, depth + 2);
      os << "\"ROUTE\": [\n";
      for (std::size_t i = 0; i < d.body.routes.size(); ++i) {
        const auto &r = d.body.routes[i];
        pad(os, depth + 3);
        os << "{ \"@fromNode\": " << jstr(r.fromNode)
           << ", \"@fromField\": " << jstr(r.fromField)
           << ", \"@toNode\": " << jstr(r.toNode)
           << ", \"@toField\": " << jstr(r.toField) << " }";
        if (i + 1 < d.body.routes.size()) os << ",";
        os << "\n";
      }
      pad(os, depth + 2);
      os << "]";
    }

    os << "\n";
    pad(os, depth + 1);
    os << "}\n";
    pad(os, depth);
    os << "} }";
  }

  /// Emit an <ExternProtoDeclare> as a JSON object entry in the -children array.
  void writeJsonExternProtoDeclare(std::ostringstream &os,
                                   const runtime::ExternProtoDeclaration &d,
                                   int depth) {
    pad(os, depth);
    os << "{ \"ExternProtoDeclare\": {\n";
    pad(os, depth + 1);
    os << "\"@name\": " << jstr(d.name);

    if (!d.appinfo.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"@appinfo\": " << jstr(d.appinfo);
    }
    if (!d.documentation.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"@documentation\": " << jstr(d.documentation);
    }

    if (!d.url.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      // Emit url as a JSON array of strings.
      os << "\"@url\": [";
      for (std::size_t i = 0; i < d.url.size(); ++i) {
        if (i) os << ", ";
        os << jstr(d.url[i]);
      }
      os << "]";
    }

    if (!d.interface.empty()) {
      os << ",\n";
      pad(os, depth + 1);
      os << "\"field\": [\n";
      for (std::size_t i = 0; i < d.interface.size(); ++i) {
        pad(os, depth + 2);
        os << jsonProtoField(d.interface[i], depth + 2);
        if (i + 1 < d.interface.size()) os << ",";
        os << "\n";
      }
      pad(os, depth + 1);
      os << "]";
    }

    os << "\n";
    pad(os, depth);
    os << "} }";
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
        writeJsonProtoInstance(os, it->second, depth);
        return;
      }
    }

    pad(os, depth);
    os << "{ " << jstr(node->nodeTypeName()) << ": {\n";

    std::vector<std::string> members;
    const std::string def = node->getDEF();

    if (seen_.count(node.get())) {
      members.push_back(std::string("\"@USE\": ") + jstr(def));
    } else {
      if (!def.empty()) {
        members.push_back(std::string("\"@DEF\": ") + jstr(def));
        seen_.insert(node.get());
      }
      // SCR-SAI-DYN (S1): a Script re-emits its inline source as "#sourceText"
      // (handled below), so suppress the plain "@sourceCode" value member to
      // avoid double-encoding the same body.
      const bool isScript = node->nodeTypeName() == "Script";
      // Value attributes (defaults elided against a fresh factory instance).
      X3DNode *defaults = defaultFor(node->nodeTypeName());
      for (const FieldInfo &f : node->fields()) {
        if (!f.isReadable() || f.isNode())
          continue;
        if (f.x3dName == "DEF" || f.x3dName == "USE" || f.x3dName == "IS")
          continue;
        if (isScript && f.x3dName == "sourceCode")
          continue;
        std::string val = jsonValue(*node, f);
        if (val.empty())
          continue;
        if (defaults && val == jsonValueOf(*defaults, f))
          continue; // equals the type default -> omit
        members.push_back("\"@" + f.x3dName + "\": " + val);
      }
      // Node child fields, grouped per containerField.
      for (const FieldInfo &f : node->fields()) {
        if (!f.isReadable() || !f.isNode())
          continue;
        std::string child = jsonNodeField(node, f, depth + 1);
        if (!child.empty())
          members.push_back(child);
      }
      // SCR-SAI-DYN (S1): re-emit a Script's author <field> declarations (from
      // the dynamic-field side-table) as a "field" array member, and its inline
      // source as a "#sourceText" array member, so a JSON round-trip preserves
      // both. Other nodes carry no author fields, so this is a no-op for them.
      if (isScript) {
        std::string fieldsMember = jsonScriptFields(*node);
        if (!fieldsMember.empty())
          members.push_back(std::move(fieldsMember));
        std::string sourceMember = jsonScriptSource(*node);
        if (!sourceMember.empty())
          members.push_back(std::move(sourceMember));
      }
      // PRF-1: re-emit IS connections bound to THIS node (at any depth) as an
      // "IS":{"connect":[{"@nodeField","@protoField"}]} member, matching the
      // shape JsonReader::collectJsonIsConnections parses.
      std::string is = jsonIsBlock(node);
      if (!is.empty())
        members.push_back(std::move(is));
      // PRF-3: inject Case-A nested ProtoInstances whose parent is THIS node,
      // grouped by parentField, as "-<slot>": [ <instance> ] members — mirroring
      // XmlWriter pushing them onto the parent element's children so the reader
      // recovers parent == this node (not Case B). depth+1 keeps the instance
      // text indented one level inside the slot array.
      for (std::string &m : jsonNestedFor(node, depth + 1))
        members.push_back(std::move(m));

      // Scene-level nested ProtoInstances: any un-expanded ProtoInstance in
      // scene.protoInstances whose parent is THIS node. These are NOT in the
      // node graph (expansion failed) and NOT scene-root (parent is live), so
      // without this injection they are lost. Emit grouped by parentField slot
      // (mirroring jsonNestedFor) so the reader recovers parent == this node.
      if (scene_) {
        for (std::string &m : jsonSceneNestedFor(node, depth + 1))
          members.push_back(std::move(m));
      }
    }

    for (std::size_t i = 0; i < members.size(); ++i) {
      pad(os, depth + 1);
      os << members[i] << (i + 1 < members.size() ? "," : "") << "\n";
    }

    pad(os, depth);
    os << "} }";
  }

  /// PRF-1: emit the "IS" member for `node` if bodyIsc_ holds any IsConnection
  /// bound to it (by identity). Returns "" when there is none. Shape:
  ///   "IS": { "connect": [ { "@nodeField":..., "@protoField":... }, ... ] }
  std::string jsonIsBlock(const std::shared_ptr<X3DNode> &node) {
    if (!bodyIsc_)
      return "";
    std::vector<const runtime::IsConnection *> mine;
    for (const auto &c : *bodyIsc_)
      if (c.node.get() == node.get())
        mine.push_back(&c);
    if (mine.empty())
      return "";
    std::string out = "\"IS\": { \"connect\": [";
    for (std::size_t i = 0; i < mine.size(); ++i) {
      if (i) out += ",";
      out += " { \"@nodeField\": " + jstr(mine[i]->nodeField) +
             ", \"@protoField\": " + jstr(mine[i]->protoField) + " }";
    }
    out += " ] }";
    return out;
  }

  /// PRF-3: build "-<slot>": [ <ProtoInstance> ] member strings for every
  /// Case-A nested instance whose parent is `node`, grouped by parentField slot
  /// so the JsonReader recovers parent == node and the same slot. Empty when
  /// there are none (or no body nested list threaded in). `depth` is the slot's
  /// own indent level; the array contents sit at depth+1.
  std::vector<std::string> jsonNestedFor(const std::shared_ptr<X3DNode> &node,
                                         int depth) {
    std::vector<std::string> out;
    if (!bodyNested_)
      return out;
    // Preserve declaration order; group consecutive-by-slot is not required by
    // the reader (it merges repeated "-slot" keys), so emit one member per slot.
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
      std::ostringstream os;
      os << "\"-" << slot << "\": [\n";
      bool first = true;
      for (const auto &ni : *bodyNested_) {
        if (ni.parent.lock().get() != node.get())
          continue;
        const std::string s =
            ni.parentField.empty() ? ni.containerField : ni.parentField;
        if (s != slot)
          continue;
        if (!first)
          os << ",\n";
        first = false;
        writeJsonProtoInstance(os, ni, depth + 1);
      }
      os << "\n";
      pad(os, depth);
      os << "]";
      out.push_back(os.str());
    }
    return out;
  }

  /// Scene-level nested ProtoInstances: build "-<slot>": [ <ProtoInstance> ]
  /// member strings for every un-expanded instance in scene.protoInstances
  /// whose parent is `node`, grouped by parentField slot. Mirrors jsonNestedFor
  /// but reads from scene_.protoInstances instead of bodyNested_. Empty when
  /// there are none or scene_ is null. `depth` is the slot's own indent level.
  std::vector<std::string>
  jsonSceneNestedFor(const std::shared_ptr<X3DNode> &node, int depth) {
    std::vector<std::string> out;
    if (!scene_)
      return out;
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
      std::ostringstream os;
      os << "\"-" << slot << "\": [\n";
      bool first = true;
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
        if (!first)
          os << ",\n";
        first = false;
        writeJsonProtoInstance(os, inst, depth + 1);
      }
      os << "\n";
      pad(os, depth);
      os << "]";
      out.push_back(os.str());
    }
    return out;
  }

  /// jsonValueOf on the (raw) defaults instance, for default elision.
  std::string jsonValueOf(const X3DNode &node, const FieldInfo &f) {
    return jsonValue(node, f);
  }

  /// Format a value field as a JSON value (scalar, string, or array). Returns
  /// "" to omit (e.g. empty MF or unsupported type).
  std::string jsonValue(const X3DNode &node, const FieldInfo &f) {
    if (f.isEnum()) {
      if (!f.getEnumString)
        return "";
      std::string s = f.getEnumString(node);
      if (f.type == X3DFieldType::MFEnum) {
        auto toks = tokenize(s);
        std::string out = "[";
        for (std::size_t i = 0; i < toks.size(); ++i)
          out += (i ? "," : "") + jstr(toks[i]);
        out += "]";
        return out;
      }
      return jstr(s);
    }
    std::any v = f.get(node);
    switch (f.type) {
    case X3DFieldType::SFBool:
      return fmtBool(std::any_cast<SFBool>(v));
    case X3DFieldType::SFInt32:
      return std::to_string(std::any_cast<int>(v));
    case X3DFieldType::SFFloat:
      return fmtFloat(std::any_cast<float>(v));
    case X3DFieldType::SFDouble:
    case X3DFieldType::SFTime:
      return fmtDouble(std::any_cast<double>(v));
    case X3DFieldType::SFString:
      return jstr(std::any_cast<std::string>(v));
    case X3DFieldType::MFString: {
      const auto &vec = std::any_cast<std::vector<std::string>>(v);
      std::string out = "[";
      for (std::size_t i = 0; i < vec.size(); ++i)
        out += (i ? "," : "") + jstr(vec[i]);
      out += "]";
      return out;
    }
    default: {
      // Structured SF (vectors/colors/rotations) and numeric MF: format to the
      // space-separated wire string, then re-emit as a JSON number array.
      std::string wire = formatValue(f.type, v);
      if (wire.empty())
        return "";
      auto toks = tokenize(wire);
      std::string out = "[";
      for (std::size_t i = 0; i < toks.size(); ++i)
        out += (i ? "," : "") + toks[i];
      out += "]";
      return out;
    }
    }
  }

  /// Emit one node child field as a "-field": [ children ] member, or "" if the
  /// field holds no node.
  std::string jsonNodeField(const std::shared_ptr<X3DNode> &node,
                            const FieldInfo &f, int depth) {
    std::any v = f.get(*node);
    std::vector<std::shared_ptr<X3DNode>> kids;
    if (f.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(v);
      if (c)
        kids.push_back(c);
    } else {
      kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
    }
    if (kids.empty())
      return "";
    std::ostringstream os;
    os << "\"-" << f.containerField << "\": [\n";
    bool first = true;
    for (const auto &k : kids) {
      if (!k)
        continue;
      if (!first)
        os << ",\n";
      first = false;
      writeNode(os, k, depth + 1);
    }
    os << "\n";
    pad(os, depth);
    os << "]";
    return os.str();
  }

  /// SCR-SAI-DYN (S1): re-emit a Script's author <field> declarations from the
  /// dynamic-field side-table as a single-line "field": [ ... ] array member,
  /// each element { "@name", "@type", "@accessType", "@value"? }. Returns "" if
  /// the Script declared no author fields. The "@value" is emitted only for
  /// fields that carry a persistent default (initializeOnly / inputOutput).
  std::string jsonScriptFields(const X3DNode &node) {
    std::vector<FieldInfo> author =
        runtime::dynamicFieldStore().authorFields(node);
    if (author.empty())
      return "";
    std::string out = "\"field\": [";
    for (std::size_t i = 0; i < author.size(); ++i) {
      const FieldInfo &f = author[i];
      if (i)
        out += ",";
      out += " { \"@name\": " + jstr(f.x3dName) +
             ", \"@type\": " + jstr(fieldTypeName(f.type)) +
             ", \"@accessType\": " + jstr(accessTypeName(f.access));
      if ((f.access == AccessType::InitializeOnly ||
           f.access == AccessType::InputOutput) &&
          f.get) {
        std::any v = f.get(node);
        if (v.has_value()) {
          std::string wire = formatValue(f.type, v);
          out += ", \"@value\": " + jstr(wire);
        }
      }
      out += " }";
    }
    out += " ]";
    return out;
  }

  /// SCR-SAI-DYN (S1): re-emit a Script's inline source (Script.sourceCode) as
  /// the canonical X3D-JSON "#sourceText" array, one element per source line so
  /// the JsonReader recovers it. Returns "" when the Script has no inline body.
  std::string jsonScriptSource(const X3DNode &node) {
    const auto *script = dynamic_cast<const Script *>(&node);
    if (!script)
      return "";
    const std::string src = script->getSourceCode();
    if (src.empty())
      return "";
    std::string out = "\"#sourceText\": [";
    std::size_t start = 0;
    bool first = true;
    while (start <= src.size()) {
      std::size_t nl = src.find('\n', start);
      std::string line =
          src.substr(start, nl == std::string::npos ? std::string::npos
                                                    : nl - start);
      if (!first)
        out += ",";
      first = false;
      out += " " + jstr(line);
      if (nl == std::string::npos)
        break;
      start = nl + 1;
    }
    out += " ]";
    return out;
  }
};

} // namespace x3d::codec

#endif // X3D_JSON_WRITER_HPP
