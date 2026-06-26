// tools/corpus_sweep.cpp
// ─────────────────────────────────────────────────────────────────────────────
// CORPUS SMOKE — drive the FULL v1 pipeline over a whole directory tree of real
// X3D scenes and report categorized stats. This is the "did we break breadth?"
// gate that the 8-scene hand-smoke could not give.
//
// Per file:  parseFile -> buildSceneGraph + buildFrom -> SceneExtractor
//            -> fullSnapshot() -> tick() -> delta()
// using ONLY the x3d::sdk façade (the exact path a real embedder takes).
//
// Outcomes per file are bucketed: OK / parse-error / pipeline-error, split by
// encoding (extension). Aggregates: total render items, files yielding 0 items,
// skipped-geometry type histogram, range/proto warning totals, error samples.
//
// Usage:
//   corpus_sweep <dir> [--limit N] [--min-success-ratio R] [--strict]
//                      [--quiet] [--list-errors K]
//
//   --limit N             process only the first N files (sorted; deterministic)
//   --min-success-ratio R fail (exit 2) if OK/total < R          (default 0.0)
//   --strict              fail (exit 2) on ANY parse/pipeline error
//   --quiet               suppress per-file lines; print only the summary
//   --list-errors K       print up to K distinct error messages   (default 12)
//
// Exit: 0 = swept clean (and ratio/strict satisfied); 0 + "SKIPPED" if <dir>
//       is absent (so a portable ctest passes on machines without the corpus);
//       2 = ratio/strict violation; a hard crash kills the process (the smoke
//       fails loudly, which is the point).
// ─────────────────────────────────────────────────────────────────────────────
#include "x3d/sdk.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace sdk = x3d::sdk;
namespace fs = std::filesystem;

namespace {

struct Args {
  std::string dir;
  long limit = -1;
  double minRatio = 0.0;
  bool strict = false;
  bool quiet = false;
  bool trace = false;
  int listErrors = 12;
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

// Bucket by encoding family for the per-encoding breakdown.
std::string encKey(const fs::path &p) {
  const std::string ext = lower(p.extension().string());
  if (ext == ".x3d" || ext == ".x3dz") return "xml";
  if (ext == ".x3dv" || ext == ".x3dvz") return "classicvrml";
  if (ext == ".wrl" || ext == ".wrz") return "vrml97";
  if (ext == ".json") return "json";
  return "other";
}

struct EncStat {
  long total = 0, ok = 0, parseErr = 0, pipeErr = 0;
};

}  // namespace

int main(int argc, char **argv) {
  Args a;
  std::vector<std::string> pos;
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    auto next = [&](double def) -> double {
      return (i + 1 < argc) ? std::stod(argv[++i]) : def;
    };
    auto nextLong = [&](long def) -> long {
      return (i + 1 < argc) ? std::stol(argv[++i]) : def;
    };
    try {
      if (s == "--limit") a.limit = nextLong(-1);
      else if (s == "--min-success-ratio") a.minRatio = next(0.0);
      else if (s == "--strict") a.strict = true;
      else if (s == "--quiet") a.quiet = true;
      else if (s == "--trace") a.trace = true;
      else if (s == "--list-errors") a.listErrors = static_cast<int>(nextLong(12));
      else pos.push_back(s);
    } catch (const std::exception &) {
      std::cerr << "usage: corpus_sweep <dir> [--limit N] [--min-success-ratio R]"
                   " [--strict] [--quiet] [--trace] [--list-errors K]\n";
      return 64;
    }
  }
  if (pos.empty()) {
    std::cerr << "usage: corpus_sweep <dir> [--limit N] [--min-success-ratio R]"
                 " [--strict] [--quiet] [--trace] [--list-errors K]\n";
    return 64;
  }
  a.dir = pos.front();

  std::error_code ec;
  if (!fs::exists(a.dir, ec) || !fs::is_directory(a.dir, ec)) {
    std::cout << "SKIPPED: corpus dir not present: " << a.dir << "\n";
    return 0;  // portable: a machine without the corpus is not a failure.
  }

  // Collect + sort for determinism.
  std::vector<fs::path> files;
  for (auto it = fs::recursive_directory_iterator(
           a.dir, fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (ec) { ec.clear(); continue; }
    if (it->is_regular_file(ec) && isSceneFile(it->path())) {
      files.push_back(it->path());
    }
  }
  std::sort(files.begin(), files.end());
  if (a.limit >= 0 && static_cast<long>(files.size()) > a.limit)
    files.resize(a.limit);

  long total = 0, ok = 0, parseErr = 0, pipeErr = 0, zeroItems = 0;
  long long totalItems = 0, totalRange = 0, totalProto = 0;
  std::map<std::string, EncStat> byEnc;
  std::map<std::string, int> skippedGeom;     // aggregate unsupported-geometry
  std::map<std::string, int> errorSamples;    // message -> count

  const auto t0 = std::chrono::steady_clock::now();
  for (const fs::path &p : files) {
    ++total;
    EncStat &es = byEnc[encKey(p)];
    ++es.total;
    const std::string rel = fs::relative(p, a.dir, ec).string();
    // Unbuffered so a SEGV/abort in a file leaves the culprit as the last line.
    if (a.trace) std::cerr << "TRACE " << rel << std::endl;

    sdk::X3DDocument doc;
    try {
      doc = sdk::parseFile(p.string());
    } catch (const std::exception &e) {
      ++parseErr; ++es.parseErr;
      ++errorSamples[std::string("parse: ") + e.what()];
      if (!a.quiet) std::cout << "PARSE-ERR  " << rel << "  : " << e.what() << "\n";
      continue;
    }
    totalRange += static_cast<long long>(doc.rangeWarnings.size());
    totalProto += static_cast<long long>(doc.protoWarnings.size());

    try {
      sdk::X3DExecutionContext ctx;
      ctx.buildSceneGraph(doc.scene);
      ctx.buildFrom(doc.scene);
      sdk::MeshBuildOptions opts;
      sdk::SceneExtractor ex(ctx, doc.scene, opts);
      sdk::RenderDelta frame0 = ex.fullSnapshot();
      (void)ex.camera();
      (void)ex.lights();
      (void)ex.sceneWorldBounds();
      ctx.tick(0.016);
      (void)ex.delta();

      const long items = static_cast<long>(frame0.added.size());
      totalItems += items;
      if (items == 0) ++zeroItems;
      for (const auto &kv : ex.skippedGeometryCounts())
        skippedGeom[kv.first] += kv.second;

      ++ok; ++es.ok;
      if (!a.quiet)
        std::cout << "OK         " << rel << "  items=" << items << "\n";
    } catch (const std::exception &e) {
      ++pipeErr; ++es.pipeErr;
      ++errorSamples[std::string("pipeline: ") + e.what()];
      if (!a.quiet) std::cout << "PIPE-ERR   " << rel << "  : " << e.what() << "\n";
    }
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double secs =
      std::chrono::duration<double>(t1 - t0).count();

  const double ratio = total ? double(ok) / double(total) : 1.0;

  std::cout << "\n=============== CORPUS SWEEP SUMMARY ===============\n";
  std::cout << "dir            : " << a.dir << "\n";
  std::cout << "files swept    : " << total << "  in " << secs << "s"
            << (total ? "  (" + std::to_string(secs * 1000.0 / total) +
                            " ms/file avg)"
                      : "")
            << "\n";
  std::cout << "OK             : " << ok << "  (" << (ratio * 100.0) << "%)\n";
  std::cout << "parse-errors   : " << parseErr << "\n";
  std::cout << "pipeline-errors: " << pipeErr << "\n";
  std::cout << "0-item OK files: " << zeroItems << "\n";
  std::cout << "render items   : " << totalItems << "\n";
  std::cout << "range warnings : " << totalRange << "\n";
  std::cout << "proto warnings : " << totalProto << "\n";

  std::cout << "\n-- by encoding --\n";
  for (const auto &kv : byEnc) {
    const EncStat &e = kv.second;
    std::cout << "  " << kv.first << ": total=" << e.total << " ok=" << e.ok
              << " parse-err=" << e.parseErr << " pipe-err=" << e.pipeErr
              << "\n";
  }

  if (!skippedGeom.empty()) {
    std::cout << "\n-- skipped (unsupported) geometry types --\n";
    std::vector<std::pair<std::string, int>> sg(skippedGeom.begin(),
                                                skippedGeom.end());
    std::sort(sg.begin(), sg.end(),
              [](auto &x, auto &y) { return x.second > y.second; });
    for (const auto &kv : sg)
      std::cout << "  " << kv.first << ": " << kv.second << "\n";
  }

  if (!errorSamples.empty() && a.listErrors > 0) {
    std::cout << "\n-- error messages (distinct, top " << a.listErrors
              << ") --\n";
    std::vector<std::pair<std::string, int>> es(errorSamples.begin(),
                                                errorSamples.end());
    std::sort(es.begin(), es.end(),
              [](auto &x, auto &y) { return x.second > y.second; });
    int shown = 0;
    for (const auto &kv : es) {
      if (shown++ >= a.listErrors) break;
      std::cout << "  [" << kv.second << "x] " << kv.first << "\n";
    }
    if (static_cast<int>(es.size()) > a.listErrors)
      std::cout << "  ... and " << (es.size() - a.listErrors)
                << " more distinct message(s)\n";
  }
  std::cout << "===================================================\n";

  if (a.strict && (parseErr + pipeErr) > 0) {
    std::cerr << "FAIL (--strict): " << (parseErr + pipeErr) << " error(s)\n";
    return 2;
  }
  if (ratio < a.minRatio) {
    std::cerr << "FAIL: success ratio " << ratio << " < floor " << a.minRatio
              << "\n";
    return 2;
  }
  return 0;
}
