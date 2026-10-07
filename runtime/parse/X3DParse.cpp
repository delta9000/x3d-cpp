#include "X3DParse.hpp"

#include "ClassicVrmlReader.hpp"
#include "Inflate.hpp"
#include "JsonReader.hpp"
#include "PathConfine.hpp"
#include "Vrml97Reader.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DRangeValidate.hpp"
#include "XmlReaderAdapter.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace x3d::codec {

namespace {

// UNIT is a header declaration, so validate it once before any scene expansion.
// In particular, a malformed factor must never reach a runtime as a scale.
void validateUnits(const runtime::X3DDocument &doc) {
  if (doc.head.units.empty()) return;
  unsigned major = 0, minor = 0;
  const char *begin = doc.version.data();
  const char *end = begin + doc.version.size();
  const char *dot = std::find(begin, end, '.');
  const auto majorResult = std::from_chars(begin, dot, major);
  const auto minorResult = dot == end
      ? std::from_chars(end, end, minor)
      : std::from_chars(dot + 1, end, minor);
  if (dot == begin || dot == end || majorResult.ec != std::errc{} ||
      majorResult.ptr != dot || minorResult.ec != std::errc{} ||
      minorResult.ptr != end ||
      major < 3 || (major == 3 && minor < 3))
    throw std::runtime_error("UNIT requires X3D version 3.3 or later");

  std::unordered_set<std::string> categories;
  for (const auto &unit : doc.head.units) {
    if (unit.category != "angle" && unit.category != "force" &&
        unit.category != "length" && unit.category != "mass")
      throw std::runtime_error("UNIT has unknown category '" + unit.category + "'");
    if (!categories.insert(unit.category).second)
      throw std::runtime_error("duplicate UNIT category '" + unit.category + "'");
    if (unit.name.empty() ||
        std::any_of(unit.name.begin(), unit.name.end(), [](unsigned char c) {
          return std::isspace(c) != 0;
        }))
      throw std::runtime_error("UNIT name must be nonempty and contain no whitespace");
    if (!std::isfinite(unit.conversionFactor) || unit.conversionFactor <= 0.0)
      throw std::runtime_error("UNIT conversionFactor must be finite and positive");
  }
}

// Reader-created declarations can also be reached through instances inside a
// ProtoBody's local scope. Walk those handles before expansion, while every
// declaration still belongs to this parsed document. External declarations
// are resolved later and retain source units and URL provenance from their
// own parseDocument call.
void snapshotSourceProvenance(runtime::X3DDocument &doc,
                              const std::string &baseUrl) {
  doc.scene.sourceUnits = doc.head.units;
  std::unordered_set<runtime::ProtoDeclaration *> visited;
  std::unordered_set<const x3d::nodes::X3DNode *> visitedNodes;
  auto visitNode = [&](auto &&self,
                       const std::shared_ptr<x3d::nodes::X3DNode> &node,
                       auto &&visitDecl) -> void {
    if (!node || !visitedNodes.insert(node.get()).second) return;
    doc.scene.nodeBaseUrls.try_emplace(node, baseUrl);
    if (auto wrapper =
            std::dynamic_pointer_cast<runtime::ProtoInstanceTemplate>(node)) {
      visitDecl(wrapper->instance.declaration);
      if (wrapper->instance.externDeclaration)
        wrapper->instance.externDeclaration->sourceBaseUrl = baseUrl;
      for (auto &value : wrapper->instance.fieldValues) {
        value.sourceUnits = doc.head.units;
        value.sourceBaseUrl = baseUrl;
        for (const auto &child : value.nodeValue)
          self(self, child, visitDecl);
      }
    }
    for (const auto &field : node->fields()) {
      if (!field.isReadable() || !field.isNode() || !field.get) continue;
      auto value = field.get(*node);
      if (field.type == core::X3DFieldType::SFNode) {
        if (auto child =
                std::any_cast<std::shared_ptr<x3d::nodes::X3DNode>>(value))
          self(self, child, visitDecl);
      } else if (field.type == core::X3DFieldType::MFNode) {
        for (const auto &child : std::any_cast<
                 std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(value))
          self(self, child, visitDecl);
      }
    }
  };
  auto visit = [&](auto &&self,
                   const std::shared_ptr<runtime::ProtoDeclaration> &decl) -> void {
    if (!decl || !visited.insert(decl.get()).second) return;
    decl->sourceUnits = doc.head.units;
    decl->sourceBaseUrl = baseUrl;
    auto visitDecl = [&](const auto &nested) { self(self, nested); };
    for (const auto &field : decl->interface)
      for (const auto &node : field.nodeDefault)
        visitNode(visitNode, node, visitDecl);
    for (const auto &node : decl->body.nodes)
      visitNode(visitNode, node, visitDecl);
    for (const auto &statement : decl->body.statements)
      if (statement.kind == runtime::ProtoBodyStatement::Kind::Proto)
        self(self, statement.proto);
      else if (statement.externProto)
        statement.externProto->sourceBaseUrl = baseUrl;
    for (const auto &[parent, statements] : decl->body.nodeStatements) {
      if (parent.expired()) continue;
      for (const auto &statement : statements) {
        if (statement.kind == runtime::ProtoBodyStatement::Kind::Proto)
          self(self, statement.proto);
        else if (statement.externProto)
          statement.externProto->sourceBaseUrl = baseUrl;
      }
    }
    for (auto &nested : decl->body.nestedInstances) {
      self(self, nested.declaration);
      if (nested.externDeclaration)
        nested.externDeclaration->sourceBaseUrl = baseUrl;
      for (auto &value : nested.fieldValues) {
        value.sourceUnits = doc.head.units;
        value.sourceBaseUrl = baseUrl;
        for (const auto &node : value.nodeValue)
          visitNode(visitNode, node, visitDecl);
      }
    }
  };
  for (const auto &decl : doc.scene.protoDeclarations)
    visit(visit, decl);
  for (const auto &decl : doc.scene.externProtoDeclarations)
    if (decl) decl->sourceBaseUrl = baseUrl;
  for (auto &inst : doc.scene.protoInstances) {
    visit(visit, inst.declaration);
    if (inst.externDeclaration) inst.externDeclaration->sourceBaseUrl = baseUrl;
    auto visitDecl = [&](const auto &nested) { visit(visit, nested); };
    for (auto &value : inst.fieldValues) {
      value.sourceUnits = doc.head.units;
      value.sourceBaseUrl = baseUrl;
      for (const auto &node : value.nodeValue)
        visitNode(visitNode, node, visitDecl);
    }
  }
  auto visitDecl = [&](const auto &nested) { visit(visit, nested); };
  for (const auto &root : doc.scene.rootNodes)
    visitNode(visitNode, root, visitDecl);
}

/// Quarantine PROTO/EXTERNPROTO declarations that reuse a built-in node type
/// name (ADR-0033, §4.4.4: node type names shall be unique; shadowing a built-in
/// is rejected — the built-in keeps the name). Detection is centralized here so
/// every encoding gets the same policy from the single front door. Lenient: the
/// rogue declaration is dropped and a BuiltinShadow diagnostic is recorded; the
/// scene still loads (a strict embedder may treat any such warning as fatal).
void quarantineBuiltinShadowingProtos(runtime::X3DDocument &doc) {
  const auto &registry = x3d::nodes::X3DNodeFactory::registry();
  auto isBuiltin = [&](const std::string &name) {
    return !name.empty() && registry.count(name) != 0;
  };
  auto &decls = doc.scene.protoDeclarations;
  decls.erase(std::remove_if(decls.begin(), decls.end(),
                             [&](const std::shared_ptr<runtime::ProtoDeclaration> &p) {
                               if (p && isBuiltin(p->name)) {
                                 doc.protoWarnings.push_back(
                                     {runtime::ProtoWarning::Kind::BuiltinShadow,
                                      p->name,
                                      "PROTO '" + p->name +
                                          "' shadows a built-in node type; "
                                          "declaration ignored"});
                                 return true;
                               }
                               return false;
                             }),
              decls.end());
  auto &externs = doc.scene.externProtoDeclarations;
  externs.erase(
      std::remove_if(
          externs.begin(), externs.end(),
          [&](const std::shared_ptr<runtime::ExternProtoDeclaration> &p) {
            if (p && isBuiltin(p->name)) {
              doc.protoWarnings.push_back(
                  {runtime::ProtoWarning::Kind::BuiltinShadow, p->name,
                   "EXTERNPROTO '" + p->name +
                       "' shadows a built-in node type; declaration ignored"});
              return true;
            }
            return false;
          }),
      externs.end());

  // Instances bound to a dropped declaration at read time resolve to the
  // built-in instead (ADR-0033: "as if the rogue PROTO were absent").
  for (runtime::ProtoInstance &inst : doc.scene.protoInstances) {
    if (!isBuiltin(inst.name)) continue;
    const bool localGone =
        inst.declaration &&
        std::find(decls.begin(), decls.end(), inst.declaration) == decls.end();
    const bool externGone =
        inst.externDeclaration &&
        std::find(externs.begin(), externs.end(), inst.externDeclaration) == externs.end();
    if (localGone || externGone) {
      inst.declaration.reset();
      inst.externDeclaration.reset();
      inst.builtinFallback = true;
    }
  }
}

} // namespace

void stripUtf8Bom(std::string &s) {
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF)
    s.erase(0, 3);
}

std::unique_ptr<X3DReader> makeReader(Encoding enc) {
  switch (enc) {
  case Encoding::XML:
    return std::make_unique<XmlReaderAdapter>();
  case Encoding::ClassicVRML:
    return std::make_unique<ClassicVrmlReader>();
  case Encoding::VRML97:
    return std::make_unique<Vrml97Reader>();
  case Encoding::JSON:
    return std::make_unique<JsonReader>();
  case Encoding::Unknown:
  default:
    return nullptr;
  }
}

runtime::X3DDocument
parseDocument(const std::string &text, Encoding hint,
              const std::string &baseUrl,
              const ProtoDeclarationResolver &resolver,
              const runtime::InlineResolver &inlineResolver,
              const std::string &confineRoot) {
  // Establish the SEC-3 confinement root (ADR-0038) for the default resolvers
  // on the outermost entry; nested re-parses inherit it. When reached via
  // parseFile the root is already set (so this is a no-op); a direct
  // parseDocument call from a tool handling a trusted content tree passes the
  // tree root here.
  const bool outermostConfine = detail::activeConfineRoot().empty();
  struct ConfineRootScope {
    bool owns;
    ~ConfineRootScope() {
      if (owns)
        detail::activeConfineRoot().clear();
    }
  } confineScope{outermostConfine};
  if (outermostConfine && !(confineRoot.empty() && baseUrl.empty()))
    detail::activeConfineRoot() = !confineRoot.empty() ? confineRoot : baseUrl;
  // Strip a leading UTF-8 BOM up front so every reader sees BOM-free bytes. The
  // bundled XmlLite byte-level parser does not skip a BOM (it expects '<' or
  // ASCII space at the head), so a BOM-prefixed XML document would otherwise be
  // rejected with "expected root element". Sniffing already ignores a BOM, so
  // doing this first is harmless and idempotent.
  std::string body = text;
  stripUtf8Bom(body);
  Encoding enc = hint;
  if (enc == Encoding::Unknown)
    enc = sniffByContent(body);
  std::unique_ptr<X3DReader> reader = makeReader(enc);
  if (!reader)
    throw std::runtime_error(
        "parseDocument: could not determine X3D encoding from content");
  runtime::X3DDocument doc = reader->readDocument(body);
  validateUnits(doc);
  snapshotSourceProvenance(doc, baseUrl);
  // ADR-0033: drop any PROTO/EXTERNPROTO that reuses a built-in node name (the
  // built-in keeps precedence) before instances are expanded against it.
  quarantineBuiltinShadowingProtos(doc);
  // Surface out-of-range values the lenient readers kept (structured channel;
  // parseFile flows through here, so this is the single collection site).
  for (const auto &root : doc.scene.rootNodes)
    if (root)
      collectRangeWarnings(*root, doc.rangeWarnings);
  // Expand captured PROTO/EXTERNPROTO instances after range-warning collection,
  // splicing primaries into their parent slots. Local PROTOs always expand;
  // EXTERN instances resolve through `resolver` (default: sibling-file lookup).
  runtime::expandScene(doc.scene, resolver, baseUrl, doc.protoWarnings);
  // Expand load=TRUE Inlines: resolve each url to a sub-scene and splice it in.
  // Mirrors the PROTO pass above; nested Inlines recurse via parseFile.
  runtime::expandInlines(doc.scene, inlineResolver, baseUrl,
                         doc.inlineWarnings);
  // Wire IMPORT statements to their Inline's exported node (the child scene is
  // now retained), then re-resolve routes so a ROUTE to/from an imported AS
  // name binds instead of being dropped as an unknown DEF.
  runtime::wireInlineImports(doc.scene);
  doc.scene.resolveRoutes();
  return doc;
}

std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u, frag;
    if (auto h = url.find('#'); h != std::string::npos) {
      frag = url.substr(h + 1);
      url = url.substr(0, h);
    }
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue; // embedder override territory; default resolver stays local
    // SEC-3: confine the resolved path to the source file's directory subtree
    // (reject absolute urls and `../` escapes) — the default local resolver
    // never reads arbitrary files. SEC-4: the returned path is canonical, so it
    // is a stable cycle-guard key that spelling variants cannot alias past.
    auto confined =
        confineLocalIncludePath(url, baseUrl, detail::activeConfineRoot());
    if (!confined)
      continue; // absolute / escaping / unresolvable -> skip this candidate
    const std::string path = std::move(*confined);
    if (std::find(activeFiles.begin(), activeFiles.end(), path) !=
        activeFiles.end())
      continue; // cycle: this file is already being resolved up the stack
    activeFiles.push_back(path);
    std::shared_ptr<runtime::ProtoDeclaration> found;
    try {
      runtime::X3DDocument sub = parseFile(path);
      if (!frag.empty())
        found = sub.scene.findProto(frag);
      else if (!sub.scene.protoDeclarations.empty())
        found = sub.scene.protoDeclarations.front();
    } catch (const std::exception &) {
      // lenient: fall through to the next candidate url
    }
    activeFiles.pop_back();
    if (found)
      return found;
  }
  return nullptr;
}

std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u;
    if (auto h = url.find('#'); h != std::string::npos)
      url = url.substr(0, h);
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue;
    auto confined =
        confineLocalIncludePath(url, baseUrl, detail::activeConfineRoot());
    if (!confined) continue;
    const std::string path = std::move(*confined);
    if (std::find(activeFiles.begin(), activeFiles.end(), path) !=
        activeFiles.end())
      continue; // cycle: this file is already being resolved up the stack
    activeFiles.push_back(path);
    std::shared_ptr<runtime::Scene> result;
    try {
      runtime::X3DDocument sub =
          parseFile(path); // recurses (expands sub-Inlines)
      result = std::make_shared<runtime::Scene>(std::move(sub.scene));
    } catch (const std::exception &) {
      // lenient: try the next candidate url
    }
    activeFiles.pop_back();
    if (result)
      return result;
  }
  return nullptr;
}

runtime::X3DDocument parseFile(const std::string &path,
                               const std::string &confineRoot) {
  // Establish the confinement root on the outermost call; inner resolver-driven
  // re-parses (activeConfineRoot already set) inherit it and restore nothing.
  const bool outermostConfine = detail::activeConfineRoot().empty();
  struct ConfineRootScope {
    bool owns;
    ~ConfineRootScope() {
      if (owns)
        detail::activeConfineRoot().clear();
    }
  } confineScope{outermostConfine};

  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("parseFile: cannot open file: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string bytes = ss.str();
  // Inflate first: sniffByExtension already strips .gz/.gzip and the inflated
  // bytes content-sniff correctly, so no further branching is needed below.
  if (isGzip(bytes))
    bytes = inflateGzip(bytes);
  // Strip a leading UTF-8 BOM before sniffing/dispatch (see stripUtf8Bom). This
  // covers every encoding at the front door; parseDocument also strips, so this
  // is belt-and-suspenders and keeps sniff(path, bytes) BOM-insensitive.
  stripUtf8Bom(bytes);
  Encoding enc = sniff(path, bytes);
  if (enc == Encoding::Unknown)
    throw std::runtime_error("parseFile: could not determine X3D encoding: " +
                             path);
  // Derive the base directory so relative EXTERNPROTO urls resolve against the
  // source file's location rather than the process cwd.
  std::string base;
  if (auto slash = path.find_last_of("/\\"); slash != std::string::npos)
    base = path.substr(0, slash);
  // Default the confinement root to this top-level file's directory (secure
  // per-file default) unless a trusted caller widened it. `.` when base is
  // empty (a bare filename) so the root is never the empty "fall back to
  // baseUrl" key.
  if (outermostConfine)
    detail::activeConfineRoot() =
        !confineRoot.empty() ? confineRoot
                             : (base.empty() ? std::string(".") : base);
  return parseDocument(bytes, enc, base);
}

} // namespace x3d::codec
