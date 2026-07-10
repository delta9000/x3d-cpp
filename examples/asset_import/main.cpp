#include "backend_registry.hpp"
#include "emit.hpp"
#include "fixture_source.hpp"
#include "glsl_emit.hpp"
#include "texture_pipeline.hpp"
#include "x3d/authoring.hpp"
#include "x3d/sdk.hpp"
#include "x3d-cli/profile_fit.hpp"

#ifdef X3D_ASSET_IMPORT_HAVE_CGLTF
#include "cgltf_source.hpp"
#endif

#ifdef X3D_ASSET_IMPORT_HAVE_ASSIMP
#include "assimp_source.hpp"
#endif

#ifdef X3D_ASSET_IMPORT_HAVE_USD
#include "usd_source.hpp"
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

// Turns a material name into a filesystem-safe stem: keep alnum/-/_, replace
// everything else with '_'. Falls back to a positional name when the
// material name is empty or collapses to nothing usable.
std::string sanitizeMaterialStem(const std::string& name, int index) {
  std::string stem;
  for (char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      stem += c;
    } else {
      stem += '_';
    }
  }
  if (stem.empty()) stem = "material_" + std::to_string(index);
  return stem;
}

// Assemble the backend registry. Registration is explicit (no static-init
// order dependence) and gated per backend by its build macro. `priority`
// inspects the whole input, so the "fixture:" prefix and file extensions
// resolve through one uniform path. glTF is claimed by both cgltf (100, the
// default) and assimp (10, the broader-format fallback).
static void registerBuiltinBackends(BackendRegistry& reg) {
  [[maybe_unused]] auto extIs = [](const std::string& in,
                                   std::initializer_list<const char*> exts) {
    auto d = in.find_last_of('.');
    if (d == std::string::npos) return false;
    std::string e = in.substr(d);
    for (auto& c : e) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (const char* x : exts)
      if (e == x) return true;
    return false;
  };
  reg.add({"fixture",
           [](const std::string& in) { return in.rfind("fixture:", 0) == 0 ? 100 : -1; },
           [] { return std::unique_ptr<ImportSource>(std::make_unique<FixtureSource>()); }});
#ifdef X3D_ASSET_IMPORT_HAVE_CGLTF
  reg.add({"cgltf",
           [extIs](const std::string& in) { return extIs(in, {".gltf", ".glb"}) ? 100 : -1; },
           [] { return std::unique_ptr<ImportSource>(std::make_unique<CgltfSource>()); }});
#endif
#ifdef X3D_ASSET_IMPORT_HAVE_ASSIMP
  reg.add({"assimp",
           [extIs](const std::string& in) {
             if (extIs(in, {".gltf", ".glb"})) return 10;
             if (extIs(in, {".obj", ".fbx", ".dae", ".ply", ".stl", ".3ds"})) return 50;
             return -1;
           },
           [] { return std::unique_ptr<ImportSource>(std::make_unique<AssimpSource>()); }});
#endif
#ifdef X3D_ASSET_IMPORT_HAVE_USD
  reg.add({"usd",
           [extIs](const std::string& in) {
             return extIs(in, {".usd", ".usda", ".usdc", ".usdz"}) ? 100 : -1;
           },
           [] { return std::unique_ptr<ImportSource>(std::make_unique<UsdSource>()); }});
#endif
}

int main(int argc, char** argv) {
  const std::string usageMsg =
      "usage: x3d_asset_import <input> [-o <output>] [--format xml|json|vrml|canonical]\n"
      "                        [--assets-dir <dir>] [--no-textures] [--recompress]\n"
      "                        [--profile auto|interchange|immersive|full] [--verify] [--stats]\n"
      "                        [--emit-glsl <dir>] [--backend cgltf|assimp|usd|fixture]\n";

  const std::string helpMsg =
      "x3d_asset_import — convert 3D assets to X3D 4.0\n\n"
      + usageMsg +
      "\nThe input selects the backend automatically (highest-priority match wins):\n"
      "  fixture:<name>            built-in synthetic scene (e.g. fixture:cube) — always available\n"
      "  *.gltf .glb               glTF via cgltf              (default; build with -DX3D_CPP_BUILD_CGLTF=ON)\n"
      "  *.usd .usda .usdc .usdz   USD / USDZ                  (build with -DX3D_CPP_BUILD_USD=ON)\n"
      "  *.obj .fbx .dae …         40+ formats via assimp      (build with -DX3D_CPP_BUILD_ASSIMP=ON;\n"
      "                            also handles .gltf/.glb as a lower-priority fallback)\n"
      "\noptions:\n"
      "  --backend <name>       force a backend: cgltf | assimp | usd | fixture (default: auto by input)\n"
      "  -o <path>              output file (default: <input-stem>.x3d)\n"
      "  --format <fmt>         xml | json | vrml | canonical (default: from -o extension, else xml)\n"
      "  --assets-dir <dir>     write textures under <dir>/assets/; URLs stay relative to the output\n"
      "  --no-textures          skip texture extraction\n"
      "  --recompress           re-encode web-safe textures to PNG (default: pass through)\n"
      "  --profile <p>          auto | interchange | immersive | full (default: auto)\n"
      "  --emit-glsl <dir>      write one portable UsdPreviewSurface .frag per material\n"
      "  --verify               re-parse the output and check node/route counts\n"
      "  --stats                print import statistics\n"
      "  -h, --help             show this help\n"
      "\nexamples:\n"
      "  x3d_asset_import fixture:cube -o cube.x3d --stats\n"
      "  x3d_asset_import model.usdz -o out/model.x3d --assets-dir out --verify\n";

  std::string input;
  std::string outPath;
  std::string format;
  std::string assetsDir;
  bool noTextures = false;
  bool recompress = false;
  std::string profileSelect = "auto";
  bool verify = false;
  bool printStats = false;
  std::string emitGlslDir;
  std::string backendFlag;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << helpMsg;
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
    } else if (arg == "--emit-glsl") {
      if (i + 1 >= argc) {
        std::cerr << "error: --emit-glsl option requires an argument\n";
        return 1;
      }
      emitGlslDir = argv[++i];
    } else if (arg == "--backend") {
      if (i + 1 >= argc) {
        std::cerr << "error: --backend option requires an argument\n";
        return 1;
      }
      backendFlag = argv[++i];
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
  BackendRegistry reg;
  registerBuiltinBackends(reg);
  const ImportBackend* backend =
      backendFlag.empty() ? reg.select(input) : reg.byName(backendFlag);
  if (!backend) {
    std::cerr << "error: no import backend for '" << input << "'.\n";
    if (!backendFlag.empty())
      std::cerr << "  unknown --backend '" << backendFlag << "'.\n";
    std::cerr << "  available backends:";
    for (const auto& n : reg.names()) std::cerr << ' ' << n;
    std::cerr << "\n  (glTF needs cgltf [default] or assimp; USD needs -DX3D_CPP_BUILD_USD=ON;"
                 " other formats need -DX3D_CPP_BUILD_ASSIMP=ON)\n";
    return 1;
  }

  std::unique_ptr<ImportSource> source;
  ImportScene scene;
  try {
    source = backend->make();
    // FixtureSource::load expects the name AFTER the "fixture:" prefix; every
    // other backend takes the path verbatim.
    std::string loadArg = input.rfind("fixture:", 0) == 0 ? input.substr(8) : input;
    scene = source->load(loadArg);
  } catch (const std::exception& e) {
    std::cerr << "error: failed to load input '" << input << "': " << e.what() << "\n";
    return 2;
  }

  // 1b. --emit-glsl: one portable, self-contained fragment shader per
  // material with UsdPreviewSurface fidelity params baked in as `const`.
  // Standalone from the X3D emit path below — the only channel that can
  // carry ior/clearcoat/specular-workflow/opacityMode faithfully, since the
  // X3D PhysicalMaterial node can't model them (see ImportMaterial).
  if (!emitGlslDir.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(emitGlslDir, ec);
    if (ec) {
      std::cerr << "error: failed to create --emit-glsl directory '" << emitGlslDir << "': " << ec.message() << "\n";
      return 2;
    }
    int written = 0;
    for (int mi = 0; mi < static_cast<int>(scene.materials.size()); ++mi) {
      const ImportMaterial& mat = scene.materials[mi];
      std::string stem = sanitizeMaterialStem(mat.name, mi);
      std::filesystem::path fragPath = std::filesystem::path(emitGlslDir) / (stem + ".frag");
      std::ofstream fragOfs(fragPath, std::ios::binary);
      if (!fragOfs) {
        std::cerr << "error: failed to open --emit-glsl output for writing: " << fragPath.string() << "\n";
        return 2;
      }
      fragOfs << emitMaterialGlsl(mat);
      fragOfs.close();
      ++written;
    }
    std::cerr << "emit-glsl: wrote " << written << " shader(s) to " << emitGlslDir << "\n";
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
  std::string outFileDir = std::filesystem::path(outPath).parent_path().string();
  if (outFileDir.empty()) outFileDir = ".";
  std::string outDir = assetsDir.empty() ? outFileDir : assetsDir;

  std::string modelDir = "";
  if (input.rfind("fixture:", 0) != 0) {
    modelDir = std::filesystem::path(input).parent_path().string();
  }

  TexturePlan plan;
  if (!noTextures) {
    plan = planTextures(scene, outDir, modelDir, recompress, outFileDir);
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

  // Always tell the user what was written and where (feedback without --stats).
  std::cerr << "wrote " << outPath << " (" << countNodes(doc.scene) << " nodes";
  if (!noTextures && !plan.urlByKey.empty()) {
    std::cerr << ", " << plan.urlByKey.size() << " texture(s) -> "
              << (std::filesystem::path(outDir) / "assets").string();
  }
  std::cerr << ")\n";

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
