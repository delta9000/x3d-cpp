// XmlReader.hpp
// Node-agnostic X3D-XML parser: turns an X3D-XML string into the runtime
// document model (x3d::runtime::X3DDocument).
//
// Strategy (all driven by reflection + the node factory, no per-node code):
//   * Instantiate a node by element name via X3DNodeFactory::create().
//   * For each XML attribute, look up the matching FieldInfo by name and set the
//     value via its `set` thunk (enum fields via setEnumString, others via
//     parseValue + set). DEF/USE/containerField are handled specially.
//   * USE='name' resolves to the shared_ptr already registered under that DEF in
//     the scene's symbol table (identity sharing), and the element's own fields
//     are ignored (per X3D USE semantics).
//   * Child elements are routed into the parent's SFNode/MFNode field whose
//     containerField matches the child's containerField attribute (or, absent
//     the attribute, the field whose name matches the child element — falling
//     back to the parent's sole/first node field).
//   * <ROUTE>, <IMPORT>, <EXPORT> under <Scene> populate the Scene lists;
//     <head> children populate the document Head.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_XML_READER_HPP
#define X3D_XML_READER_HPP

#include "DynamicField.hpp"
#include "FieldValueIO.hpp"
#include "Script.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"
#include "XmlLite.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace x3d::codec {

/// Emit a non-fatal reader diagnostic. The XML path historically threw on any
/// surprise; under the project's "lenient read" policy a recovery must stay
/// visible rather than be silent, so we surface it on stderr. (A richer
/// per-document warning sink can later replace this without touching callers.)
inline void xmlReaderWarn(const std::string &msg) {
  std::cerr << "X3D XML reader warning: " << msg << "\n";
}

/// Parses X3D-XML into the runtime document model.
class XmlReader {
public:
  /// Parse a full X3D document string. Throws std::runtime_error on malformed
  /// XML. Unknown node/element names are skipped gracefully.
  runtime::X3DDocument readDocument(const std::string &xmlText) {
    auto root = xml::parse(xmlText);
    runtime::X3DDocument doc;
    if (!root)
      return doc;

    // Accept either <X3D> root or a bare <Scene> root.
    const xml::Element *x3d = root.get();
    if (x3d->name == "X3D") {
      if (const std::string *p = x3d->attr("profile"))
        doc.profile = runtime::profileFromString(*p);
      if (const std::string *v = x3d->attr("version"))
        doc.version = *v;
      for (const auto &child : x3d->children) {
        if (child->name == "head") {
          readHead(*child, doc.head);
        } else if (child->name == "Scene") {
          readScene(*child, doc.scene);
        }
      }
    } else if (x3d->name == "Scene") {
      readScene(*x3d, doc.scene);
    }
    doc.scene.resolveRoutes();
    return doc;
  }

  /// Parse a scene fragment (the children of a <Scene>) from an XML string.
  runtime::Scene readSceneString(const std::string &xmlText) {
    auto root = xml::parse(xmlText);
    runtime::Scene scene;
    if (root && root->name == "Scene")
      readScene(*root, scene);
    scene.resolveRoutes();
    return scene;
  }

private:
  void readHead(const xml::Element &head, runtime::Head &out) {
    for (const auto &c : head.children) {
      if (c->name == "component") {
        runtime::Component comp;
        comp.name = c->attrOr("name", "");
        comp.level = parseInt(c->attrOr("level", "1"));
        out.components.push_back(std::move(comp));
      } else if (c->name == "unit") {
        runtime::Unit u;
        u.category = c->attrOr("category", "");
        u.name = c->attrOr("name", "");
        u.conversionFactor = parseDouble(c->attrOr("conversionFactor", "1"));
        out.units.push_back(std::move(u));
      } else if (c->name == "meta") {
        runtime::Meta m;
        m.name = c->attrOr("name", "");
        m.content = c->attrOr("content", "");
        m.dir = c->attrOr("dir", "");
        m.httpEquiv = c->attrOr("http-equiv", "");
        m.lang = c->attrOr("lang", "");
        m.scheme = c->attrOr("scheme", "");
        out.meta.push_back(std::move(m));
      }
    }
  }

  void readScene(const xml::Element &scene, runtime::Scene &out) {
    for (const auto &c : scene.children) {
      if (c->name == "ROUTE") {
        out.routes.emplace_back(
            c->attrOr("fromNode", ""), c->attrOr("fromField", ""),
            c->attrOr("toNode", ""), c->attrOr("toField", ""));
      } else if (c->name == "IMPORT") {
        runtime::Import imp;
        imp.inlineDEF = c->attrOr("inlineDEF", "");
        imp.importedDEF = c->attrOr("importedDEF", "");
        imp.as = c->attrOr("AS", "");
        out.imports.push_back(std::move(imp));
      } else if (c->name == "EXPORT") {
        runtime::Export exp;
        exp.localDEF = c->attrOr("localDEF", "");
        exp.as = c->attrOr("AS", "");
        out.exports.push_back(std::move(exp));
      } else if (c->name == "ProtoDeclare") {
        readProtoDeclare(*c, out);
      } else if (c->name == "ExternProtoDeclare") {
        readExternProtoDeclare(*c, out);
      } else if (c->name == "ProtoInstance") {
        // A ProtoInstance directly under <Scene> is a root with no parent node.
        readProtoInstance(*c, out, /*parent=*/nullptr, /*parentSlot=*/"");
      } else {
        auto node = readNode(*c, out);
        if (node)
          out.addRootNode(node);
      }
    }
  }

  /// Build a node (and its subtree) from an element, using `scene` for the DEF
  /// symbol table so USE shares identity.
  std::shared_ptr<X3DNode> readNode(const xml::Element &el,
                                    runtime::Scene &scene,
                                    runtime::ProtoBody *currentProtoBody = nullptr) {
    // USE: resolve to the existing shared node; ignore this element's fields.
    if (const std::string *use = el.attr("USE")) {
      return scene.resolve(*use);
    }

    auto node = X3DNodeFactory::create(el.name);
    if (!node)
      return nullptr; // unknown node type: skip

    const FieldTable &table = node->fields();

    // Set value attributes.
    for (const auto &a : el.attributes) {
      const std::string &key = a.first;
      const std::string &val = a.second;
      if (key == "containerField")
        continue; // structural, consumed by the parent
      if (key == "USE")
        continue; // handled above
      const FieldInfo *f = findField(table, key);
      if (!f)
        continue; // unknown attribute: ignore
      applyAttribute(*node, *f, val);
    }

    // Register DEF before recursing so a USE inside the subtree can resolve to
    // this node (and so addRootNode's auto-registration is consistent).
    const std::string def = node->getDEF();
    if (!def.empty())
      scene.define(def, node);

    // PRF-2: inside a ProtoBody, record any <IS><connect/></IS> block carried by
    // THIS node element (at any depth) as an IsConnection bound to the just-built
    // node. Threading collection through readNode (rather than only scanning the
    // top body node in readProtoBody) makes IS re-bind correctly for a node
    // nested arbitrarily deep inside a body node.
    if (currentProtoBody)
      collectIsConnections(el, node, *currentProtoBody);

    // SCR-SAI-DYN (S1): a <Script> carries author <field> declarations and an
    // inline <![CDATA[...]]> source body. Author <field>s are not factory nodes,
    // so they bypass the node-child loop below; capture them into the per-node
    // DynamicFieldStore (so author-field ROUTEs and SAI get/set resolve via
    // effectiveFields) and the CDATA body into Script.sourceCode. An inline
    // ecmascript:/javascript:/vrmlscript: scheme in `url` (read above as an
    // attribute) is left intact so ScriptSystem's url decode still applies.
    if (auto *script = dynamic_cast<Script *>(node.get()))
      captureScriptAuthorFields(el, *node, *script);

    // Recurse children into node fields by containerField.
    for (const auto &childEl : el.children) {
      // Author <field> declarations on a Script are captured above (into the
      // DynamicFieldStore), not routed as node children.
      if (childEl->name == "field")
        continue;
      // PROTO statements nested inside a node: capture into the scene data
      // model rather than the parent's node fields. A <ProtoInstance> records
      // its placement (this node + the slot it would occupy) so a later
      // expansion pass can splice the expanded primary node back in.
      if (childEl->name == "ProtoInstance") {
        const std::string slot =
            childEl->attrOr("containerField", "children");
        readProtoInstance(*childEl, scene, node, slot, currentProtoBody);
        continue;
      }
      if (childEl->name == "ProtoDeclare") {
        readProtoDeclare(*childEl, scene);
        continue;
      }
      if (childEl->name == "ExternProtoDeclare") {
        readExternProtoDeclare(*childEl, scene);
        continue;
      }
      // ProtoInstance/Script/etc. that are not factory-known are skipped.
      auto child = readNode(*childEl, scene, currentProtoBody);
      if (!child)
        continue;
      // The element's explicit containerField wins; absent it, fall back to the
      // CHILD node's own default containerField (e.g. Contour2D ->
      // trimmingContour, Appearance -> appearance) so a no-containerField child
      // routes to its correct slot rather than the parent's first node field.
      const std::string slot =
          childEl->attrOr("containerField", child->defaultContainerField());
      attachChild(*node, table, slot, childEl->name, child, &scene);
    }

    return node;
  }

  // -------------------------------------------------------------------------
  // Script author-field capture (SCR-SAI-DYN S1).
  // -------------------------------------------------------------------------

  /// Capture a <Script> element's author <field name accessType type value>
  /// children into the per-node DynamicFieldStore, and its <![CDATA[...]]> body
  /// (already collected into Element.text by XmlLite) into Script.sourceCode.
  /// Each author decl flows through the neutral AuthorFieldDecl seam: the boxed
  /// default goes into initialValue for initializeOnly/inputOutput (empty for
  /// input/outputOnly, which carry no persistent value), mirroring the store's
  /// own seeding contract. The CDATA body is set as sourceCode only when present
  /// (an empty body leaves the slot untouched so a url-only Script is unchanged).
  static void captureScriptAuthorFields(const xml::Element &el, X3DNode &node,
                                        Script &script) {
    std::vector<runtime::AuthorFieldDecl> decls;
    for (const auto &c : el.children) {
      if (c->name != "field")
        continue;
      runtime::AuthorFieldDecl d;
      d.x3dName = c->attrOr("name", "");
      if (d.x3dName.empty())
        continue; // a nameless field is unaddressable; skip leniently
      d.type = mapFieldType(c->attrOr("type", "SFString"));
      d.access = mapAccessType(c->attrOr("accessType", "inputOutput"));
      // Box the default only for fields that hold a persistent value.
      if ((d.access == AccessType::InitializeOnly ||
           d.access == AccessType::InputOutput)) {
        if (const std::string *v = c->attr("value"))
          d.initialValue = parseValue(d.type, *v);
      }
      decls.push_back(std::move(d));
    }
    if (!decls.empty())
      runtime::dynamicFieldStore().addAuthorFields(node, decls);

    // CDATA body -> sourceCode, stored VERBATIM. The XML writer emits
    // sourceCode inside <![CDATA[...]]> with the surrounding indentation OUTSIDE
    // the brackets (XmlWriter::render), the JSON codec is a faithful
    // split/join, and the ClassicVRML decode keeps leading/trailing whitespace
    // (genuine author bytes). Trimming here made the XML reader the lone lossy,
    // asymmetric site, corrupting round-trips (Script.sourceCode mismatch in the
    // cli-gate). ScriptSystem tolerates leading/trailing whitespace: scriptSource
    // passes sourceCode to the backend as-is and leading/trailing whitespace is
    // benign to ECMAScript, while inline-url scheme stripping happens on the url
    // path (decodeInlineSource), not here. So store the bytes unmodified.
    if (!el.text.empty())
      script.setSourceCode(el.text);
  }

  /// Trim ASCII whitespace from both ends of a string.
  static std::string trim(const std::string &s) {
    std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
      return {};
    std::size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
  }

  // -------------------------------------------------------------------------
  // PROTO family capture (ProtoDeclare / ExternProtoDeclare / ProtoInstance).
  // These weave into the same node-reading recursion as ordinary elements:
  // body nodes and node-typed defaults/values go through readNode() so there
  // is no parallel node parser. All capture is LENIENT — a malformed or partial
  // PROTO element captures what it can and skips the rest, never throwing.
  // -------------------------------------------------------------------------

  /// <ProtoDeclare name>: interface (<ProtoInterface>) + body (<ProtoBody>).
  void readProtoDeclare(const xml::Element &el, runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ProtoDeclaration>();
    decl->name = el.attrOr("name", "");
    decl->appinfo = el.attrOr("appinfo", "");
    decl->documentation = el.attrOr("documentation", "");
    for (const auto &c : el.children) {
      if (c->name == "ProtoInterface")
        readProtoInterface(*c, scene, decl->interface);
      else if (c->name == "ProtoBody")
        readProtoBody(*c, scene, decl->body);
    }
    scene.protoDeclarations.push_back(std::move(decl));
  }

  /// <ExternProtoDeclare name url>: interface <field>s + split MFString url.
  void readExternProtoDeclare(const xml::Element &el, runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ExternProtoDeclaration>();
    decl->name = el.attrOr("name", "");
    decl->appinfo = el.attrOr("appinfo", "");
    decl->documentation = el.attrOr("documentation", "");
    if (const std::string *u = el.attr("url"))
      decl->url = parseMFString(*u);
    // Extern interface <field>s carry no defaults, but reading them through the
    // shared path is harmless (no value attr / node child => no default).
    readInterfaceFields(el, scene, decl->interface);
    scene.externProtoDeclarations.push_back(std::move(decl));
  }

  /// <ProtoInterface>: a sequence of <field name type accessType value/>.
  void readProtoInterface(const xml::Element &el, runtime::Scene &scene,
                          std::vector<runtime::ProtoField> &out) {
    readInterfaceFields(el, scene, out);
  }

  /// Read every direct <field> child of `el` into `out` (shared by Proto and
  /// ExternProto). For scalar fields a `value` attribute is boxed via
  /// parseValue(type, value); for SF/MFNode fields any child node elements are
  /// read into nodeDefault.
  void readInterfaceFields(const xml::Element &el, runtime::Scene &scene,
                           std::vector<runtime::ProtoField> &out) {
    for (const auto &c : el.children) {
      if (c->name != "field")
        continue;
      runtime::ProtoField f;
      f.name = c->attrOr("name", "");
      f.type = mapFieldType(c->attrOr("type", "SFString"));
      f.access = mapAccessType(c->attrOr("accessType", "inputOutput"));
      if (f.type == X3DFieldType::SFNode || f.type == X3DFieldType::MFNode) {
        // Node-typed default: child element(s) are the default node(s).
        for (const auto &nc : c->children) {
          if (auto n = readNode(*nc, scene))
            f.nodeDefault.push_back(n);
        }
      } else if (const std::string *v = c->attr("value")) {
        f.value = parseValue(f.type, *v);
      }
      out.push_back(std::move(f));
    }
  }

  /// <ProtoBody>: child nodes into body.nodes, <ROUTE> into body.routes, and
  /// each body node's <IS><connect/></IS> into body.isConnections.
  void readProtoBody(const xml::Element &el, runtime::Scene &scene,
                     runtime::ProtoBody &out) {
    // AUD-C: a PROTO body is its OWN DEF scope. Parse body nodes into a LOCAL
    // scene so body DEFs (and the USE that resolve to them) stay body-scoped and
    // do NOT leak into the enclosing scene's DEF table (which inflated defCount on
    // round-trip and diverged from the VRML reader, which already does this).
    // Previously-declared protos are visible to nested body content, so copy the
    // declaration tables in (mirrors ClassicVrmlReader::parseProto).
    runtime::Scene local;
    local.protoDeclarations = scene.protoDeclarations;
    local.externProtoDeclarations = scene.externProtoDeclarations;
    for (const auto &c : el.children) {
      if (c->name == "ROUTE") {
        out.routes.emplace_back(
            c->attrOr("fromNode", ""), c->attrOr("fromField", ""),
            c->attrOr("toNode", ""), c->attrOr("toField", ""));
        continue;
      }
      if (c->name == "IS") {
        // A top-level <IS> in the body (no enclosing node) has nothing to bind
        // to; ignore leniently.
        continue;
      }
      // A body node may itself be a ProtoInstance; record it on the body (out)
      // with no parent so it is retained (Case B/C in the engine).
      if (c->name == "ProtoInstance") {
        readProtoInstance(*c, local, nullptr, "", &out);
        continue;
      }
      auto node = readNode(*c, local, &out);
      if (!node)
        continue;
      out.nodes.push_back(node);
      // IS collection is handled inside readNode (threaded via currentProtoBody)
      // so it applies at every depth, not just to this top body node. See PRF-2.
    }
  }

  /// Record <IS><connect nodeField='X' protoField='Y'/></IS> children of a body
  /// node element as IsConnection{node, X, Y}.
  void collectIsConnections(const xml::Element &nodeEl,
                            const std::shared_ptr<X3DNode> &node,
                            runtime::ProtoBody &out) {
    for (const auto &c : nodeEl.children) {
      if (c->name != "IS")
        continue;
      for (const auto &conn : c->children) {
        if (conn->name != "connect")
          continue;
        runtime::IsConnection ic;
        ic.node = node;
        ic.nodeField = conn->attrOr("nodeField", "");
        ic.protoField = conn->attrOr("protoField", "");
        out.isConnections.push_back(std::move(ic));
      }
    }
  }

  /// <ProtoInstance name DEF USE containerField> with <fieldValue> children.
  /// `parent` is the node this element is nested under (null at Scene root);
  /// `parentSlot` is the containerField slot it would occupy on that parent.
  void readProtoInstance(const xml::Element &el, runtime::Scene &scene,
                         const std::shared_ptr<X3DNode> &parent,
                         const std::string &parentSlot,
                         runtime::ProtoBody *body = nullptr) {
    runtime::ProtoInstance inst;
    inst.name = el.attrOr("name", "");
    inst.DEF = el.attrOr("DEF", "");
    inst.USE = el.attrOr("USE", "");
    inst.containerField = el.attrOr("containerField", "children");
    if (parent)
      inst.parent = parent;
    inst.parentField = parentSlot.empty() ? inst.containerField : parentSlot;
    // Resolve the declaration if already known so <fieldValue> types can be
    // recovered from the interface.
    if (auto d = scene.findProto(inst.name))
      inst.declaration = d;
    else
      inst.externDeclaration = findExternProtoDecl(scene, inst.name);

    for (const auto &c : el.children) {
      if (c->name == "IS") {
        for (const auto &conn : c->children) {
          if (conn->name != "connect")
            continue;
          runtime::ProtoInstanceIsConnection ic;
          ic.nodeField = conn->attrOr("nodeField", "");
          ic.protoField = conn->attrOr("protoField", "");
          inst.isConnections.push_back(std::move(ic));
        }
        continue;
      }
      if (c->name != "fieldValue")
        continue;
      runtime::ProtoFieldValue fv;
      fv.name = c->attrOr("name", "");
      // Node-typed override: child node element(s).
      bool hasNodeChild = false;
      const runtime::ProtoField *decl = findInterfaceField(inst, fv.name);
      bool nodeTyped = decl && (decl->type == X3DFieldType::SFNode ||
                                decl->type == X3DFieldType::MFNode);
      if (nodeTyped || (!c->attr("value") && !c->children.empty())) {
        for (const auto &nc : c->children) {
          if (auto n = readNode(*nc, scene)) {
            fv.nodeValue.push_back(n);
            hasNodeChild = true;
          }
        }
      }
      if (!hasNodeChild) {
        if (const std::string *v = c->attr("value")) {
          // <fieldValue> carries no declared type. If the declaration is known,
          // box the value with the interface field's type; otherwise store the
          // raw string (boxed as SFString). A later re-typing pass can refine
          // this once declarations are fully resolved.
          if (decl)
            fv.value = parseValue(decl->type, *v);
          else
            fv.value = std::any(*v);
        }
      }
      inst.fieldValues.push_back(std::move(fv));
    }
    if (body)
      body->nestedInstances.push_back(std::move(inst));
    else
      scene.protoInstances.push_back(std::move(inst));
  }

  static std::shared_ptr<runtime::ExternProtoDeclaration>
  findExternProtoDecl(const runtime::Scene &scene, const std::string &name) {
    for (const auto &e : scene.externProtoDeclarations)
      if (e && e->name == name)
        return e;
    return nullptr;
  }

  static const runtime::ProtoField *
  findInterfaceField(const runtime::ProtoInstance &inst,
                     const std::string &name) {
    auto search = [&](const std::vector<runtime::ProtoField> &iface)
        -> const runtime::ProtoField * {
      for (const auto &f : iface)
        if (f.name == name)
          return &f;
      return nullptr;
    };
    if (inst.declaration)
      return search(inst.declaration->interface);
    if (inst.externDeclaration)
      return search(inst.externDeclaration->interface);
    return nullptr;
  }

  /// Map an X3D field-type string (e.g. "SFVec3f") to X3DFieldType.
  static X3DFieldType mapFieldType(const std::string &w) {
    static const struct {
      const char *name;
      X3DFieldType type;
    } kMap[] = {
        {"SFBool", X3DFieldType::SFBool},
        {"SFColor", X3DFieldType::SFColor},
        {"SFColorRGBA", X3DFieldType::SFColorRGBA},
        {"SFDouble", X3DFieldType::SFDouble},
        {"SFFloat", X3DFieldType::SFFloat},
        {"SFImage", X3DFieldType::SFImage},
        {"SFInt32", X3DFieldType::SFInt32},
        {"SFMatrix3d", X3DFieldType::SFMatrix3d},
        {"SFMatrix3f", X3DFieldType::SFMatrix3f},
        {"SFMatrix4d", X3DFieldType::SFMatrix4d},
        {"SFMatrix4f", X3DFieldType::SFMatrix4f},
        {"SFNode", X3DFieldType::SFNode},
        {"SFRotation", X3DFieldType::SFRotation},
        {"SFString", X3DFieldType::SFString},
        {"SFTime", X3DFieldType::SFTime},
        {"SFVec2d", X3DFieldType::SFVec2d},
        {"SFVec2f", X3DFieldType::SFVec2f},
        {"SFVec3d", X3DFieldType::SFVec3d},
        {"SFVec3f", X3DFieldType::SFVec3f},
        {"SFVec4d", X3DFieldType::SFVec4d},
        {"SFVec4f", X3DFieldType::SFVec4f},
        {"MFBool", X3DFieldType::MFBool},
        {"MFColor", X3DFieldType::MFColor},
        {"MFColorRGBA", X3DFieldType::MFColorRGBA},
        {"MFDouble", X3DFieldType::MFDouble},
        {"MFFloat", X3DFieldType::MFFloat},
        {"MFImage", X3DFieldType::MFImage},
        {"MFInt32", X3DFieldType::MFInt32},
        {"MFMatrix3d", X3DFieldType::MFMatrix3d},
        {"MFMatrix3f", X3DFieldType::MFMatrix3f},
        {"MFMatrix4d", X3DFieldType::MFMatrix4d},
        {"MFMatrix4f", X3DFieldType::MFMatrix4f},
        {"MFNode", X3DFieldType::MFNode},
        {"MFRotation", X3DFieldType::MFRotation},
        {"MFString", X3DFieldType::MFString},
        {"MFTime", X3DFieldType::MFTime},
        {"MFVec2d", X3DFieldType::MFVec2d},
        {"MFVec2f", X3DFieldType::MFVec2f},
        {"MFVec3d", X3DFieldType::MFVec3d},
        {"MFVec3f", X3DFieldType::MFVec3f},
        {"MFVec4d", X3DFieldType::MFVec4d},
        {"MFVec4f", X3DFieldType::MFVec4f},
    };
    for (const auto &e : kMap)
      if (w == e.name)
        return e.type;
    return X3DFieldType::SFString; // unknown: harmless string fallback
  }

  /// Map an X3D accessType string to AccessType (X3D and VRML spellings).
  static AccessType mapAccessType(const std::string &w) {
    if (w == "inputOnly" || w == "eventIn")
      return AccessType::InputOnly;
    if (w == "outputOnly" || w == "eventOut")
      return AccessType::OutputOnly;
    if (w == "initializeOnly" || w == "field")
      return AccessType::InitializeOnly;
    return AccessType::InputOutput; // inputOutput / exposedField / default
  }

  /// Set one value attribute on a node via its FieldInfo thunks.
  static void applyAttribute(X3DNode &node, const FieldInfo &f,
                             const std::string &val) {
    if (f.isEnum()) {
      if (f.setEnumString)
        f.setEnumString(node, stripEnumQuotes(val)); // AUD-D
      return;
    }
    if (!f.isWritable())
      return; // read-only (outputOnly): skip; initializeOnly is now data-layer writable
    std::any v = parseValue(f.type, val);
    if (v.has_value())
      f.set(node, v);
  }

  /// Attach a child node into the correct SF/MFNode field of `parent`.
  /// Resolution order:
  ///   1) the field whose containerField == the child's containerField attr;
  ///   2) failing that (no attr), the field named after the child's slot;
  ///   3) failing that, the parent's first node field (best-effort).
  static void attachChild(X3DNode &parent, const FieldTable &table,
                          const std::string &slot,
                          const std::string & /*childTypeName*/,
                          const std::shared_ptr<X3DNode> &child,
                          runtime::Scene *scene = nullptr) {
    const FieldInfo *target = nullptr;
    if (!slot.empty()) {
      for (const FieldInfo &f : table) {
        if (f.isNode() && f.containerField == slot) {
          target = &f;
          break;
        }
      }
      // Some authors put the field NAME in containerField even when it differs
      // from the reflected containerField; fall back to a name match.
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
      // No usable containerField: default to "children" if present, else the
      // first writable node field.
      for (const FieldInfo &f : table) {
        if (f.isNode() && f.containerField == "children") {
          target = &f;
          break;
        }
      }
      if (!target) {
        // First *storage* node field: writable, and (for MFNode) also readable
        // so it can be appended to. This skips set-only InputOnly event sinks
        // (e.g. addChildren / addTrimmingContour), which are not storage and
        // whose empty getter would crash the MFNode append path below.
        for (const FieldInfo &f : table) {
          if (f.isNode() && f.isWritable() &&
              (f.type == X3DFieldType::SFNode || f.isReadable())) {
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
      // Appending requires reading the current vector first. A set-only field
      // (an InputOnly event sink such as addChildren / addTrimmingContour,
      // emitted with set but no get) has an empty `get` thunk; invoking it would
      // std::bad_function_call. This happens when a no-containerField child is
      // routed to such a sink by the fallback selector below. Skip it (warn) so
      // the document keeps parsing rather than crashing. The child is dropped
      // here; routing it to the semantically-correct storage field requires a
      // virtual default-containerField lookup and is tracked as follow-up.
      if (!target->isReadable()) {
        xmlReaderWarn("skipping child for set-only MFNode field '" +
                      target->x3dName + "' (no getter; cannot append)");
        return;
      }
      std::any cur = target->get(parent);
      auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(cur);
      vec.push_back(child);
      target->set(parent, std::any(vec));
    }

    if (scene)
      scene->recordChildField(&parent, target->x3dName);
  }

  static const FieldInfo *findField(const FieldTable &table,
                                    const std::string &name) {
    for (const FieldInfo &f : table) {
      if (f.x3dName == name)
        return &f;
    }
    return nullptr;
  }
};

} // namespace x3d::codec

#endif // X3D_XML_READER_HPP
