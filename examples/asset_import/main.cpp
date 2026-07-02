#include "emit.hpp"
#include "fixture_source.hpp"
#include "texture_pipeline.hpp"
#include "x3d/authoring.hpp"
#include "x3d/sdk.hpp"
#include "x3d-cli/profile_fit.hpp"

#ifdef X3D_ASSET_IMPORT_HAVE_ASSIMP
#include "assimp_source.hpp"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <memory>

using namespace x3d::asset_import;

void countNodesRecursive(const x3d::nodes::X3DNode& node, int& count, std::unordered_set<const x3d::nodes::X3DNode*>& visited) {
  if (!visited.insert(&node).second) return;
  count++;
  for (const auto& f : node.fields()) {
    if (!f.isNode() || !f.isReadable()) continue;
    std::any val = f.get(node);
    if (f.type == x3d::core::X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<x3d::nodes::X3DNode>>(val);
      if (child) countNodesRecursive(*child, count, visited);
    } else if (f.type == x3d::core::X3DFieldType::MFNode) {
      auto children = std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(val);
      for (const auto& child : children) {
        if (child) countNodesRecursive(*child, count, visited);
      }
    }
  }
}

int countNodes(const x3d::runtime::Scene& scene) {
  int count = 0;
  std::unordered_set<const x3d::nodes::X3DNode*> visited;
  for (const auto& root : scene.rootNodes) {
    if (root) countNodesRecursive(*root, count, visited);
  }
  return count;
}

int main(int argc, char** argv) {
  const std::string usageMsg =
      "usage: x3d_asset_import <input> [-o <output>] [--format xml|json|vrml|canonical]\n"
      "                        [--assets-dir <dir>] [--no-textures] [--recompress]\n"
      "                        [--profile auto|interchange|immersive|full] [--verify] [--stats]\n";

  std::string input;
  std::string outPath;
  std::string format;
  std::string assetsDir;
  bool noTextures = false;
  bool recompress = false;
  std::string profileSelect = "auto";
  bool verify = false;
  bool printStats = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << usageMsg;
      return 0;
    } else if (arg == "-o") {
      if (i + 1 >= argc) {
        std::cerr << "error: -o option requires an argument\n";
        return 1;
      }
      outPath = argv[++i];
    } else if (arg == "--format") {
      if (i + 1 >= argc) {
        std::cerr << "error: --format option requires an argument\n";
        return 1;
      }
      format = argv[++i];
      if (format != "xml" && format != "json" && format != "vrml" && format != "canonical") {
        std::cerr << "error: unknown format: " << format << "\n";
        return 1;
      }
    } else if (arg == "--assets-dir") {
      if (i + 1 >= argc) {
        std::cerr << "error: --assets-dir option requires an argument\n";
        return 1;
      }
      assetsDir = argv[++i];
    } else if (arg == "--no-textures") {
      noTextures = true;
    } else if (arg == "--recompress") {
      recompress = true;
    } else if (arg == "--profile") {
      if (i + 1 >= argc) {
        std::cerr << "error: --profile option requires an argument\n";
        return 1;
      }
      profileSelect = argv[++i];
      if (profileSelect != "auto" && profileSelect != "interchange" &&
          profileSelect != "immersive" && profileSelect != "full") {
        std::cerr << "error: unknown profile: " << profileSelect << "\n";
        return 1;
      }
    } else if (arg == "--verify") {
      verify = true;
    } else if (arg == "--stats") {
      printStats = true;
    } else if (arg[0] == '-') {
      std::cerr << "error: unknown option: " << arg << "\n" << usageMsg;
      return 1;
    } else {
      if (input.empty()) {
        input = arg;
      } else {
        std::cerr << "error: multiple input files specified: " << input << " and " << arg << "\n" << usageMsg;
        return 1;
      }
    }
  }

  if (input.empty()) {
    std::cerr << "error: missing input file\n" << usageMsg;
    return 1;
  }

  // 1. Select Backend & Load
  std::unique_ptr<ImportSource> source;
  ImportScene scene;
  try {
    if (input.rfind("fixture:", 0) == 0) {
      source = std::make_unique<FixtureSource>();
      std::string name = input.substr(8);
      scene = source->load(name);
    } else {
#ifdef X3D_ASSET_IMPORT_HAVE_ASSIMP
      source = std::make_unique<AssimpSource>();
      scene = source->load(input);
#else
      std::cerr << "error: rebuild with -DX3D_CPP_BUILD_ASSIMP=ON to import real asset files\n";
      return 1;
#endif
    }
  } catch (const std::exception& e) {
    std::cerr << "error: failed to load input '" << input << "': " << e.what() << "\n";
    return 2;
  }

  // 2. Derive Default Output Path & Format
  if (outPath.empty()) {
    std::string stem;
    if (input.rfind("fixture:", 0) == 0) {
      stem = input.substr(8);
    } else {
      stem = std::filesystem::path(input).stem().string();
    }
    outPath = stem + ".x3d";
  }

  if (format.empty()) {
    std::string ext = std::filesystem::path(outPath).extension().string();
    if (ext == ".json") {
      format = "json";
    } else if (ext == ".x3dv" || ext == ".wrl") {
      format = "vrml";
    } else {
      format = "xml";
    }
  }

  // 3. Plan textures
  std::string outDir = ".";
  if (!assetsDir.empty()) {
    outDir = assetsDir;
  } else {
    outDir = std::filesystem::path(outPath).parent_path().string();
    if (outDir.empty()) outDir = ".";
  }

  std::string modelDir = "";
  if (input.rfind("fixture:", 0) != 0) {
    modelDir = std::filesystem::path(input).parent_path().string();
  }

  TexturePlan plan;
  if (!noTextures) {
    plan = planTextures(scene, outDir, modelDir, recompress);
  }

  // 4. Emit Document Model
  EmitOptions emitOpts;
  emitOpts.includeTextures = !noTextures;
  if (!noTextures) {
    emitOpts.textures = &plan;
  }

  x3d::runtime::X3DDocument doc = emit(scene, emitOpts);

  // 5. Profile Fit
  if (profileSelect == "interchange") {
    doc.profile = x3d::runtime::Profile::Interchange;
  } else if (profileSelect == "immersive") {
    doc.profile = x3d::runtime::Profile::Immersive;
  } else if (profileSelect == "full") {
    doc.profile = x3d::runtime::Profile::Full;
  } else {
    // auto
    auto usage = profile_fit::sceneComponentUsage(doc.scene);
    const profile_fit::ProfileDef* minProf = profile_fit::findMinimalProfile(usage);
    if (minProf) {
      doc.profile = minProf->profile;
    } else {
      doc.profile = x3d::runtime::Profile::Full;
    }
  }

  // 6. Range check / Self-validation
  std::vector<x3d::core::RangeDiagnostic> rangeWarnings;
  for (const auto& root : doc.scene.rootNodes) {
    if (root) {
      ::collectRangeWarnings(*root, rangeWarnings);
    }
  }

  if (!rangeWarnings.empty()) {
    std::cerr << "Validation warnings/errors found:\n";
    for (const auto& w : rangeWarnings) {
      std::cerr << "  " << w.message() << "\n";
    }
  }

  // 7. Stats
  if (printStats) {
    std::cout << "Import stats:\n"
              << "  Nodes emitted: " << countNodes(doc.scene) << "\n"
              << "  Routes emitted: " << doc.scene.routes.size() << "\n"
              << "  Textures planned: " << plan.urlByKey.size() << "\n"
              << "  Profile: " << x3d::runtime::toString(doc.profile) << "\n"
              << "  Range warnings: " << rangeWarnings.size() << "\n";
  }

  if (!rangeWarnings.empty()) {
    std::cerr << "error: scene contains out-of-range field values (hard-invalid)\n";
    return 3;
  }

  // 8. Write file
  std::string content;
  try {
    if (format == "xml") {
      content = x3d::authoring::XmlWriter{}.writeDocument(doc);
    } else if (format == "json") {
      content = x3d::authoring::JsonWriter{}.writeDocument(doc);
    } else if (format == "vrml") {
      content = x3d::authoring::VrmlWriter{}.writeDocument(doc);
    } else if (format == "canonical") {
      content = x3d::authoring::CanonicalXmlWriter{}.writeDocument(doc);
    }
  } catch (const std::exception& e) {
    std::cerr << "error: failed to serialize document: " << e.what() << "\n";
    return 2;
  }

  std::ofstream ofs(outPath, std::ios::binary);
  if (!ofs) {
    std::cerr << "error: failed to open output file for writing: " << outPath << "\n";
    return 2;
  }
  ofs << content;
  ofs.close();

  // 9. Verify re-parse
  if (verify) {
    try {
      x3d::runtime::X3DDocument reparsed = x3d::codec::parseFile(outPath);
      int origNodes = countNodes(doc.scene);
      int repNodes = countNodes(reparsed.scene);
      int origRoutes = doc.scene.routes.size();
      int repRoutes = reparsed.scene.routes.size();

      std::cout << "Verification re-parse results:\n"
                << "  Original nodes: " << origNodes << ", Re-parsed nodes: " << repNodes << "\n"
                << "  Original routes: " << origRoutes << ", Re-parsed routes: " << repRoutes << "\n";

      if (origNodes != repNodes || origRoutes != repRoutes) {
        std::cerr << "error: verification mismatch between original and re-parsed scene structure\n";
        return 3;
      }
    } catch (const std::exception& e) {
      std::cerr << "error: verification re-parse failed: " << e.what() << "\n";
      return 2;
    }
  }

  return 0;
}
