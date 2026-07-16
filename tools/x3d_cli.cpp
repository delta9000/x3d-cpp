// tools/x3d_cli.cpp
// ─────────────────────────────────────────────────────────────────────────────
// x3d — thin CLI over the x3d::sdk façade.
//
// Shape: x3d <subcommand> [args]
//   x3d --help / -h                 list subcommands
//   x3d <cmd> --help                subcommand help
//   x3d convert <in> [-o <out>] [-f xml|vrml|json]
//   x3d validate <in> [--json]
//
// Subcommands are registered in a dispatch table (see SubCmd); adding a new
// command requires only a new entry in kCmds + a Cmd* impl.
//
// Exit codes: 0 = ok, 1 = usage error, 2 = parse/IO failure, 3 = issues found.
//
// Encoding inference:
//   Input:  parseFile() sniffs encoding; gzip transparent (SDK handles it).
//   Output: inferred from -o extension: .x3d→XML, .x3dv→ClassicVRML, .json→JSON
//           (.wrl is not supported by VrmlWriter which emits ClassicVRML only;
//            see limitation note in convert --help).
//           Override with -f xml|vrml|json.
// ─────────────────────────────────────────────────────────────────────────────
#include "x3d/sdk.hpp"

// Generated node headers needed for concrete-type validation checks.
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/IndexedLineSet.hpp"

// STL writer (binary-STL export; core, NOT ext-gated).
#include "stl_write.hpp"

// sim subcommand: full-runtime wiring helper + field-change tracer.
#include "sim_runtime.hpp"
#include "sim_tracer.hpp"

// Profile-fit machinery (used by `validate --profile-fit`); promoted to a
// reusable header so other tools/consumers can compute the narrowest profile.
#include "x3d-cli/profile_fit.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;
namespace sdk = x3d::sdk;

using namespace x3d::core;

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace {

std::string toLower(std::string s) {
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Returns the lower-cased extension including the dot, e.g. ".x3d".
std::string ext(const std::string &path) {
    return toLower(fs::path(path).extension().string());
}

// Write bytes to a file, or to stdout if path is empty.
void writeOutput(const std::string &out, const std::string &content) {
    if (out.empty()) {
        std::cout << content;
    } else {
        std::ofstream ofs(out, std::ios::binary | std::ios::trunc);
        if (!ofs)
            throw std::runtime_error("cannot open output file: " + out);
        ofs << content;
    }
}

// Emit each range/proto warning to stderr (non-fatal).
void surfaceWarnings(const sdk::X3DDocument &doc) {
    for (const auto &w : doc.rangeWarnings)
        std::cerr << "warning: out-of-range: " << w.message() << "\n";
    for (const auto &w : doc.protoWarnings)
        std::cerr << "warning: proto/inline: " << w.detail << "\n";
}

// ─── output format tag ───────────────────────────────────────────────────────

enum class OutFmt { XML, ClassicVRML, JSON, Unknown };

OutFmt fmtFromExt(const std::string &e) {
    if (e == ".x3d")  return OutFmt::XML;
    if (e == ".x3dv") return OutFmt::ClassicVRML;
    if (e == ".json") return OutFmt::JSON;
    // .wrl: the SDK VrmlWriter only emits ClassicVRML, not VRML97.
    // Treat .wrl as ClassicVRML (the closest we support) and note this.
    if (e == ".wrl")  return OutFmt::ClassicVRML;
    return OutFmt::Unknown;
}

OutFmt fmtFromFlag(const std::string &f) {
    const std::string lf = toLower(f);
    if (lf == "xml")  return OutFmt::XML;
    if (lf == "vrml") return OutFmt::ClassicVRML;
    if (lf == "json") return OutFmt::JSON;
    return OutFmt::Unknown;
}

// ─── subcommand registry ─────────────────────────────────────────────────────

// A subcommand: name + one-liner + a handler.
// The handler receives the argv tail after the subcommand name.
// Returns an exit code.
struct SubCmd {
    std::string name;
    std::string brief;
    std::function<int(const std::vector<std::string> &)> run;
};

// ─── convert ─────────────────────────────────────────────────────────────────

int cmdConvert(const std::vector<std::string> &args) {
    // x3d convert <in> [-o <out>] [-f xml|vrml|json]
    static const char *kUsage =
        "usage: x3d convert <input> [-o <output>] [-f xml|vrml|json]\n"
        "\n"
        "  Convert an X3D scene between encodings.\n"
        "\n"
        "  Input encoding is inferred automatically (XML/ClassicVRML/VRML97/JSON;\n"
        "  gzip variants .x3dz/.x3dvz/.wrz are transparently decompressed).\n"
        "\n"
        "  Output encoding is inferred from <output>'s extension:\n"
        "    .x3d   → X3D-XML\n"
        "    .x3dv  → ClassicVRML (X3D ISO 19776-2)\n"
        "    .json  → X3D-JSON\n"
        "    .wrl   → ClassicVRML (LIMITATION: VrmlWriter emits ClassicVRML only;\n"
        "                          true VRML97 dialect not yet supported)\n"
        "  Override with -f xml|vrml|json.\n"
        "  If no -o and no -f, output is written to stdout and format must be set"
        " via -f.\n"
        "\n"
        "  Exit codes: 0 ok, 1 usage error, 2 parse/IO failure.\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    std::string outPath;
    std::string fmtFlag;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        if (a == "-o") {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: -o requires an argument\n" << kUsage;
                return 1;
            }
            outPath = args[static_cast<size_t>(++i)];
        } else if (a == "-f") {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: -f requires an argument\n" << kUsage;
                return 1;
            }
            fmtFlag = args[static_cast<size_t>(++i)];
        } else if (a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }

    // Resolve output format.
    OutFmt fmt = OutFmt::Unknown;
    if (!fmtFlag.empty()) {
        fmt = fmtFromFlag(fmtFlag);
        if (fmt == OutFmt::Unknown) {
            std::cerr << "error: unknown format '" << fmtFlag << "'; use xml, vrml, or json\n";
            return 1;
        }
    } else if (!outPath.empty()) {
        fmt = fmtFromExt(ext(outPath));
        if (fmt == OutFmt::Unknown) {
            std::cerr << "error: cannot infer output format from extension '"
                      << ext(outPath) << "'; use -f xml|vrml|json\n";
            return 1;
        }
        if (ext(outPath) == ".wrl")
            std::cerr << "warning: .wrl output uses ClassicVRML encoding "
                         "(true VRML97 not yet supported)\n";
    } else {
        // stdout without -f: require -f
        std::cerr << "error: cannot infer output format; use -f xml|vrml|json "
                     "or provide -o with a known extension\n";
        return 1;
    }

    // Parse.
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }
    surfaceWarnings(doc);

    // Serialize.
    std::string output;
    try {
        switch (fmt) {
            case OutFmt::XML: {
                sdk::XmlWriter w;
                output = w.writeDocument(doc);
                break;
            }
            case OutFmt::ClassicVRML: {
                sdk::VrmlWriter w;
                output = w.writeDocument(doc);
                break;
            }
            case OutFmt::JSON: {
                sdk::JsonWriter w;
                output = w.writeDocument(doc);
                break;
            }
            default:
                assert(false && "unreachable: fmt resolved above");
                return 2;
        }
    } catch (const std::exception &e) {
        std::cerr << "error: serialization failed: " << e.what() << "\n";
        return 2;
    }

    // Write.
    try {
        writeOutput(outPath, output);
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }

    return 0;
}

// ─── validate ────────────────────────────────────────────────────────────────

// Profile-fit machinery lives in x3d-cli/profile_fit.hpp (included above).

// ─── JSON helpers ─────────────────────────────────────────────────────────────

namespace json_out {

// Very small JSON builder — no dependency on a JSON library.
std::string escapeString(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    out += '"';
    return out;
}

struct Diag {
    std::string category;
    std::string severity;
    std::string message;
};

std::string buildReport(const std::string &file, bool valid,
                        const std::string &profileFit,
                        const std::vector<Diag> &diags) {
    std::string s;
    s += "{\n";
    s += "  \"file\": "       + escapeString(file) + ",\n";
    s += std::string("  \"valid\": ") + (valid ? "true" : "false") + ",\n";
    s += "  \"profileFit\": " + escapeString(profileFit) + ",\n";
    s += "  \"diagnostics\": [\n";
    for (size_t i = 0; i < diags.size(); ++i) {
        const auto &d = diags[i];
        s += "    {";
        s += "\"category\": " + escapeString(d.category) + ", ";
        s += "\"severity\": " + escapeString(d.severity) + ", ";
        s += "\"message\": "  + escapeString(d.message)  + "}";
        if (i + 1 < diags.size()) s += ",";
        s += "\n";
    }
    s += "  ],\n";
    s += "  \"counts\": {";
    s += "\"total\": " + std::to_string(diags.size());
    s += "}\n";
    s += "}\n";
    return s;
}

} // namespace json_out

// ─── conformance_checks ──────────────────────────────────────────────────────
// Targeted semantic conformance checks that mirror X3DJSAIL's -validate output.
// Each check populates `diags` with one or more json_out::Diag entries.

namespace conformance_checks {

// ── Check 1: Duplicate <meta> statements ────────────────────────────────────
// X3DJSAIL: WARNING_MESSAGE: duplicate statement found: <meta name='X' content='Y'/>
// A <head> must not contain two <meta> elements with the same (name, content)
// pair — or more broadly the same name value.  We flag any pair with identical
// name AND identical content as a duplicate.
void checkDuplicateMeta(const sdk::X3DDocument &doc,
                        std::vector<json_out::Diag> &diags) {
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

// ── Check 2: ProtoDeclare / ExternProtoDeclare with no ProtoInstance ─────────
// X3DJSAIL: WARNING_PROTOINSTANCE_NOT_FOUND, ProtoDeclare/ExternProtoDeclare X
// has no corresponding ProtoInstance attached to the scene graph.  Best practice:
// every declared prototype should have at least one instantiation.
void checkUnusedProtoDeclare(const sdk::X3DDocument &doc,
                             std::vector<json_out::Diag> &diags) {
    const auto &scene = doc.scene;
    // Build a set of all ProtoInstance type names used in the scene.
    std::unordered_set<std::string> usedProtos;
    for (const auto &pi : scene.protoInstances) {
        usedProtos.insert(pi.name);  // ProtoInstance::name = proto being instantiated
    }
    // Also scan expanded sources (ProtoInstances that were expanded into the graph).
    for (const auto &[node, pi] : scene.expandedSources) {
        usedProtos.insert(pi.name);
    }
    // Check ProtoDeclare (local prototypes).
    for (const auto &pd : scene.protoDeclarations) {
        if (pd && usedProtos.find(pd->name) == usedProtos.end()) {
            std::string msg = "ProtoDeclare '" + pd->name
                            + "' has no corresponding ProtoInstance in the scene";
            diags.push_back({"unused-proto", "warning", msg});
        }
    }
    // Check ExternProtoDeclare (externally-defined prototypes).
    for (const auto &epd : scene.externProtoDeclarations) {
        if (epd && usedProtos.find(epd->name) == usedProtos.end()) {
            std::string msg = "ExternProtoDeclare '" + epd->name
                            + "' has no corresponding ProtoInstance in the scene";
            diags.push_back({"unused-proto", "warning", msg});
        }
    }
}

// ── Check 3: IndexedFaceSet / IndexedLineSet coord with empty coordIndex ─────
// X3DJSAIL: InvalidFieldException: IndexedFaceSet/IndexedLineSet containing
// Coordinate node with N values must also include coordIndex field.
// When a coord Coordinate node is present with point data but coordIndex is
// empty, the geometry is ill-formed.
static void walkForCoordCheck(const X3DNode &node,
                              std::vector<json_out::Diag> &diags,
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

void checkIndexedGeometryCoordIndex(const sdk::X3DDocument &doc,
                                    std::vector<json_out::Diag> &diags) {
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

// ── Run all conformance checks ───────────────────────────────────────────────
void runAll(const sdk::X3DDocument &doc, std::vector<json_out::Diag> &diags) {
    checkDuplicateMeta(doc, diags);
    checkUnusedProtoDeclare(doc, diags);
    checkIndexedGeometryCoordIndex(doc, diags);
}

} // namespace conformance_checks

// ─── cmdValidate ─────────────────────────────────────────────────────────────

int cmdValidate(const std::vector<std::string> &args) {
    // x3d validate <in> [--json]
    static const char *kUsage =
        "usage: x3d validate <input> [--json]\n"
        "\n"
        "  Validate an X3D scene and report diagnostics.\n"
        "\n"
        "  Checks performed:\n"
        "    1. Range diagnostics — out-of-range field values (SFColor [0,1], etc.)\n"
        "    2. Proto/Inline warnings — unresolved EXTERNPROTO, missing declarations,\n"
        "       unresolved Inline urls, etc.\n"
        "    3. Profile-fit — the minimal X3D profile the scene requires; flags nodes\n"
        "       that exceed a declared profile.\n"
        "    4. Duplicate <meta> — same (name, content) pair appears more than once.\n"
        "    5. Unused ProtoDeclare — a ProtoDeclare has no ProtoInstance in the scene.\n"
        "    6. IFS/ILS coord-without-index — IndexedFaceSet or IndexedLineSet has a\n"
        "       Coordinate node but an empty coordIndex field.\n"
        "\n"
        "  Output: human-readable report by default; --json for machine-readable JSON.\n"
        "\n"
        "  Exit codes:\n"
        "    0  no diagnostics found (clean)\n"
        "    1  usage error\n"
        "    2  parse / IO failure\n"
        "    3  validation issues found (diagnostics or profile exceedance)\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    bool jsonMode = false;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        if (a == "--json") {
            jsonMode = true;
        } else if (a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }

    // ── Parse ────────────────────────────────────────────────────────────────
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }

    // ── Collect diagnostics ───────────────────────────────────────────────────
    std::vector<json_out::Diag> diags;

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

    // 4. Conformance checks (X3DJSAIL-equivalent semantic validation).
    conformance_checks::runAll(doc, diags);

    // ── Profile-fit ───────────────────────────────────────────────────────────
    profile_fit::ComponentUsage usage = profile_fit::sceneComponentUsage(doc.scene);
    const profile_fit::ProfileDef *minProf = profile_fit::findMinimalProfile(usage);
    std::string profileFitName = minProf ? minProf->name : "unknown (exceeds Full)";

    // Check declared profile vs minimal required.
    const std::string declaredProfile = x3d::runtime::toString(doc.profile);
    std::vector<std::string> exceedances;

    // Find the declared profile def.
    const profile_fit::ProfileDef *declProf = nullptr;
    for (const auto &pd : profile_fit::profileDefs()) {
        if (pd.name == declaredProfile) { declProf = &pd; break; }
    }

    if (declProf) {
        // Walk scene to find nodes that exceed the declared profile.
        for (const auto &[comp, lvl] : usage) {
            if (!profile_fit::allowedInProfile(*declProf, comp, lvl)) {
                // Which node types in the scene use this component?
                auto types = profile_fit::nodeTypeNamesForComponent(doc.scene, comp);
                std::string nodeList;
                for (size_t i = 0; i < types.size() && i < 3; ++i) {
                    if (i) nodeList += ", ";
                    nodeList += types[i];
                }
                if (types.size() > 3) nodeList += ", ...";

                std::string msg = "component " + comp + ":" + std::to_string(lvl)
                                + " exceeds declared profile " + declaredProfile;
                if (!nodeList.empty()) msg += " (nodes: " + nodeList + ")";
                diags.push_back({"profile", "error", msg});
                if (!nodeList.empty()) exceedances.push_back(nodeList);
            }
        }
    }

    const bool valid = diags.empty();
    const int exitCode = valid ? 0 : 3;

    // ── Output ────────────────────────────────────────────────────────────────
    if (jsonMode) {
        std::cout << json_out::buildReport(inPath, valid, profileFitName, diags);
    } else {
        // Human-readable.
        std::cout << "file:          " << inPath << "\n";
        std::cout << "declared:      " << declaredProfile << " " << doc.version << "\n";
        std::cout << "profile fit:   " << profileFitName << "\n";
        std::cout << "valid:         " << (valid ? "yes" : "no") << "\n";
        std::cout << "\n";

        if (diags.empty()) {
            std::cout << "No diagnostics found.\n";
        } else {
            // Group by category.
            std::string curCat;
            for (const auto &d : diags) {
                if (d.category != curCat) {
                    curCat = d.category;
                    std::cout << "[" << curCat << "]\n";
                }
                std::cout << "  " << d.severity << ": " << d.message << "\n";
            }
            std::cout << "\n";
            std::cout << "Total: " << diags.size() << " diagnostic(s)\n";
        }
    }

    return exitCode;
}

// ─── cmdExtract ──────────────────────────────────────────────────────────────

int cmdExtract(const std::vector<std::string> &args) {
    // x3d extract <in> [-o out.stl]
    static const char *kUsage =
        "usage: x3d extract <input> [-o <output.stl>]\n"
        "\n"
        "  Export the tessellated geometry of an X3D scene to a single merged\n"
        "  binary STL file (world-space, one triangle soup).\n"
        "\n"
        "  The STL is produced by running the full SceneExtractor pipeline\n"
        "  (parse → buildSceneGraph → fullSnapshot) and baking every RenderItem's\n"
        "  local-frame triangles to world space using its per-path world transform.\n"
        "  All geometry types supported by MeshBuilder are included; unsupported\n"
        "  types (NURBS, 2D primitives, etc.) are counted and reported to stderr.\n"
        "\n"
        "  Output: binary STL (80-byte header + N × 50-byte facet records).\n"
        "    -o <output.stl>  write to file (binary); stdout if omitted\n"
        "\n"
        "  Exit codes:\n"
        "    0  ok (including zero-geometry scenes — valid empty STL)\n"
        "    1  usage error\n"
        "    2  parse / IO failure\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    std::string outPath;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        if (a == "-o") {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: -o requires an argument\n" << kUsage;
                return 1;
            }
            outPath = args[static_cast<size_t>(++i)];
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }

    // Parse.
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }
    surfaceWarnings(doc);

    // Build scene graph + extract.
    std::vector<SFVec3f> positions; // world-space, 3 per tri.
    std::vector<SFVec3f> normals;   // world-space per-vertex normals (3 per tri).
    std::map<std::string, int> skipped;

    try {
        sdk::X3DExecutionContext ctx;
        ctx.buildSceneGraph(doc.scene);
        ctx.buildFrom(doc.scene);

        sdk::MeshBuildOptions opts;
        sdk::SceneExtractor ex(ctx, doc.scene, opts);
        sdk::RenderDelta frame0 = ex.fullSnapshot();

        // Gather skipped geometry types.
        skipped = ex.skippedGeometryCounts();

        // For each RenderItem, transform its local-frame triangles to world space.
        for (sdk::RenderItemId id : frame0.added) {
            const sdk::RenderItem &item = ex.item(id);
            const sdk::MeshData &mesh = *item.mesh;
            const sdk::Mat4 &W = item.worldTransform;

            // MeshData uses expanded layout: indices are trivial 0..N-1 (or
            // explicitly listed); positions[indices[i]] is the i-th vertex.
            // Walk triangles via the index buffer (3 indices per triangle).
            const std::size_t triCount = mesh.indices.size() / 3;
            for (std::size_t t = 0; t < triCount; ++t) {
                for (int v = 0; v < 3; ++v) {
                    const std::uint32_t idx = mesh.indices[t * 3 + static_cast<std::size_t>(v)];
                    if (idx >= mesh.positions.size()) continue; // guard malformed

                    // Transform position to world space.
                    positions.push_back(W.transformPoint(mesh.positions[idx]));

                    // Transform normal to world space (direction, w=0).
                    if (mesh.hasNormals && idx < mesh.normals.size()) {
                        normals.push_back(W.transformDirection(mesh.normals[idx]));
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "error: scene extraction failed: " << e.what() << "\n";
        return 2;
    }

    // Report skipped geometry types to stderr.
    for (const auto &kv : skipped) {
        std::cerr << "note: skipped geometry type '" << kv.first
                  << "' (" << kv.second << " instance(s)) — not supported by MeshBuilder\n";
    }

    const std::size_t triCount = positions.size() / 3;
    if (triCount == 0)
        std::cerr << "note: no geometry found in '" << inPath << "' — writing empty STL\n";

    // Write binary STL.
    std::string stlBytes;
    try {
        stlBytes = x3d::cli::writeStlBinary(positions, normals);
    } catch (const std::exception &e) {
        std::cerr << "error: STL serialization failed: " << e.what() << "\n";
        return 2;
    }

    // Write output (file or stdout).
    try {
        if (outPath.empty()) {
            std::cout.write(stlBytes.data(), static_cast<std::streamsize>(stlBytes.size()));
            if (!std::cout)
                throw std::runtime_error("write to stdout failed");
        } else {
            std::ofstream ofs(outPath, std::ios::binary | std::ios::trunc);
            if (!ofs)
                throw std::runtime_error("cannot open output file: " + outPath);
            ofs.write(stlBytes.data(), static_cast<std::streamsize>(stlBytes.size()));
            if (!ofs)
                throw std::runtime_error("write to '" + outPath + "' failed");
        }
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }

    std::cerr << "extract: " << triCount << " triangle(s) written"
              << (outPath.empty() ? " (stdout)" : " to '" + outPath + "'") << "\n";
    return 0;
}

// ─── cmdCanonicalize ─────────────────────────────────────────────────────────

int cmdCanonicalize(const std::vector<std::string> &args) {
    // x3d canonicalize <in> [-o out.x3d]
    static const char *kUsage =
        "usage: x3d canonicalize <input> [-o <output>]\n"
        "\n"
        "  Emit the X3D Canonical Form (X3DC14N) of a scene.\n"
        "\n"
        "  The canonical form is a deterministic, byte-stable XML serialization\n"
        "  suitable for diffing, deduplication, VCS storage, and signatures.\n"
        "  Input: any supported encoding (XML/ClassicVRML/VRML97/JSON; gzip OK).\n"
        "  Output: always X3DC14N XML (sorted attrs, single-quoted, DTD line,\n"
        "          minimal number format, 2-space indent).\n"
        "\n"
        "  -o <output>  write to file; stdout if omitted\n"
        "\n"
        "  Exit codes: 0 ok, 1 usage error, 2 parse/IO failure.\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    std::string outPath;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        if (a == "-o") {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: -o requires an argument\n" << kUsage;
                return 1;
            }
            outPath = args[static_cast<size_t>(++i)];
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }

    // Parse (any encoding).
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }
    surfaceWarnings(doc);

    // Serialize to canonical form.
    std::string output;
    try {
        sdk::CanonicalXmlWriter w;
        output = w.writeDocument(doc);
    } catch (const std::exception &e) {
        std::cerr << "error: canonicalization failed: " << e.what() << "\n";
        return 2;
    }

    // Write.
    try {
        writeOutput(outPath, output);
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }

    return 0;
}

// ─── cmdSim ──────────────────────────────────────────────────────────────────

namespace sim_cmd {

// Parse a --move 'x y z -> x y z over Ds' driver spec. Returns true on success.
struct MovePath {
  bool active = false;
  SFVec3f from{0, 0, 0};
  SFVec3f to{0, 0, 0};
  double durationSec = 0.0;
};

bool parseMove(const std::string &spec, MovePath &out, std::string &err) {
  // Grammar: <fx> <fy> <fz> -> <tx> <ty> <tz> over <Ds>
  // Tolerant tokenizer: split on whitespace, find "->" and "over".
  std::vector<std::string> toks;
  {
    std::string cur;
    for (char c : spec) {
      if (std::isspace(static_cast<unsigned char>(c))) {
        if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
      } else {
        cur += c;
      }
    }
    if (!cur.empty()) toks.push_back(cur);
  }
  // Expect: f0 f1 f2 -> t0 t1 t2 over Ds  (11 tokens).
  auto arrow = std::find(toks.begin(), toks.end(), "->");
  auto over = std::find(toks.begin(), toks.end(), "over");
  if (arrow == toks.end() || over == toks.end() || arrow >= over) {
    err = "expected 'x y z -> x y z over Ds'";
    return false;
  }
  std::vector<std::string> fromToks(toks.begin(), arrow);
  std::vector<std::string> toToks(arrow + 1, over);
  std::vector<std::string> durToks(over + 1, toks.end());
  if (fromToks.size() != 3 || toToks.size() != 3 || durToks.size() != 1) {
    err = "expected 3 from-coords, 3 to-coords, 1 duration";
    return false;
  }
  try {
    out.from = {std::stof(fromToks[0]), std::stof(fromToks[1]), std::stof(fromToks[2])};
    out.to = {std::stof(toToks[0]), std::stof(toToks[1]), std::stof(toToks[2])};
    // strip a trailing 's' on the duration (e.g. "2s" or "2").
    std::string d = durToks[0];
    if (!d.empty() && (d.back() == 's' || d.back() == 'S')) d.pop_back();
    out.durationSec = std::stod(d);
  } catch (const std::exception &) {
    err = "non-numeric coordinate or duration";
    return false;
  }
  out.active = true;
  return true;
}

// JSON-escape (reuse json_out::escapeString idiom; local minimal copy is the
// same as json_out::escapeString — kept local to avoid coupling sim to that
// validate-specific namespace).
std::string jstr(const std::string &s) { return json_out::escapeString(s); }

// Try to parse a watch spec "Node.field"; warns if it has no dot.
bool validWatch(const std::string &w) {
  return w.find('.') != std::string::npos;
}

}  // namespace sim_cmd

int cmdSim(const std::vector<std::string> &args) {
    // x3d sim <in> [--fps F] [--ticks N | --duration D] [--move '...']
    //         [--watch DEF.field ...] [--json]
    static const char *kUsage =
        "usage: x3d sim <input> [--fps F] [--ticks N | --duration D]\n"
        "               [--move 'x y z -> x y z over Ds'] [--watch DEF.field ...] [--json]\n"
        "\n"
        "  Headless behavior simulation: drive the full event/behavior runtime\n"
        "  (TimeSensors, interpolators, event utilities, scripts, view-dependent\n"
        "  sensors) for N ticks and trace every field that changes per tick.\n"
        "\n"
        "  Drivers:\n"
        "    Time (always): tick at 1/fps from t=0; loop=true TimeSensors animate.\n"
        "    --move 'a -> b over Ds': sweep the viewer position linearly from a to\n"
        "      b over D seconds (drives ProximitySensor/VisibilitySensor/LOD/Billboard).\n"
        "\n"
        "  Options:\n"
        "    --fps F        ticks per second (default 30)\n"
        "    --ticks N      number of ticks (default 60)\n"
        "    --duration D   seconds to simulate => N = round(D * F) (overrides --ticks)\n"
        "    --watch S      restrict the trace to field spec 'Node.field' (repeatable)\n"
        "    --json         emit a machine trace: [{tick,t,changes:[{node,field,value}]}]\n"
        "\n"
        "  Node names: DEF name if present, else a stable '<Type>#<index>' id.\n"
        "\n"
        "  Exit codes: 0 ok, 1 usage error, 2 parse/IO failure.\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    double fps = 30.0;
    long ticks = 60;
    bool ticksFromDuration = false;
    double duration = 0.0;
    bool jsonMode = false;
    std::string moveSpec;
    std::vector<std::string> watch;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        auto needArg = [&](const char *flag) -> bool {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: " << flag << " requires an argument\n" << kUsage;
                return false;
            }
            return true;
        };
        if (a == "--fps") {
            if (!needArg("--fps")) return 1;
            try { fps = std::stod(args[static_cast<size_t>(++i)]); }
            catch (...) { std::cerr << "error: --fps must be numeric\n"; return 1; }
            if (fps <= 0) { std::cerr << "error: --fps must be > 0\n"; return 1; }
        } else if (a == "--ticks") {
            if (!needArg("--ticks")) return 1;
            try { ticks = std::stol(args[static_cast<size_t>(++i)]); }
            catch (...) { std::cerr << "error: --ticks must be an integer\n"; return 1; }
            if (ticks < 0) { std::cerr << "error: --ticks must be >= 0\n"; return 1; }
        } else if (a == "--duration") {
            if (!needArg("--duration")) return 1;
            try { duration = std::stod(args[static_cast<size_t>(++i)]); }
            catch (...) { std::cerr << "error: --duration must be numeric\n"; return 1; }
            if (duration < 0) { std::cerr << "error: --duration must be >= 0\n"; return 1; }
            ticksFromDuration = true;
        } else if (a == "--move") {
            if (!needArg("--move")) return 1;
            moveSpec = args[static_cast<size_t>(++i)];
        } else if (a == "--watch") {
            if (!needArg("--watch")) return 1;
            watch.push_back(args[static_cast<size_t>(++i)]);
        } else if (a == "--json") {
            jsonMode = true;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }
    for (const auto &w : watch)
        if (!sim_cmd::validWatch(w))
            std::cerr << "warning: --watch '" << w
                      << "' has no '.'; expected 'Node.field'\n";

    if (ticksFromDuration)
        ticks = static_cast<long>(std::lround(duration * fps));

    // Parse the --move driver spec, if any.
    sim_cmd::MovePath move;
    if (!moveSpec.empty()) {
        std::string err;
        if (!sim_cmd::parseMove(moveSpec, move, err)) {
            std::cerr << "error: bad --move spec: " << err << "\n" << kUsage;
            return 1;
        }
    }

    // ── Parse the scene ────────────────────────────────────────────────────────
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }
    surfaceWarnings(doc);

    // ── Wire the full behavior runtime ─────────────────────────────────────────
    sdk::X3DExecutionContext ctx;
    x3d::sim::RuntimeWiring wiring;
    try {
        ctx.buildSceneGraph(doc.scene);
        ctx.buildFrom(doc.scene);
        wiring = x3d::sim::attachFullRuntime(doc.scene, ctx);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to build runtime: " << e.what() << "\n";
        return 2;
    }
    if (!wiring.scriptEngineLinked)
        std::cerr << "note: no script engine linked in this build — Script nodes "
                     "are inert (not a failure)\n";
    else if (wiring.scriptsEnrolled > 0)
        std::cerr << "note: enrolled " << wiring.scriptsEnrolled
                  << " Script node(s)\n";

    // ── Build the tracer ───────────────────────────────────────────────────────
    // Wrap construction: a bad-any-cast or other exception in buildNodeIndex
    // must yield exit 2 + clean stderr, not an unhandled exception / wrong code.
    x3d::sim::FieldTracer tracer = [&]() -> x3d::sim::FieldTracer {
        try {
            return x3d::sim::FieldTracer(doc.scene, watch);
        } catch (const std::exception &e) {
            std::cerr << "error: failed to build field tracer: " << e.what() << "\n";
            std::exit(2);
        } catch (...) {
            std::cerr << "error: failed to build field tracer (unknown exception)\n";
            std::exit(2);
        }
    }();

    // ── Tick loop ──────────────────────────────────────────────────────────────
    std::vector<x3d::sim::TickTrace> traces;
    traces.reserve(static_cast<size_t>(ticks) + 1);
    const double dt = 1.0 / fps;

    auto applyMove = [&](double t) {
        if (!move.active) return;
        double frac = (move.durationSec > 0.0)
                          ? std::min(1.0, std::max(0.0, t / move.durationSec))
                          : 1.0;
        SFVec3f p{
            move.from.x + (move.to.x - move.from.x) * static_cast<float>(frac),
            move.from.y + (move.to.y - move.from.y) * static_cast<float>(frac),
            move.from.z + (move.to.z - move.from.z) * static_cast<float>(frac)};
        // Drive the viewer via the head-pose seam; cameraWorldPosition() advances,
        // so ViewDependentSystem evaluates Proximity/Visibility/LOD against it.
        ctx.setHeadPose(p, SFRotation{0, 0, 1, 0});
    };

    try {
        // Baseline at t=0 BEFORE the first tick so tick 0's deltas reflect what
        // the first tick produced (initial sensor activations, fraction=0, ...).
        applyMove(0.0);
        tracer.baseline();
        for (long k = 0; k < ticks; ++k) {
            const double t = static_cast<double>(k) * dt;
            applyMove(t);
            ctx.tick(t);
            traces.push_back(tracer.diff(static_cast<int>(k), t));
        }
    } catch (const std::exception &e) {
        std::cerr << "error: simulation failed: " << e.what() << "\n";
        return 2;
    }

    // ── Emit the trace ─────────────────────────────────────────────────────────
    if (jsonMode) {
        std::string s;
        s += "[\n";
        for (size_t ti = 0; ti < traces.size(); ++ti) {
            const auto &tr = traces[ti];
            s += "  {\"tick\": " + std::to_string(tr.tick);
            // Fixed 6-decimal time so the JSON is byte-stable across platforms.
            char tbuf[32];
            std::snprintf(tbuf, sizeof(tbuf), "%.6f", tr.t);
            s += ", \"t\": " + std::string(tbuf) + ", \"changes\": [";
            for (size_t ci = 0; ci < tr.changes.size(); ++ci) {
                const auto &c = tr.changes[ci];
                s += "{\"node\": " + sim_cmd::jstr(c.node) +
                     ", \"field\": " + sim_cmd::jstr(c.field) +
                     ", \"value\": " + sim_cmd::jstr(c.value) + "}";
                if (ci + 1 < tr.changes.size()) s += ", ";
            }
            s += "]}";
            if (ti + 1 < traces.size()) s += ",";
            s += "\n";
        }
        s += "]\n";
        std::cout << s;
    } else {
        for (const auto &tr : traces) {
            if (tr.changes.empty()) continue;
            char tbuf[32];
            std::snprintf(tbuf, sizeof(tbuf), "%.4f", tr.t);
            std::cout << "tick " << tr.tick << " (t=" << tbuf << "):\n";
            for (const auto &c : tr.changes)
                std::cout << "  " << c.node << "." << c.field << " = " << c.value
                          << "\n";
        }
    }

    return 0;
}

// ─── top-level dispatch ───────────────────────────────────────────────────────

static const SubCmd kCmds[] = {
    // Task A: convert
    {"convert",     "Convert an X3D scene between encodings",       cmdConvert},
    // Task B: validate
    {"validate",    "Validate an X3D scene and report diagnostics", cmdValidate},
    // Task C: extract
    {"extract",     "Export X3D geometry to a binary STL file",     cmdExtract},
    // Task D: canonicalize
    {"canonicalize","Emit the X3D Canonical Form (X3DC14N)",        cmdCanonicalize},
    // sim: headless behavior simulation / field-change trace
    {"sim",         "Headless behavior simulation + field-change trace", cmdSim},
};

static const char *kTopUsage =
    "x3d — X3D scene tool\n"
    "\n"
    "usage: x3d <subcommand> [args]\n"
    "       x3d --help\n"
    "       x3d <subcommand> --help\n"
    "\n"
    "subcommands:\n"
    "  convert      Convert an X3D scene between encodings\n"
    "  validate     Validate an X3D scene and report diagnostics\n"
    "  extract      Export X3D geometry to a binary STL file\n"
    "  canonicalize Emit the X3D Canonical Form (X3DC14N)\n"
    "  sim          Headless behavior simulation + field-change trace\n"
    "\n"
    "Run 'x3d <subcommand> --help' for subcommand-specific usage.\n";

}  // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << kTopUsage;
        return 1;
    }

    const std::string sub = argv[1];

    if (sub == "--help" || sub == "-h") {
        std::cout << kTopUsage;
        return 0;
    }

    // Build args tail (argv[2..]).
    std::vector<std::string> tail;
    tail.reserve(static_cast<size_t>(argc - 2));
    for (int i = 2; i < argc; ++i)
        tail.emplace_back(argv[i]);

    // Dispatch.
    for (const auto &cmd : kCmds) {
        if (cmd.name == sub)
            return cmd.run(tail);
    }

    std::cerr << "error: unknown subcommand '" << sub << "'\n" << kTopUsage;
    return 1;
}
