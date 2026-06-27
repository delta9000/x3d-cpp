// ClassicVrmlReader.hpp
// Node-agnostic ClassicVRML (.x3dv) reader (ISO/IEC 19776-2). The text-encoding
// sibling of codecs/XmlReader and the inverse of codecs/VrmlWriter: it turns an
// X3D ClassicVRML document string into the runtime document model
// (x3d::runtime::X3DDocument).
//
// Like every other codec it contains ZERO per-node code: node creation goes
// through X3DNodeFactory (via build::beginNode), all field population through
// the reflection FieldTable (via build::applyField / collectFieldValue), and
// all wire-value interpretation through FieldValueIO::parseValue. The one
// divergence from XmlReader is token re-join: a field's value is a run of lexer
// tokens, not a single attribute string, so the value tokens are gathered by
// build::collectFieldValue before being handed to parseValue.
//
// Strategy (recursive descent over VrmlTokenizer with one-token lookahead):
//   * Header line `#X3D V<major>.<minor> <enc>` -> doc.version (encoding is
//     informational). A `#VRML V2.0` header is accepted best-effort.
//   * Header statements PROFILE / COMPONENT / UNIT / META populate doc + Head.
//   * Scene body: DEF/USE/<TypeName> nodes, ROUTE, PROTO/EXTERNPROTO,
//     IMPORT/EXPORT.
//   * Node body: a leading token that names a field on this node => field
//     statement (SFNode / MFNode / enum / value via reflection); an `IS` /
//     access-type interface declaration is captured/consumed; ROUTE/PROTO
//     recurse in proto-local scope.
//
// Tolerance matches XmlReader: unknown node types / unknown field names are
// skipped gracefully (balanced block / value consumed), never thrown. Genuinely
// unrecoverable malformation (unbalanced braces/brackets, unterminated string,
// IS outside a proto) throws std::runtime_error with line:col.
//
// Header-only, namespace x3d::codec. Implements the X3DReader front-end
// interface so X3DParse.hpp can dispatch a sniffed Encoding::ClassicVRML here.
#ifndef X3D_PARSE_CLASSIC_VRML_READER_HPP
#define X3D_PARSE_CLASSIC_VRML_READER_HPP

#include "DynamicField.hpp"  // S1: AuthorFieldDecl + dynamicFieldStore() (Task B)
#include "Encoding.hpp"
#include "FieldValueIO.hpp" // parseValue, parseInt, parseDouble (x3d::codec)
#include "NodeBuilder.hpp"  // build:: helpers (findField, applyField, ...)
#include "RecursionLimits.hpp" // SEC-1: kMaxNestingDepth (DoS guard)
#include "VrmlTokenizer.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DReader.hpp"
#include "X3DRuntime.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::codec {

/// Reads X3D ClassicVRML (.x3dv) into the runtime document model.
class ClassicVrmlReader : public X3DReader {
public:
  Encoding encoding() const override { return Encoding::ClassicVRML; }

  /// Parse a complete ClassicVRML document. Throws std::runtime_error only on
  /// unrecoverable malformation; unknown node/field names are skipped. The
  /// returned document already has scene.resolveRoutes() applied.
  runtime::X3DDocument readDocument(const std::string &text) override {
    runtime::X3DDocument doc;
    std::string_view src(text);

    // ----- Header line (`#X3D V<maj>.<min> <enc>` or `#VRML V2.0 utf8`). -----
    // VrmlTokenizer skips comment lines, including the header, so we read the
    // version off the raw first line here and let the tokenizer skip it.
    parseHeaderLine(src, doc);

    // Dialect hook: validate / react to the header line (VRML97 rejects V1.0,
    // sets a default profile, etc.). Base accepts everything best-effort.
    onHeaderLine(src, doc);

    VrmlTokenizer tok(src, /*firstLineIsHeader=*/true);

    // ----- Header statements (PROFILE/COMPONENT/UNIT/META), any order. -----
    parseHeaderStatements(tok, doc);

    // ----- Scene body. -----
    parseSceneBody(tok, doc.scene, /*inProto=*/false);

    doc.scene.resolveRoutes();
    return doc;
  }

protected:
  // -------------------------------------------------------------------------
  // Dialect hooks. The base reader (ClassicVRML) is the identity dialect: node
  // and field names pass through unchanged, no warnings are collected, and any
  // header line is accepted. A subclass (Vrml97Reader) overrides these to add a
  // VRML97->X3D rename table, header validation, and a warnings() sink — so the
  // tokenizer/parser below are written ONCE and shared between encodings.
  // -------------------------------------------------------------------------

  /// Map a node type token to its X3D type name. Base: identity.
  virtual std::string mapNodeName(const std::string &token) const {
    return token;
  }

  /// Map a field-name token (on a node of `nodeType`) to its X3D field name.
  /// Base: identity.
  virtual std::string mapFieldName(const std::string & /*nodeType*/,
                                   const std::string &token) const {
    return token;
  }

  /// React to the raw first line. Base: accept anything (the version was
  /// already read by parseHeaderLine). A subclass may throw on an unsupported
  /// header or set document defaults.
  virtual void onHeaderLine(std::string_view /*src*/,
                            runtime::X3DDocument & /*doc*/) {}

  /// Non-fatal diagnostic sink. Base: discard (ClassicVRML is tolerant-silent,
  /// matching XmlReader). Vrml97Reader collects these into warnings().
  virtual void warn(const std::string & /*message*/) {}

  /// First physical line of `src` (BOM-stripped, sans trailing CR), or empty.
  static std::string_view firstLine(std::string_view src) {
    if (src.size() >= 3 && static_cast<unsigned char>(src[0]) == 0xEF &&
        static_cast<unsigned char>(src[1]) == 0xBB &&
        static_cast<unsigned char>(src[2]) == 0xBF)
      src.remove_prefix(3);
    std::size_t nl = src.find('\n');
    std::string_view line = src.substr(0, nl);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    return line;
  }

private:
  std::size_t depth_ = 0; // SEC-1: node nesting depth (DoS guard).

  // -------------------------------------------------------------------------
  // Header line.
  // -------------------------------------------------------------------------
  static void parseHeaderLine(std::string_view src, runtime::X3DDocument &doc) {
    // Skip a leading UTF-8 BOM.
    if (src.size() >= 3 && static_cast<unsigned char>(src[0]) == 0xEF &&
        static_cast<unsigned char>(src[1]) == 0xBB &&
        static_cast<unsigned char>(src[2]) == 0xBF)
      src.remove_prefix(3);
    // Isolate the first physical line.
    std::size_t nl = src.find('\n');
    std::string_view line = src.substr(0, nl);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    if (line.empty() || line.front() != '#')
      return; // no header; leave doc.version at its default
    // Tokenize the header line on whitespace: `#X3D`, `V4.0`, `utf8`.
    std::vector<std::string> parts;
    std::string cur;
    for (char c : line) {
      if (c == ' ' || c == '\t') {
        if (!cur.empty()) {
          parts.push_back(cur);
          cur.clear();
        }
      } else {
        cur += c;
      }
    }
    if (!cur.empty())
      parts.push_back(cur);
    // A `#VRML`/`#VRML97` payload is VRML97 — floor to the X3D 3.0 baseline
    // (VP-2 §8). X3D has no V2.0/V1.0, so never let such a token reach a writer.
    if (!parts.empty() && (parts[0] == "#VRML" || parts[0] == "#VRML97")) {
      doc.version = "3.0";
      return;
    }
    if (parts.size() >= 2) {
      // parts[1] is like "V4.0" or "V2.0"; strip a leading 'V'/'v'.
      std::string ver = parts[1];
      if (!ver.empty() && (ver.front() == 'V' || ver.front() == 'v'))
        ver.erase(ver.begin());
      if (!ver.empty()) {
        // Clamp a sub-3.0 (legacy/VRML) major up to the 3.0 floor; leave >= 3.0
        // (incl. future versions we have no manifest for) untouched.
        int major = 0;
        try { major = std::stoi(ver.substr(0, ver.find('.'))); } catch (...) { major = 3; }
        doc.version = (major < 3) ? std::string("3.0") : ver;
      }
    }
  }

  // -------------------------------------------------------------------------
  // Header statements.
  // -------------------------------------------------------------------------
  static void parseHeaderStatements(VrmlTokenizer &tok,
                                    runtime::X3DDocument &doc) {
    for (;;) {
      const VrmlToken &t = tok.peek();
      if (t.atEnd())
        return;
      if (t.isWord("PROFILE")) {
        tok.next();
        std::string name = expectWord(tok, "PROFILE name");
        doc.profile = runtime::profileFromString(name);
      } else if (t.isWord("COMPONENT")) {
        tok.next();
        // `COMPONENT name:level` — the lexer keeps "name:level" as one token.
        std::string spec = expectWord(tok, "COMPONENT spec");
        runtime::Component comp;
        std::size_t colon = spec.find(':');
        if (colon == std::string::npos) {
          comp.name = spec;
          comp.level = 1;
        } else {
          comp.name = spec.substr(0, colon);
          comp.level = parseInt(spec.substr(colon + 1));
        }
        doc.head.components.push_back(std::move(comp));
      } else if (t.isWord("UNIT")) {
        tok.next();
        runtime::Unit u;
        u.category = expectWord(tok, "UNIT category");
        u.name = expectWord(tok, "UNIT name");
        u.conversionFactor = parseDouble(expectWord(tok, "UNIT factor"));
        doc.head.units.push_back(std::move(u));
      } else if (t.isWord("META")) {
        tok.next();
        runtime::Meta m;
        m.name = expectString(tok, "META name");
        m.content = expectString(tok, "META content");
        doc.head.meta.push_back(std::move(m));
      } else {
        return; // first non-header token: the scene body begins
      }
    }
  }

  // -------------------------------------------------------------------------
  // Scene body: a sequence of top-level statements until EOF (or, inside a
  // proto body, until the closing '}').
  // -------------------------------------------------------------------------
  // `currentProtoBody` is non-null only while parsing the body of a PROTO
  // declaration; it lets a `field IS protoField` statement deep in the body
  // record an IsConnection on the enclosing ProtoBody (Task 8). Root-level
  // statements at scene scope leave it null.
  void parseSceneBody(VrmlTokenizer &tok, runtime::Scene &scene, bool inProto,
                      runtime::ProtoBody *currentProtoBody = nullptr) {
    for (;;) {
      const VrmlToken &t = tok.peek();
      if (t.atEnd())
        return;
      if (inProto && t.isPunct('}'))
        return; // proto body terminator (consumed by caller)

      if (t.isWord("ROUTE")) {
        parseRoute(tok, scene);
      } else if (t.isWord("PROTO")) {
        parseProto(tok, scene);
      } else if (t.isWord("EXTERNPROTO")) {
        parseExternProto(tok, scene);
      } else if (t.isWord("IMPORT")) {
        parseImport(tok, scene);
      } else if (t.isWord("EXPORT")) {
        parseExport(tok, scene);
      } else {
        // A node statement (DEF/USE/<TypeName>). A proto-instance is detected
        // inside parseNode by checking the scene proto table first. At scene
        // root the parent is null (Scene root) and the parent field is empty.
        auto node = parseNode(tok, scene, currentProtoBody,
                              /*parentNode=*/nullptr, /*parentField=*/"");
        if (node)
          scene.addRootNode(node);
      }
    }
  }

  // -------------------------------------------------------------------------
  // Node production:
  //   node := "USE" Name
  //         | ["DEF" Name] TypeName "{" body "}"
  // -------------------------------------------------------------------------
  // `currentProtoBody` (non-null inside a PROTO body) threads through to the IS
  // site in parseNodeBody so `field IS protoField` records an IsConnection.
  // `parentNode` / `parentField` describe where a detected ProtoInstance sits
  // in the graph (null parent => Scene root) so its parent linkage is recorded.
  std::shared_ptr<X3DNode> parseNode(
      VrmlTokenizer &tok, runtime::Scene &scene,
      runtime::ProtoBody *currentProtoBody = nullptr,
      const std::shared_ptr<X3DNode> &parentNode = nullptr,
      const std::string &parentField = std::string()) {
    NestingGuard guard(depth_, "ClassicVRML"); // SEC-1: bound recursive node nesting.
    std::string def;
    if (tok.peek().isWord("USE")) {
      tok.next();
      std::string name = expectWord(tok, "USE name");
      return build::resolveUse(scene, name); // identity sharing; null tolerated
    }
    if (tok.peek().isWord("DEF")) {
      tok.next();
      def = expectWord(tok, "DEF name");
    }

    std::string rawTypeName = expectWord(tok, "node type name");

    // Proto-vs-node: a declared (or extern) proto instantiates a ProtoInstance,
    // not a factory node. Check the proto table BEFORE the factory (under the
    // raw name; proto names are user-defined and never go through the dialect).
    if (scene.findProto(rawTypeName) || findExternProto(scene, rawTypeName)) {
      // Proto instances are carried as data on the scene, not inserted into the
      // node graph (the model has no node wrapper for them yet). Parse and skip
      // the slot for the parent; the instance is recorded for round-tripping.
      // Record the just-appended instance's placement (parent node + the
      // containerField slot it filled) so PROTO expansion can splice it back.
      parseProtoInstance(tok, scene, def, rawTypeName);
      if (!scene.protoInstances.empty()) {
        scene.protoInstances.back().parent = parentNode;
        scene.protoInstances.back().parentField = parentField;
        if (currentProtoBody) {
          // Inside a PROTO body: this instance belongs to the body template, not
          // the flat scene list — move it so it expands per outer instantiation.
          currentProtoBody->nestedInstances.push_back(
              std::move(scene.protoInstances.back()));
          scene.protoInstances.pop_back();
        }
      }
      return nullptr;
    }

    // Apply the dialect node rename (identity for ClassicVRML) before the
    // factory lookup.
    std::string typeName = mapNodeName(rawTypeName);

    // Stray prose recovery: a KNOWN type name immediately FOLLOWED BY ANOTHER
    // WORD (never '{') is a word that merely happens to collide with a node type
    // (e.g. leaked from a misused comment block such as
    // ` * ... creates a Box with ...` — `Box` is a real type, `with` follows).
    // A real node value is always `[DEF name] Type {`. Gate tightly:
    //   * `def.empty()` — a preceding DEF is an unambiguous authoring signal of
    //     a real node, so `DEF n Type` with a missing '{' still throws below;
    //   * the next token is a bare Identifier word (prose continuation), NOT '{'
    //     (a real body) and NOT structural punctuation `[`/`]`/`}` (which would
    //     indicate a genuine — possibly binary/garbage — value/closer, left for
    //     the existing unknown-value / missing-'{' paths rather than diverted
    //     here). This keeps the genuine-node path and all currently-passing
    //     files on their exact existing code path.
    if (def.empty() && tok.peek().kind == VrmlToken::Kind::Identifier &&
        !tok.peek().isPunct('{')) {
      warn("ignored stray token '" + rawTypeName +
           "' (known type name not followed by '{') at line " +
           std::to_string(tok.peek().line));
      return nullptr;
    }

    auto node = build::beginNode(typeName);
    if (!node) {
      // Unknown node type (a forward/undeclared EXTERNPROTO name used as a
      // node, or a genuinely unknown type). If a `{` body follows, consume the
      // balanced block and skip it. If NO `{` follows (e.g. an EXTERNPROTO
      // whose declaration is mid-file, or a malformed body), warn and return
      // without consuming a block so the caller keeps going. If a value-shaped
      // token follows, skip the value instead.
      warn("unknown node '" + rawTypeName + "' at line " +
           std::to_string(tok.peek().line) + " (skipped)");
      if (tok.peek().isPunct('{'))
        skipBalancedBraceBlock(tok);
      else if (tok.peek().isPunct('[') ||
               tok.peek().kind == VrmlToken::Kind::Number ||
               tok.peek().kind == VrmlToken::Kind::String)
        skipFieldValue(tok);
      return nullptr;
    }

    if (!def.empty()) {
      node->setDEF(def);
      // Register DEF BEFORE recursing the body so a nested USE resolves to it.
      build::defineDef(scene, def, node);
    }

    expectPunct(tok, '{', "node body open");
    parseNodeBody(tok, scene, *node, typeName, currentProtoBody, node);
    // Normally the body is closed by its own '}'. If parseNodeBody returned on a
    // stray container-closer (a ']' belonging to an enclosing array, or an
    // over-count '}'), the body had a missing/swallowed '}': consume the '}' if
    // present, otherwise warn and leave the closer for the enclosing container
    // (the MFNode list's expectPunct(']'), or the parent body). Gate the
    // leniency on a stray-closer being ahead so a genuinely truncated document
    // (any other token / EOF) still throws via expectPunct.
    if (tok.peek().isPunct('}')) {
      tok.next();
    } else if (tok.peek().isPunct(']')) {
      warn("missing '}' closing " + typeName +
           " node body at line " + std::to_string(tok.peek().line) +
           " — recovered (enclosing container will close)");
    } else {
      expectPunct(tok, '}', "node body close");
    }
    // Task B: mirror a Script's inline `url ["ecmascript:..."]` body into its
    // sourceCode slot so the runtime has a uniform source path (Phase-1-D
    // prefers getSourceCode() over the url decode). Done through reflection so
    // the reader stays node-agnostic.
    if (typeName == "Script")
      mirrorScriptSource(*node);
    return node;
  }

  // Decode the first inline-scheme entry of a Script's `url` field and write the
  // decoded body into its `sourceCode` field — both reached via reflection.
  // No-op if the node lacks either field, the url has no inline entry, or
  // sourceCode is already set (an explicit `sourceCode` field wins).
  static void mirrorScriptSource(X3DNode &node) {
    const FieldTable &table = node.fields();
    const FieldInfo *urlF = build::findField(table, "url");
    const FieldInfo *srcF = build::findField(table, "sourceCode");
    if (!urlF || !srcF || !urlF->get || !srcF->set || !srcF->get)
      return;
    // Respect an explicit sourceCode value (do not overwrite).
    std::any srcAny = srcF->get(node);
    if (srcAny.has_value()) {
      const auto *cur = std::any_cast<SFString>(&srcAny);
      if (cur && !cur->empty())
        return;
    }
    std::any urlAny = urlF->get(node);
    const auto *url = std::any_cast<MFString>(&urlAny);
    if (!url)
      return;
    std::string source = decodeInlineScriptSource(*url);
    if (source.empty())
      return;
    srcF->set(node, std::any(SFString(source)));
  }

  // The first `ecmascript:`/`javascript:`/`vrmlscript:` entry's body (text after
  // the scheme prefix), or empty if none. Mirrors ScriptSystem's
  // decodeInlineSource so the captured sourceCode matches the runtime decode.
  static std::string decodeInlineScriptSource(const MFString &url) {
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

  // -------------------------------------------------------------------------
  // Node body: field / nested-node / interface-decl / IS / route statements.
  // -------------------------------------------------------------------------
  // `currentProtoBody` is non-null when this body sits inside a PROTO body: a
  // `field IS protoField` statement then records an IsConnection on it bound to
  // `nodeShared` (the shared_ptr of the node whose body this is). `nodeShared`
  // also serves as the parent for any ProtoInstance nested as a child here.
  void parseNodeBody(VrmlTokenizer &tok, runtime::Scene &scene, X3DNode &node,
                     const std::string &nodeType = std::string(),
                     runtime::ProtoBody *currentProtoBody = nullptr,
                     const std::shared_ptr<X3DNode> &nodeShared = nullptr) {
    const FieldTable &table = node.fields();
    for (;;) {
      const VrmlToken &t = tok.peek();
      if (t.atEnd())
        throw error(t, "unexpected EOF inside node body (missing '}')");
      if (t.isPunct('}'))
        return;

      // Stray container-closer where a field name is expected. This only ever
      // arises when an enclosing MF array / node body was left un-terminated
      // upstream (e.g. its intended `]`/`}` was swallowed by an inline `#`
      // comment, or a child node omitted its '}'). A bare ']' (or an over-count
      // '}', already handled above) is never a valid node-body statement head,
      // so treat the node as closed and let the enclosing container consume the
      // closer (parseNode's lenient body-close, or the MFNode list's ']'). Leave
      // the token UNCONSUMED so that resync.
      if (t.isPunct(']')) {
        warn("unexpected ']' inside " +
             (nodeType.empty() ? node.nodeTypeName() : nodeType) +
             " body at line " + std::to_string(t.line) +
             " — treating node as closed (missing '}' / un-terminated array)");
        return;
      }

      // ROUTE / PROTO inside a node body (proto-local; tolerated anywhere).
      if (t.isWord("ROUTE")) {
        parseRoute(tok, scene);
        continue;
      }
      if (t.isWord("PROTO")) {
        parseProto(tok, scene);
        continue;
      }
      if (t.isWord("EXTERNPROTO")) {
        parseExternProto(tok, scene);
        continue;
      }

      // Inline interface declaration (Script/ProtoBody fields):
      //   accessType FieldType fieldName [value]
      // On a Script node these are AUTHOR field declarations that must become
      // visible to the runtime (ROUTE wiring, SAI get/set): capture them into
      // the S1 DynamicFieldStore as AuthorFieldDecls. On any other node the
      // model has no slot for them, so they are consumed/skipped as before.
      if (isAccessTypeWord(t.text) && !t.isString) {
        consumeInterfaceDeclaration(tok, scene, node);
        continue;
      }

      // Otherwise the leading token is a field name. Apply the dialect rename
      // (identity for ClassicVRML; e.g. VRML97 LOD.level -> children) before
      // looking it up in the reflection table.
      int fieldLine = t.line;
      std::string rawName = expectWord(tok, "field name");
      std::string name = mapFieldName(nodeType, rawName);

      // `name IS protoField` — proto interface mapping. Inside a PROTO body
      // (currentProtoBody non-null) record an IsConnection binding this body
      // node's field to the named interface field; outside a proto body there
      // is nowhere to record it, so it is consumed (skipped) as before.
      if (tok.peek().isWord("IS")) {
        tok.next();
        std::string protoField = expectWord(tok, "IS proto-field name");
        if (currentProtoBody && nodeShared)
          currentProtoBody->isConnections.push_back(
              runtime::IsConnection{nodeShared, name, protoField});
        continue;
      }

      const FieldInfo *f = build::findField(table, name);
      if (!f) {
        // Unknown field: skip its value (a node, a bracketed list, or a run).
        warn("unknown field '" + rawName + "' on " +
             (nodeType.empty() ? node.nodeTypeName() : nodeType) + " at line " +
             std::to_string(fieldLine) + " (skipped)");
        skipFieldValue(tok);
        continue;
      }

      if (f->isNode()) {
        applyNodeField(tok, scene, node, *f, currentProtoBody, nodeShared);
        continue;
      }
      if (f->isEnum()) {
        // Enum: exactly one token, routed through setEnumString (never
        // parseValue). MFEnum bracketed lists are rare; collect a single token
        // unless a '[' run is present.
        applyEnumField(tok, node, *f);
        continue;
      }
      // Value field: gather the value tokens, hand the wire string to
      // parseValue + set (read-only fields skipped by applyField).
      std::string wire = build::collectFieldValue(tok, f->type);
      build::applyField(node, name, wire);
    }
  }

  // SFNode / MFNode field. `currentProtoBody` (non-null inside a PROTO body) and
  // `parentShared` (the shared_ptr of `parent`) thread on to parseNode so a
  // nested node's own IS statements are captured and a nested ProtoInstance
  // records its parent (`parentShared`) + the slot it fills (containerField).
  void applyNodeField(VrmlTokenizer &tok, runtime::Scene &scene,
                      X3DNode &parent, const FieldInfo &f,
                      runtime::ProtoBody *currentProtoBody = nullptr,
                      const std::shared_ptr<X3DNode> &parentShared = nullptr) {
    const std::string slot =
        f.containerField.empty() ? f.x3dName : f.containerField;
    if (f.type == X3DFieldType::SFNode) {
      // A bare child node, or `NULL`.
      if (tok.peek().isWord("NULL")) {
        tok.next();
        return;
      }
      // Tolerance: an older-encoding fixture may write an inline literal value
      // for what this (newer) model exposes as an SFNode field (e.g. X3D 3.x
      // NurbsPatchSurface.controlPoint as a raw `[ ... ]` coordinate list).
      // That is not a node; skip the value rather than mis-parsing it.
      if (tok.peek().isPunct('[') ||
          tok.peek().kind == VrmlToken::Kind::Number ||
          tok.peek().kind == VrmlToken::Kind::String) {
        skipFieldValue(tok);
        return;
      }
      // An inline EXTERNPROTO/PROTO declaration (or ROUTE) may precede the node
      // value in this field slot (corpus: `geometry EXTERNPROTO X[...][url] X
      // {...}`). Parse those declarations in place, then capture the node.
      if (!drainLeadingDeclarations(tok, scene))
        return;
      auto child =
          parseNode(tok, scene, currentProtoBody, parentShared, slot);
      if (child)
        build::attachChild(parent, slot, child, &scene);
      return;
    }
    // MFNode: `[ node* ]` (or, for resilience, a single bare node).
    if (tok.peek().isPunct('[')) {
      tok.next(); // consume '['
      while (!tok.atEnd() && !tok.peek().isPunct(']')) {
        if (tok.peek().isWord("NULL")) {
          tok.next();
          continue;
        }
        // Tolerate embedded statements (ROUTE/PROTO/...), a stray `}`, or a raw
        // scalar where a node type name is expected — skip-and-recover instead
        // of throwing at expectWord("node type name").
        if (skipNonNodeListItem(tok, scene))
          continue;
        auto child =
            parseNode(tok, scene, currentProtoBody, parentShared, slot);
        if (child)
          build::attachChild(parent, slot, child, &scene);
      }
      expectPunct(tok, ']', "MFNode list close");
    } else if (!tok.peek().isPunct('}') && !tok.peek().isPunct(']')) {
      // A single bare node value. Guard against a raw scalar / stray token and
      // drain any inline declarations preceding the node.
      if (!drainLeadingDeclarations(tok, scene))
        return;
      auto child =
          parseNode(tok, scene, currentProtoBody, parentShared, slot);
      if (child)
        build::attachChild(parent, slot, child, &scene);
    }
  }

  // Enum field: one token via setEnumString. A bracketed MFEnum run is joined.
  static void applyEnumField(VrmlTokenizer &tok, X3DNode &node,
                             const FieldInfo &f) {
    std::string wire;
    if (tok.peek().isPunct('[')) {
      tok.next();
      while (!tok.atEnd() && !tok.peek().isPunct(']')) {
        if (!wire.empty())
          wire += ' ';
        wire += tok.next().text;
      }
      if (tok.peek().isPunct(']'))
        tok.next();
    } else {
      wire = tok.next().text;
    }
    if (f.setEnumString)
      f.setEnumString(node, x3d::codec::stripEnumQuotes(wire)); // AUD-D
  }

  // -------------------------------------------------------------------------
  // ROUTE statement: ROUTE Name1.field1 TO Name2.field2
  // -------------------------------------------------------------------------
  void parseRoute(VrmlTokenizer &tok, runtime::Scene &scene) {
    tok.next(); // ROUTE
    std::string from = expectWord(tok, "ROUTE source endpoint");
    if (!tok.peek().isWord("TO"))
      throw error(tok.peek(), "ROUTE: expected 'TO'");
    tok.next(); // TO
    std::string to = expectWord(tok, "ROUTE target endpoint");

    auto split = [](const std::string &ep, std::string &n, std::string &fld) {
      std::size_t dot = ep.find('.');
      if (dot == std::string::npos) {
        n = ep;
        fld.clear();
      } else {
        n = ep.substr(0, dot);
        fld = ep.substr(dot + 1);
      }
    };
    std::string fromNode, fromField, toNode, toField;
    split(from, fromNode, fromField);
    split(to, toNode, toField);
    scene.routes.emplace_back(fromNode, fromField, toNode, toField);
  }

  // -------------------------------------------------------------------------
  // IMPORT / EXPORT. Two corpus shapes are accepted for each:
  //   * Single-line dot form (ISO/IEC 19776-2):
  //       IMPORT inlineDEF.importedDEF [AS localName]
  //       EXPORT localDEF [AS exportedName]
  //   * Non-standard block form (emitted by some X3D->ClassicVRML serializers,
  //     keys in any order; unknown keys ignored):
  //       IMPORT { inlineDEF Inl importedDEF Imported AS Local }
  //       EXPORT { localDEF Local AS Exported }
  // The block reader is deliberately tolerant: it populates whatever recognized
  // keys appear and never throws over an unexpected one. Both shapes record one
  // structural runtime::Import / runtime::Export so the document round-trips.
  // -------------------------------------------------------------------------
  void parseImport(VrmlTokenizer &tok, runtime::Scene &scene) {
    tok.next(); // IMPORT
    runtime::Import imp;
    if (tok.peek().isPunct('{')) {
      // Block form: `IMPORT { key value ... }`, keys in any order.
      parseImportExportBlock(
          tok, [&](const std::string &key, const std::string &value) {
            if (key == "inlineDEF")
              imp.inlineDEF = value;
            else if (key == "importedDEF")
              imp.importedDEF = value;
            else if (key == "AS" || key == "as")
              imp.as = value;
            // unknown key: ignore the value (already consumed)
          });
      scene.imports.push_back(std::move(imp));
      return;
    }
    std::string spec = expectWord(tok, "IMPORT inlineDEF.importedDEF");
    std::size_t dot = spec.find('.');
    if (dot == std::string::npos) {
      imp.inlineDEF = spec;
    } else {
      imp.inlineDEF = spec.substr(0, dot);
      imp.importedDEF = spec.substr(dot + 1);
    }
    if (tok.peek().isWord("AS")) {
      tok.next();
      imp.as = expectWord(tok, "IMPORT AS name");
    }
    // Defensive recovery: a malformed file may bleed an inline node body (`{`)
    // into the IMPORT slot. Skip it balanced rather than aborting the document;
    // the structural IMPORT is still recorded.
    if (tok.peek().isPunct('{')) {
      warn("unexpected '{' after IMPORT at line " +
           std::to_string(tok.peek().line) + " (skipped)");
      skipBalancedBraceBlock(tok);
    }
    scene.imports.push_back(std::move(imp));
  }

  void parseExport(VrmlTokenizer &tok, runtime::Scene &scene) {
    tok.next(); // EXPORT
    runtime::Export exp;
    if (tok.peek().isPunct('{')) {
      // Block form: `EXPORT { key value ... }`, keys in any order.
      parseImportExportBlock(
          tok, [&](const std::string &key, const std::string &value) {
            if (key == "localDEF")
              exp.localDEF = value;
            else if (key == "AS" || key == "as")
              exp.as = value;
            // unknown key: ignore the value (already consumed)
          });
      scene.exports.push_back(std::move(exp));
      return;
    }
    exp.localDEF = expectWord(tok, "EXPORT localDEF");
    if (tok.peek().isWord("AS")) {
      tok.next();
      exp.as = expectWord(tok, "EXPORT AS name");
    }
    if (tok.peek().isPunct('{')) {
      warn("unexpected '{' after EXPORT at line " +
           std::to_string(tok.peek().line) + " (skipped)");
      skipBalancedBraceBlock(tok);
    }
    scene.exports.push_back(std::move(exp));
  }

  // Read a `{ (key value)* }` block, invoking `sink(key, value)` for each pair.
  // Tolerant: a key with no following value (the closing `}` or EOF) is
  // ignored; the block is always closed balanced. Shared by the IMPORT/EXPORT
  // block forms.
  template <typename Sink>
  void parseImportExportBlock(VrmlTokenizer &tok, Sink &&sink) {
    expectPunct(tok, '{', "IMPORT/EXPORT block open");
    while (!tok.atEnd() && !tok.peek().isPunct('}')) {
      // A key must be a bare word; a stray `{`/`[`/value is skipped
      // defensively.
      if (tok.peek().kind != VrmlToken::Kind::Identifier) {
        warn("unexpected '" + tok.peek().text +
             "' in IMPORT/EXPORT block at line " +
             std::to_string(tok.peek().line) + " (skipped)");
        tok.next();
        continue;
      }
      std::string key = tok.next().text;
      if (tok.atEnd() || tok.peek().isPunct('}'))
        break; // dangling key with no value
      std::string value = tok.next().text;
      sink(key, value);
    }
    expectPunct(tok, '}', "IMPORT/EXPORT block close");
  }

  // -------------------------------------------------------------------------
  // PROTO / EXTERNPROTO declarations (structural; no expansion).
  //   PROTO Name "[" interface "]" "{" body "}"
  //   EXTERNPROTO Name "[" interface "]" urlList
  // -------------------------------------------------------------------------
  void parseProto(VrmlTokenizer &tok, runtime::Scene &scene) {
    tok.next(); // PROTO
    auto decl = std::make_shared<runtime::ProtoDeclaration>();
    decl->name = expectWord(tok, "PROTO name");
    expectPunct(tok, '[', "PROTO interface open");
    decl->interface = parseInterface(tok, /*allowDefaults=*/true);
    expectPunct(tok, ']', "PROTO interface close");
    expectPunct(tok, '{', "PROTO body open");
    // The proto body is a nested scene-body in a fresh DEF scope. Parse into a
    // local scene, then attach its nodes/routes to the declaration. Pass
    // `&decl->body` as the currentProtoBody so `field IS protoField` statements
    // deep in the body record IsConnections directly on it (Task 8).
    runtime::Scene local;
    // Inherit outer-scope proto declarations so a nested ProtoInstance can
    // look up a previously-declared proto (e.g. `Leaf` inside `Wrap`'s body).
    local.protoDeclarations = scene.protoDeclarations;
    local.externProtoDeclarations = scene.externProtoDeclarations;
    parseSceneBody(tok, local, /*inProto=*/true, &decl->body);
    expectPunct(tok, '}', "PROTO body close");
    decl->body.nodes = std::move(local.rootNodes);
    decl->body.routes = std::move(local.routes);
    scene.protoDeclarations.push_back(decl);
  }

  void parseExternProto(VrmlTokenizer &tok, runtime::Scene &scene) {
    tok.next(); // EXTERNPROTO
    auto decl = std::make_shared<runtime::ExternProtoDeclaration>();
    decl->name = expectWord(tok, "EXTERNPROTO name");
    expectPunct(tok, '[', "EXTERNPROTO interface open");
    decl->interface = parseInterface(tok, /*allowDefaults=*/false);
    expectPunct(tok, ']', "EXTERNPROTO interface close");
    // urlList: one quoted string or `[ "..." ... ]`.
    if (tok.peek().isPunct('[')) {
      tok.next();
      while (!tok.atEnd() && !tok.peek().isPunct(']'))
        decl->url.push_back(tok.next().text);
      expectPunct(tok, ']', "EXTERNPROTO url list close");
    } else {
      decl->url.push_back(expectString(tok, "EXTERNPROTO url"));
    }
    scene.externProtoDeclarations.push_back(decl);
  }

  // interface := ( accessType FieldType fieldName [defaultValue] )*
  std::vector<runtime::ProtoField> parseInterface(VrmlTokenizer &tok,
                                                  bool allowDefaults) {
    std::vector<runtime::ProtoField> out;
    while (!tok.atEnd() && !tok.peek().isPunct(']')) {
      std::string accessTok = expectWord(tok, "interface accessType");
      runtime::ProtoField field;
      field.access = accessTypeFromString(accessTok);
      std::string typeTok = expectWord(tok, "interface field type");
      field.type = fieldTypeFromString(typeTok);
      field.name = expectWord(tok, "interface field name");
      // A default value is present only for non-event fields
      // (field/exposedField i.e. initializeOnly/inputOutput). For node-typed
      // defaults, capture child nodes; for scalar defaults, parse a value.
      bool isEvent = field.access == AccessType::InputOnly ||
                     field.access == AccessType::OutputOnly;
      if (allowDefaults && !isEvent && hasDefaultValueAhead(tok)) {
        if (field.type == X3DFieldType::SFNode ||
            field.type == X3DFieldType::MFNode) {
          // SFNode default node / MFNode list. Parsed against a throwaway scope
          // (proto interface defaults do not share the document DEF table).
          captureNodeDefault(tok, field);
        } else if (field.type == X3DFieldType::SFEnum ||
                   field.type == X3DFieldType::MFEnum) {
          // Enum default is a bare token (no concrete enum type at this level).
          field.value = std::any(tok.next().text);
        } else {
          std::string wire = build::collectFieldValue(tok, field.type);
          field.value = parseValue(field.type, wire);
        }
      }
      out.push_back(std::move(field));
    }
    return out;
  }

  // Proto instance: `TypeName { (fieldName fieldValue)* }` recorded as data.
  void parseProtoInstance(VrmlTokenizer &tok, runtime::Scene &scene,
                          const std::string &def, const std::string &typeName) {
    runtime::ProtoInstance inst;
    inst.name = typeName;
    inst.DEF = def;
    if (auto d = scene.findProto(typeName))
      inst.declaration = d;
    else if (auto e = findExternProto(scene, typeName))
      inst.externDeclaration = e;

    expectPunct(tok, '{', "proto instance open");
    while (!tok.atEnd() && !tok.peek().isPunct('}')) {
      // Embedded statements (ROUTE/PROTO/...) and stray tokens (an extra `}`
      // from a malformed sibling, a leading value) may appear where a field
      // name is expected; skip-and-recover rather than throwing.
      if (skipNonNodeListItem(tok, scene))
        continue;
      // A field name must be a bare word; if it is punctuation (e.g. a stray
      // `[`), skipNonNodeListItem already consumed it, so anything left that is
      // not a word is skipped defensively.
      if (tok.peek().kind != VrmlToken::Kind::Identifier &&
          tok.peek().kind != VrmlToken::Kind::Number) {
        warn("unexpected '" + tok.peek().text +
             "' in proto instance body at line " +
             std::to_string(tok.peek().line) + " (skipped)");
        tok.next();
        continue;
      }
      std::string fname = tok.next().text;
      // `name IS protoField` inside a proto instance/body: capture-and-skip.
      if (tok.peek().isWord("IS")) {
        tok.next();
        runtime::ProtoInstanceIsConnection ic;
        ic.nodeField = fname;
        ic.protoField = expectWord(tok, "IS proto-field name");
        inst.isConnections.push_back(std::move(ic));
        continue;
      }
      runtime::ProtoFieldValue fv;
      fv.name = fname;
      // Determine the value shape from the declaration's interface if known;
      // otherwise gather conservatively (a node, a bracketed run, or one
      // token).
      const runtime::ProtoField *decl = findProtoField(inst, fname);
      if (decl && (decl->type == X3DFieldType::SFNode ||
                   decl->type == X3DFieldType::MFNode)) {
        captureInstanceNodeValue(tok, scene, fv, decl->type);
      } else if (decl && decl->type != X3DFieldType::SFEnum &&
                 decl->type != X3DFieldType::MFEnum) {
        std::string wire = build::collectFieldValue(tok, decl->type);
        fv.value = parseValue(decl->type, wire);
      } else if (nextIsNodeValue(tok)) {
        // Unknown (or enum) declaration but the value is plainly a node
        // (`DEF`/`USE`/`NULL`/`[`/`Type {`): capture it structurally as a node
        // value so the `{ ... }` body is consumed balanced instead of leaking a
        // stray `{` to the field-name position next iteration.
        captureInstanceNodeValue(tok, scene, fv, X3DFieldType::MFNode);
      } else {
        // Unknown declaration or enum scalar: store the raw token / list text.
        if (tok.peek().isPunct('[')) {
          tok.next();
          std::string raw;
          while (!tok.atEnd() && !tok.peek().isPunct(']')) {
            if (!raw.empty())
              raw += ' ';
            raw += tok.next().text;
          }
          if (tok.peek().isPunct(']'))
            tok.next();
          fv.value = std::any(raw);
        } else {
          fv.value = std::any(tok.next().text);
        }
      }
      inst.fieldValues.push_back(std::move(fv));
    }
    expectPunct(tok, '}', "proto instance close");
    scene.protoInstances.push_back(std::move(inst));
  }

  // -------------------------------------------------------------------------
  // Inline interface declaration inside a node body (Script user fields, etc.):
  //   accessType FieldType fieldName [ IS protoField | defaultValue ]
  // On a Script node (Task B) the declaration is captured into the S1
  // DynamicFieldStore as an AuthorFieldDecl so it becomes visible to the
  // runtime (ROUTE endpoint resolution, SAI get/set). The access keyword maps
  // per accessTypeFromString (field->initializeOnly, exposedField->inputOutput,
  // eventIn->inputOnly, eventOut->outputOnly); the boxed default seeds the
  // value store (empty for input/outputOnly). On a non-Script node (e.g. a
  // proto-body field that is not bound via IS) the model has no slot for it, so
  // it is consumed (and any default value / IS mapping skipped) as before —
  // matching the "carry structure, skip what the model can't hold" tolerance of
  // the codecs.
  // -------------------------------------------------------------------------
  void consumeInterfaceDeclaration(VrmlTokenizer &tok, runtime::Scene &scene,
                                   X3DNode &node) {
    std::string accessTok = expectWord(tok, "interface accessType");
    AccessType access = accessTypeFromString(accessTok);
    std::string typeTok = expectWord(tok, "interface field type");
    X3DFieldType type = fieldTypeFromString(typeTok);
    std::string fieldName = expectWord(tok, "interface field name");

    const bool captureAuthor = node.nodeTypeName() == "Script";

    // A Script/proto-body interface field may bind to a proto interface field
    // with `IS protoField` in place of a literal default (e.g.
    // `field SFTime duration IS duration`). Consume that mapping and stop — no
    // default value follows. An IS-bound author field carries no own default,
    // so it is still captured (its value flows through the proto interface).
    if (tok.peek().isWord("IS")) {
      tok.next();
      expectWord(tok, "IS proto-field name");
      if (captureAuthor)
        captureAuthorField(node, fieldName, type, access, std::any{});
      return;
    }

    bool isEvent =
        access == AccessType::InputOnly || access == AccessType::OutputOnly;
    std::any initialValue;
    if (!isEvent && hasDefaultValueAhead(tok)) {
      if (type == X3DFieldType::SFNode || type == X3DFieldType::MFNode) {
        runtime::ProtoField sink;
        sink.type = type;
        captureNodeDefault(tok, sink);
      } else if (type == X3DFieldType::SFEnum || type == X3DFieldType::MFEnum) {
        tok.next();
      } else {
        // Gather the value tokens and box them so the author field carries a
        // real default (parseValue produces the boxed C++ field type).
        std::string wire = build::collectFieldValue(tok, type);
        if (captureAuthor)
          initialValue = parseValue(type, wire);
      }
    }
    if (captureAuthor)
      captureAuthorField(node, fieldName, type, access, std::move(initialValue));
  }

  // Register one author field declaration on `node` in the S1 store. Empty
  // initialValue for input/outputOnly (they carry no persistent value) is the
  // store's own contract; we pass through whatever the caller boxed.
  static void captureAuthorField(const X3DNode &node, const std::string &name,
                                 X3DFieldType type, AccessType access,
                                 std::any initialValue) {
    runtime::AuthorFieldDecl decl;
    decl.x3dName = name;
    decl.type = type;
    decl.access = access;
    decl.initialValue = std::move(initialValue);
    runtime::dynamicFieldStore().addAuthorField(node, decl);
  }

  // -------------------------------------------------------------------------
  // Node-list recovery helpers.
  // -------------------------------------------------------------------------

  // True when the upcoming tokens begin a node value rather than a scalar:
  //   DEF / USE / NULL / `[`  -> always a node value, or
  //   Identifier immediately followed by `{`  -> a `Type { ... }` node body.
  // Used where the grammar is otherwise ambiguous (an unknown proto-instance
  // field, an MFNode list element) so a node body is captured structurally
  // instead of mis-read as a scalar token (which would desync the parser).
  static bool nextIsNodeValue(VrmlTokenizer &tok) {
    const VrmlToken &t = tok.peek();
    if (t.isWord("DEF") || t.isWord("USE") || t.isWord("NULL") ||
        t.isPunct('['))
      return true;
    return t.kind == VrmlToken::Kind::Identifier && tok.peek2().isPunct('{');
  }

  // Inside an MFNode list (or a bare-node field position), the corpus mixes in
  // statements and stray tokens that are not node type names: ROUTE/PROTO/...
  // declarations, an extra `}` from a malformed sibling, a leading `[`, or a
  // raw scalar where this (newer) model exposes an SFNode/MFNode field. Returns
  // true if it recognized and consumed such a non-node item (so the caller's
  // loop should `continue`); false if `peek()` genuinely begins a node and
  // `parseNode` should run. Never throws.
  bool skipNonNodeListItem(VrmlTokenizer &tok, runtime::Scene &scene) {
    const VrmlToken &t = tok.peek();
    // Embedded statements are valid inside node bodies/lists; handle in place.
    if (t.isWord("ROUTE")) {
      parseRoute(tok, scene);
      return true;
    }
    if (t.isWord("PROTO")) {
      parseProto(tok, scene);
      return true;
    }
    if (t.isWord("EXTERNPROTO")) {
      parseExternProto(tok, scene);
      return true;
    }
    if (t.isWord("IMPORT")) {
      parseImport(tok, scene);
      return true;
    }
    if (t.isWord("EXPORT")) {
      parseExport(tok, scene);
      return true;
    }
    // A stray closing `}` (malformed sibling) where a node is expected: consume
    // it and recover rather than aborting the document.
    if (t.isPunct('}')) {
      warn("stray '}' in node list at line " + std::to_string(t.line) +
           " (skipped)");
      tok.next();
      return true;
    }
    // A raw scalar / nested `[` where a node type name is expected (an
    // X3D-3.x-style raw value field exposed as a node field by this model):
    // skip the value rather than mis-parsing it as a node.
    if (t.kind == VrmlToken::Kind::Number ||
        t.kind == VrmlToken::Kind::String || t.isPunct('[')) {
      warn("non-node value in node list at line " + std::to_string(t.line) +
           " (skipped)");
      skipFieldValue(tok);
      return true;
    }
    return false; // peek() begins a real node
  }

  // Drain any inline EXTERNPROTO/PROTO/ROUTE declarations (and stray tokens)
  // that precede a node value in an SFNode field slot — the corpus writes
  // `field EXTERNPROTO X[...][url] X { ... }`. Returns false if the value
  // position is exhausted (`}` / `]` / EOF) so the caller should not parseNode.
  bool drainLeadingDeclarations(VrmlTokenizer &tok, runtime::Scene &scene) {
    while (skipNonNodeListItem(tok, scene)) {
      if (tok.atEnd() || tok.peek().isPunct('}') || tok.peek().isPunct(']'))
        return false;
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Skipping helpers.
  // -------------------------------------------------------------------------

  // Consume a balanced `{ ... }` block (the lexer already split punctuation).
  // Used to skip the body of an unknown node type.
  void skipBalancedBraceBlock(VrmlTokenizer &tok) {
    expectPunct(tok, '{', "unknown-node body open");
    int depth = 1;
    while (!tok.atEnd() && depth > 0) {
      const VrmlToken t = tok.next();
      if (t.isPunct('{'))
        ++depth;
      else if (t.isPunct('}'))
        --depth;
    }
    if (depth != 0)
      throw error(tok.peek(), "unbalanced '{' in skipped node body");
  }

  // Skip the value of an unknown field: a child node `Type{...}` / `USE x`, a
  // bracketed `[ ... ]` list (balanced), or a single bare token.
  void skipFieldValue(VrmlTokenizer &tok) {
    const VrmlToken &t = tok.peek();
    // No value present: the field name was the last token before a container
    // closer ('}' / ']') or EOF. This happens when a preceding field over- or
    // under-collected its value run and a leftover token got (mis)read as a
    // field name, leaving the real node/array closer in the value position
    // (corpus: OrthoViewpoint `fieldOfView 1 3 2 4`, where a bare MFFloat reads
    // only the first element and the trailing scalars are seen as fields). Never
    // consume the closer — it belongs to the enclosing container. Returning here
    // lets parseNodeBody / the MFNode loop see the closer and resync cleanly.
    if (t.atEnd() || t.isPunct('}') || t.isPunct(']'))
      return;
    // An implicit-containerField child node: the type name was already eaten as
    // the (unknown) field name, so the value position opens directly on the
    // child's '{' body. Skip it as one balanced unit. Without this, the bare-
    // token branch below would consume only the lone '{' and desync by one
    // brace (the XML default-containerField idiom written into ClassicVRML).
    if (t.isPunct('{')) {
      skipBalancedBraceBlock(tok);
      return;
    }
    if (t.isWord("USE")) {
      tok.next();
      if (!tok.atEnd())
        tok.next(); // USE name
      return;
    }
    if (t.isPunct('[')) {
      tok.next();
      int depth = 1;
      while (!tok.atEnd() && depth > 0) {
        const VrmlToken x = tok.next();
        if (x.isPunct('['))
          ++depth;
        else if (x.isPunct(']'))
          --depth;
        else if (x.isPunct('{'))
          skipRestOfBraceBlock(tok); // a node inside the list
      }
      return;
    }
    // A node value: `[DEF n] Type {`  -> look ahead for an upcoming '{'.
    // We consume one token and, if the *next* is '{' (a node body), skip it.
    // Otherwise it was a scalar token and we are done.
    if (!t.atEnd()) {
      tok.next(); // first value token (Type name, DEF, or a scalar)
      if (tok.peek().isPunct('{'))
        skipBalancedBraceBlock(tok);
      else if (tok.peek().isWord("DEF") || tok.peek().isWord("USE")) {
        // `DEF name Type { ... }` form after a leading DEF token was eaten.
        // Conservative: consume up to and including a following brace block.
        while (!tok.atEnd() && !tok.peek().isPunct('{') &&
               !tok.peek().isPunct('}'))
          tok.next();
        if (tok.peek().isPunct('{'))
          skipBalancedBraceBlock(tok);
      }
    }
  }

  // Skip the remainder of a `{ ... }` block whose opening '{' was already
  // consumed by the caller.
  void skipRestOfBraceBlock(VrmlTokenizer &tok) {
    int depth = 1;
    while (!tok.atEnd() && depth > 0) {
      const VrmlToken t = tok.next();
      if (t.isPunct('{'))
        ++depth;
      else if (t.isPunct('}'))
        --depth;
    }
  }

  // -------------------------------------------------------------------------
  // Proto default / instance node-value capture (parsed into a throwaway scope;
  // proto-interface node defaults do not share the document DEF table).
  // -------------------------------------------------------------------------
  void captureNodeDefault(VrmlTokenizer &tok, runtime::ProtoField &field) {
    runtime::Scene throwaway;
    if (field.type == X3DFieldType::SFNode) {
      if (tok.peek().isWord("NULL")) {
        tok.next();
        return;
      }
      // A raw `[ ... ]`/scalar default for an SFNode field (older encoding):
      // skip the value rather than mis-parsing it as a node.
      if (tok.peek().isPunct('[') ||
          tok.peek().kind == VrmlToken::Kind::Number ||
          tok.peek().kind == VrmlToken::Kind::String) {
        skipFieldValue(tok);
        return;
      }
      if (!drainLeadingDeclarations(tok, throwaway))
        return;
      auto n = parseNode(tok, throwaway);
      if (n)
        field.nodeDefault.push_back(n);
      return;
    }
    // MFNode default.
    if (tok.peek().isPunct('[')) {
      tok.next();
      while (!tok.atEnd() && !tok.peek().isPunct(']')) {
        if (tok.peek().isWord("NULL")) {
          tok.next();
          continue;
        }
        if (skipNonNodeListItem(tok, throwaway))
          continue;
        auto n = parseNode(tok, throwaway);
        if (n)
          field.nodeDefault.push_back(n);
      }
      expectPunct(tok, ']', "MFNode default close");
    } else if (!tok.peek().isPunct('}') && !tok.peek().isPunct(']')) {
      if (!drainLeadingDeclarations(tok, throwaway))
        return;
      auto n = parseNode(tok, throwaway);
      if (n)
        field.nodeDefault.push_back(n);
    }
  }

  void captureInstanceNodeValue(VrmlTokenizer &tok, runtime::Scene &scene,
                                runtime::ProtoFieldValue &fv, X3DFieldType ty) {
    if (ty == X3DFieldType::SFNode) {
      if (tok.peek().isWord("NULL")) {
        tok.next();
        return;
      }
      // A raw `[ ... ]`/scalar value assigned to an SFNode proto field (an
      // X3D-3.x controlPoint coordinate list, etc.): skip the value rather than
      // calling parseNode on `[`/a number (which would throw).
      if (tok.peek().isPunct('[') ||
          tok.peek().kind == VrmlToken::Kind::Number ||
          tok.peek().kind == VrmlToken::Kind::String) {
        skipFieldValue(tok);
        return;
      }
      if (!drainLeadingDeclarations(tok, scene))
        return;
      auto n = parseNode(tok, scene);
      if (n)
        fv.nodeValue.push_back(n);
      return;
    }
    if (tok.peek().isPunct('[')) {
      tok.next();
      while (!tok.atEnd() && !tok.peek().isPunct(']')) {
        if (tok.peek().isWord("NULL")) {
          tok.next();
          continue;
        }
        if (skipNonNodeListItem(tok, scene))
          continue;
        auto n = parseNode(tok, scene);
        if (n)
          fv.nodeValue.push_back(n);
      }
      expectPunct(tok, ']', "MFNode instance value close");
    } else if (!tok.peek().isPunct('}') && !tok.peek().isPunct(']')) {
      // A single bare node value, possibly preceded by inline declarations.
      if (!drainLeadingDeclarations(tok, scene))
        return;
      auto n = parseNode(tok, scene);
      if (n)
        fv.nodeValue.push_back(n);
    }
  }

  // -------------------------------------------------------------------------
  // Small token utilities + error formatting.
  // -------------------------------------------------------------------------
  static std::runtime_error error(const VrmlToken &t, const std::string &msg) {
    return std::runtime_error("ClassicVRML parse error at line " +
                              std::to_string(t.line) + ":" +
                              std::to_string(t.col) + ": " + msg);
  }

  // Consume a bare word/number token; throw if the next token is punctuation,
  // EOF, or a string.
  static std::string expectWord(VrmlTokenizer &tok, const std::string &what) {
    const VrmlToken &t = tok.peek();
    if (t.atEnd())
      throw error(t, "expected " + what + ", got end of input");
    if (t.kind == VrmlToken::Kind::Punct)
      throw error(t, "expected " + what + ", got '" + t.text + "'");
    return tok.next().text;
  }

  // Consume a quoted-string token (its unescaped contents). Tolerates a bare
  // word too (some generators omit quotes), but flags pure punctuation/EOF.
  static std::string expectString(VrmlTokenizer &tok, const std::string &what) {
    const VrmlToken &t = tok.peek();
    if (t.atEnd())
      throw error(t, "expected " + what + ", got end of input");
    if (t.kind == VrmlToken::Kind::Punct)
      throw error(t, "expected " + what + ", got '" + t.text + "'");
    return tok.next().text;
  }

  static void expectPunct(VrmlTokenizer &tok, char c, const std::string &what) {
    const VrmlToken &t = tok.peek();
    if (!t.isPunct(c))
      throw error(t, std::string("expected '") + c + "' (" + what + ")");
    tok.next();
  }

  // True if the next token plausibly begins a default value (not a closing
  // bracket and not the start of the next interface declaration's accessType).
  static bool hasDefaultValueAhead(VrmlTokenizer &tok) {
    const VrmlToken &t = tok.peek();
    if (t.atEnd() || t.isPunct(']') || t.isPunct('}'))
      return false;
    // Next interface field begins with an accessType keyword.
    if (!t.isString && isAccessTypeWord(t.text))
      return false;
    return true;
  }

  static bool isAccessTypeWord(const std::string &w) {
    return w == "inputOnly" || w == "outputOnly" || w == "initializeOnly" ||
           w == "inputOutput" || w == "eventIn" || w == "eventOut" ||
           w == "field" || w == "exposedField";
  }

  // Map an accessType token (incl. VRML97 aliases) to AccessType.
  static AccessType accessTypeFromString(const std::string &w) {
    if (w == "inputOnly" || w == "eventIn")
      return AccessType::InputOnly;
    if (w == "outputOnly" || w == "eventOut")
      return AccessType::OutputOnly;
    if (w == "initializeOnly" || w == "field")
      return AccessType::InitializeOnly;
    // inputOutput / exposedField (and any default)
    return AccessType::InputOutput;
  }

  // Map a field-type token (e.g. "SFVec3f", "MFNode") to X3DFieldType.
  static X3DFieldType fieldTypeFromString(const std::string &w) {
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

  static std::shared_ptr<runtime::ExternProtoDeclaration>
  findExternProto(const runtime::Scene &scene, const std::string &name) {
    for (const auto &e : scene.externProtoDeclarations)
      if (e && e->name == name)
        return e;
    return nullptr;
  }

  static const runtime::ProtoField *
  findProtoField(const runtime::ProtoInstance &inst, const std::string &name) {
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
};

} // namespace x3d::codec

#endif // X3D_PARSE_CLASSIC_VRML_READER_HPP
