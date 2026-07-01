// tools/x3d-cli/profile_fit.hpp
// ─────────────────────────────────────────────────────────────────────────────
// Profile-fit machinery — promoted out of tools/x3d_cli.cpp (see task-2-report)
// so it can be reused by other tools/consumers (e.g. the asset-import
// consumer), not just the CLI's `validate --profile-fit` flag.
//
// Each X3D 4.0 profile is defined by a set of (component, maxLevel) pairs.
// A scene "fits" a profile if every (component, level) it uses is ≤ the
// profile's allowed level for that component (and the component is listed at all).
//
// Profile table sources:
//   Interchange   — ISO/IEC 19775-1:2023 Annex B.5
//   CADInterchange — ISO/IEC 19775-1:2023 Annex C (informative)
//   Interactive   — ISO/IEC 19775-1:2023 Annex C.5
//   Immersive     — ISO/IEC 19775-1:2023 Annex D.5 / E.5
//   Full          — ISO/IEC 19775-1:2023 Annex F.5 (all components)
//
// LIMITATION: This table was assembled from the spec profile appendices.
// Components not listed in a profile are not allowed at any level for that
// profile. "Full" allows all known components at their maximum supported level;
// any component not in the table is treated as requiring Full.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_CLI_PROFILE_FIT_HPP
#define X3D_CLI_PROFILE_FIT_HPP

// x3d/sdk.hpp pulls in x3d::runtime::Scene/Profile (as sdk::Scene/sdk::Profile)
// plus x3d::nodes::X3DNode and the x3d::core reflection types (FieldInfo,
// X3DFieldType) used below. The generated *.gen.inc fragments included further
// down were emitted with literal `sdk::Profile::...` tokens (see
// scripts/gen_profile_tables.py), so the `sdk` alias must be in scope here.
#include "x3d/sdk.hpp"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sdk = x3d::sdk;

// Brings X3DNode, FieldInfo, X3DFieldType into unqualified scope (matches the
// convention already used at tools/x3d_cli.cpp file scope).
using namespace x3d::core;

namespace profile_fit {

// (component, max allowed level) pairs per profile.
// Profiles are ordered from smallest to largest for minimal-profile search.
//
// Both tables below are GENERATED FROM THE UOM by scripts/gen_profile_tables.py
// and included as golden-tracked fragments (tools/x3d-cli/*.gen.inc) so they can
// never drift from the spec: node→component comes from each node's <componentInfo>,
// and each profile→level table is derived as the max component level over that
// profile's UOM allowed-node list. Regenerate with `mise run gen`.

using ComponentMap = std::unordered_map<std::string, int>;

// Returns the profile component maps. Initialised once (function-local static).
struct ProfileDef {
    sdk::Profile profile;
    std::string  name;
    ComponentMap allowed; // component name → max allowed level
};

const std::vector<ProfileDef> &profileDefs() {
    // clang-format off
    static const std::vector<ProfileDef> kDefs = {
        #include "profile_defs.gen.inc"
    };
    // clang-format on
    return kDefs;
}

// Returns true if (component, level) is allowed in the given profile def.
bool allowedInProfile(const ProfileDef &prof, const std::string &comp, int level) {
    auto it = prof.allowed.find(comp);
    if (it == prof.allowed.end()) return false;
    return level <= it->second;
}

// Walk the scene graph, depth-first, collecting (nodeTypeName, component, level)
// for every concrete node encountered. Uses reflection to find SFNode/MFNode
// children. Cycle-safe (same guard as collectRangeWarnings).
struct NodeUsage {
    std::string typeName;
    std::string component;
    int         level;
};

// Static node-type → (component, level) lookup table, GENERATED FROM THE UOM by
// scripts/gen_profile_tables.py (node_component_table.gen.inc); covers all 4.0
// standard concrete nodes. ProtoInstance = Core:2. Abstract base types and custom
// PROTO instances are not in the table; PROTOs expand to their body nodes.
const std::unordered_map<std::string, std::pair<std::string, int>> &nodeComponentTable() {
    static const std::unordered_map<std::string, std::pair<std::string, int>> kTable = {
        #include "node_component_table.gen.inc"
    };
    return kTable;
}

// Walk the scene graph, collecting the maximum level used per component.
// Uses virtual nodeTypeName() + static table for (component,level).
// Cycle-safe via the same visited-on-path guard as collectRangeWarnings.
using ComponentUsage = std::map<std::string, int>; // component → max level used

void collectComponentUsage(const X3DNode &node,
                           ComponentUsage &usage,
                           std::unordered_set<const X3DNode *> &onPath) {
    if (!onPath.insert(&node).second)
        return; // cycle

    const std::string typeName = node.nodeTypeName();
    const auto &table = nodeComponentTable();
    auto it = table.find(typeName);
    if (it != table.end()) {
        const std::string &comp = it->second.first;
        int lvl = it->second.second;
        auto &cur = usage[comp];
        if (lvl > cur) cur = lvl;
    }
    // else: unknown type (e.g. abstract base, custom PROTO instance not in table)
    // → treated as "unknown component"; will force Full if not in Full either.

    for (const FieldInfo &f : node.fields()) {
        if (!f.isNode() || !f.isReadable()) continue;
        std::any v = f.get(node);
        if (f.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
            if (child) collectComponentUsage(*child, usage, onPath);
        } else {
            auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
            for (const auto &child : vec)
                if (child) collectComponentUsage(*child, usage, onPath);
        }
    }
    onPath.erase(&node);
}

ComponentUsage sceneComponentUsage(const sdk::Scene &scene) {
    ComponentUsage usage;
    std::unordered_set<const X3DNode *> onPath;
    for (const auto &root : scene.rootNodes)
        if (root) collectComponentUsage(*root, usage, onPath);
    return usage;
}

// Find the minimal profile that contains all (component, level) in usage.
// Returns the ProfileDef if found, or nullptr if nothing fits (shouldn't happen
// since Full covers everything in the table).
const profile_fit::ProfileDef *findMinimalProfile(const ComponentUsage &usage) {
    for (const auto &prof : profileDefs()) {
        bool fits = true;
        for (const auto &[comp, lvl] : usage) {
            if (!allowedInProfile(prof, comp, lvl)) { fits = false; break; }
        }
        if (fits) return &prof;
    }
    return nullptr; // usage has a component not in Full (e.g. unknown type)
}

// (profileExceedances removed: dead — no callers after the profile-fit emit
// was inlined directly into the CLI flag path.)

// Find which node type names from the scene map to a given component.
std::vector<std::string> nodeTypeNamesForComponent(const sdk::Scene &scene,
                                                    const std::string &comp) {
    const auto &table = nodeComponentTable();
    std::set<std::string> found;
    std::function<void(const X3DNode &, std::unordered_set<const X3DNode *> &)> walk;
    walk = [&](const X3DNode &node, std::unordered_set<const X3DNode *> &onPath) {
        if (!onPath.insert(&node).second) return;
        const std::string tname = node.nodeTypeName();
        auto it = table.find(tname);
        if (it != table.end() && it->second.first == comp)
            found.insert(tname);
        for (const FieldInfo &f : node.fields()) {
            if (!f.isNode() || !f.isReadable()) continue;
            std::any v = f.get(node);
            if (f.type == X3DFieldType::SFNode) {
                auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
                if (child) walk(*child, onPath);
            } else {
                auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
                for (const auto &child : vec) if (child) walk(*child, onPath);
            }
        }
        onPath.erase(&node);
    };
    std::unordered_set<const X3DNode *> onPath;
    for (const auto &root : scene.rootNodes) if (root) walk(*root, onPath);
    return {found.begin(), found.end()};
}

} // namespace profile_fit

#endif // X3D_CLI_PROFILE_FIT_HPP
