// examples/x3d2svg/main.cpp
//
// x3d2svg — a headless X3D → SVG "technical illustration" projector, and the
// smallest possible reference consumer of the x3d-cpp extraction seam: it links
// ONLY the public façade (`x3d_cpp::sdk`) and uses no GPU, display, image codec,
// or font backend. It parses a scene, resolves a camera, projects every
// extracted triangle, flat-shades each facet with a headlight, and paints them
// back-to-front into an SVG. With --animate it steps the simulation clock and
// emits one SMIL-keyed <g> per frame, so an animated X3D world becomes a single
// self-contained animated SVG.
//
// Scope (see README.md): Text renders nothing (glyph geometry needs a
// FontMetrics backend this tool does not wire), and triangles crossing the near
// plane are culled rather than clipped.
#include "x3d2svg.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace sdk = x3d::sdk;

namespace {

struct Options {
  std::string in, out = "out.svg";
  int W = 900, H = 600, frames = 1;
  double fps = 30.0;
  bool forceFit = false, headless = false;
};

int usage(const char *prog, int code) {
  std::fprintf(code ? stderr : stdout,
      "usage: %s scene.x3d [-o out.svg] [-w W] [-h H]\n"
      "                    [--animate N] [--fps F] [--fit] [--headless]\n"
      "  -o FILE      output SVG (default out.svg)\n"
      "  -w, -h       pixel dimensions (default 900x600)\n"
      "  --animate N  emit N frames as one animated SVG (SMIL)\n"
      "  --fps F      frames per second for --animate (default 30)\n"
      "  --fit        force a view-all camera, ignoring any bound Viewpoint\n"
      "  --headless   extract + report counts, write no file (CI probe)\n",
      prog);
  return code;
}

} // namespace

int main(int argc, char **argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto val = [&](const char *what) -> std::string {
      if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", what);
                           usage(argv[0], 2); std::exit(2); }
      return argv[++i];
    };
    if (a == "--help") return usage(argv[0], 0);
    else if (a == "-o") o.out = val("-o");
    else if (a == "-w") o.W = std::atoi(val("-w").c_str());
    else if (a == "-h") o.H = std::atoi(val("-h").c_str());
    else if (a == "--animate") o.frames = std::max(1, std::atoi(val("--animate").c_str()));
    else if (a == "--fps") o.fps = std::atof(val("--fps").c_str());
    else if (a == "--fit") o.forceFit = true;
    else if (a == "--headless") o.headless = true;
    else if (!a.empty() && a[0] == '-') {
      std::fprintf(stderr, "unknown option: %s\n", a.c_str());
      return usage(argv[0], 2);
    } else if (o.in.empty()) o.in = a;
    else { std::fprintf(stderr, "unexpected argument: %s\n", a.c_str());
           return usage(argv[0], 2); }
  }
  if (o.in.empty()) return usage(argv[0], 2);
  if (o.W < 1 || o.H < 1 || o.fps <= 0) {
    std::fprintf(stderr, "bad dimensions or fps\n");
    return 2;
  }

  std::unique_ptr<sdk::RuntimeSession> session;
  try {
    session = sdk::RuntimeSession::create(sdk::parseFile(o.in));
  } catch (const std::exception &e) {
    std::fprintf(stderr, "x3d2svg: failed to load %s: %s\n", o.in.c_str(), e.what());
    return 1;
  }
  auto &ex = session->extractor();
  session->fullSnapshot();
  session->tick(0.0);
  session->delta();

  // Resolve the camera. Respect a bound Viewpoint by default; synthesize a
  // view-all fit when none is bound OR --fit forces it. Safety net: if the
  // bound Viewpoint frames nothing (a geo-located tile, an off-scene camera),
  // fall back to the fit so an arbitrary scene still yields an image.
  const sdk::Aabb bounds = ex.sceneWorldBounds();
  const bool hasVP = session->context().boundViewpoint() != nullptr;
  const double fov = ex.camera().fieldOfView;

  x3d2svg::Camera cam;
  bool usingFit = o.forceFit || !hasVP;
  if (usingFit) {
    cam = x3d2svg::fitCamera(bounds, fov);
  } else {
    cam.useMatrix = true;
    cam.view = ex.camera().viewMatrix;
    cam.fov = fov;
  }

  std::vector<x3d2svg::Facet> frame0 = x3d2svg::collect(ex, cam, o.W, o.H);
  bool autoFit = false;
  if (frame0.empty() && !usingFit && !bounds.empty) {
    cam = x3d2svg::fitCamera(bounds, fov);
    usingFit = true;
    autoFit = true;
    frame0 = x3d2svg::collect(ex, cam, o.W, o.H);
  }
  const char *camName = usingFit ? "view-all-fit" : "bound-viewpoint";

  if (o.headless) {
    long long items = session->fullSnapshot().added.size();
    std::printf("x3d2svg headless: render_items=%lld facets=%zu camera=%s%s\n",
                items, frame0.size(), camName, autoFit ? " (auto-fallback)" : "");
    return frame0.empty() ? 1 : 0;
  }

  std::ostringstream svg;
  x3d2svg::emitSvgHeader(svg, o.W, o.H);
  long long totalFacets = frame0.size();
  x3d2svg::emitGroup(svg, frame0, 0, o.frames, o.fps);
  for (int f = 1; f < o.frames; ++f) {
    session->tick(f / o.fps);
    session->delta();
    std::vector<x3d2svg::Facet> facets = x3d2svg::collect(ex, cam, o.W, o.H);
    totalFacets += (long long)facets.size();
    x3d2svg::emitGroup(svg, facets, f, o.frames, o.fps);
  }
  svg << "</svg>\n";

  std::ofstream ofs(o.out);
  if (!ofs) { std::fprintf(stderr, "x3d2svg: cannot write %s\n", o.out.c_str()); return 1; }
  ofs << svg.str();
  std::printf("x3d2svg: %s -> %s  (%dx%d, %d frame(s), %lld facets, camera=%s%s)\n",
              o.in.c_str(), o.out.c_str(), o.W, o.H, o.frames, totalFacets, camName,
              autoFit ? " auto-fallback" : "");
  return 0;
}
