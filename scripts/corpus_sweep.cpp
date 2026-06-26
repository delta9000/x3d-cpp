// corpus_sweep.cpp — measure parser conformance against an external corpus.
//
// Walks a corpus root, runs every .wrl/.x3dv/.x3d/.json/.gz file through
// x3d::codec::parseFile, and buckets the outcomes by encoding. Failures are
// grouped by a normalized signature (numbers/identifiers elided) so the long
// tail collapses into a rankable list of distinct parser gaps.
//
// Usage: corpus_sweep <corpus-root> [--ext wrl,x3dv,...] [--show N] [--enc VRML97,ClassicVRML]
// Not built by CI; a throwaway diagnostic driver. Exit code 0 always.

#include "X3DParse.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using x3d::codec::Encoding;

namespace {

const char *encName(Encoding e) {
  switch (e) {
  case Encoding::XML: return "XML";
  case Encoding::ClassicVRML: return "ClassicVRML";
  case Encoding::VRML97: return "VRML97";
  case Encoding::JSON: return "JSON";
  default: return "Unknown";
  }
}

// Collapse a concrete error message into a signature: drop quoted tokens,
// numbers, and file paths so "expected '{', got 'Foo'" and "...got 'Bar'"
// bucket together.
std::string signature(const std::string &msg) {
  std::string s = msg;
  static const std::regex quoted("'[^']*'");
  static const std::regex num("[0-9]+");
  s = std::regex_replace(s, quoted, "'X'");
  s = std::regex_replace(s, num, "N");
  if (s.size() > 120) s = s.substr(0, 120);
  return s;
}

Encoding sniffPath(const std::string &path) {
  std::string ext = fs::path(path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  if (ext == ".gz" || ext == ".gzip") {
    std::string stem = fs::path(path).stem().extension().string();
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
    ext = stem;
  }
  if (ext == ".wrl" || ext == ".wrz") return Encoding::VRML97;
  if (ext == ".x3dv" || ext == ".x3dvz") return Encoding::ClassicVRML;
  if (ext == ".x3d" || ext == ".x3dz") return Encoding::XML;
  if (ext == ".json") return Encoding::JSON;
  return Encoding::Unknown;
}

struct Bucket {
  int count = 0;
  std::vector<std::string> samples; // up to 3
};
struct Stats {
  int total = 0, ok = 0;
  std::map<std::string, Bucket> failsBySig; // sig -> bucket
};

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: corpus_sweep <root> [--ext a,b] [--show N] [--enc E,F]\n";
    return 0;
  }
  std::string root = argv[1];
  std::vector<std::string> exts = {"wrl", "x3dv"};
  std::vector<std::string> encFilter;
  std::string dumpFails;
  int show = 12;
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    auto split = [](const std::string &v) {
      std::vector<std::string> out; std::string cur;
      for (char c : v) { if (c == ',') { out.push_back(cur); cur.clear(); } else cur += c; }
      if (!cur.empty()) out.push_back(cur); return out;
    };
    if (a == "--ext" && i + 1 < argc) exts = split(argv[++i]);
    else if (a == "--enc" && i + 1 < argc) encFilter = split(argv[++i]);
    else if (a == "--show" && i + 1 < argc) show = std::stoi(argv[++i]);
    else if (a == "--dumpfails" && i + 1 < argc) dumpFails = argv[++i];
  }
  auto wantExt = [&](const std::string &e) {
    return std::find(exts.begin(), exts.end(), e) != exts.end();
  };
  auto wantEnc = [&](Encoding e) {
    if (encFilter.empty()) return true;
    return std::find(encFilter.begin(), encFilter.end(), encName(e)) != encFilter.end();
  };

  std::map<Encoding, Stats> byEnc;
  std::vector<std::string> allFails; // "ENC\tsig\tpath" when --dumpfails set
  std::error_code ec;
  for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (ec) { ec.clear(); continue; }
    if (!it->is_regular_file(ec)) continue;
    std::string path = it->path().string();
    std::string ext = it->path().extension().string();
    std::string e = ext.size() > 1 ? ext.substr(1) : "";
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    // resolve .gz to inner ext for the --ext filter
    if (e == "gz" || e == "gzip") {
      std::string inner = it->path().stem().extension().string();
      e = inner.size() > 1 ? inner.substr(1) : "";
      std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    }
    if (!wantExt(e)) continue;
    Encoding enc = sniffPath(path);
    if (!wantEnc(enc)) continue;
    Stats &st = byEnc[enc];
    st.total++;
    try {
      auto doc = x3d::codec::parseFile(path);
      (void)doc;
      st.ok++;
    } catch (const std::exception &ex) {
      std::string sig = signature(ex.what());
      auto &b = st.failsBySig[sig];
      b.count++;
      if (b.samples.size() < 3) b.samples.push_back(path);
      if (!dumpFails.empty())
        allFails.push_back(std::string(encName(enc)) + "\t" + sig + "\t" + path);
    } catch (...) {
      auto &b = st.failsBySig["<non-std exception>"];
      b.count++;
      if (b.samples.size() < 3) b.samples.push_back(path);
    }
  }

  std::cout << "\n=== Corpus conformance sweep: " << root << " ===\n";
  for (auto &[enc, st] : byEnc) {
    int fails = st.total - st.ok;
    std::cout << "\n## " << encName(enc) << "  " << st.ok << "/" << st.total
              << "  (" << fails << " failing)\n";
    std::vector<std::pair<std::string, Bucket>> ranked(
        st.failsBySig.begin(), st.failsBySig.end());
    std::sort(ranked.begin(), ranked.end(),
              [](auto &a, auto &b) { return a.second.count > b.second.count; });
    int shown = 0;
    for (auto &[sig, b] : ranked) {
      if (shown++ >= show) { std::cout << "   ... (" << (int(ranked.size()) - show) << " more signatures)\n"; break; }
      std::cout << "   [" << b.count << "x] " << sig << "\n";
      std::cout << "        e.g. " << b.samples.front() << "\n";
    }
  }
  std::cout << "\n";
  if (!dumpFails.empty()) {
    std::ofstream out(dumpFails);
    for (auto &line : allFails) out << line << "\n";
    std::cerr << "wrote " << allFails.size() << " failing paths to " << dumpFails << "\n";
  }
  return 0;
}
