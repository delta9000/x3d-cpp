// AssetImport.cpp — backend selection, texture plan and emit in one call.
#include "AssetImport.hpp"

#include "gltf/CgltfSource.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace x3d::runtime::importer {

void addGltfBackend(BackendRegistry &registry) {
  registry.add({"gltf",
                {".gltf", ".glb"},
                [](const std::string &p) { return hasExtension(p, {".gltf", ".glb"}) ? 100 : -1; },
                [] { return std::unique_ptr<ImportSource>(std::make_unique<CgltfSource>()); }});
}

BackendRegistry builtinBackends() {
  BackendRegistry r;
  addGltfBackend(r);
  return r;
}

ImportResult importScene(ImportScene scene, const std::string &modelDir,
                         const ImportOptions &opts) {
  namespace fs = std::filesystem;
  ImportResult out;
  out.report.warnings = scene.warnings;
  std::string docDir = opts.outputPath.empty() ? "." : fs::path(opts.outputPath).parent_path().string();
  if (docDir.empty()) docDir = ".";
  TexturePlan plan;
  if (opts.textures) {
    TexturePlanOptions t;
    t.assetsRoot = opts.assetsRoot.empty() ? docDir : opts.assetsRoot;
    t.modelDir = modelDir;
    t.documentDir = docDir;
    t.recompress = opts.recompress;
    t.decoder = opts.decoder;
    plan = planTextures(scene, t, &out.report.textures);
  }
  EmitOptions e;
  e.textures = opts.textures ? &plan : nullptr;
  e.autoplay = opts.autoplay;
  e.samplesPerInterval = opts.samplesPerInterval;
  out.document = emitDocument(scene, e, &out.report.emit);
  out.scene = std::move(scene);
  return out;
}

ImportResult importFile(const std::string &path, const ImportOptions &opts,
                        const BackendRegistry &registry) {
  const ImportBackend *b = opts.backend.empty() ? registry.select(path) : registry.byName(opts.backend);
  if (!b) {
    std::string names;
    for (const auto &x : registry.backends()) names += (names.empty() ? "" : ", ") + x.name;
    throw std::runtime_error(
        (opts.backend.empty() ? "no import backend reads '" + path + "'"
                              : "unknown import backend '" + opts.backend + "'") +
        " (available: " + (names.empty() ? "none" : names) + ")");
  }
  std::unique_ptr<ImportSource> source = b->make();
  ImportScene scene = source->load(path);
  ImportResult r = importScene(std::move(scene), std::filesystem::path(path).parent_path().string(), opts);
  r.report.backend = b->name;
  return r;
}

} // namespace x3d::runtime::importer
