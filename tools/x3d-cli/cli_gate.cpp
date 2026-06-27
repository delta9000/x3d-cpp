// tools/x3d-cli/cli_gate.cpp
// ─────────────────────────────────────────────────────────────────────────────
// x3d-cli-gate — Java-free differential validation gate for the x3d CLI.
//
// Reads:
//   tools/x3d-cli/goldens/subset.txt          — relative paths to the subset
//   tools/x3d-cli/goldens/validate-verdicts.tsv — X3DJSAIL reference verdicts
//
// Runs:
//   1. validate-diff: for each file in the subset, invoke our SDK validate path
//      (same logic as `x3d validate`) and compare our VALID/INVALID verdict to
//      the golden. Collect agreements, disagreements, and parse failures.
//
//   2. convert-roundtrip: for each file our parser accepts, convert to each
//      other supported encoding via the SDK writers, reparse, and assert
//      sceneEquivalent(original, roundtrip). Collect pass/fail per conversion.
//
// Produces:
//   tools/x3d-cli/goldens/divergence-report.md — full divergence report
//
// Exit modes:
//   Default (informative): exits 0 always; divergences are printed for triage.
//   --write-baseline: runs checks, writes tools/x3d-cli/goldens/cli-gate-baseline.tsv,
//                     exits 0. Run once to lock in the current PASS/FAIL state.
//   --gate: reads cli-gate-baseline.tsv; exits non-zero if any item that was
//           PASS in the baseline is now FAIL (regression). PASS->FAIL is a bug.
//           FAIL->PASS is fine (improvement); prints a note to refresh baseline.
//
// Usage:
//   x3d_cli_gate <x3d_binary> <corpus_root> <goldens_dir> [--gate | --write-baseline]
//
//   x3d_binary   — path to the built x3d CLI binary (build/x3d)
//   corpus_root  — a local X3D conformance archive checkout ($X3D_CORPUS_DIR)
//   goldens_dir  — tools/x3d-cli/goldens
// ─────────────────────────────────────────────────────────────────────────────
#include "scene_equiv.hpp"
#include "x3d/sdk.hpp"
#include "Encoding.hpp"

// Generated node headers for conformance checks (mirrors x3d_cli.cpp).
#include "Coordinate.hpp"
#include "IndexedFaceSet.hpp"
#include "IndexedLineSet.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;
namespace sdk = x3d::sdk;

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string toLower(std::string s) {
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string extOf(const std::string &p) {
    return toLower(fs::path(p).extension().string());
}

// ── Golden verdict loading ─────────────────────────────────────────────────

struct GoldenVerdict {
    std::string relpath;
    std::string verdict;  // "VALID" or "INVALID"
    std::string note;
};

static std::vector<GoldenVerdict> loadVerdicts(const std::string &tsvPath) {
    std::vector<GoldenVerdict> out;
    std::ifstream f(tsvPath);
    if (!f) throw std::runtime_error("cannot open verdicts TSV: " + tsvPath);
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (header) { header = false; continue; }
        if (line.empty()) continue;
        // tab-separated: relpath \t verdict \t note
        std::vector<std::string> cols;
        std::string cur;
        for (char c : line) {
            if (c == '\t') { cols.push_back(cur); cur.clear(); }
            else cur += c;
        }
        cols.push_back(cur);
        if (cols.size() < 2) continue;
        GoldenVerdict gv;
        gv.relpath = cols[0];
        gv.verdict = cols[1];
        gv.note = cols.size() >= 3 ? cols[2] : "";
        out.push_back(gv);
    }
    return out;
}

// (loadSubset removed: dead — no callers after the gate stopped reading subset.txt)

// ── Conformance checks (mirrors conformance_checks namespace in x3d_cli.cpp) ─

static bool hasDuplicateMeta(const sdk::X3DDocument &doc) {
    const auto &metas = doc.head.meta;
    for (size_t i = 0; i < metas.size(); ++i)
        for (size_t j = i + 1; j < metas.size(); ++j)
            if (metas[i].name == metas[j].name && metas[i].content == metas[j].content)
                return true;
    return false;
}

static bool hasUnusedProtoDeclare(const sdk::X3DDocument &doc) {
    const auto &scene = doc.scene;
    std::unordered_set<std::string> usedProtos;
    for (const auto &pi : scene.protoInstances)
        usedProtos.insert(pi.name);
    for (const auto &[node, pi] : scene.expandedSources)
        usedProtos.insert(pi.name);
    for (const auto &pd : scene.protoDeclarations)
        if (pd && usedProtos.find(pd->name) == usedProtos.end())
            return true;
    for (const auto &epd : scene.externProtoDeclarations)
        if (epd && usedProtos.find(epd->name) == usedProtos.end())
            return true;
    return false;
}

static bool walkCoordCheck(const X3DNode &node,
                            std::unordered_set<const X3DNode *> &visited) {
    if (!visited.insert(&node).second) return false;

    const std::string typeName = node.nodeTypeName();
    if (typeName == "IndexedFaceSet") {
        const auto *ifs = dynamic_cast<const IndexedFaceSet *>(&node);
        if (ifs && ifs->getCoordIndex().empty()) {
            auto coordNode = ifs->getCoord();
            if (coordNode) {
                const auto *coord = dynamic_cast<const Coordinate *>(coordNode.get());
                if (coord && !coord->getPoint().empty()) { visited.erase(&node); return true; }
            }
        }
    } else if (typeName == "IndexedLineSet") {
        const auto *ils = dynamic_cast<const IndexedLineSet *>(&node);
        if (ils && ils->getCoordIndex().empty()) {
            auto coordNode = ils->getCoord();
            if (coordNode) {
                const auto *coord = dynamic_cast<const Coordinate *>(coordNode.get());
                if (coord && !coord->getPoint().empty()) { visited.erase(&node); return true; }
            }
        }
    }

    for (const FieldInfo &f : node.fields()) {
        if (!f.isNode() || !f.isReadable()) continue;
        std::any v = f.get(node);
        if (f.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
            if (child && walkCoordCheck(*child, visited)) { visited.erase(&node); return true; }
        } else {
            auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
            for (const auto &child : vec)
                if (child && walkCoordCheck(*child, visited)) { visited.erase(&node); return true; }
        }
    }
    visited.erase(&node);
    return false;
}

static bool hasIndexedGeomCoordIssue(const sdk::X3DDocument &doc) {
    // Only flag on X3D 4.0+; older versions treat missing coordIndex as valid
    // (renders nothing). Parser normalises absent attributes to empty arrays, so
    // we can't distinguish them for pre-4.0 files.
    const std::string &ver = doc.version;
    if (ver.empty() || ver < "4.0") return false;
    std::unordered_set<const X3DNode *> visited;
    for (const auto &root : doc.scene.rootNodes)
        if (root && walkCoordCheck(*root, visited)) return true;
    return false;
}

// ── Our validate logic (mirrors cmdValidate in x3d_cli.cpp) ─────────────────
// Returns "VALID" or "INVALID", and populates reason on INVALID.
static std::string ourValidate(const std::string &path, std::string &reason) {
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(path);
    } catch (const std::exception &e) {
        reason = "parse-fail: ";
        reason += e.what();
        return "PARSE_FAIL";
    }

    // Collect diagnostics (same order as cmdValidate)
    if (!doc.rangeWarnings.empty()) {
        reason = "range: " + doc.rangeWarnings[0].message();
        return "INVALID";
    }
    if (!doc.protoWarnings.empty()) {
        reason = "proto: " + doc.protoWarnings[0].instanceName + ": " +
                 doc.protoWarnings[0].detail;
        return "INVALID";
    }
    if (!doc.inlineWarnings.empty()) {
        reason = "inline: " + doc.inlineWarnings[0].detail;
        return "INVALID";
    }

    // Conformance checks (Check 1: duplicate meta)
    if (hasDuplicateMeta(doc)) {
        reason = "dup-meta: duplicate <meta> statement";
        return "INVALID";
    }
    // Conformance check 2: unused ProtoDeclare
    if (hasUnusedProtoDeclare(doc)) {
        reason = "unused-proto: ProtoDeclare with no ProtoInstance";
        return "INVALID";
    }
    // Conformance check 3: IFS/ILS coord without coordIndex
    if (hasIndexedGeomCoordIssue(doc)) {
        reason = "coord-index: IndexedFaceSet/IndexedLineSet has Coordinate but empty coordIndex";
        return "INVALID";
    }

    return "VALID";
}

// ── Disagreement category classification ─────────────────────────────────────
// Returns a short category string for a disagreement pair.
static std::string classifyDisagreement(const std::string &ourVerdict,
                                        const std::string &ourReason,
                                        const std::string &goldenVerdict,
                                        const std::string &goldenNote) {
    if (ourVerdict == "PARSE_FAIL") return "parse-fail";

    // We say INVALID (range/proto/inline/conformance), X3DJSAIL says VALID.
    if (ourVerdict == "INVALID" && goldenVerdict == "VALID") {
        if (ourReason.rfind("range:", 0) == 0) return "range-only-us";
        if (ourReason.rfind("proto:", 0) == 0) return "proto-warn-us";
        if (ourReason.rfind("inline:", 0) == 0) return "inline-warn-us";
        if (ourReason.rfind("dup-meta:", 0) == 0) return "dup-meta-only-us";
        if (ourReason.rfind("unused-proto:", 0) == 0) return "unused-proto-only-us";
        if (ourReason.rfind("coord-index:", 0) == 0) return "coord-index-only-us";
        return "invalid-only-us";
    }
    // We say VALID, X3DJSAIL says INVALID.
    if (ourVerdict == "VALID" && goldenVerdict == "INVALID") {
        // X3DJSAIL rejects non-.x3d files entirely (not a validation finding,
        // a JSAIL limitation: it only reads XML via -validate).
        if (goldenNote.find("does not end with extension") != std::string::npos ||
            goldenNote.find("X3DException: fileName") != std::string::npos) {
            return "jsail-non-xml-rejection";
        }
        if (goldenNote.find("WARNING_MESSAGE") != std::string::npos ||
            goldenNote.find("duplicate") != std::string::npos) return "jsail-dup-meta";
        if (goldenNote.find("[error]") != std::string::npos ||
            goldenNote.find("ERROR") != std::string::npos) return "jsail-error";
        if (goldenNote.rfind("validate results:", 0) == 0) return "jsail-schema-warn";
        return "invalid-only-jsail";
    }
    return "other";
}

// ── Convert roundtrip ─────────────────────────────────────────────────────────

struct ConvertResult {
    std::string relpath;
    std::string fromEnc;
    std::string toEnc;
    bool ok;
    std::string why;
};

// Serialize a document to the given encoding.
static std::string serialize(const sdk::X3DDocument &doc, const std::string &enc) {
    if (enc == "xml") { sdk::XmlWriter w; return w.writeDocument(doc); }
    if (enc == "vrml") { sdk::VrmlWriter w; return w.writeDocument(doc); }
    if (enc == "json") { sdk::JsonWriter w; return w.writeDocument(doc); }
    throw std::runtime_error("unknown enc: " + enc);
}

static x3d::codec::Encoding parseHint(const std::string &enc) {
    if (enc == "xml") return x3d::codec::Encoding::XML;
    if (enc == "vrml") return x3d::codec::Encoding::ClassicVRML;
    if (enc == "json") return x3d::codec::Encoding::JSON;
    return x3d::codec::Encoding::Unknown;
}

static ConvertResult testConvert(const std::string &relpath,
                                 const std::string &absPath,
                                 const sdk::X3DDocument &src,
                                 const std::string &fromEnc,
                                 const std::string &toEnc) {
    ConvertResult r;
    r.relpath = relpath;
    r.fromEnc = fromEnc;
    r.toEnc = toEnc;
    r.ok = false;

    // Serialize to toEnc.
    std::string serialized;
    try {
        serialized = serialize(src, toEnc);
    } catch (const std::exception &e) {
        r.why = "serialize-fail: " + std::string(e.what());
        return r;
    }

    // Reparse from the serialized text. Use the source file's directory as the
    // baseUrl so relative Inline/EXTERNPROTO urls resolve symmetrically with the
    // source parse (parseFile derives the same base). Without this, the source
    // side expands externals into rootNodes/children while the reparse leaves
    // them as unexpanded stubs, producing spurious "not equivalent" verdicts.
    sdk::X3DDocument rt;
    const std::string base = fs::path(absPath).parent_path().string();
    try {
        rt = sdk::parseDocument(serialized, parseHint(toEnc), base);
    } catch (const std::exception &e) {
        r.why = "reparse-fail (" + toEnc + "): " + std::string(e.what());
        return r;
    }

    // Compare scenes.
    std::string why;
    if (!x3d_cli::sceneEquivalent(src.scene, rt.scene, why)) {
        r.why = "not equivalent: " + why;
        return r;
    }

    r.ok = true;
    return r;
}

// Encoding family from extension.
static std::string encFamily(const std::string &ext) {
    if (ext == ".x3d"  || ext == ".x3dz") return "xml";
    if (ext == ".x3dv" || ext == ".x3dvz") return "vrml";
    if (ext == ".wrl"  || ext == ".wrz") return "vrml";
    if (ext == ".json") return "json";
    return "";
}

// ── Markdown report ───────────────────────────────────────────────────────────

static void writeReport(const std::string &outPath,
                        [[maybe_unused]] const std::vector<GoldenVerdict> &verdicts,
                        const std::vector<std::pair<GoldenVerdict, std::pair<std::string, std::string>>> &divergences,
                        const std::vector<ConvertResult> &convertResults,
                        int totalValidate, int agree, int parseFailOurs) {
    std::ofstream f(outPath);
    if (!f) throw std::runtime_error("cannot write report: " + outPath);

    // Header
    f << "# X3DJSAIL Differential Divergence Report\n\n";
    f << "Generated by `x3d_cli_gate`. First-run policy: divergences are **informative**, "
         "not CI failures — triage before allowlisting.\n\n";

    // Validate summary
    int disagree = static_cast<int>(divergences.size()) - parseFailOurs;
    if (disagree < 0) disagree = 0;
    double agreeRate = totalValidate > 0 ? 100.0 * agree / totalValidate : 0.0;
    f << "## Validate Agreement\n\n";
    f << "| Metric | Value |\n|---|---|\n";
    f << "| Subset size | " << totalValidate << " |\n";
    f << "| Agreement | **" << agree << "/" << totalValidate
      << "** (" << static_cast<int>(agreeRate + 0.5) << "%) |\n";
    f << "| Disagreements | " << disagree << " |\n";
    f << "| Parse failures (ours) | " << parseFailOurs << " |\n\n";

    // Disagreement categories
    if (!divergences.empty()) {
        // Group by category
        std::map<std::string, std::vector<const std::pair<GoldenVerdict, std::pair<std::string,std::string>> *>> byCategory;
        for (const auto &d : divergences) {
            const auto &gv = d.first;
            const auto &[ourV, ourR] = d.second;
            std::string cat = classifyDisagreement(ourV, ourR, gv.verdict, gv.note);
            byCategory[cat].push_back(&d);
        }

        f << "## Disagreement Categories\n\n";
        for (const auto &[cat, items] : byCategory) {
            f << "### " << cat << " (" << items.size() << " files)\n\n";
            // Print up to 3 examples
            size_t nex = std::min(items.size(), size_t(3));
            for (size_t i = 0; i < nex; ++i) {
                const auto &gv = items[i]->first;
                const auto &[ourV, ourR] = items[i]->second;
                f << "- **" << gv.relpath.substr(gv.relpath.rfind('/') + 1) << "**\n";
                f << "  - Ours: `" << ourV << "` — " << (ourR.empty() ? "(no reason)" : ourR) << "\n";
                f << "  - X3DJSAIL: `" << gv.verdict << "` — " << (gv.note.empty() ? "(none)" : gv.note) << "\n";
            }
            if (items.size() > 3) f << "- *(+" << (items.size() - 3) << " more)*\n";
            f << "\n";
        }

        f << "## Full Disagreement List\n\n";
        f << "| File | Ours | X3DJSAIL | Our reason | JSAIL note |\n|---|---|---|---|---|\n";
        for (const auto &[gv, ov] : divergences) {
            const auto &[ourV, ourR] = ov;
            std::string fname = gv.relpath.substr(gv.relpath.rfind('/') + 1);
            // Escape pipes in cells
            auto esc = [](std::string s) {
                for (size_t i = 0; i < s.size(); ++i)
                    if (s[i] == '|') s.replace(i, 1, "\\|");
                return s;
            };
            f << "| " << esc(fname) << " | " << esc(ourV) << " | " << esc(gv.verdict)
              << " | " << esc(ourR.substr(0, 80)) << " | " << esc(gv.note.substr(0, 80)) << " |\n";
        }
        f << "\n";
    } else {
        f << "No validate disagreements.\n\n";
    }

    // Convert roundtrip summary
    int rtTotal = 0, rtPass = 0;
    std::map<std::string, std::pair<int,int>> byPair; // "xml->vrml" -> {pass,fail}
    for (const auto &r : convertResults) {
        ++rtTotal;
        std::string key = r.fromEnc + "->" + r.toEnc;
        auto &pr = byPair[key];
        if (r.ok) { ++rtPass; ++pr.first; }
        else { ++pr.second; }
    }

    f << "## Convert Roundtrip\n\n";
    double rtRate = rtTotal > 0 ? 100.0 * rtPass / rtTotal : 0.0;
    f << "**Pass rate: " << rtPass << "/" << rtTotal << " (" << static_cast<int>(rtRate + 0.5) << "%)**\n\n";
    if (!byPair.empty()) {
        f << "| Pair | Pass | Fail |\n|---|---|---|\n";
        for (const auto &[key, pf] : byPair)
            f << "| " << key << " | " << pf.first << " | " << pf.second << " |\n";
        f << "\n";
    }

    // Convert failures (up to 20)
    std::vector<const ConvertResult *> rtFails;
    for (const auto &r : convertResults) if (!r.ok) rtFails.push_back(&r);
    if (!rtFails.empty()) {
        f << "### Roundtrip Failures (up to 20)\n\n";
        size_t nf = std::min(rtFails.size(), size_t(20));
        f << "| File | Pair | Reason |\n|---|---|---|\n";
        for (size_t i = 0; i < nf; ++i) {
            const auto *r = rtFails[i];
            std::string fname = r->relpath.substr(r->relpath.rfind('/') + 1);
            std::string why80 = r->why.substr(0, 120);
            // escape pipes
            for (size_t j = 0; j < why80.size(); ++j)
                if (why80[j] == '|') why80.replace(j, 1, "\\|");
            f << "| " << fname << " | " << r->fromEnc << "->" << r->toEnc << " | " << why80 << " |\n";
        }
        if (rtFails.size() > 20) f << "\n*(+" << (rtFails.size() - 20) << " more)*\n";
        f << "\n";
    }

    f << "---\n*Generated by x3d_cli_gate*\n";
}

// ── Baseline PASS/FAIL state (regression gate) ───────────────────────────────
//
// Key format: "validate\t<relpath>" or "convert\t<relpath>:<from>-><to>"
// Value: "PASS" or "FAIL"
using BaselineMap = std::unordered_map<std::string, std::string>;

static BaselineMap loadBaseline(const std::string &path) {
    BaselineMap out;
    std::ifstream f(path);
    if (!f) return out; // empty → no baseline loaded
    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (header) { header = false; continue; }
        if (line.empty()) continue;
        auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        std::string key = line.substr(0, tab);
        std::string val = line.substr(tab + 1);
        out[key] = val;
    }
    return out;
}

static void writeBaseline(const std::string &path, const BaselineMap &bm,
                          int validateTotal, int validateAgree,
                          int convertTotal, int convertPass) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("cannot write baseline: " + path);
    // Header comment with summary numbers for human readers.
    f << "# cli-gate baseline — generated by x3d_cli_gate --write-baseline\n";
    f << "# validate: " << validateAgree << "/" << validateTotal << " agree"
      << "  convert: " << convertPass << "/" << convertTotal << " pass\n";
    f << "check_key\tresult\n";
    // Write sorted for stable diffs.
    std::vector<std::pair<std::string,std::string>> rows(bm.begin(), bm.end());
    std::sort(rows.begin(), rows.end());
    for (const auto &[k, v] : rows)
        f << k << "\t" << v << "\n";
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    // Parse optional mode flag (last arg, if it starts with '--').
    bool gateMode = false;
    bool writeBaselineMode = false;
    int positionalArgc = argc;
    if (argc >= 2) {
        std::string last = argv[argc - 1];
        if (last == "--gate") { gateMode = true; positionalArgc = argc - 1; }
        else if (last == "--write-baseline") { writeBaselineMode = true; positionalArgc = argc - 1; }
    }

    if (positionalArgc < 4) {
        std::cerr << "usage: x3d_cli_gate <x3d_binary> <corpus_root> <goldens_dir>"
                     " [--gate | --write-baseline]\n";
        return 1;
    }
    const std::string x3dBin     = argv[1];
    const std::string corpusRoot = argv[2];
    const std::string goldensDir = argv[3];

    const std::string verdictsTsv  = goldensDir + "/validate-verdicts.tsv";
    const std::string reportMd     = goldensDir + "/divergence-report.md";
    const std::string baselinePath = goldensDir + "/cli-gate-baseline.tsv";

    if (gateMode)
        std::cout << "[--gate mode] Regression check against baseline: " << baselinePath << "\n\n";
    else if (writeBaselineMode)
        std::cout << "[--write-baseline mode] Capturing current state to: " << baselinePath << "\n\n";

    // Load baseline if in gate mode.
    BaselineMap baseline;
    if (gateMode) {
        baseline = loadBaseline(baselinePath);
        if (baseline.empty()) {
            std::cerr << "ERROR: baseline not found or empty: " << baselinePath << "\n";
            std::cerr << "Run 'mise run cli-gate-baseline' first.\n";
            return 1;
        }
        std::cout << "Loaded " << baseline.size() << " baseline entries.\n\n";
    }

    // Load golden data.
    std::cout << "Loading golden verdicts from " << verdictsTsv << "...\n";
    std::vector<GoldenVerdict> verdicts;
    try {
        verdicts = loadVerdicts(verdictsTsv);
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << "Run 'mise run cli-golden-gen' first.\n";
        return 1;
    }

    std::cout << "Loaded " << verdicts.size() << " verdicts.\n\n";

    // ── Phase 1: validate-diff ─────────────────────────────────────────────

    std::cout << "=== Phase 1: validate-diff ===\n";

    int totalValidate = 0, agree = 0, parseFail = 0;
    std::vector<std::pair<GoldenVerdict, std::pair<std::string, std::string>>> divergences;

    // Per-item PASS/FAIL for baseline writing / regression checking.
    BaselineMap currentState;

    // Only run on files that parse successfully (skip file-not-found).
    for (const auto &gv : verdicts) {
        if (gv.verdict == "INVALID" && gv.note == "file-not-found") continue;
        const std::string absPath = corpusRoot + "/" + gv.relpath;
        if (!fs::exists(absPath)) continue;

        std::string ourReason;
        std::string ourV = ourValidate(absPath, ourReason);

        std::string itemKey = "validate:" + gv.relpath;
        bool itemPass = false;

        if (ourV == "PARSE_FAIL") {
            ++parseFail;
            divergences.push_back({gv, {ourV, ourReason}});
            ++totalValidate;
            itemPass = false;
        } else {
            ++totalValidate;
            if (ourV == gv.verdict) {
                ++agree;
                itemPass = true;
            } else {
                divergences.push_back({gv, {ourV, ourReason}});
                itemPass = false;
            }
        }

        currentState[itemKey] = itemPass ? "PASS" : "FAIL";
    }

    double agreeRate = totalValidate > 0 ? 100.0 * agree / totalValidate : 0.0;
    std::cout << "Validated " << totalValidate << " files.\n";
    std::cout << "Agreement: " << agree << "/" << totalValidate
              << " (" << static_cast<int>(agreeRate + 0.5) << "%)\n";
    std::cout << "Disagreements: " << (divergences.size() - parseFail)
              << " + " << parseFail << " parse failures\n\n";

    // Category breakdown
    std::map<std::string, int> catCount;
    for (const auto &[gv, ov] : divergences) {
        catCount[classifyDisagreement(ov.first, ov.second, gv.verdict, gv.note)]++;
    }
    if (!catCount.empty()) {
        std::cout << "Disagreement categories:\n";
        for (const auto &[cat, n] : catCount)
            std::cout << "  " << cat << ": " << n << "\n";
        std::cout << "\n";
    }

    // ── Phase 2: convert roundtrip ────────────────────────────────────────────

    std::cout << "=== Phase 2: convert roundtrip ===\n";

    const std::vector<std::string> kEncodings = {"xml", "vrml", "json"};
    std::vector<ConvertResult> convertResults;

    // Convert-roundtrip: only run on a capped subset to keep gate fast.
    // We take up to 80 files that parse OK (different from validate-diff).
    int rtRun = 0, rtOk = 0;
    for (const auto &gv : verdicts) {
        if (rtRun >= 80) break;
        if (gv.verdict == "INVALID" && gv.note == "file-not-found") continue;
        const std::string absPath = corpusRoot + "/" + gv.relpath;
        if (!fs::exists(absPath)) continue;

        // Parse source.
        sdk::X3DDocument srcDoc;
        try {
            srcDoc = sdk::parseFile(absPath);
        } catch (...) {
            continue; // skip files that don't parse
        }
        ++rtRun;

        const std::string fromEnc = encFamily(extOf(absPath));
        if (fromEnc.empty()) continue;

        for (const auto &toEnc : kEncodings) {
            if (toEnc == fromEnc) continue;
            auto r = testConvert(gv.relpath, absPath, srcDoc, fromEnc, toEnc);
            std::string itemKey = "convert:" + gv.relpath + ":" + fromEnc + "->" + toEnc;
            currentState[itemKey] = r.ok ? "PASS" : "FAIL";
            if (r.ok) ++rtOk;
            convertResults.push_back(std::move(r));
        }
    }

    int rtTotal = static_cast<int>(convertResults.size());
    double rtRate = rtTotal > 0 ? 100.0 * rtOk / rtTotal : 0.0;
    std::cout << "Roundtrip: " << rtOk << "/" << rtTotal
              << " (" << static_cast<int>(rtRate + 0.5) << "%) conversions pass\n\n";

    // ── Phase 3: write report ─────────────────────────────────────────────────

    std::cout << "Writing divergence report to " << reportMd << "...\n";
    writeReport(reportMd, verdicts, divergences, convertResults,
                totalValidate, agree, parseFail);
    std::cout << "Done: " << reportMd << "\n\n";

    // Summary
    std::cout << "=== SUMMARY ===\n";
    std::cout << "Validate agreement: " << agree << "/" << totalValidate
              << " (" << static_cast<int>(agreeRate + 0.5) << "%)\n";
    std::cout << "Convert roundtrip:  " << rtOk << "/" << rtTotal
              << " (" << static_cast<int>(rtRate + 0.5) << "%)\n";
    std::cout << "Divergence report:  " << reportMd << "\n\n";

    // ── Write baseline mode ───────────────────────────────────────────────────
    if (writeBaselineMode) {
        writeBaseline(baselinePath, currentState, totalValidate, agree, rtTotal, rtOk);
        std::cout << "Baseline written: " << baselinePath
                  << " (" << currentState.size() << " entries)\n";
        return 0;
    }

    // ── Gate mode: regression check ──────────────────────────────────────────
    if (gateMode) {
        // A gate that cannot find its inputs must NOT green. If every subset file
        // was skipped (e.g. the corpus dir is absent), currentState is empty —
        // refuse to pass rather than silently report "no regressions".
        if (currentState.empty()) {
            std::cerr << "ERROR: gate ran 0 checks — no corpus files found under '"
                      << corpusRoot << "'. Refusing to pass (a gate with no inputs "
                         "must not green).\n";
            return 1;
        }
        std::vector<std::string> regressions;  // PASS in baseline, FAIL now
        std::vector<std::string> improvements; // FAIL in baseline, PASS now

        for (const auto &[key, curVal] : currentState) {
            auto it = baseline.find(key);
            if (it == baseline.end()) continue; // new item, not in baseline → skip
            const std::string &baseVal = it->second;
            if (baseVal == "PASS" && curVal == "FAIL") {
                regressions.push_back(key);
            } else if (baseVal == "FAIL" && curVal == "PASS") {
                improvements.push_back(key);
            }
        }

        std::sort(regressions.begin(), regressions.end());
        std::sort(improvements.begin(), improvements.end());

        std::cout << "=== REGRESSION GATE ===\n";
        if (!regressions.empty()) {
            std::cout << "FAIL — " << regressions.size() << " regression(s) detected:\n";
            for (const auto &k : regressions)
                std::cout << "  REGRESSION: " << k << "\n";
        } else {
            std::cout << "PASS — no regressions (all baseline-PASS items still pass).\n";
        }

        if (!improvements.empty()) {
            std::cout << "\nNOTE: " << improvements.size() << " improvement(s) detected"
                         " (baseline-FAIL now passes):\n";
            for (const auto &k : improvements)
                std::cout << "  IMPROVED: " << k << "\n";
            std::cout << "  -> Refresh baseline with: mise run cli-gate-baseline\n";
        }

        return regressions.empty() ? 0 : 1;
    }

    // Informative mode: always exit 0.
    return 0;
}
