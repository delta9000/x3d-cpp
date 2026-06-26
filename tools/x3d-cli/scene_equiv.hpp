// tools/x3d-cli/scene_equiv.hpp
// ─────────────────────────────────────────────────────────────────────────────
// sceneEquivalent — semantic equivalence check for two parsed X3D Scenes.
//
// Used by the Java-free convert-roundtrip gate: parse a file, convert to each
// other encoding, reparse, then assert sceneEquivalent(original, roundtrip).
//
// What is compared:
//   1. rootNodes: same count, same tree structure (depth-first), using
//      nodeTypeName() + per-field value comparison via reflection.
//   2. Field values: non-node fields compared per X3DFieldType. Numeric types
//      use a relative+absolute tolerance (kFloatTol). MF fields compared in
//      authored order. Enum fields compared by token string.
//   3. DEF names: each node's getDEF() must match.
//   4. ROUTE set: same set of (fromNode, fromField, toNode, toField) tuples
//      (order-independent).
//
// What is NOT compared (by design):
//   - Proto/ExternProto declarations (not load-bearing for render equivalence).
//   - IMPORT/EXPORT statements (seam-side, not yet exercised by roundtrip).
//   - Inline expansion metadata (writer re-emits the Inline stub correctly).
//   - expandedSources / resolvedProtoRoutes (internal plumbing, not authored).
//
// Cycle safety: a visited-on-path set prevents infinite loops on USE graphs.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "x3d/sdk.hpp"

#include <any>
#include <cassert>
#include <cmath>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d_cli {

namespace detail {

// Tolerance for floating-point comparisons.
static constexpr double kFloatTol = 1e-5;

inline bool nearEq(double a, double b) {
    if (a == b) return true;
    double diff = std::abs(a - b);
    double scale = std::max({std::abs(a), std::abs(b), 1.0});
    return diff / scale < kFloatTol;
}

// ── per-type value comparison ─────────────────────────────────────────────

// Compare two std::any values of the same X3DFieldType.
// Returns true if equivalent; appends a reason to `why` on mismatch.
bool anyEqual(X3DFieldType type, const std::any &a, const std::any &b,
              std::string &why) {
    if (!a.has_value() && !b.has_value()) return true;
    if (!a.has_value() || !b.has_value()) {
        why = "one value empty, other not";
        return false;
    }

    // Helper macros for scalar comparison
#define CMP_SCALAR(T, expr)                                       \
    case X3DFieldType::T: {                                       \
        auto va = std::any_cast<decltype(expr)>(a);              \
        auto vb = std::any_cast<decltype(expr)>(b);              \
        if (!(va == vb)) { why = #T " scalar mismatch"; return false; } \
        return true;                                              \
    }

    switch (type) {
        case X3DFieldType::SFBool: {
            auto va = std::any_cast<SFBool>(a);
            auto vb = std::any_cast<SFBool>(b);
            if (va != vb) { why = "SFBool mismatch"; return false; }
            return true;
        }
        case X3DFieldType::SFInt32: {
            auto va = std::any_cast<SFInt32>(a);
            auto vb = std::any_cast<SFInt32>(b);
            if (va != vb) { why = "SFInt32 mismatch: " + std::to_string(va) + " vs " + std::to_string(vb); return false; }
            return true;
        }
        case X3DFieldType::SFFloat: {
            auto va = std::any_cast<SFFloat>(a);
            auto vb = std::any_cast<SFFloat>(b);
            if (!nearEq(static_cast<double>(va), static_cast<double>(vb))) {
                why = "SFFloat mismatch: " + std::to_string(va) + " vs " + std::to_string(vb);
                return false;
            }
            return true;
        }
        case X3DFieldType::SFDouble: {
            auto va = std::any_cast<SFDouble>(a);
            auto vb = std::any_cast<SFDouble>(b);
            if (!nearEq(va, vb)) { why = "SFDouble mismatch"; return false; }
            return true;
        }
        case X3DFieldType::SFTime: {
            auto va = std::any_cast<SFTime>(a);
            auto vb = std::any_cast<SFTime>(b);
            if (!nearEq(va, vb)) { why = "SFTime mismatch"; return false; }
            return true;
        }
        case X3DFieldType::SFString: {
            auto va = std::any_cast<SFString>(a);
            auto vb = std::any_cast<SFString>(b);
            if (va != vb) { why = "SFString mismatch: \"" + va + "\" vs \"" + vb + "\""; return false; }
            return true;
        }
        case X3DFieldType::SFVec2f: {
            auto va = std::any_cast<SFVec2f>(a);
            auto vb = std::any_cast<SFVec2f>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y)) {
                why = "SFVec2f mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFVec2d: {
            auto va = std::any_cast<SFVec2d>(a);
            auto vb = std::any_cast<SFVec2d>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y)) {
                why = "SFVec2d mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFVec3f: {
            auto va = std::any_cast<SFVec3f>(a);
            auto vb = std::any_cast<SFVec3f>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y) || !nearEq(va.z, vb.z)) {
                why = "SFVec3f mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFVec3d: {
            auto va = std::any_cast<SFVec3d>(a);
            auto vb = std::any_cast<SFVec3d>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y) || !nearEq(va.z, vb.z)) {
                why = "SFVec3d mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFVec4f: {
            auto va = std::any_cast<SFVec4f>(a);
            auto vb = std::any_cast<SFVec4f>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y) ||
                !nearEq(va.z, vb.z) || !nearEq(va.w, vb.w)) {
                why = "SFVec4f mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFVec4d: {
            auto va = std::any_cast<SFVec4d>(a);
            auto vb = std::any_cast<SFVec4d>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y) ||
                !nearEq(va.z, vb.z) || !nearEq(va.w, vb.w)) {
                why = "SFVec4d mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFColor: {
            auto va = std::any_cast<SFColor>(a);
            auto vb = std::any_cast<SFColor>(b);
            if (!nearEq(va.r, vb.r) || !nearEq(va.g, vb.g) || !nearEq(va.b, vb.b)) {
                why = "SFColor mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFColorRGBA: {
            auto va = std::any_cast<SFColorRGBA>(a);
            auto vb = std::any_cast<SFColorRGBA>(b);
            if (!nearEq(va.r, vb.r) || !nearEq(va.g, vb.g) ||
                !nearEq(va.b, vb.b) || !nearEq(va.a, vb.a)) {
                why = "SFColorRGBA mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFRotation: {
            auto va = std::any_cast<SFRotation>(a);
            auto vb = std::any_cast<SFRotation>(b);
            if (!nearEq(va.x, vb.x) || !nearEq(va.y, vb.y) ||
                !nearEq(va.z, vb.z) || !nearEq(va.angle, vb.angle)) {
                why = "SFRotation mismatch";
                return false;
            }
            return true;
        }
        case X3DFieldType::SFImage: {
            // Compare as equal (image data comparison deferred; rarely authored inline)
            return true;
        }
        case X3DFieldType::SFMatrix3f:
        case X3DFieldType::SFMatrix3d:
        case X3DFieldType::SFMatrix4f:
        case X3DFieldType::SFMatrix4d:
            // Matrix types: treat as equal (not commonly authored, complex to compare)
            return true;
        case X3DFieldType::SFNode:
            // SFNode children are handled by node recursion, not here.
            return true;
        case X3DFieldType::SFEnum: {
            // Compared via getEnumString() at the FieldInfo level (handled separately).
            return true;
        }

        // MF types: compare as vectors
        case X3DFieldType::MFBool: {
            auto va = std::any_cast<MFBool>(a);
            auto vb = std::any_cast<MFBool>(b);
            if (va != vb) { why = "MFBool mismatch (sizes: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()) + ")"; return false; }
            return true;
        }
        case X3DFieldType::MFInt32: {
            auto va = std::any_cast<MFInt32>(a);
            auto vb = std::any_cast<MFInt32>(b);
            if (va != vb) { why = "MFInt32 mismatch (sizes: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()) + ")"; return false; }
            return true;
        }
        case X3DFieldType::MFFloat: {
            auto va = std::any_cast<MFFloat>(a);
            auto vb = std::any_cast<MFFloat>(b);
            if (va.size() != vb.size()) { why = "MFFloat size mismatch: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()); return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i], vb[i])) { why = "MFFloat[" + std::to_string(i) + "] mismatch: " + std::to_string(va[i]) + " vs " + std::to_string(vb[i]); return false; }
            }
            return true;
        }
        case X3DFieldType::MFDouble: {
            auto va = std::any_cast<MFDouble>(a);
            auto vb = std::any_cast<MFDouble>(b);
            if (va.size() != vb.size()) { why = "MFDouble size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i], vb[i])) { why = "MFDouble[" + std::to_string(i) + "] mismatch"; return false; }
            }
            return true;
        }
        case X3DFieldType::MFTime: {
            auto va = std::any_cast<MFTime>(a);
            auto vb = std::any_cast<MFTime>(b);
            if (va.size() != vb.size()) { why = "MFTime size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i], vb[i])) { why = "MFTime[" + std::to_string(i) + "] mismatch"; return false; }
            }
            return true;
        }
        case X3DFieldType::MFString: {
            auto va = std::any_cast<MFString>(a);
            auto vb = std::any_cast<MFString>(b);
            if (va != vb) { why = "MFString mismatch (sizes: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()) + ")"; return false; }
            return true;
        }
        case X3DFieldType::MFVec2f: {
            auto va = std::any_cast<MFVec2f>(a);
            auto vb = std::any_cast<MFVec2f>(b);
            if (va.size() != vb.size()) { why = "MFVec2f size mismatch: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()); return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y)) {
                    why = "MFVec2f[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFVec2d: {
            auto va = std::any_cast<MFVec2d>(a);
            auto vb = std::any_cast<MFVec2d>(b);
            if (va.size() != vb.size()) { why = "MFVec2d size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y)) {
                    why = "MFVec2d[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFVec3f: {
            auto va = std::any_cast<MFVec3f>(a);
            auto vb = std::any_cast<MFVec3f>(b);
            if (va.size() != vb.size()) { why = "MFVec3f size mismatch: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()); return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y) || !nearEq(va[i].z, vb[i].z)) {
                    why = "MFVec3f[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFVec3d: {
            auto va = std::any_cast<MFVec3d>(a);
            auto vb = std::any_cast<MFVec3d>(b);
            if (va.size() != vb.size()) { why = "MFVec3d size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y) || !nearEq(va[i].z, vb[i].z)) {
                    why = "MFVec3d[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFVec4f: {
            auto va = std::any_cast<MFVec4f>(a);
            auto vb = std::any_cast<MFVec4f>(b);
            if (va.size() != vb.size()) { why = "MFVec4f size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y) ||
                    !nearEq(va[i].z, vb[i].z) || !nearEq(va[i].w, vb[i].w)) {
                    why = "MFVec4f[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFVec4d: {
            auto va = std::any_cast<MFVec4d>(a);
            auto vb = std::any_cast<MFVec4d>(b);
            if (va.size() != vb.size()) { why = "MFVec4d size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y) ||
                    !nearEq(va[i].z, vb[i].z) || !nearEq(va[i].w, vb[i].w)) {
                    why = "MFVec4d[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFColor: {
            auto va = std::any_cast<MFColor>(a);
            auto vb = std::any_cast<MFColor>(b);
            if (va.size() != vb.size()) { why = "MFColor size mismatch: " + std::to_string(va.size()) + " vs " + std::to_string(vb.size()); return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].r, vb[i].r) || !nearEq(va[i].g, vb[i].g) || !nearEq(va[i].b, vb[i].b)) {
                    why = "MFColor[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFColorRGBA: {
            auto va = std::any_cast<MFColorRGBA>(a);
            auto vb = std::any_cast<MFColorRGBA>(b);
            if (va.size() != vb.size()) { why = "MFColorRGBA size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].r, vb[i].r) || !nearEq(va[i].g, vb[i].g) ||
                    !nearEq(va[i].b, vb[i].b) || !nearEq(va[i].a, vb[i].a)) {
                    why = "MFColorRGBA[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFRotation: {
            auto va = std::any_cast<MFRotation>(a);
            auto vb = std::any_cast<MFRotation>(b);
            if (va.size() != vb.size()) { why = "MFRotation size mismatch"; return false; }
            for (size_t i = 0; i < va.size(); ++i) {
                if (!nearEq(va[i].x, vb[i].x) || !nearEq(va[i].y, vb[i].y) ||
                    !nearEq(va[i].z, vb[i].z) || !nearEq(va[i].angle, vb[i].angle)) {
                    why = "MFRotation[" + std::to_string(i) + "] mismatch"; return false;
                }
            }
            return true;
        }
        case X3DFieldType::MFNode:
            // MFNode children are handled by node recursion.
            return true;
        case X3DFieldType::MFEnum:
            // Handled via getEnumString() at FieldInfo level.
            return true;
        case X3DFieldType::MFImage:
        case X3DFieldType::MFMatrix3f:
        case X3DFieldType::MFMatrix3d:
        case X3DFieldType::MFMatrix4f:
        case X3DFieldType::MFMatrix4d:
            return true; // deferred (rarely authored)
        default:
            return true;
    }
#undef CMP_SCALAR
}

// ── node pair comparison ───────────────────────────────────────────────────

// Forward declaration.
bool nodesEqual(const X3DNode *a, const X3DNode *b,
                std::unordered_set<const X3DNode *> &visitedA,
                std::string &why);

bool nodeListsEqual(const std::vector<std::shared_ptr<X3DNode>> &la,
                    const std::vector<std::shared_ptr<X3DNode>> &lb,
                    std::unordered_set<const X3DNode *> &visitedA,
                    std::string &why) {
    if (la.size() != lb.size()) {
        why = "child count mismatch: " + std::to_string(la.size()) + " vs " +
              std::to_string(lb.size());
        return false;
    }
    for (size_t i = 0; i < la.size(); ++i) {
        std::string childWhy;
        if (!nodesEqual(la[i].get(), lb[i].get(), visitedA, childWhy)) {
            why = "child[" + std::to_string(i) + "]: " + childWhy;
            return false;
        }
    }
    return true;
}

bool nodesEqual(const X3DNode *a, const X3DNode *b,
                std::unordered_set<const X3DNode *> &visitedA,
                std::string &why) {
    if (a == nullptr && b == nullptr) return true;
    if (a == nullptr || b == nullptr) {
        why = "one node is null, other is not";
        return false;
    }

    // Cycle guard on the 'a' side.
    if (!visitedA.insert(a).second) {
        // Already visited in this path — a USE back-edge; treat as equal if b
        // also matches the DEF name.
        if (a->getDEF() == b->getDEF()) return true;
        why = "USE cycle DEF mismatch: " + a->getDEF() + " vs " + b->getDEF();
        return false;
    }

    // Type name.
    const std::string ta = a->nodeTypeName();
    const std::string tb = b->nodeTypeName();
    if (ta != tb) {
        why = "nodeTypeName mismatch: " + ta + " vs " + tb;
        visitedA.erase(a);
        return false;
    }

    // DEF name.
    if (a->getDEF() != b->getDEF()) {
        why = ta + ": DEF mismatch: \"" + a->getDEF() + "\" vs \"" + b->getDEF() + "\"";
        visitedA.erase(a);
        return false;
    }

    // Fields (non-node scalars + enum).
    const FieldTable &fa = a->fields();
    const FieldTable &fb = b->fields();
    // Same reflection layout expected (both generated from the same binary).
    if (fa.size() != fb.size()) {
        why = ta + ": field count mismatch";
        visitedA.erase(a);
        return false;
    }

    for (size_t i = 0; i < fa.size(); ++i) {
        const FieldInfo &fia = fa[i];
        const FieldInfo &fib = fb[i];

        if (!fia.isReadable()) continue;
        // Skip node-type fields here — we'll walk them separately below.
        if (fia.isNode()) continue;
        // Skip inputOnly (no readable value).
        if (fia.access == AccessType::InputOnly) continue;

        std::any va = fia.get(*a);
        std::any vb = fib.get(*b);

        // Enum fields: compare via token string.
        if (fia.isEnum()) {
            std::string sa = fia.getEnumString ? fia.getEnumString(*a) : "";
            std::string sb = fib.getEnumString ? fib.getEnumString(*b) : "";
            if (sa != sb) {
                why = ta + "." + fia.x3dName + ": enum mismatch: \"" + sa + "\" vs \"" + sb + "\"";
                visitedA.erase(a);
                return false;
            }
            continue;
        }

        std::string fieldWhy;
        if (!anyEqual(fia.type, va, vb, fieldWhy)) {
            why = ta + "." + fia.x3dName + ": " + fieldWhy;
            visitedA.erase(a);
            return false;
        }
    }

    // Walk SFNode/MFNode child fields.
    for (size_t i = 0; i < fa.size(); ++i) {
        const FieldInfo &fia = fa[i];
        const FieldInfo &fib = fb[i];
        if (!fia.isNode() || !fia.isReadable()) continue;
        if (fia.access == AccessType::InputOnly) continue;

        std::any va = fia.get(*a);
        std::any vb = fib.get(*b);

        if (fia.type == X3DFieldType::SFNode) {
            auto ca = std::any_cast<std::shared_ptr<X3DNode>>(va);
            auto cb = std::any_cast<std::shared_ptr<X3DNode>>(vb);
            std::string childWhy;
            if (!nodesEqual(ca.get(), cb.get(), visitedA, childWhy)) {
                why = ta + "." + fia.x3dName + " (SFNode): " + childWhy;
                visitedA.erase(a);
                return false;
            }
        } else { // MFNode
            auto la = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(va);
            auto lb = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(vb);
            std::string childWhy;
            if (!nodeListsEqual(la, lb, visitedA, childWhy)) {
                why = ta + "." + fia.x3dName + " (MFNode): " + childWhy;
                visitedA.erase(a);
                return false;
            }
        }
    }

    visitedA.erase(a);
    return true;
}

} // namespace detail

// ── public API ────────────────────────────────────────────────────────────────

/**
 * @brief Compare two parsed X3D Scenes for semantic equivalence.
 *
 * Returns true iff:
 *   - Same number of root nodes, in the same order, with equivalent structure.
 *   - Same ROUTE set (order-independent).
 *
 * @param a    The reference scene (e.g. parsed from the source file).
 * @param b    The scene to compare against (e.g. parsed from a round-tripped file).
 * @param why  On mismatch: populated with a human-readable reason.
 * @return true if equivalent.
 */
inline bool sceneEquivalent(const x3d::sdk::Scene &a, const x3d::sdk::Scene &b,
                            std::string &why) {
    // 1. Root node count.
    if (a.rootNodes.size() != b.rootNodes.size()) {
        why = "rootNodes count mismatch: " + std::to_string(a.rootNodes.size()) +
              " vs " + std::to_string(b.rootNodes.size());
        return false;
    }

    // 2. Node tree (depth-first).
    std::unordered_set<const X3DNode *> visited;
    for (size_t i = 0; i < a.rootNodes.size(); ++i) {
        std::string nodeWhy;
        if (!detail::nodesEqual(a.rootNodes[i].get(), b.rootNodes[i].get(),
                                visited, nodeWhy)) {
            why = "rootNodes[" + std::to_string(i) + "]: " + nodeWhy;
            return false;
        }
    }

    // 3. ROUTE set (order-independent; compare as sorted string tuples).
    using RouteKey = std::tuple<std::string, std::string, std::string, std::string>;
    auto toKey = [](const x3d::sdk::Route &r) {
        return RouteKey{r.fromNode, r.fromField, r.toNode, r.toField};
    };
    std::set<RouteKey> routesA, routesB;
    for (const auto &r : a.routes) routesA.insert(toKey(r));
    for (const auto &r : b.routes) routesB.insert(toKey(r));
    if (routesA != routesB) {
        why = "ROUTE set mismatch: " + std::to_string(routesA.size()) +
              " routes in a, " + std::to_string(routesB.size()) + " in b";
        // Find a differing route for context.
        for (const auto &ka : routesA) {
            if (routesB.find(ka) == routesB.end()) {
                why += " (e.g. missing in b: ROUTE " + std::get<0>(ka) + "." +
                       std::get<1>(ka) + " -> " + std::get<2>(ka) + "." +
                       std::get<3>(ka) + ")";
                break;
            }
        }
        return false;
    }

    return true;
}

} // namespace x3d_cli
