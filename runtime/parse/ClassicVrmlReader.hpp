// ClassicVRML reader (ISO/IEC 19776-2). Unknown node and field names are skipped.
// Unrecoverable syntax errors throw std::runtime_error with line and column.

#ifndef X3D_PARSE_CLASSIC_VRML_READER_HPP
#define X3D_PARSE_CLASSIC_VRML_READER_HPP

#include "Encoding.hpp"
#include "VrmlTokenizer.hpp"
#include "X3DReader.hpp"
#include "X3DRuntime.hpp"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::codec {
using namespace x3d::core;

class ClassicVrmlReader : public X3DReader {
public:
  Encoding encoding() const override;

  /// Parse a complete ClassicVRML document. Throws std::runtime_error only on
  /// unrecoverable malformation; unknown node/field names are skipped. The
  /// returned document already has scene.resolveRoutes() applied.
  runtime::X3DDocument readDocument(const std::string &text) override;

protected:
  // Dialect hooks: the base passes names through and discards warnings.

  /// Map a field-name token (on a node of `nodeType`) to its X3D field name.
  /// Base: identity.
  virtual std::string mapFieldName(const std::string & /*nodeType*/,
                                   const std::string &token) const;

  /// React to the tokenized first line. Base: accept anything (the version was
  /// already read by parseHeaderLine). A subclass may throw on an unsupported
  /// header or set document defaults.
  virtual void onHeaderLine(const std::vector<std::string> & /*parts*/,
                            runtime::X3DDocument & /*doc*/);

  /// Non-fatal diagnostic sink. Base: discard (ClassicVRML is tolerant-silent,
  /// matching XmlReader). Vrml97Reader collects these into warnings().
  virtual void warn(const std::string & /*message*/);

  /// First physical line of `src` (BOM-stripped, sans trailing CR), or empty.
  static std::string_view firstLine(std::string_view src);

private:
  std::size_t depth_ = 0; // node nesting depth (DoS guard).
  bool capturingProtoFieldValue_ = false;

  // Reader-recovery diagnostics accumulated during the current readDocument()
  // call, moved into X3DDocument.readerWarnings on return.
  std::vector<runtime::ReaderWarning> readerWarnings_;
  static std::vector<std::string> splitOnSpace(std::string_view line);
  static void parseHeaderLine(const std::vector<std::string> &parts,
                               runtime::X3DDocument &doc);
  static void parseHeaderStatements(VrmlTokenizer &tok,
                                    runtime::X3DDocument &doc);
  // Scene body: a sequence of top-level statements until EOF (or, inside a
  // proto body, until the closing '}').
  // `currentProtoBody` is non-null only while parsing the body of a PROTO
  // declaration; it lets a `field IS protoField` statement deep in the body
  // record an IsConnection on the enclosing ProtoBody. Root-level
  // statements at scene scope leave it null.
  void parseSceneBody(VrmlTokenizer &tok, runtime::Scene &scene, bool inProto,
                      runtime::ProtoBody *currentProtoBody = nullptr);
  // Node production:
  //   node := "USE" Name
  //         | ["DEF" Name] TypeName "{" body "}"
  // `currentProtoBody` (non-null inside a PROTO body) threads through to the IS
  // site in parseNodeBody so `field IS protoField` records an IsConnection.
  // `parentNode` / `parentField` describe where a detected ProtoInstance sits
  // in the graph (null parent => Scene root) so its parent linkage is recorded.
  std::shared_ptr<X3DNode>
  parseNode(VrmlTokenizer &tok, runtime::Scene &scene,
            runtime::ProtoBody *currentProtoBody = nullptr,
            const std::shared_ptr<X3DNode> &parentNode = nullptr,
            const std::string &parentField = std::string());

  // Decode the first inline-scheme entry of a Script's `url` field and write
  // the decoded body into its `sourceCode` field — both reached via reflection.
  // No-op if the node lacks either field, the url has no inline entry, or
  // sourceCode is already set (an explicit `sourceCode` field wins).
  static void mirrorScriptSource(X3DNode &node);

  // Node body: field / nested-node / interface-decl / IS / route statements.
  // `currentProtoBody` is non-null when this body sits inside a PROTO body: a
  // `field IS protoField` statement then records an IsConnection on it bound to
  // `nodeShared` (the shared_ptr of the node whose body this is). `nodeShared`
  // also serves as the parent for any ProtoInstance nested as a child here.
  void parseNodeBody(VrmlTokenizer &tok, runtime::Scene &scene, X3DNode &node,
                     const std::string &nodeType = std::string(),
                     runtime::ProtoBody *currentProtoBody = nullptr,
                     const std::shared_ptr<X3DNode> &nodeShared = nullptr);

  // SFNode / MFNode field. `currentProtoBody` (non-null inside a PROTO body)
  // and `parentShared` (the shared_ptr of `parent`) thread on to parseNode so a
  // nested node's own IS statements are captured and a nested ProtoInstance
  // records its parent (`parentShared`) + the slot it fills (containerField).
  void applyNodeField(VrmlTokenizer &tok, runtime::Scene &scene,
                      X3DNode &parent, const FieldInfo &f,
                      runtime::ProtoBody *currentProtoBody = nullptr,
                      const std::shared_ptr<X3DNode> &parentShared = nullptr);

  // Enum field: one token via setEnumString. A bracketed MFEnum run is joined.
  static bool applyEnumField(VrmlTokenizer &tok, X3DNode &node,
                             const FieldInfo &f);
  // ROUTE statement: ROUTE Name1.field1 TO Name2.field2
  void parseRoute(VrmlTokenizer &tok, runtime::Scene &scene);
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
  void parseImport(VrmlTokenizer &tok, runtime::Scene &scene);

  void parseExport(VrmlTokenizer &tok, runtime::Scene &scene);

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
  // PROTO / EXTERNPROTO declarations (structural; no expansion).
  //   PROTO Name "[" interface "]" "{" body "}"
  //   EXTERNPROTO Name "[" interface "]" urlList
  std::shared_ptr<runtime::ProtoDeclaration>
  parseProto(VrmlTokenizer &tok, runtime::Scene &scene);

  std::shared_ptr<runtime::ExternProtoDeclaration>
  parseExternProto(VrmlTokenizer &tok, runtime::Scene &scene);

  // interface := ( accessType FieldType fieldName [defaultValue] )*
  std::vector<runtime::ProtoField> parseInterface(VrmlTokenizer &tok,
                                                  bool allowDefaults,
                                                  runtime::Scene *scope = nullptr,
                                                  runtime::ProtoBody *body = nullptr);

  // Proto instance: `TypeName { (fieldName fieldValue)* }` recorded as data.
  void parseProtoInstance(VrmlTokenizer &tok, runtime::Scene &scene,
                          const std::string &def, const std::string &typeName);
  // Inline interface declaration inside a node body (Script user fields, etc.):
  //   accessType FieldType fieldName [ IS protoField | defaultValue ]
  // On Script and programmable-shader nodes (ISO/IEC 19776-2 A.3), the
  // declaration is captured into the
  // DynamicFieldStore as an AuthorFieldDecl so it becomes visible to the
  // runtime (ROUTE endpoint resolution, SAI get/set). The access keyword maps
  // per accessTypeFromName (field->initializeOnly, exposedField->inputOutput,
  // eventIn->inputOnly, eventOut->outputOnly); the boxed default seeds the
  // value store (empty for input/outputOnly). Declaration IS mappings are
  // recorded on the enclosing ProtoBody (ISO/IEC 19776-2 A.3).
  void consumeInterfaceDeclaration(VrmlTokenizer &tok,
                                   runtime::Scene &scene,
                                   const std::shared_ptr<X3DNode> &nodeShared,
                                   runtime::ProtoBody *currentProtoBody);

  // Register one author field declaration on `node` in the author-field store. Empty
  // initialValue for input/outputOnly (they carry no persistent value) is the
  // store's own contract; we pass through whatever the caller boxed.
  static void captureAuthorField(runtime::Scene &scene,
                                 const std::shared_ptr<X3DNode> &node,
                                 const std::string &name,
                                 X3DFieldType type, AccessType access,
                                 std::any initialValue);
  // Node-list recovery helpers.

  // True when the upcoming tokens begin a node value rather than a scalar:
  //   DEF / USE / NULL / `[`  -> always a node value, or
  //   Identifier immediately followed by `{`  -> a `Type { ... }` node body.
  // Used where the grammar is otherwise ambiguous (an unknown proto-instance
  // field, an MFNode list element) so a node body is captured structurally
  // instead of mis-read as a scalar token (which would desync the parser).
  static bool nextIsNodeValue(VrmlTokenizer &tok);

  // Inside an MFNode list (or a bare-node field position), the corpus mixes in
  // statements and stray tokens that are not node type names: ROUTE/PROTO/...
  // declarations, an extra `}` from a malformed sibling, a leading `[`, or a
  // raw scalar where this (newer) model exposes an SFNode/MFNode field. Returns
  // true if it recognized and consumed such a non-node item (so the caller's
  // loop should `continue`); false if `peek()` genuinely begins a node and
  // `parseNode` should run. Never throws.
  bool skipNonNodeListItem(
      VrmlTokenizer &tok, runtime::Scene &scene,
      runtime::ProtoBody *body = nullptr,
      const std::shared_ptr<X3DNode> &parent = nullptr);

  // Drain any inline EXTERNPROTO/PROTO/ROUTE declarations (and stray tokens)
  // that precede a node value in an SFNode field slot — the corpus writes
  // `field EXTERNPROTO X[...][url] X { ... }`. Returns false if the value
  // position is exhausted (`}` / `]` / EOF) so the caller should not parseNode.
  bool drainLeadingDeclarations(
      VrmlTokenizer &tok, runtime::Scene &scene,
      runtime::ProtoBody *body = nullptr,
      const std::shared_ptr<X3DNode> &parent = nullptr);
  // Skipping helpers.

  // Consume a balanced `{ ... }` block (the lexer already split punctuation).
  // Used to skip the body of an unknown node type.
  void skipBalancedBraceBlock(VrmlTokenizer &tok);

  // Skip the value of an unknown field: a child node `Type{...}` / `USE x`, a
  // bracketed `[ ... ]` list (balanced), or a single bare token.
  void skipFieldValue(VrmlTokenizer &tok);

  // Skip the remainder of a `{ ... }` block whose opening '{' was already
  // consumed by the caller.
  void skipRestOfBraceBlock(VrmlTokenizer &tok);
  // Proto defaults use the declaration scope; Script defaults use a temporary
  // scope. Neither shares the enclosing document's DEF table.
  void captureNodeDefault(VrmlTokenizer &tok, runtime::ProtoField &field,
                          runtime::Scene *scope = nullptr,
                          runtime::ProtoBody *body = nullptr);

  void captureInstanceNodeValue(VrmlTokenizer &tok, runtime::Scene &scene,
                                runtime::ProtoFieldValue &fv, X3DFieldType ty);
  // Small token utilities + error formatting.
  static std::runtime_error error(const VrmlToken &t, const std::string &msg);

  // Consume a word, number, or string token; reject punctuation and EOF.
  static std::string expectWord(VrmlTokenizer &tok, const std::string &what);

  static void expectPunct(VrmlTokenizer &tok, char c, const std::string &what);

  // True if the next token plausibly begins a default value (not a closing
  // bracket and not the start of the next interface declaration's accessType).
  static bool hasDefaultValueAhead(VrmlTokenizer &tok);

  static bool isAccessTypeWord(const std::string &w);

  static std::shared_ptr<runtime::ExternProtoDeclaration>
  findExternProto(const runtime::Scene &scene, const std::string &name);

};

} // namespace x3d::codec

#endif // X3D_PARSE_CLASSIC_VRML_READER_HPP
