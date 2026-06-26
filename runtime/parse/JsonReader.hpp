// JsonReader.hpp
// Node-agnostic X3D-JSON parser: turns an X3D-JSON string into the runtime
// document model (x3d::runtime::X3DDocument). It is the inverse of JsonWriter
// and completes read/write symmetry for the .json encoding.
//
// It walks exactly the shape JsonWriter emits (Web3D X3D-JSON conventions):
//
//   { "X3D": { "@profile": "...", "@version": "...",
//              "head": { "component": [ {"@name","@level"} ],
//                        "meta":      [ {"@name","@content"} ],
//                        "unit":      [
//                        {"@category","@name","@conversionFactor"} ] },
//              "Scene": { "-children": [ <node>, ... ] } } }
//
// where each node object is  { "TypeName": { <members> } }  and members are:
//   * "@DEF" / "@USE"        — DEF/USE identity (USE shares the prior
//   shared_ptr);
//   * "@field": value        — a value field (scalar / array / string), mapped
//                              back to the X3D wire string parseValue()
//                              expects;
//   * "-containerField": [..] — an SF/MFNode child slot holding child node
//                              objects (the de-prefixed key is the
//                              containerField).
//   * "ROUTE": {...} or [..] — scene-scope event routes (under "Scene").
//
// Strategy (all driven by reflection + the node factory, no per-node code):
//   * Instantiate a node by its single object key via X3DNodeFactory::create.
//   * For each "@field" member, find the matching FieldInfo and set the value
//     via build::applyField (enum -> setEnumString, else parseValue + set),
//     after converting the JSON value to the encoding-independent wire string.
//   * "-childField" members route into the parent's SF/MFNode field via
//     build::attachChild using the de-prefixed key as the containerField slot.
//
// JSON value -> wire string conversion bypasses collectFieldValue (that is the
// VRML-token path); here JSON already gives structured values, so numbers are
// space-joined from their original lexemes, bools become true/false, SFString
// is taken verbatim, and MFString elements are re-quoted so parseMFString sees
// them.
//
// Mirrors XmlReader's tolerance: unknown node types / fields are skipped, never
// throwing (malformed JSON itself does throw, via JsonLite).
//
// Header-only, namespace x3d::codec.
#ifndef X3D_PARSE_JSON_READER_HPP
#define X3D_PARSE_JSON_READER_HPP

#include "DynamicField.hpp" // AuthorFieldDecl + dynamicFieldStore() (S1)
#include "FieldValueIO.hpp" // parseInt/parseDouble (x3d::codec)
#include "JsonLite.hpp"     // x3d::json::parse, Value
#include "NodeBuilder.hpp"  // build::beginNode/applyField/attachChild/etc.
#include "Script.hpp"       // Script::setSourceCode (inline source capture)
#include "X3DReader.hpp"
#include "X3DRuntime.hpp"

#include <memory>
#include <string>

namespace x3d::codec {

/// Parses X3D-JSON into the runtime document model. The inverse of JsonWriter.
class JsonReader : public X3DReader {
public:
  Encoding encoding() const override { return Encoding::JSON; }

  /// Parse a full X3D-JSON document string. Throws std::runtime_error on
  /// malformed JSON (via JsonLite). Unknown node/field names are skipped.
  runtime::X3DDocument readDocument(const std::string &text) override {
    auto root = json::parse(text);
    runtime::X3DDocument doc;
    if (!root || !root->isObject())
      return doc;

    // Accept either an "X3D" wrapper or a bare "Scene" object as the root.
    const json::Value *x3d = root->member("X3D");
    const json::Value *sceneObj = nullptr;
    if (x3d && x3d->isObject()) {
      if (const json::Value *p = x3d->member("@profile"); p && p->isString())
        doc.profile = runtime::profileFromString(p->str);
      if (const json::Value *v = x3d->member("@version"); v && v->isString())
        doc.version = v->str;
      if (const json::Value *h = x3d->member("head"); h && h->isObject())
        readHead(*h, doc.head);
      sceneObj = x3d->member("Scene");
    } else {
      sceneObj = root->member("Scene");
    }

    if (sceneObj && sceneObj->isObject())
      readScene(*sceneObj, doc.scene);

    doc.scene.resolveRoutes();
    return doc;
  }

private:
  // -------------------------------------------------------------------------
  // Head.
  // -------------------------------------------------------------------------
  void readHead(const json::Value &head, runtime::Head &out) {
    if (const json::Value *comps = head.member("component");
        comps && comps->isArray()) {
      for (const auto &c : comps->array) {
        if (!c || !c->isObject())
          continue;
        runtime::Component comp;
        comp.name = strMember(*c, "@name");
        comp.level = intMember(*c, "@level", 1);
        out.components.push_back(std::move(comp));
      }
    }
    if (const json::Value *units = head.member("unit");
        units && units->isArray()) {
      for (const auto &u : units->array) {
        if (!u || !u->isObject())
          continue;
        runtime::Unit unit;
        unit.category = strMember(*u, "@category");
        unit.name = strMember(*u, "@name");
        unit.conversionFactor = doubleMember(*u, "@conversionFactor", 1.0);
        out.units.push_back(std::move(unit));
      }
    }
    if (const json::Value *metas = head.member("meta");
        metas && metas->isArray()) {
      for (const auto &m : metas->array) {
        if (!m || !m->isObject())
          continue;
        runtime::Meta meta;
        meta.name = strMember(*m, "@name");
        meta.content = strMember(*m, "@content");
        meta.dir = strMember(*m, "@dir");
        meta.httpEquiv = strMember(*m, "@http-equiv");
        meta.lang = strMember(*m, "@lang");
        meta.scheme = strMember(*m, "@scheme");
        out.meta.push_back(std::move(meta));
      }
    }
  }

  // -------------------------------------------------------------------------
  // Scene.
  // -------------------------------------------------------------------------
  void readScene(const json::Value &scene, runtime::Scene &out) {
    // The Scene object carries a "-children" array of root nodes, plus possibly
    // "ROUTE" / "IMPORT" / "EXPORT" members at scene scope.
    if (const json::Value *children = scene.member("-children");
        children && children->isArray()) {
      for (const auto &c : children->array) {
        if (!c || !c->isObject())
          continue;
        // PROTO statements at scene scope: capture, don't pass to factory.
        if (tryReadProtoStatement(*c, out, nullptr, "", nullptr))
          continue;
        auto node = readNode(*c, out);
        if (node)
          out.addRootNode(node);
      }
    }
    readRoutes(scene.member("ROUTE"), out);
    readImports(scene.member("IMPORT"), out);
    readExports(scene.member("EXPORT"), out);
  }

  /// Fill a Route vector from a "ROUTE" member (single object or array).
  void readRoutesInto(const json::Value *v, std::vector<runtime::Route> &routes) {
    if (!v)
      return;
    auto one = [&](const json::Value &r) {
      if (!r.isObject())
        return;
      routes.emplace_back(
          strMember(r, "@fromNode"), strMember(r, "@fromField"),
          strMember(r, "@toNode"), strMember(r, "@toField"));
    };
    if (v->isArray()) {
      for (const auto &r : v->array)
        if (r)
          one(*r);
    } else {
      one(*v);
    }
  }

  void readRoutes(const json::Value *v, runtime::Scene &out) {
    readRoutesInto(v, out.routes);
  }

  void readImports(const json::Value *v, runtime::Scene &out) {
    if (!v)
      return;
    auto one = [&](const json::Value &im) {
      if (!im.isObject())
        return;
      runtime::Import imp;
      imp.inlineDEF = strMember(im, "@inlineDEF");
      imp.importedDEF = strMember(im, "@importedDEF");
      imp.as = strMember(im, "@AS");
      out.imports.push_back(std::move(imp));
    };
    if (v->isArray()) {
      for (const auto &r : v->array)
        if (r)
          one(*r);
    } else {
      one(*v);
    }
  }

  void readExports(const json::Value *v, runtime::Scene &out) {
    if (!v)
      return;
    auto one = [&](const json::Value &ex) {
      if (!ex.isObject())
        return;
      runtime::Export exp;
      exp.localDEF = strMember(ex, "@localDEF");
      exp.as = strMember(ex, "@AS");
      out.exports.push_back(std::move(exp));
    };
    if (v->isArray()) {
      for (const auto &r : v->array)
        if (r)
          one(*r);
    } else {
      one(*v);
    }
  }

  // -------------------------------------------------------------------------
  // Node.
  // -------------------------------------------------------------------------
  /// Build a node (and subtree) from a `{ "TypeName": { ... } }` wrapper
  /// object. `currentProtoBody` is non-null when reading inside a ProtoBody,
  /// and is threaded through to intercept nested PROTO statements.
  std::shared_ptr<X3DNode> readNode(const json::Value &wrapper,
                                    runtime::Scene &scene,
                                    runtime::ProtoBody *currentProtoBody = nullptr) {
    // The wrapper has exactly one member: TypeName -> body object.
    if (wrapper.object.empty())
      return nullptr;
    const std::string &typeName = wrapper.object.front().first;
    const json::Value *body = wrapper.object.front().second.get();
    if (!body || !body->isObject())
      return nullptr;

    // USE: resolve to the existing shared node; ignore this object's fields.
    if (const json::Value *use = body->member("@USE"); use && use->isString()) {
      return build::resolveUse(scene, use->str);
    }

    auto node = build::beginNode(typeName);
    if (!node)
      return nullptr; // unknown node type: skip

    // Pass 1: value "@field" members (and @DEF).
    for (const auto &kv : body->object) {
      const std::string &key = kv.first;
      const json::Value *val = kv.second.get();
      if (key.empty() || !val)
        continue;
      if (key[0] != '@')
        continue; // child slots handled in pass 2
      if (key == "@USE")
        continue;                          // handled above
      std::string x3dName = key.substr(1); // strip '@'
      applyJsonField(*node, x3dName, *val);
    }

    // Register DEF before recursing so a nested USE resolves to this node.
    const std::string def = node->getDEF();
    if (!def.empty())
      build::defineDef(scene, def, node);

    // PRF-1/PRF-2: inside a ProtoBody, record any "IS":{"connect":[...]} block
    // carried by THIS node wrapper (at any depth) as an IsConnection bound to
    // the just-built node — mirrors XmlReader threading collection through
    // readNode so a deeply nested body node's IS re-binds correctly.
    if (currentProtoBody)
      collectJsonIsConnections(wrapper, node, *currentProtoBody);

    // SCR-SAI-DYN (S1): a Script's author <field> declarations + inline source.
    // The "field" member array (un-prefixed, mirroring ProtoInterface's "field")
    // carries the decls; the "#sourceText" array (canonical X3D-JSON CDATA) or
    // an @url inline scheme carries the body. Both are no-ops on any other node.
    if (typeName == "Script")
      captureScriptInterface(*body, node, scene);

    // Pass 2: "-childField" members (arrays of child node objects).
    for (const auto &kv : body->object) {
      const std::string &key = kv.first;
      const json::Value *val = kv.second.get();
      if (key.empty() || !val)
        continue;
      if (key[0] != '-')
        continue;
      std::string slot = key.substr(1); // strip '-'
      attachChildren(*node, slot, *val, scene, node, currentProtoBody);
    }

    return node;
  }

  /// Attach the child node objects under one "-slot" member into `node`.
  /// `parentNode` and `currentProtoBody` allow PROTO statement interception.
  void attachChildren(X3DNode &node, const std::string &slot,
                      const json::Value &val, runtime::Scene &scene,
                      const std::shared_ptr<X3DNode> &parentNode,
                      runtime::ProtoBody *currentProtoBody) {
    auto attachOne = [&](const json::Value &childWrapper) {
      if (!childWrapper.isObject())
        return;
      // PROTO statements nested in a node's child slot: capture, don't pass to
      // the factory. A ProtoInstance records its placement (parent + slot) so
      // the expansion engine can splice the expanded primary node back in.
      if (tryReadProtoStatement(childWrapper, scene, parentNode, slot,
                                currentProtoBody))
        return;
      auto child = readNode(childWrapper, scene, currentProtoBody);
      if (child)
        build::attachChild(node, slot, child, &scene);
    };
    if (val.isArray()) {
      for (const auto &c : val.array)
        if (c)
          attachOne(*c);
    } else if (val.isObject()) {
      // Tolerate a bare object (single SFNode child) even though JsonWriter
      // always wraps children in an array.
      attachOne(val);
    }
  }

  // -------------------------------------------------------------------------
  // PROTO capture (parity with XmlReader).
  // All capture is LENIENT — a malformed or partial PROTO element captures what
  // it can and skips the rest, never throwing.
  // -------------------------------------------------------------------------

  /// Returns true and captures if `wrapper` is a PROTO statement object
  /// ({ "ProtoDeclare"|"ExternProtoDeclare"|"ProtoInstance": {...} }).
  /// `parent`/`slot` give a nested instance's placement; `body` (non-null
  /// inside a ProtoBody) routes a captured instance into that body's
  /// nestedInstances.
  bool tryReadProtoStatement(const json::Value &wrapper, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot,
                             runtime::ProtoBody *body) {
    if (wrapper.object.empty())
      return false;
    const std::string &key = wrapper.object.front().first;
    const json::Value *obj = wrapper.object.front().second.get();
    if (!obj || !obj->isObject())
      return false;
    if (key == "ProtoDeclare") {
      readJsonProtoDeclare(*obj, scene);
      return true;
    }
    if (key == "ExternProtoDeclare") {
      readJsonExternProtoDeclare(*obj, scene);
      return true;
    }
    if (key == "ProtoInstance") {
      readJsonProtoInstance(*obj, scene, parent, slot, body);
      return true;
    }
    return false;
  }

  /// Read every "field" entry of `iface` into `out` (shared by ProtoDeclare
  /// and ExternProtoDeclare). `field` may be a single object or an array.
  void readJsonInterfaceFields(const json::Value &iface,
                               std::vector<runtime::ProtoField> &out,
                               runtime::Scene &scene) {
    const json::Value *fields = iface.member("field");
    if (!fields)
      return;
    auto one = [&](const json::Value &f) {
      if (!f.isObject())
        return;
      runtime::ProtoField pf;
      pf.name = strMember(f, "@name");
      pf.type = mapProtoFieldType(strMember(f, "@type"));
      pf.access = mapProtoAccessType(
          strMemberOr(f, "@accessType", "inputOutput"));
      if (pf.type == X3DFieldType::SFNode || pf.type == X3DFieldType::MFNode) {
        if (const json::Value *kids = f.member("-children");
            kids && kids->isArray()) {
          for (const auto &c : kids->array)
            if (c && c->isObject())
              if (auto n = readNode(*c, scene))
                pf.nodeDefault.push_back(n);
        }
      } else {
        if (const json::Value *v = f.member("@value"); v && v->isString())
          pf.value = parseValue(pf.type, v->str);
      }
      out.push_back(std::move(pf));
    };
    if (fields->isArray()) {
      for (const auto &f : fields->array)
        if (f)
          one(*f);
    } else {
      one(*fields);
    }
  }

  void readJsonProtoDeclare(const json::Value &obj, runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ProtoDeclaration>();
    decl->name = strMember(obj, "@name");
    decl->appinfo = strMember(obj, "@appinfo");
    decl->documentation = strMember(obj, "@documentation");
    if (const json::Value *iface = obj.member("ProtoInterface");
        iface && iface->isObject())
      readJsonInterfaceFields(*iface, decl->interface, scene);
    if (const json::Value *bodyObj = obj.member("ProtoBody");
        bodyObj && bodyObj->isObject())
      readJsonProtoBody(*bodyObj, scene, decl->body);
    scene.protoDeclarations.push_back(std::move(decl));
  }

  void readJsonExternProtoDeclare(const json::Value &obj,
                                  runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ExternProtoDeclaration>();
    decl->name = strMember(obj, "@name");
    decl->appinfo = strMember(obj, "@appinfo");
    decl->documentation = strMember(obj, "@documentation");
    // @url may be a lone JSON string OR (as the JsonWriter emits it) a JSON
    // array of strings. Mirror readRoutesInto's array/single leniency: collect
    // the elements verbatim into the url list. The XML path stores the same
    // list via parseMFString of the quoted MFString attribute.
    if (const json::Value *u = obj.member("@url")) {
      if (u->isArray()) {
        for (const auto &e : u->array)
          if (e && e->isString())
            decl->url.push_back(e->str);
      } else if (u->isString()) {
        decl->url = parseMFString(u->str);
      }
    }
    readJsonInterfaceFields(obj, decl->interface, scene);
    scene.externProtoDeclarations.push_back(std::move(decl));
  }

  /// Read a ProtoBody object: iterate "-children" (PROTO statements go into
  /// `out`, ordinary nodes into `out.nodes`); collect IS connections; collect
  /// any inline ROUTE statements into `out.routes`.
  void readJsonProtoBody(const json::Value &obj, runtime::Scene &scene,
                         runtime::ProtoBody &out) {
    // AUD-C: a PROTO body is its own DEF scope. Parse into a LOCAL scene so body
    // DEFs/USE stay body-scoped and do not leak into the enclosing scene's DEF
    // table (mirrors XmlReader::readProtoBody + ClassicVrmlReader::parseProto).
    runtime::Scene local;
    local.protoDeclarations = scene.protoDeclarations;
    local.externProtoDeclarations = scene.externProtoDeclarations;
    if (const json::Value *kids = obj.member("-children");
        kids && kids->isArray()) {
      for (const auto &c : kids->array) {
        if (!c || !c->isObject())
          continue;
        // A body child may itself be a PROTO statement (nested instance).
        if (tryReadProtoStatement(*c, local, nullptr, "", &out))
          continue;
        auto node = readNode(*c, local, &out);
        if (!node)
          continue;
        out.nodes.push_back(node);
        // IS collection for this body node (and any nested body node) is handled
        // inside readNode now (threaded via currentProtoBody), so deep IS blocks
        // are captured too — no separate top-level scan needed here.
      }
    }
    readRoutesInto(obj.member("ROUTE"), out.routes);
  }

  /// Record { "IS": { "connect": [ { "@nodeField", "@protoField" } ] } }
  /// found inside a body-node wrapper object.
  void collectJsonIsConnections(const json::Value &wrapper,
                                const std::shared_ptr<X3DNode> &node,
                                runtime::ProtoBody &out) {
    if (wrapper.object.empty())
      return;
    const json::Value *bodyObj = wrapper.object.front().second.get();
    if (!bodyObj || !bodyObj->isObject())
      return;
    const json::Value *is = bodyObj->member("IS");
    if (!is || !is->isObject())
      return;
    const json::Value *conns = is->member("connect");
    if (!conns)
      return;
    auto one = [&](const json::Value &c) {
      if (!c.isObject())
        return;
      out.isConnections.push_back(
          {node, strMember(c, "@nodeField"), strMember(c, "@protoField")});
    };
    if (conns->isArray()) {
      for (const auto &c : conns->array)
        if (c)
          one(*c);
    } else {
      one(*conns);
    }
  }

  // -------------------------------------------------------------------------
  // Script author interface (SCR-SAI-DYN S1).
  // -------------------------------------------------------------------------
  /// Capture a Script object's author <field> declarations + inline source.
  /// `node` is the freshly-built Script (its identity keys the DynamicFieldStore).
  /// Author decls live under the un-prefixed "field" member (mirroring
  /// ProtoInterface's "field"); inline source under "#sourceText" (canonical
  /// X3D-JSON CDATA, an array of lines) or — failing that — an @url inline
  /// scheme. Lenient: a malformed entry is skipped, never thrown.
  void captureScriptInterface(const json::Value &body,
                              const std::shared_ptr<X3DNode> &node,
                              runtime::Scene & /*scene*/) {
    // 1) Author field declarations.
    std::vector<runtime::AuthorFieldDecl> decls;
    if (const json::Value *fields = body.member("field")) {
      auto one = [&](const json::Value &f) {
        if (!f.isObject())
          return;
        const std::string name = strMember(f, "@name");
        if (name.empty())
          return;
        runtime::AuthorFieldDecl decl;
        decl.x3dName = name;
        decl.type = mapProtoFieldType(strMember(f, "@type"));
        decl.access =
            mapProtoAccessType(strMemberOr(f, "@accessType", "inputOutput"));
        // Box the default only for fields that carry a persistent value
        // (initializeOnly / inputOutput); inputOnly / outputOnly stay empty.
        if (decl.access == AccessType::InitializeOnly ||
            decl.access == AccessType::InputOutput) {
          // SF/MFNode author fields carry node defaults, not a wire value; we
          // leave initialValue empty for them (node-valued author fields are an
          // out-of-scope refinement). Everything else parses from "@value".
          if (decl.type != X3DFieldType::SFNode &&
              decl.type != X3DFieldType::MFNode) {
            if (const json::Value *v = f.member("@value"); v && v->isString())
              decl.initialValue = parseValue(decl.type, v->str);
          }
        }
        decls.push_back(std::move(decl));
      };
      if (fields->isArray()) {
        for (const auto &f : fields->array)
          if (f)
            one(*f);
      } else {
        one(*fields);
      }
    }
    if (!decls.empty())
      runtime::dynamicFieldStore().addAuthorFields(*node, decls);

    // 2) Inline source. "#sourceText" is an array of lines (the X3D-JSON CDATA
    // convention); join with newlines into Script.sourceCode. A lone string is
    // tolerated too.
    std::string source;
    if (const json::Value *st = body.member("#sourceText")) {
      if (st->isArray()) {
        bool first = true;
        for (const auto &line : st->array) {
          if (!line || !line->isString())
            continue;
          if (!first)
            source += '\n';
          first = false;
          source += line->str;
        }
      } else if (st->isString()) {
        source = st->str;
      }
    }
    // Fallback: an inline ecmascript:/javascript:/vrmlscript: scheme in @url
    // (the url field is already set on the node from applyJsonField).
    if (source.empty()) {
      if (auto *script = dynamic_cast<Script *>(node.get()))
        source = decodeInlineUrl(script->getUrl());
    }
    if (!source.empty()) {
      if (auto *script = dynamic_cast<Script *>(node.get()))
        script->setSourceCode(source);
    }
  }

  /// Return the body of the first ecmascript:/javascript:/vrmlscript: entry in
  /// `url`, or empty if none is an inline script. Mirrors
  /// ScriptSystem::decodeInlineSource so the reader can pre-stage sourceCode.
  static std::string decodeInlineUrl(const MFString &url) {
    static const char *kSchemes[] = {"ecmascript:", "javascript:",
                                     "vrmlscript:"};
    for (const std::string &entry : url) {
      for (const char *scheme : kSchemes) {
        const std::size_t n = std::char_traits<char>::length(scheme);
        if (entry.size() >= n && entry.compare(0, n, scheme) == 0)
          return entry.substr(n);
      }
    }
    return {};
  }

  /// Capture a ProtoInstance: name, DEF/USE, fieldValues (single-or-array).
  /// `parent`/`slot` give placement; `body` routes into body->nestedInstances.
  void readJsonProtoInstance(const json::Value &obj, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot,
                             runtime::ProtoBody *body) {
    x3d::runtime::ProtoInstance inst;
    inst.name = strMember(obj, "@name");
    inst.DEF = strMember(obj, "@DEF");
    inst.USE = strMember(obj, "@USE");
    inst.containerField = strMemberOr(obj, "@containerField", "children");
    if (parent)
      inst.parent = parent;
    inst.parentField = slot.empty() ? inst.containerField : slot;
    // Resolve declaration so fieldValue types can be recovered from the
    // interface.
    if (auto d = scene.findProto(inst.name))
      inst.declaration = d;
    else
      for (const auto &e : scene.externProtoDeclarations)
        if (e && e->name == inst.name) {
          inst.externDeclaration = e;
          break;
        }

    if (const json::Value *fvs = obj.member("fieldValue")) {
      auto one = [&](const json::Value &fv) {
        if (!fv.isObject())
          return;
        runtime::ProtoFieldValue pv;
        pv.name = strMember(fv, "@name");
        // Find the interface field for type information.
        const runtime::ProtoField *decl = nullptr;
        if (inst.declaration)
          for (const auto &f : inst.declaration->interface)
            if (f.name == pv.name) { decl = &f; break; }
        if (!decl && inst.externDeclaration)
          for (const auto &f : inst.externDeclaration->interface)
            if (f.name == pv.name) { decl = &f; break; }
        bool nodeTyped = decl && (decl->type == X3DFieldType::SFNode ||
                                  decl->type == X3DFieldType::MFNode);
        // Node-typed override: child node element(s) in "-children".
        if (const json::Value *kids = fv.member("-children");
            (nodeTyped || !fv.member("@value")) && kids && kids->isArray()) {
          for (const auto &c : kids->array)
            if (c && c->isObject())
              if (auto n = readNode(*c, scene))
                pv.nodeValue.push_back(n);
        }
        if (pv.nodeValue.empty()) {
          if (const json::Value *v = fv.member("@value"); v && v->isString())
            pv.value = decl ? parseValue(decl->type, v->str)
                            : std::any(v->str);
        }
        inst.fieldValues.push_back(std::move(pv));
      };
      if (fvs->isArray()) {
        for (const auto &fv : fvs->array)
          if (fv)
            one(*fv);
      } else {
        one(*fvs);
      }
    }

    if (const json::Value *is = obj.member("IS"); is && is->isObject()) {
      if (const json::Value *conns = is->member("connect")) {
        auto one = [&](const json::Value &c) {
          if (!c.isObject())
            return;
          runtime::ProtoInstanceIsConnection ic;
          ic.nodeField = strMember(c, "@nodeField");
          ic.protoField = strMember(c, "@protoField");
          inst.isConnections.push_back(std::move(ic));
        };
        if (conns->isArray()) {
          for (const auto &c : conns->array)
            if (c)
              one(*c);
        } else {
          one(*conns);
        }
      }
    }

    if (body)
      body->nestedInstances.push_back(std::move(inst));
    else
      scene.protoInstances.push_back(std::move(inst));
  }

  /// Map an X3D field-type string (e.g. "SFVec3f") to X3DFieldType.
  static X3DFieldType mapProtoFieldType(const std::string &w) {
    static const struct { const char *name; X3DFieldType type; } kMap[] = {
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
  static AccessType mapProtoAccessType(const std::string &w) {
    if (w == "inputOnly" || w == "eventIn")
      return AccessType::InputOnly;
    if (w == "outputOnly" || w == "eventOut")
      return AccessType::OutputOnly;
    if (w == "initializeOnly" || w == "field")
      return AccessType::InitializeOnly;
    return AccessType::InputOutput; // inputOutput / exposedField / default
  }

  // -------------------------------------------------------------------------
  // Field value application.
  // -------------------------------------------------------------------------
  /// Apply one "@field" member: find the FieldInfo, convert the JSON value to
  /// the X3D wire string, and set it via build::applyField. Unknown fields are
  /// skipped. DEF (a normal SFString field on every node) flows through here.
  void applyJsonField(X3DNode &node, const std::string &x3dName,
                      const json::Value &val) {
    const FieldInfo *f = build::findField(node.fields(), x3dName);
    if (!f)
      return; // unknown field: ignore
    std::string wire = jsonToWire(val, f->type);
    build::applyField(node, x3dName, wire);
  }

  /// Convert a parsed JSON value to the space-/quote-delimited X3D wire string
  /// that FieldValueIO::parseValue expects for the given field type.
  static std::string jsonToWire(const json::Value &val, X3DFieldType type) {
    // SFString: take the string verbatim (parseValue(SFString) is identity).
    if (type == X3DFieldType::SFString) {
      if (val.isString())
        return val.str;
      return scalarToken(val);
    }
    // MFString: a JSON array of strings (or a lone string) -> re-quoted list so
    // parseMFString preserves each element and any embedded quotes/backslashes.
    if (type == X3DFieldType::MFString) {
      std::string out;
      auto emit = [&](const json::Value &e) {
        if (!out.empty())
          out += ' ';
        out += requoteString(e.isString() ? e.str : scalarToken(e));
      };
      if (val.isArray()) {
        for (const auto &e : val.array)
          if (e)
            emit(*e);
      } else {
        emit(val);
      }
      return out;
    }

    // Everything else (scalars, vectors, colors, rotations, numeric MF, enum
    // tokens): space-join the JSON scalar(s). Arrays flatten one level (the
    // JsonWriter never nests numeric arrays).
    if (val.isArray()) {
      std::string out;
      for (const auto &e : val.array) {
        if (!e)
          continue;
        if (!out.empty())
          out += ' ';
        out += scalarToken(*e);
      }
      return out;
    }
    return scalarToken(val);
  }

  /// One JSON scalar -> its wire token. Numbers use their original lexeme so an
  /// integer stays integral; bools become true/false; strings pass through (an
  /// enum token emitted by JsonWriter as a JSON string lands here).
  static std::string scalarToken(const json::Value &v) {
    switch (v.type) {
    case json::Type::Number:
      return v.numberLexeme;
    case json::Type::Bool:
      return v.boolean ? "true" : "false";
    case json::Type::String:
      return v.str;
    case json::Type::Null:
      return "";
    default:
      return "";
    }
  }

  /// Re-quote a string for parseMFString, escaping embedded quote/backslash.
  static std::string requoteString(const std::string &s) {
    std::string out = "\"";
    for (char c : s) {
      if (c == '"' || c == '\\')
        out += '\\';
      out += c;
    }
    out += '"';
    return out;
  }

  // -------------------------------------------------------------------------
  // Small member accessors.
  // -------------------------------------------------------------------------
  static std::string strMemberOr(const json::Value &obj, const std::string &key,
                                 const std::string &fallback) {
    const json::Value *v = obj.member(key);
    if (!v)
      return fallback;
    if (v->isString())
      return v->str;
    const std::string s = scalarToken(*v);
    return s.empty() ? fallback : s;
  }

  static std::string strMember(const json::Value &obj, const std::string &key) {
    const json::Value *v = obj.member(key);
    if (!v)
      return "";
    if (v->isString())
      return v->str;
    return scalarToken(*v);
  }

  static int intMember(const json::Value &obj, const std::string &key,
                       int fallback) {
    const json::Value *v = obj.member(key);
    if (!v)
      return fallback;
    if (v->isNumber())
      return static_cast<int>(v->number);
    if (v->isString())
      return parseInt(v->str);
    return fallback;
  }

  static double doubleMember(const json::Value &obj, const std::string &key,
                             double fallback) {
    const json::Value *v = obj.member(key);
    if (!v)
      return fallback;
    if (v->isNumber())
      return v->number;
    if (v->isString())
      return parseDouble(v->str);
    return fallback;
  }
};

} // namespace x3d::codec

#endif // X3D_PARSE_JSON_READER_HPP
