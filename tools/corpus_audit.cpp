// tools/corpus_audit.cpp
// ─────────────────────────────────────────────────────────────────────────────
// DIFFERENTIAL AUDIT (Phase 0) — drive the ORACLE-FREE conformance audit over a
// whole directory tree of real X3D scenes and emit a machine-clusterable JSONL
// of failures. Built as the single binary `x3d_corpus_audit`.
//
// Per file that PARSES OK (parse-rejects are skipped — not this audit's concern):
//   A. INVARIANTS — bounds/transform finiteness, static-scene tick determinism,
//      re-extract stability.
//   B. ROUND-TRIP FIDELITY — write to {XML,JSON,ClassicVRML}, reparse, and
//      compare a SEMANTIC order-independent fingerprint with the original.
//
// All check logic lives in the shared header tools/corpus_audit.hpp (also driven
// by the x3d_corpus_audit_selftest unit test). This file owns ONLY: directory
// walking, flag parsing, JSONL writing, and the aggregate summary.
//
// Usage:
//   corpus_audit <dir> [--roundtrip-limit N] [--invariants-only] [--out PATH]
//                      [--limit N] [--quiet]
//
//   <dir>                 root of the scene tree to audit (recursive).
//   --roundtrip-limit N   run round-trip family B on only the first N parsed
//                         files (invariants still run on ALL). -1 = all.
//   --invariants-only     skip round-trip family B entirely.
//   --out PATH            JSONL findings output (default /tmp/corpus_audit_findings.jsonl).
//   --limit N             process only the first N scene files (deterministic).
//   --quiet               suppress per-file lines; print only the summary.
//
// Exit: 0 = swept (findings are DATA, not a build failure — Phase 1 clusters
//       them). 0 + "SKIPPED" if <dir> is absent (portable ctest). 64 = bad args.
// ─────────────────────────────────────────────────────────────────────────────
#include "corpus_audit.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace audit = x3d::audit;
namespace sdk = x3d::sdk;
namespace fs = std::filesystem;

namespace {

struct Args {
  std::string dir;
  long limit = -1;
  long roundtripLimit = -1; // -1 = all parsed files.
  bool invariantsOnly = false;
  bool quiet = false;
  std::string out = "/tmp/corpus_audit_findings.jsonl";
};

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

bool isSceneFile(const fs::path &p) {
  const std::string ext = lower(p.extension().string());
  return ext == ".x3d" || ext == ".x3dv" || ext == ".wrl" || ext == ".json" ||
         ext == ".x3dz" || ext == ".x3dvz" || ext == ".wrz";
}

// Minimal JSON string escaper (control chars, quotes, backslashes).
std::string jesc(const std::string &s) {
  std::string o;
  o.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
    case '"': o += "\\\""; break;
    case '\\': o += "\\\\"; break;
    case '\n': o += "\\n"; break;
    case '\r': o += "\\r"; break;
    case '\t': o += "\\t"; break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof buf, "\\u%04x", c & 0xff);
        o += buf;
      } else {
        o += c;
      }
    }
  }
  return o;
}

void writeJsonl(std::ostream &os, const audit::Finding &f) {
  os << '{' << "\"file\":\"" << jesc(f.file) << "\","
     << "\"check\":\"" << jesc(f.check) << "\","
     << "\"encoding\":\"" << jesc(f.encoding) << "\","
     << "\"nodeType\":\"" << jesc(f.nodeType) << "\","
     << "\"detail\":\"" << jesc(f.detail) << "\","
     << "\"signature\":\"" << jesc(f.signature) << "\"}" << '\n';
}

} // namespace

int main(int argc, char **argv) {
  Args a;
  std::vector<std::string> pos;
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    auto nextLong = [&](long def) -> long {
      return (i + 1 < argc) ? std::stol(argv[++i]) : def;
    };
    try {
      if (s == "--limit") a.limit = nextLong(-1);
      else if (s == "--roundtrip-limit") a.roundtripLimit = nextLong(-1);
      else if (s == "--invariants-only") a.invariantsOnly = true;
      else if (s == "--quiet") a.quiet = true;
      else if (s == "--out") a.out = (i + 1 < argc) ? argv[++i] : a.out;
      else pos.push_back(s);
    } catch (const std::exception &) {
      std::cerr << "usage: corpus_audit <dir> [--roundtrip-limit N] "
                   "[--invariants-only] [--out PATH] [--limit N] [--quiet]\n";
      return 64;
    }
  }
  if (pos.empty()) {
    std::cerr << "usage: corpus_audit <dir> [--roundtrip-limit N] "
                 "[--invariants-only] [--out PATH] [--limit N] [--quiet]\n";
    return 64;
  }
  a.dir = pos.front();

  std::error_code ec;
  if (!fs::exists(a.dir, ec) || !fs::is_directory(a.dir, ec)) {
    std::cout << "SKIPPED: audit dir not present: " << a.dir << "\n";
    return 0; // portable: a machine without the corpus is not a failure.
  }

  // Collect + sort for determinism.
  std::vector<fs::path> files;
  for (auto it = fs::recursive_directory_iterator(
           a.dir, fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (ec) { ec.clear(); continue; }
    if (it->is_regular_file(ec) && isSceneFile(it->path()))
      files.push_back(it->path());
  }
  std::sort(files.begin(), files.end());
  if (a.limit >= 0 && static_cast<long>(files.size()) > a.limit)
    files.resize(a.limit);

  std::ofstream jsonl(a.out, std::ios::trunc);
  if (!jsonl) {
    std::cerr << "FATAL: cannot open findings output: " << a.out << "\n";
    return 1;
  }

  long total = 0, parsed = 0, parseErr = 0, rtRan = 0;
  long long findingCount = 0;
  std::map<std::string, long long> byCheck, byEnc, bySig;

  const auto t0 = std::chrono::steady_clock::now();
  for (const fs::path &p : files) {
    ++total;
    const std::string rel = fs::relative(p, a.dir, ec).string();

    sdk::X3DDocument doc;
    try {
      doc = sdk::parseFile(p.string());
    } catch (const std::exception &) {
      ++parseErr; // parse-reject: skip — not this audit's concern.
      continue;
    }
    ++parsed;

    audit::AuditOptions ao;
    ao.invariantsOnly = a.invariantsOnly;
    ao.doRoundTrip = !a.invariantsOnly &&
                     (a.roundtripLimit < 0 || rtRan < a.roundtripLimit);
    if (ao.doRoundTrip) ++rtRan;

    std::vector<audit::Finding> findings;
    // A crash in one file should not lose the whole run's JSONL; guard the
    // per-file audit so a thrown (non-aborting) failure still records + moves on.
    try {
      audit::auditDocument(doc, rel, ao, findings);
    } catch (const std::exception &ex) {
      findings.push_back(audit::makeFinding(
          rel, "audit-throw", "", "",
          std::string("audit driver threw: ") + ex.what()));
    }

    for (const audit::Finding &f : findings) {
      writeJsonl(jsonl, f);
      ++findingCount;
      ++byCheck[f.check];
      if (!f.encoding.empty()) ++byEnc[f.encoding];
      ++bySig[f.signature];
    }
    if (!a.quiet)
      std::cout << (findings.empty() ? "CLEAN " : "FIND  ") << rel << "  ("
                << findings.size() << " finding(s))\n";
  }
  jsonl.flush();
  const auto t1 = std::chrono::steady_clock::now();
  const double secs = std::chrono::duration<double>(t1 - t0).count();

  std::cout << "\n=============== CORPUS AUDIT SUMMARY ===============\n";
  std::cout << "dir              : " << a.dir << "\n";
  std::cout << "files seen       : " << total << "  in " << secs << "s\n";
  std::cout << "parsed OK        : " << parsed << "\n";
  std::cout << "parse-rejects    : " << parseErr << "  (skipped)\n";
  std::cout << "round-trip ran   : " << rtRan
            << (a.invariantsOnly ? "  (--invariants-only)" : "") << "\n";
  std::cout << "findings         : " << findingCount << "\n";
  std::cout << "findings JSONL   : " << a.out << "\n";

  auto dumpMap = [](const char *title, const std::map<std::string, long long> &m) {
    if (m.empty()) return;
    std::vector<std::pair<std::string, long long>> v(m.begin(), m.end());
    std::sort(v.begin(), v.end(),
              [](auto &x, auto &y) { return x.second > y.second; });
    std::cout << "\n-- " << title << " --\n";
    for (const auto &kv : v)
      std::cout << "  " << kv.second << "  " << kv.first << "\n";
  };
  dumpMap("by check", byCheck);
  dumpMap("by encoding", byEnc);

  // Top signatures (the Phase 1 cluster heads) — cap the print.
  if (!bySig.empty()) {
    std::vector<std::pair<std::string, long long>> v(bySig.begin(), bySig.end());
    std::sort(v.begin(), v.end(),
              [](auto &x, auto &y) { return x.second > y.second; });
    std::cout << "\n-- top signatures (cluster heads) --\n";
    int shown = 0;
    for (const auto &kv : v) {
      if (shown++ >= 25) break;
      std::cout << "  " << kv.second << "  " << kv.first << "\n";
    }
    if (static_cast<int>(v.size()) > 25)
      std::cout << "  ... and " << (v.size() - 25) << " more signature(s)\n";
  }
  std::cout << "===================================================\n";
  return 0;
}
