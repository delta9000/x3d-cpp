// Shared validation diagnostics for the CLI and differential gate.
#pragma once

#include "x3d/sdk.hpp"
#include "proto_use.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/IndexedLineSet.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::cli_detail {

namespace sdk = x3d::sdk;
using namespace x3d::core;
using x3d::nodes::X3DNode;

struct Diag {
    std::string category;
    std::string severity;
    std::string message;
};

// Check 1: Duplicate <meta> statements
// X3DJSAIL: WARNING_MESSAGE: duplicate statement found: <meta name='X' content='Y'/>
// A <head> must not contain two <meta> elements with the same (name, content)
// pair — or more broadly the same name value.  We flag any pair with identical
// name AND identical content as a duplicate.
inline void checkDuplicateMeta(const sdk::X3DDocument &doc,
                        std::vector<Diag> &diags) {
    const auto &metas = doc.head.meta;
    for (size_t i = 0; i < metas.size(); ++i) {
        for (size_t j = i + 1; j < metas.size(); ++j) {
            if (metas[i].name == metas[j].name &&
                metas[i].content == metas[j].content) {
                std::string msg = "duplicate <meta name='" + metas[i].name
                                + "' content='" + metas[i].content + "'/>";
                diags.push_back({"dup-meta", "warning", msg});
            }
        }
    }
}

// Check 2: ProtoDeclare / ExternProtoDeclare with no ProtoInstance
// X3DJSAIL: WARNING_PROTOINSTANCE_NOT_FOUND, ProtoDeclare/ExternProtoDeclare X
// has no corresponding ProtoInstance attached to the scene graph.  Best practice:
// every declared prototype should have at least one instantiation.
inline void checkUnusedProtoDeclare(const sdk::X3DDocument &doc,
                             std::vector<Diag> &diags) {
    const auto &scene = doc.scene;
    const auto used = x3d::cli_detail::collectProtoUse(doc);
    // Check ProtoDeclare (local prototypes).
    for (const auto &pd : scene.protoDeclarations) {
        if (pd && !used.local.contains(pd.get())) {
            std::string msg = "ProtoDeclare '" + pd->name
                            + "' has no corresponding ProtoInstance in the scene";
            diags.push_back({"unused-proto", "warning", msg});
        }
    }
    // Check ExternProtoDeclare (externally-defined prototypes).
    for (const auto &epd : scene.externProtoDeclarations) {
        if (epd && !used.external.contains(epd.get())) {
            std::string msg = "ExternProtoDeclare '" + epd->name
                            + "' has no corresponding ProtoInstance in the scene";
            diags.push_back({"unused-proto", "warning", msg});
        }
    }
}

// Check 3: IndexedFaceSet / IndexedLineSet coord with empty coordIndex
// X3DJSAIL: InvalidFieldException: IndexedFaceSet/IndexedLineSet containing
// Coordinate node with N values must also include coordIndex field.
// When a coord Coordinate node is present with point data but coordIndex is
// empty, the geometry is ill-formed.
static void walkForCoordCheck(const X3DNode &node,
                              std::vector<Diag> &diags,
                              std::unordered_set<const X3DNode *> &visited) {
    if (!visited.insert(&node).second) return; // cycle guard

    const std::string typeName = node.nodeTypeName();

    if (typeName == "IndexedFaceSet") {
        const auto *ifs = dynamic_cast<const x3d::nodes::IndexedFaceSet *>(&node);
        if (ifs) {
            const auto coordNode = ifs->getCoord();
            const auto &coordIdx = ifs->getCoordIndex();
            if (coordNode && coordIdx.empty()) {
                // Only flag when the Coordinate actually has point data;
                // an empty Coordinate element (no points) is not an error.
                const auto *coord = dynamic_cast<const x3d::nodes::Coordinate *>(coordNode.get());
                if (coord && !coord->getPoint().empty()) {
                    std::string msg = "IndexedFaceSet has Coordinate with "
                                    + std::to_string(coord->getPoint().size())
                                    + " point(s) but empty coordIndex"
                                    + " — coordIndex is required when coord is set";
                    diags.push_back({"ifs-coord-index", "error", msg});
                }
            }
        }
    } else if (typeName == "IndexedLineSet") {
        const auto *ils = dynamic_cast<const x3d::nodes::IndexedLineSet *>(&node);
        if (ils) {
            const auto coordNode = ils->getCoord();
            const auto &coordIdx = ils->getCoordIndex();
            if (coordNode && coordIdx.empty()) {
                const auto *coord = dynamic_cast<const x3d::nodes::Coordinate *>(coordNode.get());
                if (coord && !coord->getPoint().empty()) {
                    std::string msg = "IndexedLineSet has Coordinate with "
                                    + std::to_string(coord->getPoint().size())
                                    + " point(s) but empty coordIndex"
                                    + " — coordIndex is required when coord is set";
                    diags.push_back({"ils-coord-index", "error", msg});
                }
            }
        }
    }

    // Recurse into child SFNode/MFNode fields.
    for (const FieldInfo &f : node.fields()) {
        if (!f.isNode() || !f.isReadable()) continue;
        std::any v = f.get(node);
        if (f.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
            if (child) walkForCoordCheck(*child, diags, visited);
        } else {
            auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
            for (const auto &child : vec)
                if (child) walkForCoordCheck(*child, diags, visited);
        }
    }
    visited.erase(&node);
}

inline void checkIndexedGeometryCoordIndex(const sdk::X3DDocument &doc,
                                    std::vector<Diag> &diags) {
    // This check applies only to X3D 4.0+; in older versions an empty coordIndex
    // is valid (renders nothing). Parsing normalises missing attributes to empty
    // arrays, so we cannot distinguish "missing" from "explicitly empty" for
    // older files — restrict to 4.0 to avoid false positives.
    const std::string &ver = doc.version;
    if (ver.empty() || ver < "4.0") return;

    std::unordered_set<const X3DNode *> visited;
    for (const auto &root : doc.scene.rootNodes) {
        if (root) walkForCoordCheck(*root, diags, visited);
    }
}

// Collect parser diagnostics and semantic conformance checks in report order.
// Check: ROUTE endpoints. A ROUTE naming a DEF that does not exist is skipped
// silently by the runtime bridge; one with an unknown field, wrong access type
// or mismatched field types is rejected. Both are errors in the document.
// IMPORTed names may legitimately stay unresolved until their Inline loads.
inline void checkRoutes(sdk::X3DDocument &doc, std::vector<Diag> &diags) {
    using x3d::runtime::Route;
    using x3d::runtime::RouteError;
    auto &scene = doc.scene;
    std::unordered_set<std::string> imported;
    for (const auto &im : scene.imports)
        imported.insert(im.as.empty() ? im.importedDEF : im.as);
    auto label = [](const Route &r) {
        return "ROUTE " + r.fromNode + "." + r.fromField + " TO " + r.toNode +
               "." + r.toField;
    };
    scene.resolveRoutes();
    for (const auto &r : scene.routes) {
        if (r.binding != Route::Binding::DefNames) continue;
        if (r.from.expired() && !imported.count(r.fromNode))
            diags.push_back({"route", "error",
                             label(r) + ": no node is DEF'd as '" + r.fromNode + "'"});
        if (r.to.expired() && !imported.count(r.toNode))
            diags.push_back({"route", "error",
                             label(r) + ": no node is DEF'd as '" + r.toNode + "'"});
    }
    sdk::X3DExecutionContext ctx(scene.authorFields);
    const auto result = x3d::runtime::buildRoutes(scene, ctx);
    for (const auto &e : result.rejected) {
        std::string where;
        if (e.scope == RouteError::Scope::Scene && e.index < scene.routes.size())
            where = label(scene.routes[e.index]);
        else
            where = std::string(e.scope == RouteError::Scope::ProtoBody
                                    ? "PROTO body ROUTE #"
                                    : "Inline ROUTE #") +
                    std::to_string(e.index);
        diags.push_back({"route", "error", where + ": " + e.reason});
    }
}

inline void collectDiagnostics(const sdk::X3DDocument &doc, std::vector<Diag> &diags) {
    // 1. Range diagnostics.
    for (const auto &w : doc.rangeWarnings) {
        diags.push_back({"range", "warning", w.message()});
    }

    // 2. Proto warnings.
    for (const auto &w : doc.protoWarnings) {
        diags.push_back({"proto", "warning", w.instanceName + ": " + w.detail});
    }

    // 3. Inline warnings.
    for (const auto &w : doc.inlineWarnings) {
        std::string msg = w.detail;
        if (!w.inlineDEF.empty()) msg = "Inline DEF=" + w.inlineDEF + ": " + msg;
        diags.push_back({"inline", "warning", msg});
    }

    // 3b. Reader-recovery diagnostics (unknown node element, coerced profile,
    // coerced value, dropped cyclic USE).
    for (const auto &w : doc.readerWarnings) {
        using Kind = x3d::runtime::ReaderWarning::Kind;
        const char *cat = w.kind == Kind::UnknownNode ? "node"
                          : w.kind == Kind::Value     ? "value"
                          : w.kind == Kind::CyclicUse ? "use"
                                                      : "profile";
        diags.push_back({cat, "warning", w.detail});
    }

    checkDuplicateMeta(doc, diags);
    checkUnusedProtoDeclare(doc, diags);
    checkIndexedGeometryCoordIndex(doc, diags);
}

} // namespace x3d::cli_detail
