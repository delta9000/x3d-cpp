// XmlWriter.hpp
// Node-agnostic X3D-XML serializer: turns an x3d::runtime::X3DDocument (or a
// single node / Scene) into the X3D-XML encoding.
//
// Strategy (all driven by reflection, no per-node code):
//   * A node element is named by node->nodeTypeName().
//   * Each reflected value field (SF*/MF* that is not a node) becomes an XML
//     attribute, formatted via FieldValueIO. Enum fields use the FieldInfo
//     string thunk. DEF/USE are emitted from the node's DEF/USE fields.
//   * Fields equal to their type's default are omitted (clean output). The
//     default is discovered by formatting the same field on a fresh factory
//     instance of the node's type.
//   * SFNode/MFNode fields become child elements, placed under the child's
//     containerField (the field's own name unless the child overrides it).
//   * DEF/USE: the first time a shared node (by identity) is written it gets a
//     DEF; subsequent writes emit a <Type USE='...'/> reference instead.
//   * The document wraps <X3D><head>...</head><Scene>...</Scene></X3D>; ROUTEs
//     and IMPORT/EXPORT are emitted as Scene children.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_XML_WRITER_HPP
#define X3D_XML_WRITER_HPP

#include "DynamicField.hpp"
#include "FieldValueIO.hpp"
#include "ProtoNameMaps.hpp"
#include "Script.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"
#include "XmlLite.hpp"
#include "parse/NodeBuilder.hpp" // build::orderedChildFields (authored child order)

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::codec {

/// Serializes the runtime document model to X3D-XML.
class XmlWriter {
public:
  /// Serialize a full document to an X3D-XML string (with <?xml?> prolog).
  std::string writeDocument(const runtime::X3DDocument &doc) {
    seen_.clear();
    auto root = std::make_unique<xml::Element>();
    root->name = "X3D";
    root->setAttr("profile", doc.profileName());
    root->setAttr("version", doc.version);

    // <head>
    if (!doc.head.empty()) {
      xml::Element *head = root->addChild("head");
      for (const auto &c : doc.head.components) {
        xml::Element *e = head->addChild("component");
        e->setAttr("name", c.name);
        e->setAttr("level", std::to_string(c.level));
      }
      for (const auto &u : doc.head.units) {
        xml::Element *e = head->addChild("unit");
        e->setAttr("category", u.category);
        e->setAttr("name", u.name);
        e->setAttr("conversionFactor", fmtDouble(u.conversionFactor));
      }
      for (const auto &m : doc.head.meta) {
        xml::Element *e = head->addChild("meta");
        e->setAttr("name", m.name);
        e->setAttr("content", m.content);
        if (!m.dir.empty()) e->setAttr("dir", m.dir);
        if (!m.httpEquiv.empty()) e->setAttr("http-equiv", m.httpEquiv);
        if (!m.lang.empty()) e->setAttr("lang", m.lang);
        if (!m.scheme.empty()) e->setAttr("scheme", m.scheme);
      }
    }

    // <Scene>
    xml::Element *scene = root->addChild("Scene");
    writeSceneInto(scene, doc.scene);

    std::ostringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    render(*root, os, 0);
    return os.str();
  }

  /// Serialize just a Scene's children/routes (no <X3D> wrapper).
  std::string writeScene(const runtime::Scene &scene) {
    seen_.clear();
    auto el = std::make_unique<xml::Element>();
    el->name = "Scene";
    writeSceneInto(el.get(), scene);
    std::ostringstream os;
    render(*el, os, 0);
    return os.str();
  }

  /// Serialize a single node subtree (no Scene/X3D wrapper). Useful for tests.
  std::string writeNode(const std::shared_ptr<X3DNode> &node) {
    seen_.clear();
    scene_ = nullptr; // no scene context: expandedSources lookup is disabled
    auto el = writeNodeElement(node, "");
    std::ostringstream os;
    if (el)
      render(*el, os, 0);
    return os.str();
  }

private:
  // Identity set of nodes already written with a DEF (for USE on revisits).
  std::unordered_set<const X3DNode *> seen_;
  // Cache of fresh "default" instances per node type, for default elision.
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup at node-emit sites. Null
  // when serializing a bare node (writeNode) — expansion round-trip is then off.
  const runtime::Scene *scene_ = nullptr;
  // PRF-2: while re-emitting a ProtoBody, the body's IsConnection list so that
  // writeNodeElement can attach an <IS> block to EVERY emitted body node at any
  // depth (not just the top body node). Null outside ProtoBody re-emit.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;

  void writeSceneInto(xml::Element *scene, const runtime::Scene &s) {
    scene_ = &s; // enable <ProtoInstance> re-emit for expanded primaries below
    // Emit declarations before nodes: X3D requires declarations before use.
    for (const auto &d : s.protoDeclarations)
      if (d) scene->children.push_back(writeProtoDeclareElement(*d));
    for (const auto &e : s.externProtoDeclarations)
      if (e) scene->children.push_back(writeExternProtoDeclareElement(*e));
    for (const auto &n : s.rootNodes) {
      auto child = writeNodeElement(n, "");
      if (child)
        scene->children.push_back(std::move(child));
    }
    // AUD-B: re-emit scene-root ProtoInstances that did NOT expand (e.g.
    // unresolvable EXTERNPROTO). Expanded instances are already re-emitted above
    // via expandedSources at their primary node; un-expanded ones have no graph
    // node, so emit them directly or they are lost on round-trip.
    for (const auto &inst : s.protoInstances)
      if (!inst.expanded && inst.parent.expired())
        scene->children.push_back(writeProtoInstanceElement(inst, ""));
    for (const auto &r : s.routes) {
      xml::Element *e = scene->addChild("ROUTE");
      e->setAttr("fromNode", r.fromNode);
      e->setAttr("fromField", r.fromField);
      e->setAttr("toNode", r.toNode);
      e->setAttr("toField", r.toField);
    }
    for (const auto &imp : s.imports) {
      xml::Element *e = scene->addChild("IMPORT");
      e->setAttr("inlineDEF", imp.inlineDEF);
      e->setAttr("importedDEF", imp.importedDEF);
      if (!imp.as.empty())
        e->setAttr("AS", imp.as);
    }
    for (const auto &exp : s.exports) {
      xml::Element *e = scene->addChild("EXPORT");
      e->setAttr("localDEF", exp.localDEF);
      if (!exp.as.empty())
        e->setAttr("AS", exp.as);
    }
  }

  /// Return the fresh "all-defaults" instance for a node type (cached).
  X3DNode *defaultFor(const std::string &typeName) {
    auto it = defaults_.find(typeName);
    if (it != defaults_.end())
      return it->second.get();
    auto fresh = X3DNodeFactory::create(typeName);
    X3DNode *raw = fresh.get();
    defaults_[typeName] = std::move(fresh);
    return raw;
  }

  /// Build the XML element for one node, recursing into node fields.
  /// `containerOverride` is the containerField the parent placed this child in;
  /// if it differs from the node's default container, emit a containerField attr.
  std::unique_ptr<xml::Element>
  writeNodeElement(const std::shared_ptr<X3DNode> &node,
                   const std::string &containerOverride) {
    if (!node)
      return nullptr;

    // Inline round-trip: if this node is the synthetic Group of an expanded
    // <Inline>, re-emit the original <Inline url=.../> and do NOT descend into
    // the Group's child content (which the reader will re-fetch on load).
    if (scene_) {
      auto il = scene_->expandedInlines.find(node.get());
      if (il != scene_->expandedInlines.end())
        return writeNodeElement(il->second, containerOverride);
    }

    // PROTO round-trip: if this node is the expanded primary of a captured
    // <ProtoInstance>, re-emit the original instance and do NOT descend into the
    // expansion's cloned subtree (which the reader will regenerate on load).
    if (scene_) {
      auto it = scene_->expandedSources.find(node.get());
      if (it != scene_->expandedSources.end())
        return writeProtoInstanceElement(it->second, containerOverride);
    }

    const std::string typeName = node->nodeTypeName();
    auto el = std::make_unique<xml::Element>();
    el->name = typeName;

    // DEF/USE handling by node identity.
    const std::string def = node->getDEF();
    if (seen_.count(node.get())) {
      // Already written: emit a USE reference (no fields, no children).
      el->setAttr("USE", def);
      if (!containerOverride.empty())
        el->setAttr("containerField", containerOverride);
      return el;
    }
    if (!def.empty()) {
      el->setAttr("DEF", def);
      seen_.insert(node.get());
    }

    X3DNode *defaults = defaultFor(typeName);

    // Emit value attributes; recurse node fields into children.
    const FieldTable &table = node->fields();
    for (const FieldInfo &f : table) {
      if (!f.isReadable())
        continue;
      if (f.x3dName == "DEF" || f.x3dName == "USE")
        continue; // handled above
      if (f.x3dName == "IS")
        continue; // prototype plumbing, not a serializable attribute
      // SCR-SAI-DYN (S1): a Script's sourceCode is re-emitted as a <![CDATA[]]>
      // body (below), not as a flat attribute, so the inline source round-trips
      // through the same channel the reader captures it from.
      if (f.x3dName == "sourceCode" && dynamic_cast<const Script *>(node.get()))
        continue;

      if (f.isNode())
        continue; // node-child fields emitted in authored order below

      // Value (attribute) field.
      std::string text;
      std::string defText;
      if (f.isEnum()) {
        if (!f.getEnumString)
          continue;
        text = f.getEnumString(*node);
        if (defaults && f.getEnumString)
          defText = f.getEnumString(*defaults);
      } else {
        std::any v = f.get(*node);
        text = formatValue(f.type, v);
        if (defaults) {
          // Find the matching field on the defaults instance (same name).
          for (const FieldInfo &df : defaults->fields()) {
            if (df.x3dName == f.x3dName && df.isReadable()) {
              defText = formatValue(df.type, df.get(*defaults));
              break;
            }
          }
        }
      }
      // Omit fields equal to their default to keep output clean.
      if (defaults && text == defText)
        continue;
      // An empty SF/MFString that is also the default-empty is already skipped;
      // emit non-default empties (rare) too.
      el->setAttr(f.x3dName, text);
    }

    // Node-child fields, in authored order (round-trip fidelity) so a node
    // shared across fields keeps its authored DEF placement; declaration order
    // when nothing was recorded for this node.
    for (const FieldInfo *cf : x3d::codec::build::orderedChildFields(*node, scene_))
      writeNodeField(*el, node, *cf);

    // Scene-level nested ProtoInstances: any un-expanded ProtoInstance whose
    // parent is THIS node (scene.protoInstances, !expanded, parent==node) must
    // be re-emitted as a child of this element in its parentField slot.
    // These instances are NOT in the node graph (expansion failed) and NOT
    // scene-root (parent is live), so without this injection they are lost.
    // Scene-root instances (parent.expired()) are handled in writeSceneInto.
    if (scene_) {
      for (const auto &inst : scene_->protoInstances) {
        if (inst.expanded)
          continue;
        auto p = inst.parent.lock();
        if (!p || p.get() != node.get())
          continue;
        const std::string slot =
            inst.parentField.empty() ? inst.containerField : inst.parentField;
        el->children.push_back(writeProtoInstanceElement(inst, slot));
      }
    }

    if (!containerOverride.empty()) {
      // Stamp the parent's slot so the reader can route this child back to the
      // correct field by matching containerField against the parent's node-field
      // names — fully reflection-driven, no per-node default-container table.
      el->setAttr("containerField", containerOverride);
    }
    // PRF-2: inside a ProtoBody re-emit, attach this node's <IS> block (if any)
    // as its last child. Threading via bodyIsc_ through the recursion means a
    // node nested arbitrarily deep in a body node gets its IS re-emitted, not
    // just the top body node.
    if (bodyIsc_)
      attachIsBlocks(*el, node, *bodyIsc_);
    // SCR-SAI-DYN (S1): re-emit a Script's author <field> declarations (from the
    // DynamicFieldStore) and its inline source body so a captured Script
    // round-trips read -> write -> reparse. Done last so author <field>s follow
    // any node children, matching the authoring convention.
    if (const auto *script = dynamic_cast<const Script *>(node.get()))
      writeScriptAuthorFields(*el, *script);
    return el;
  }

  /// Emit a Script's captured author fields as <field name accessType type
  /// [value]/> children, and its inline source as the element's CDATA text. The
  /// author FieldInfos come from the DynamicFieldStore (effectiveFields minus the
  /// node's static table); each readable inputOutput/initializeOnly field emits
  /// its current boxed value, mirroring writeProtoFieldElement. An empty
  /// sourceCode leaves the element bodyless (a url-only Script is unchanged).
  void writeScriptAuthorFields(xml::Element &el, const Script &script) {
    const std::size_t staticCount = script.fields().size();
    FieldTable eff = runtime::effectiveFields(script);
    for (std::size_t i = staticCount; i < eff.size(); ++i) {
      const FieldInfo &f = eff[i];
      xml::Element *fe = el.addChild("field");
      fe->setAttr("name", f.x3dName);
      fe->setAttr("type", fieldTypeName(f.type));
      fe->setAttr("accessType", accessTypeName(f.access));
      // Re-emit a value only for fields that hold a persistent default and can
      // be read back (initializeOnly/inputOutput); input/outputOnly carry none.
      if (f.isReadable() && (f.access == AccessType::InitializeOnly ||
                             f.access == AccessType::InputOutput)) {
        std::any v = f.get(script);
        if (v.has_value())
          fe->setAttr("value", formatValue(f.type, v));
      }
    }
    const std::string src = script.getSourceCode();
    if (!src.empty())
      el.text = src;
  }

  /// Recurse an SFNode (single child) or MFNode (list) field into child els.
  /// `f.containerField` is the parent's slot name for this child. We always
  /// stamp the child with that slot as its containerField attribute so the
  /// reader can place it back unambiguously by matching attribute -> field,
  /// purely from reflection (no per-node default-container table needed).
  void writeNodeField(xml::Element &parent,
                      const std::shared_ptr<X3DNode> &owner,
                      const FieldInfo &f) {
    const std::string slot = f.containerField; // the parent field name
    std::any v = f.get(*owner);
    if (f.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
      if (!child)
        return;
      auto el = writeNodeElement(child, slot);
      if (el)
        parent.children.push_back(std::move(el));
    } else { // MFNode
      auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
      for (const auto &child : vec) {
        if (!child)
          continue;
        auto el = writeNodeElement(child, slot);
        if (el)
          parent.children.push_back(std::move(el));
      }
    }
  }

  /// Re-emit a captured <ProtoInstance> for an expanded primary node. Scalar
  /// fieldValue overrides become `<fieldValue name=... value=.../>`; node-valued
  /// overrides nest the child node(s). The scalar value's wire type is taken
  /// from the (extern)declaration interface (a ProtoFieldValue carries only a
  /// type-erased std::any); a value whose type is unknown is emitted nameless of
  /// value rather than mis-formatted.
  std::unique_ptr<xml::Element>
  writeProtoInstanceElement(const runtime::ProtoInstance &src,
                            const std::string &containerOverride) {
    auto el = std::make_unique<xml::Element>();
    el->name = "ProtoInstance";
    el->setAttr("name", src.name);
    if (!src.DEF.empty())
      el->setAttr("DEF", src.DEF);
    if (!containerOverride.empty())
      el->setAttr("containerField", containerOverride);
    else if (!src.containerField.empty() && src.containerField != "children")
      el->setAttr("containerField", src.containerField);

    for (const runtime::ProtoFieldValue &fv : src.fieldValues) {
      xml::Element *fe = el->addChild("fieldValue");
      fe->setAttr("name", fv.name);
      if (!fv.nodeValue.empty()) {
        for (const auto &child : fv.nodeValue) {
          auto ce = writeNodeElement(child, "");
          if (ce)
            fe->children.push_back(std::move(ce));
        }
      } else if (fv.value.has_value()) {
        if (const runtime::ProtoField *pf = interfaceField(src, fv.name))
          fe->setAttr("value", formatValue(pf->type, fv.value));
      }
    }
    return el;
  }

  /// Emit a <field name type accessType [value]> element; node-typed defaults
  /// nest their default child node(s).
  std::unique_ptr<xml::Element> writeProtoFieldElement(const runtime::ProtoField &f) {
    auto fe = std::make_unique<xml::Element>();
    fe->name = "field";
    fe->setAttr("name", f.name);
    fe->setAttr("type", fieldTypeName(f.type));
    fe->setAttr("accessType", accessTypeName(f.access));
    if (!f.nodeDefault.empty()) {
      for (const auto &n : f.nodeDefault) {
        auto ce = writeNodeElement(n, "");
        if (ce) fe->children.push_back(std::move(ce));
      }
    } else if (f.value.has_value() &&
               (f.access == AccessType::InitializeOnly ||
                f.access == AccessType::InputOutput)) {
      fe->setAttr("value", formatValue(f.type, f.value));
    }
    return fe;
  }

  std::unique_ptr<xml::Element>
  writeProtoDeclareElement(const runtime::ProtoDeclaration &d) {
    auto el = std::make_unique<xml::Element>();
    el->name = "ProtoDeclare";
    el->setAttr("name", d.name);
    if (!d.appinfo.empty()) el->setAttr("appinfo", d.appinfo);
    if (!d.documentation.empty()) el->setAttr("documentation", d.documentation);
    auto iface = std::make_unique<xml::Element>();
    iface->name = "ProtoInterface";
    for (const auto &f : d.interface)
      iface->children.push_back(writeProtoFieldElement(f));
    if (!iface->children.empty()) el->children.push_back(std::move(iface));

    auto body = std::make_unique<xml::Element>();
    body->name = "ProtoBody";
    // Re-emit the body TEMPLATE nodes (not expansion clones). Use a fresh writer
    // so the body's DEF/USE bookkeeping is independent of the surrounding scene's
    // seen_ set and expandedSources does not redirect the template's own nodes.
    XmlWriter bodyWriter;
    // PRF-2: hand the body's IS list to the body writer so writeNodeElement
    // attaches <IS> blocks at every depth during the recursive descent (the top
    // body node and any node nested inside it). No separate top-level attach.
    bodyWriter.bodyIsc_ = &d.body.isConnections;
    for (const auto &n : d.body.nodes) {
      auto ne = bodyWriter.writeNodeElement(n, "");
      if (!ne) continue;
      // Re-emit nested ProtoInstances placed under this body node (Case A).
      for (const auto &ni : d.body.nestedInstances) {
        if (ni.parent.lock().get() == n.get()) {
          auto ie = writeProtoInstanceElement(
              ni, ni.parentField.empty() ? std::string() : ni.parentField);
          if (ie) ne->children.push_back(std::move(ie));
        }
      }
      body->children.push_back(std::move(ne));
    }
    for (const auto &r : d.body.routes) {
      xml::Element *re = body->addChild("ROUTE");
      re->setAttr("fromNode", r.fromNode); re->setAttr("fromField", r.fromField);
      re->setAttr("toNode", r.toNode);     re->setAttr("toField", r.toField);
    }
    // Re-emit nested ProtoInstances that are direct ProtoBody children (Case B).
    for (const auto &ni : d.body.nestedInstances) {
      if (!ni.parent.lock()) {
        auto ie = writeProtoInstanceElement(ni, std::string());
        if (ie) body->children.push_back(std::move(ie));
      }
    }
    el->children.push_back(std::move(body));
    return el;
  }

  std::unique_ptr<xml::Element>
  writeExternProtoDeclareElement(const runtime::ExternProtoDeclaration &d) {
    auto el = std::make_unique<xml::Element>();
    el->name = "ExternProtoDeclare";
    el->setAttr("name", d.name);
    if (!d.appinfo.empty()) el->setAttr("appinfo", d.appinfo);
    if (!d.documentation.empty()) el->setAttr("documentation", d.documentation);
    if (!d.url.empty()) el->setAttr("url", fmtMFString(d.url));
    for (const auto &f : d.interface)
      el->children.push_back(writeProtoFieldElement(f));
    return el;
  }

  /// Append <IS><connect nodeField protoField/></IS> for every IsConnection
  /// whose body node matches `node` (by identity) onto its emitted element.
  static void attachIsBlocks(xml::Element &nodeEl,
                             const std::shared_ptr<X3DNode> &node,
                             const std::vector<runtime::IsConnection> &isc) {
    xml::Element *is = nullptr;
    for (const auto &c : isc) {
      if (c.node.get() != node.get()) continue;
      if (!is) is = nodeEl.addChild("IS");
      xml::Element *conn = is->addChild("connect");
      conn->setAttr("nodeField", c.nodeField);
      conn->setAttr("protoField", c.protoField);
    }
  }

  /// Look up a ProtoInstance's interface field by name in its resolved
  /// (extern)declaration, for the field's wire type. Null if not found.
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

  // ---- rendering ----
  static void indent(std::ostringstream &os, int depth) {
    for (int i = 0; i < depth; ++i)
      os << "  ";
  }

  void render(const xml::Element &el, std::ostringstream &os, int depth) {
    indent(os, depth);
    os << "<" << el.name;
    for (const auto &a : el.attributes) {
      os << " " << a.first << "=\"" << xml::escape(a.second) << "\"";
    }
    if (el.children.empty() && el.text.empty()) {
      os << "/>\n";
      return;
    }
    os << ">\n";
    for (const auto &c : el.children) {
      render(*c, os, depth + 1);
    }
    // Character content (e.g. a Script's inline source) is emitted as a CDATA
    // section so special characters need no escaping and the reader recovers it
    // verbatim from Element.text (SCR-SAI-DYN S1). Empty text is skipped above.
    if (!el.text.empty()) {
      indent(os, depth + 1);
      os << "<![CDATA[" << el.text << "]]>\n";
    }
    indent(os, depth);
    os << "</" << el.name << ">\n";
  }
};

} // namespace x3d::codec

#endif // X3D_XML_WRITER_HPP
