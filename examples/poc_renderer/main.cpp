// examples/poc_renderer/main.cpp
//
// PoC OpenGL renderer — an out-of-SDK CONSUMER of the headless x3d_cpp runtime
// SDK. The renderer is NOT part of the SDK; it links only the INTERFACE alias
// x3d_cpp::x3d_cpp across the extraction seam and is firewalled behind
// X3D_CPP_BUILD_POC=OFF so it never touches the default build/golden/ctest path.
//
// MILESTONE COVERAGE
//   M0 (task T9): Wayland GLFW window + glad core-3.3 context + camera + a
//     clear-to-Background-color render loop with a GL_KHR_debug callback and a
//     forced full-screen clear sanity step (so a black screen is diagnosable).
//   M1 (task T10): draw one extracted mesh, unlit-flat, via the T7a minimal
//     SceneExtractor (GpuMesh cache keyed by GeomId, GL_CULL_FACE OFF).
//   M2 (task T11): incremental delta()-driven GpuMesh cache + a USE'd shape
//     rendered at both per-path world transforms.
//   M3 (task T12): per-pixel LIGHTING (Lambert + ambient, two-sided
//     N=gl_FrontFacing?N:-N via lit.vert/lit.frag), MaterialDesc color
//     (diffuse/emissive/transparency + per-vertex Color override), and per-draw
//     back-face CULLING honoring MeshData.ccw/solid. The bound NavigationInfo
//     headlight (§23.4.4) is an additional camera-space directional light
//     whenever headlight is TRUE (default), independent of the scene's lights;
//     the extractor's world-resolved LightDescs (global=false lights
//     are NOT promoted to scene-wide) light the scene too.
//
// PHASE 5 (material-shader PoC program):
//   Per-program dispatch in drawItem: unlit / phong / pbr / author-shader.
//   lit.frag extended: normal-map + emissive-tex + specular-tex + slot uniforms.
//   pbr.frag: metallic-roughness analytic BRDF (NO IBL — Phase 4 deferred).
//   author-shader: ComposedShader dispatch via ShaderBindingPlan vocabulary.
//   Gamma/sRGB: LINEARtoSRGB at fragment output; sRGB texture internal format.
//
// #39 (Background gradient): the M0 flat clear above only ever painted the
// bound Background's first skyColor. bg.vert/bg.frag now paint the FULL
// sky/ground gradient (skyColor/skyAngle/groundColor/groundAngle) as a
// depth-test-disabled fullscreen dome drawn before scene geometry, ported
// independently from examples/cpu_raster's skyGroundColor()/colorRamp() (same
// §Background semantics: groundColor never extends past its last groundAngle,
// so a last angle < pi/2 leaves that band showing sky through it — pinned by
// the NIST verify_lastground.x3d conformance case). The flat clear remains
// the fast path for a single-skyColor Background (or none bound).
//
// The M0 GATE is configure+build+LINK. The interactive GUI is run by the user on
// a real Wayland session, but CI's `examples-gate` (and `mise run validate-examples`)
// now also exercise the GL pipeline headlessly: the --screenshot path runs under
// Xvfb + mesa software GL (llvmpipe), so a renderer-side break can no longer hide.
//
// USAGE:  x3d_poc_renderer [--headless] [scene.x3d]
//   With no scene argument it loads the bundled assets/triangle.x3d (first-light).
//   --headless parses + extracts the scene, prints the RenderItem count, the
//     first item's vertex count, and the T12 lit self-check (headlight flag,
//     active light count incl. the §23.4.4 headlight, and item[0]'s material
//     color + whether its mesh carries shading normals), then exits 0 WITHOUT
//     ever creating a GL context or a window. This is the build-side acceptance
//     probe: it runs in CI / over SSH with no display, proving the asset parses
//     and SceneExtractor yields a lit-drawable mesh, independent of the
//     on-screen visual verify.

#include <glad/gl.h>     // MUST precede GLFW so GLFW does not pull in <GL/gl.h>.
#include <GLFW/glfw3.h>

#include "imgui.h"                   // Dear ImGui overlay (PoC-owned).
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "stb_image.h"   // B8 texture decode (PoC-owned; the SDK never decodes).

#include "AssetResolver.hpp"        // B8 asset-resolver seam (the bytes path)
#include "GeometryBounds.hpp"      // geombounds::getField (Background skyColor read)
#include "Mat4.hpp"                 // x3d::runtime::Mat4 (column-major, GL-native)
#include "MovieDecoder.hpp"         // MovieTexture frame-decode seam (ADR-0041)
#include "PlMpegMovieDecoder.hpp"   // MovieDecoder backend A (x3d_plmpeg io backend)
#include "RenderItem.hpp"          // extract descriptors
#include "SceneExtractor.hpp"      // T7a minimal extractor
#include "ShaderOptions.hpp"        // §31 shader validator/resolver seam
#include "io/file/FileResolver.hpp" // §31 ShaderPart urls (scene-relative files)
#include "ShaderBindingPlan.hpp"   // Phase 5 author-shader vocab dispatch
#include "StbttGlyphAtlas.hpp"     // T-TEXT glyph atlas (x3d_stbtt io backend)
#include "TextureResolver.hpp"     // T-TEX decoded-pixel seam (the consumer decode)
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"            // x3d::codec::parseFile
#include "X3DSceneBridge.hpp"     // BridgeResult + buildFrom + attachInteractive
#include "draw_math.hpp"          // renderer-side hot-path normal-matrix helper
#include "input.hpp"              // InputBridge + unproject + keycode map

#include <algorithm> // std::sort (B7 back-to-front transparency sort)
#include <array>
#include <cmath>
#include <cstddef> // offsetof
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator> // std::istreambuf_iterator (B8 local-file resolver)
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility> // std::pair (B7 blended-item sort key)
#include <variant>
#include <vector>

namespace {

using x3d::runtime::Mat4;
using x3d::runtime::Aabb;
namespace ex = x3d::runtime::extract;
// ADR-0039 moved the value types to x3d::core and the node types to x3d::nodes.
// This out-of-SDK consumer names SFVec3f/SFColor/SFImage/X3DNode &c. unqualified,
// so pull both namespaces in (scoped to this anonymous namespace).
using namespace x3d::core;
using namespace x3d::nodes;

// --------------------------------------------------------------------------
// Small GL helpers. PoC-local; nothing here belongs in the SDK.
// --------------------------------------------------------------------------

// PoC perspective. fieldOfView from CameraDesc is X3D's MIN-dimension FOV; we
// map it to the SHORTER window dimension and derive the other axis by aspect.
// Right-handed, Y-up, clip-z in [-1,1] (GL convention). Column-major so it
// uploads directly via .m.data().
Mat4 perspective(float minFov, float aspect, float zNear, float zFar) {
  Mat4 r{}; // all zero
  // Map the min-dimension FOV to vertical/horizontal depending on orientation.
  float fovY = minFov;
  if (aspect < 1.0f) {
    // Portrait: the shorter dimension is the WIDTH, so minFov is horizontal.
    // fovY = 2*atan(tan(fovX/2)/aspect).
    fovY = 2.0f * std::atan(std::tan(minFov * 0.5f) / aspect);
  }
  const float f = 1.0f / std::tan(fovY * 0.5f);
  r.m[0] = f / aspect;
  r.m[5] = f;
  r.m[10] = (zFar + zNear) / (zNear - zFar);
  r.m[11] = -1.0f;
  r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
  return r;
}

// Minimal vec3 helpers + a right-handed, Y-up, column-major lookAt (GL convention,
// camera looks down its -Z). Used to FRAME a scene that authors no Viewpoint —
// X3DExecutionContext::viewMatrix() returns identity in that case (eye at origin),
// so a large/offset model would sit mostly behind or far from the camera. A real
// browser "views all"; we do the same: place the eye back along a 3/4 direction at
// a distance that fits the scene's bounding sphere in the FOV, looking at its center.
SFVec3f v3sub(const SFVec3f &a, const SFVec3f &b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
SFVec3f v3add(const SFVec3f &a, const SFVec3f &b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
SFVec3f v3mul(const SFVec3f &a, float s) { return {a.x*s, a.y*s, a.z*s}; }
float v3dot(const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
SFVec3f v3cross(const SFVec3f &a, const SFVec3f &b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
SFVec3f v3norm(const SFVec3f &a) {
  float L = std::sqrt(v3dot(a, a));
  return L > 1e-9f ? v3mul(a, 1.0f/L) : SFVec3f{0,0,1};
}
Mat4 lookAt(const SFVec3f &eye, const SFVec3f &center, const SFVec3f &up) {
  SFVec3f f = v3norm(v3sub(center, eye));
  SFVec3f s = v3norm(v3cross(f, up));
  SFVec3f u = v3cross(s, f);
  Mat4 m = Mat4::identity();
  m.m[0]=s.x; m.m[4]=s.y; m.m[8]=s.z;   m.m[12]=-v3dot(s, eye);
  m.m[1]=u.x; m.m[5]=u.y; m.m[9]=u.z;   m.m[13]=-v3dot(u, eye);
  m.m[2]=-f.x; m.m[6]=-f.y; m.m[10]=-f.z; m.m[14]= v3dot(f, eye);
  return m;
}

// Orthographic projection over [-r,r]^2 and [zNear,zFar] (GL clip-z [-1,1]).
Mat4 orthographic(float r, float zNear, float zFar) {
  Mat4 m = Mat4::identity();
  m.m[0] = 1.0f / r;
  m.m[5] = 1.0f / r;
  m.m[10] = -2.0f / (zFar - zNear);
  m.m[14] = -(zFar + zNear) / (zFar - zNear);
  return m;
}

// §17.3.1 shadows (shadow.glsl): the world view-projection of each depth-map
// layer for one shadow-enabled light, framed on the scene's bounding sphere.
// A DirectionalLight gets one orthographic layer; a SpotLight one perspective
// layer over its cone (or the six cube faces when the cone is too wide for a
// single frustum); a PointLight six 92-degree faces (slight overlap, so every
// direction lands in one).
std::vector<Mat4> shadowLayers(const ex::LightDesc &L, const SFVec3f &center,
                               float radius) {
  const auto upFor = [](const SFVec3f &d) {
    return std::fabs(d.y) > 0.99f ? SFVec3f{1, 0, 0} : SFVec3f{0, 1, 0};
  };
  if (L.type == ex::LightDesc::Type::Directional) {
    const SFVec3f dir = v3norm(L.worldDirection);
    const SFVec3f eye = v3sub(center, v3mul(dir, 2.0f * radius));
    return {orthographic(radius, 0.5f * radius, 3.5f * radius) *
            lookAt(eye, center, upFor(dir))};
  }
  const SFVec3f at = L.worldLocation;
  const float zFar = std::sqrt(v3dot(v3sub(at, center), v3sub(at, center))) +
                     radius * 1.01f;
  const float zNear = (std::max)(zFar * 1e-3f, 1e-4f);
  if (L.type == ex::LightDesc::Type::Spot && L.cutOffAngle < 1.4f) {
    const SFVec3f dir = v3norm(L.worldDirection);
    return {perspective(2.0f * L.cutOffAngle + 0.02f, 1.0f, zNear, zFar) *
            lookAt(at, v3add(at, dir), upFor(dir))};
  }
  const Mat4 proj = perspective(92.0f * 3.14159265f / 180.0f, 1.0f, zNear, zFar);
  const SFVec3f axes[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                           {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  std::vector<Mat4> out;
  for (const SFVec3f &a : axes) out.push_back(proj * lookAt(at, v3add(at, a), upFor(a)));
  return out;
}

// Reads a shader source. A line `#include "name"` is replaced by the file
// `name` beside it (one level; the PoC shares multitexture.glsl this way).
std::string readTextFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "[poc] cannot open %s\n", path.c_str());
    return {};
  }
  const std::string dir = path.substr(0, path.find_last_of('/') + 1);
  std::ostringstream ss;
  for (std::string line; std::getline(in, line);) {
    const std::string directive = "#include \"";
    if (line.compare(0, directive.size(), directive) == 0 && line.back() == '"') {
      std::ifstream part(dir + line.substr(directive.size(),
                                           line.size() - directive.size() - 1),
                         std::ios::binary);
      if (!part) std::fprintf(stderr, "[poc] cannot open include in %s\n", path.c_str());
      ss << part.rdbuf() << '\n';
      continue;
    }
    ss << line << '\n';
  }
  return ss.str();
}

GLuint compileShader(GLenum stage, const std::string &src, const char *label) {
  GLuint sh = glCreateShader(stage);
  const char *c = src.c_str();
  glShaderSource(sh, 1, &c, nullptr);
  glCompileShader(sh);
  GLint ok = GL_FALSE;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
    std::fprintf(stderr, "[poc] %s compile failed:\n%s\n", label, log);
    glDeleteShader(sh);
    return 0;
  }
  return sh;
}

// §31.3.2 isValid for this host: the program compiles and links in the
// current GL context. Results are cached by source so selection (extractor and
// ShaderSystem) compiles each distinct program once. Before the context exists
// (the --headless probe) only the structural check runs.
x3d::runtime::extract::ShaderValidator makeGlShaderValidator(std::shared_ptr<bool> glReady) {
  namespace ex = x3d::runtime::extract;
  auto cache = std::make_shared<std::unordered_map<std::string, ex::ShaderValidation>>();
  return [glReady, cache](const ex::ShaderProgramDesc &p) -> ex::ShaderValidation {
    ex::ShaderValidation v = ex::structuralShaderValidation(p);
    if (!v.valid || !*glReady) return v;
    std::string key;
    for (const auto &stage : p.stages) key += stage.source + "\n---\n";
    if (auto it = cache->find(key); it != cache->end()) return it->second;
    auto compile = [&](GLenum type, const std::string &src) -> GLuint {
      GLuint sh = glCreateShader(type);
      const char *c = src.c_str();
      glShaderSource(sh, 1, &c, nullptr);
      glCompileShader(sh);
      GLint ok = GL_FALSE;
      glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
      if (ok) return sh;
      char log[2048] = {};
      glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
      v = {false, log};
      glDeleteShader(sh);
      return 0;
    };
    GLuint vs = 0, fs = 0;
    for (const auto &stage : p.stages) {
      if (stage.stage == ex::ShaderStageDesc::Stage::Vertex)
        vs = compile(GL_VERTEX_SHADER, stage.source);
      else if (stage.stage == ex::ShaderStageDesc::Stage::Fragment)
        fs = compile(GL_FRAGMENT_SHADER, stage.source);
    }
    if (vs && fs) {
      GLuint prog = glCreateProgram();
      glAttachShader(prog, vs);
      glAttachShader(prog, fs);
      glLinkProgram(prog);
      GLint ok = GL_FALSE;
      glGetProgramiv(prog, GL_LINK_STATUS, &ok);
      if (ok) {
        v = {true, {}};
      } else {
        char log[2048] = {};
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        v = {false, log};
      }
      glDeleteProgram(prog);
    } else if (v.valid) {
      v = {false, "vertex/fragment compile failed"};
    }
    if (vs) glDeleteShader(vs);
    if (fs) glDeleteShader(fs);
    if (!v.valid) std::fprintf(stderr, "[poc] author shader invalid: %s\n", v.error.c_str());
    (*cache)[key] = v;
    return v;
  };
}

GLuint linkProgram(GLuint vs, GLuint fs) {
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = GL_FALSE;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetProgramInfoLog(p, sizeof(log), nullptr, log);
    std::fprintf(stderr, "[poc] program link failed:\n%s\n", log);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

// GL_KHR_debug callback. Wired in M0 so a wrong winding / matrix / state bug
// surfaces as a logged message instead of a silent black screen.
void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id,
                              GLenum severity, GLsizei /*length*/,
                              const GLchar *message, const void * /*user*/) {
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return; // quiet the spam
  std::fprintf(stderr, "[GL] src=0x%x type=0x%x id=%u sev=0x%x: %s\n", source,
               type, id, severity, message);
}

void glfwErrorCallback(int code, const char *desc) {
  std::fprintf(stderr, "[GLFW] error %d: %s\n", code, desc);
}

// One GPU mesh per GeomId (upload-once / instance-N). M3 (T12) interleaves
// position + normal + color per vertex so the lit shader has a shading normal
// and an optional per-vertex Color override. Texcoords land at M4.
//
// The extractor guarantees parallel arrays: normals is either empty or the same
// length as positions (flat per-face normals, EXPANDED so every vertex has one),
// and colors likewise. When a stream is absent we synthesize a sane default
// (up-normal / opaque-white) so the attribute layout is uniform — the shader
// gates on uHasColors / a degenerate-normal guard rather than on buffer shape.
struct GpuVertex {
  SFVec3f pos;
  SFVec3f normal;
  SFColorRGBA color;
  SFVec2f texcoord; // B8: X3D LOCAL origin = bottom-left = GL; NO v-flip here.
  // §18.4.3 MultiTexture: UV sets 1..3 (aTexCoord1..3); a missing set repeats
  // set 0, so a stage whose channel exceeds the authored sets still samples.
  SFVec2f texcoordSet[3];
};

struct GpuMesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  GLsizei indexCount = 0;
  ex::Topology topology = ex::Topology::Triangles; // B4: draw-call primitive.
  bool ccw = true;       // winding: front face is CCW when true.
  bool solid = true;     // solid => back-face cullable; false => double-sided.
  bool hasNormals = false; // false => route to the unlit program (B4 contract).
  bool hasColors = false; // per-vertex Color present => overrides Material diffuse.
  bool hasTexcoords = false; // B8: authored TextureCoordinate present => sampleable.
  bool isGlyphMesh = false;  // T-TEXT: texcoords index the font atlas, not a material slot.
  // B7: the mesh's LOCAL-frame centroid (mean of positions). Transformed by the
  // per-path worldTransform at draw time to get a world-space sort key for the
  // back-to-front transparency pass. Cheap to keep here (computed once on upload).
  SFVec3f localCentroid{0.0f, 0.0f, 0.0f};
};

GpuMesh uploadMesh(const ex::MeshData &m);

// A skin is placement-specific: its source-index remap need not match another
// placement of the same geometry. The influence TBO is immutable; only the
// palette TBO is replaced on pose deltas.
struct GpuSkin {
  GpuMesh mesh;
  GLuint ranges = 0, influences = 0, palette = 0;
  GLuint influenceBuffer = 0, paletteBuffer = 0;
  bool cpuFallback = false;
};

void destroySkin(GpuSkin &s) {
  glDeleteTextures(1, &s.influences);
  glDeleteTextures(1, &s.palette);
  glDeleteBuffers(1, &s.ranges);
  glDeleteBuffers(1, &s.influenceBuffer);
  glDeleteBuffers(1, &s.paletteBuffer);
  glDeleteVertexArrays(1, &s.mesh.vao);
  glDeleteBuffers(1, &s.mesh.vbo);
  glDeleteBuffers(1, &s.mesh.ebo);
}

GpuSkin uploadSkin(const ex::RenderItem &item, const ex::MeshData &mesh,
                   bool cpuFallback) {
  GpuSkin s;
  s.mesh = uploadMesh(mesh);
  s.cpuFallback = cpuFallback;
  if (cpuFallback) return s;
  const auto &b = *item.skin->binding;
  // Per corner: (offset, count) into the influence buffer for the corner's
  // source COORDINATE. Its normal is skinned with the same influences.
  std::vector<std::uint32_t> ranges(item.skin->sourceCoordIndex.size() * 2);
  for (std::size_t i = 0; i < item.skin->sourceCoordIndex.size(); ++i) {
    auto source = item.skin->sourceCoordIndex[i];
    if (source < b.bindPositions.size() && source + 1 < b.influenceOffset.size()) {
      ranges[i * 2] = b.influenceOffset[source];
      ranges[i * 2 + 1] = b.influenceOffset[source + 1] - ranges[i * 2];
    }
  }
  glGenBuffers(1, &s.ranges);
  glBindVertexArray(s.mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, s.ranges);
  glBufferData(GL_ARRAY_BUFFER, ranges.size() * sizeof(std::uint32_t), ranges.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(4);
  glVertexAttribIPointer(4, 2, GL_UNSIGNED_INT, 2 * sizeof(std::uint32_t), nullptr);
  glBindVertexArray(0);
  std::vector<float> pairs;
  pairs.reserve(b.influences.size() * 2);
  for (const auto &in : b.influences) {
    pairs.push_back(static_cast<float>(in.joint));
    pairs.push_back(in.weight);
  }
  glGenBuffers(1, &s.influenceBuffer);
  glBindBuffer(GL_TEXTURE_BUFFER, s.influenceBuffer);
  glBufferData(GL_TEXTURE_BUFFER, pairs.size() * sizeof(float), pairs.data(), GL_STATIC_DRAW);
  glGenTextures(1, &s.influences);
  glBindTexture(GL_TEXTURE_BUFFER, s.influences);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, s.influenceBuffer);
  glGenBuffers(1, &s.paletteBuffer);
  glGenTextures(1, &s.palette);
  return s;
}

void updateSkinPose(GpuSkin &s, const ex::RenderItem &item,
                    const x3d::runtime::geo::GeoProjection &projection) {
  if (s.cpuFallback) return;
  auto pose = x3d::runtime::hanim::evaluatePose(*item.skin->binding, projection);
  // Seven RGBA texels per joint: four matrix columns, then three columns of
  // inverse-transpose 3x3. The reference skinner uses the same inverse.
  std::vector<float> data;
  data.reserve(pose.palette.size() * 28);
  for (const auto &m : pose.palette) {
    data.insert(data.end(), m.m.begin(), m.m.end());
    auto normal = poc::normalMatrix3(x3d::runtime::Mat4::identity(), m);
    for (int c = 0; c < 3; ++c)
      for (int r = 0; r < 4; ++r)
        data.push_back(r < 3 ? normal[c * 3 + r] : 0.0f);
  }
  glBindBuffer(GL_TEXTURE_BUFFER, s.paletteBuffer);
  glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
  glBindTexture(GL_TEXTURE_BUFFER, s.palette);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, s.paletteBuffer);
}

GpuMesh uploadMesh(const ex::MeshData &m) {
  GpuMesh g;
  g.topology = m.topology;
  g.ccw = m.ccw;
  g.solid = m.solid;
  g.hasNormals = m.hasNormals;
  g.hasColors = m.hasColors;
  g.hasTexcoords = !m.texcoords.empty();
  g.isGlyphMesh = m.isGlyphMesh;

  // LOCAL-frame centroid for the B7 transparency back-to-front sort.
  if (!m.positions.empty()) {
    SFVec3f sum{0.0f, 0.0f, 0.0f};
    for (const SFVec3f &p : m.positions) { sum.x += p.x; sum.y += p.y; sum.z += p.z; }
    const float inv = 1.0f / static_cast<float>(m.positions.size());
    g.localCentroid = SFVec3f{sum.x * inv, sum.y * inv, sum.z * inv};
  }

  // Interleave into one packed vertex stream. Fill missing normal/color streams
  // with defaults so the attribute layout is uniform across every mesh.
  const std::size_t n = m.positions.size();
  std::vector<GpuVertex> verts(n);
  for (std::size_t i = 0; i < n; ++i) {
    verts[i].pos = m.positions[i];
    verts[i].normal = (i < m.normals.size()) ? m.normals[i]
                                             : SFVec3f{0.0f, 1.0f, 0.0f};
    verts[i].color = (m.hasColors && i < m.colors.size())
                         ? m.colors[i]
                         : SFColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};
    // B8: texcoords are parallel to positions when authored (X3D bottom-left =
    // GL convention; NO v-flip). Default (0,0) when the mesh has none.
    verts[i].texcoord =
        (i < m.texcoords.size()) ? m.texcoords[i] : SFVec2f{0.0f, 0.0f};
    for (std::size_t s = 0; s < 3; ++s) {
      const auto *set = s + 1 < m.texcoordSets.size() ? &m.texcoordSets[s + 1]
                                                       : nullptr;
      verts[i].texcoordSet[s] =
          set && i < set->size() ? (*set)[i] : verts[i].texcoord;
    }
  }

  glGenVertexArrays(1, &g.vao);
  glGenBuffers(1, &g.vbo);
  glGenBuffers(1, &g.ebo);
  glBindVertexArray(g.vao);

  glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(verts.size() * sizeof(GpuVertex)),
               verts.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0); // aPos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GpuVertex),
                        reinterpret_cast<void *>(offsetof(GpuVertex, pos)));
  glEnableVertexAttribArray(1); // aNormal
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GpuVertex),
                        reinterpret_cast<void *>(offsetof(GpuVertex, normal)));
  glEnableVertexAttribArray(2); // aColor (rgba)
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GpuVertex),
                        reinterpret_cast<void *>(offsetof(GpuVertex, color)));
  glEnableVertexAttribArray(3); // aTexCoord (B8)
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GpuVertex),
                        reinterpret_cast<void *>(offsetof(GpuVertex, texcoord)));
  for (GLuint s = 0; s < 3; ++s) { // aTexCoord1..3 (§18.4.3 UV sets)
    glEnableVertexAttribArray(5 + s);
    glVertexAttribPointer(5 + s, 2, GL_FLOAT, GL_FALSE, sizeof(GpuVertex),
                          reinterpret_cast<void *>(offsetof(GpuVertex, texcoordSet) +
                                                   s * sizeof(SFVec2f)));
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m.indices.size() * sizeof(std::uint32_t)),
               m.indices.data(), GL_STATIC_DRAW);
  g.indexCount = static_cast<GLsizei>(m.indices.size());

  glBindVertexArray(0);
  return g;
}

// Transform a world-space direction into eye space by the view matrix (w=0).
SFVec3f toEyeDir(const Mat4 &view, const SFVec3f &worldDir) {
  return view.transformDirection(worldDir);
}

// An extracted light in eye space, with RGB color premultiplied by intensity.
struct EyeLight {
  SFVec3f dirEye{0.0f, 0.0f, -1.0f};
  SFColor color{1.0f, 1.0f, 1.0f}; // already * intensity.
  float ambientIntensity = 0.0f;   // §17.2.2.4 per-light ambientIntensity.
  bool positional = false;
  SFVec3f posEye{0.0f, 0.0f, 0.0f};
  SFVec3f attenuation{1.0f, 0.0f, 0.0f};
  float radius = 100.0f;
  bool spot = false;
  float beamWidth = 1.5708f;
  float cutOffAngle = 0.7854f;
};

inline constexpr int kMaxLights = 8;

// #39: max sky/ground color bands the background gradient shader (bg.frag)
// accepts, mirrored there as `kMaxBands`. NIST conformance content never
// comes close; extra bands beyond this are silently dropped (matches the
// existing kMaxLights convention above).
inline constexpr int kMaxBgBands = 8;

// Build the active eye-space light set for this frame from the
// extractor's world-resolved LightDesc list and the bound NavigationInfo
// headlight flag.
//
// HEADLIGHT (§23.4.4): headlight TRUE (default) means the browser turns the
// headlight ON regardless of the scene's own lights; FALSE turns it off. So the
// camera-space headlight is added whenever headlightOn, on top of any authored
// directional lights (it is not a no-lights fallback). global=false
// DirectionalLights are NOT promoted to scene-wide here; for this PoC we still
// draw them (the dominant corpus case is a single global headlight-style light),
// but a scoped light's scopeRoot is carried on LightDesc for a future
// per-subtree pass — we do not blanket-illuminate from it beyond the documented
// PoC simplification.
std::vector<EyeLight> buildEyeLights(const std::vector<ex::LightDesc> &lights,
                                     const Mat4 &view, bool headlightOn) {
  std::vector<EyeLight> out;
  // Reserve a slot for the headlight so it is never dropped by the cap.
  const std::size_t cap = headlightOn ? static_cast<std::size_t>(kMaxLights - 1)
                                      : static_cast<std::size_t>(kMaxLights);
  for (const ex::LightDesc &L : lights) {
    if (out.size() >= cap) break;
    EyeLight e;
    e.dirEye = toEyeDir(view, L.worldDirection);
    if (L.type != ex::LightDesc::Type::Directional) {
      e.positional = true;
      e.posEye = view.transformPoint(L.worldLocation);
      e.attenuation = L.attenuation;
      e.radius = L.radius;
      e.spot = L.type == ex::LightDesc::Type::Spot;
      e.beamWidth = L.beamWidth;
      e.cutOffAngle = L.cutOffAngle;
    }
    e.color = SFColor{L.color.r * L.intensity, L.color.g * L.intensity,
                      L.color.b * L.intensity};
    e.ambientIntensity = L.ambientIntensity;
    out.push_back(e);
  }

  // §23.4.4: headlight TRUE => a headlight regardless of scene lights.
  if (headlightOn) {
    EyeLight head;
    head.dirEye = SFVec3f{0.0f, 0.0f, -1.0f}; // straight down the camera's -Z.
    head.color = SFColor{1.0f, 1.0f, 1.0f};
    // §23.4.4 pins the headlight exactly: intensity 1, color (1 1 1),
    // ambientIntensity 0.0, direction (0 0 -1).
    head.ambientIntensity = 0.0f;
    out.push_back(head);
  }
  return out;
}

// ==========================================================================
// B8 TEXTURE PATH (PoC-owned consumer layer; the SDK never does I/O / decode).
//
// The SDK surfaces a TextureRef per material slot: Source::Url carries the URL
// list VERBATIM, Source::Inline carries PixelTexture bytes. The PoC:
//   * Source::Url   -> the asset RESOLVER (a local-file resolver relative to the
//       scene dir, OWNED here, NOT the SDK) yields encoded bytes -> stb_image
//       decode (NO vertical flip — X3D texcoords are already GL bottom-left).
//   * Source::Inline-> decode the SFImage pixels directly (no resolver call).
// then upload a GL texture cached by URL, wrap from repeatS/T + mipmap. When the
// resolver returns Pending/Failed (or no texture slot exists) the draw falls
// back to the flat material color.
//
// Contract (A) (render-time, MAY be Pending) is the one wired here. Contract (B)
// (parse-time Inline/EXTERNPROTO node bytes, SYNC) is out of this PoC's scope —
// the SDK already had nodes materialized before extraction.
// ==========================================================================

// Directory portion of the scene path (so a relative texture URL resolves next
// to the scene file, like a browser resolves relative to the document).
std::string dirOf(const std::string &path) {
  auto slash = path.find_last_of("/\\");
  return (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);
}

// A local-file asset resolver bound to the scene directory. This is the
// CONSUMER's resolver (Decision B8: owned outside the SDK). It honors contract
// (A): a missing file => Failed (the draw falls back to flat color); a real
// async backend would return Pending here without blocking the frame.
ex::AssetResolver makeLocalFileResolver(const std::string &sceneDir) {
  return [sceneDir](const std::string &url, ex::AssetKind /*kind*/) -> ex::AssetResult {
    // data: URIs and absolute http(s) are out of PoC scope -> Failed (flat color).
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("data:", 0) == 0)
      return ex::AssetResult::makeFailed();
    // Resolve relative to the scene dir unless the URL is already absolute.
    const std::string path =
        (!url.empty() && (url[0] == '/' )) ? url : (sceneDir + "/" + url);
    std::ifstream in(path, std::ios::binary);
    if (!in) return ex::AssetResult::makeFailed();
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    if (bytes.empty()) return ex::AssetResult::makeFailed();
    return ex::AssetResult::makeReady(std::move(bytes));
  };
}

// A TextureResolver (T-TEX seam) bound to the scene directory. This is the
// CONSUMER's decode path (the SDK never decodes image bytes): it reads the file
// next to the scene, decodes it with stb_image to RGBA8, and hands the SDK the
// decoded surface. The SDK threads it onto TextureRef.resolvedPixels so the draw
// loop binds the GPU texture WITHOUT re-running the resolver itself.
//
// Pixel format contract (TextureResolver.hpp): RGBA8, tightly packed, origin
// bottom-left = GL convention. stb_image decodes rows TOP-first, but GL treats
// the first row as the texture's BOTTOM, so we flip on load to land bottom-up
// (without this, file textures render vertically mirrored: see ConformanceNist
// Geometry/Box/texture.x3d, where the VTS decal appeared upside-down).
// http(s)/data: URIs and missing files => Failed (the PoC falls back to white);
// a real async backend would return Pending here without blocking the frame.
ex::TextureResolver makeLocalTextureResolver(const std::string &sceneDir) {
  return [sceneDir](const std::string &url) -> ex::TexturePixelResult {
    if (url.empty() || url.rfind("http://", 0) == 0 ||
        url.rfind("https://", 0) == 0 || url.rfind("data:", 0) == 0)
      return ex::TexturePixelResult::makeFailed();
    const std::string path =
        (url[0] == '/') ? url : (sceneDir + "/" + url);
    std::ifstream in(path, std::ios::binary);
    if (!in) return ex::TexturePixelResult::makeFailed();
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    if (bytes.empty()) return ex::TexturePixelResult::makeFailed();

    int w = 0, h = 0, comp = 0;
    stbi_set_flip_vertically_on_load(1); // top-first decode -> GL bottom-up rows.
    unsigned char *px = stbi_load_from_memory(
        bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp, 4);
    if (!px || w <= 0 || h <= 0) {
      if (px) stbi_image_free(px);
      return ex::TexturePixelResult::makeFailed();
    }
    ex::TexturePixels out;
    out.width = static_cast<std::uint32_t>(w);
    out.height = static_cast<std::uint32_t>(h);
    out.rgba.assign(px, px + static_cast<std::size_t>(w) * h * 4);
    stbi_image_free(px);
    return ex::TexturePixelResult::makeReady(std::move(out));
  };
}

// Map the X3D font families (SERIF / SANS / TYPEWRITER, §15.4.1) to TTF paths
// for the glyph atlas. Resolution order per family: env override
// (X3D_POC_FONT_SERIF/SANS/TYPEWRITER) -> bundled assets/fonts/ -> a few common
// system locations. Only families whose file exists are added; an empty map
// leaves the SDK's monospace stub in place (Text falls back to blank cells).
std::map<std::string, std::string> resolveFontFaces() {
  // Bundled Liberation faces (third_party/fonts/, shared with the SDK text
  // tests; SIL OFL). X3D_POC_FONT_DIR overrides the directory; each family also
  // takes a per-family env override (X3D_POC_FONT_SERIF/SANS/TYPEWRITER).
  const char *dirEnv = std::getenv("X3D_POC_FONT_DIR");
  const std::string dir = std::string(dirEnv && *dirEnv ? dirEnv : X3D_POC_FONT_DIR) + "/";
  struct Fam { const char *key; const char *env; const char *file; };
  const Fam fams[] = {
    {"SERIF", "X3D_POC_FONT_SERIF", "LiberationSerif-Regular.ttf"},
    {"SANS", "X3D_POC_FONT_SANS", "LiberationSans-Regular.ttf"},
    {"TYPEWRITER", "X3D_POC_FONT_TYPEWRITER", "LiberationMono-Regular.ttf"},
  };
  const auto exists = [](const std::string &p) {
    std::ifstream f(p, std::ios::binary);
    return static_cast<bool>(f);
  };
  std::map<std::string, std::string> out;
  for (const Fam &fam : fams) {
    if (const char *e = std::getenv(fam.env); e && exists(e)) { out[fam.key] = e; continue; }
    if (const std::string a = dir + fam.file; exists(a)) out[fam.key] = a;
  }
  return out;
}

// Upload one decoded RGBA8 image to a GL texture; wrap + mipmap from sampler.
// Phase 5.5: srgb=true → GL_SRGB8_ALPHA8 internal format so the driver
// linearises the texels on sample (correct for BaseColor/Diffuse/Emissive).
// srgb=false → GL_RGBA (data textures: Normal, MetallicRoughness, Occlusion,
// Specular — stored in linear space already; must NOT be decoded again).
GLuint uploadGlTexture(const unsigned char *rgba, int w, int h, bool repeatS,
                       bool repeatT, bool mipmap, bool srgb = false) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLint internalFmt = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  repeatS ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                  repeatT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  if (mipmap) {
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

// Decode an inline X3D SFImage (1..4 components, row-major, BOTTOM-UP per the
// X3D convention so it matches GL with NO flip) into a GL texture. Returns 0 on
// a malformed image.
std::vector<unsigned char> sfImageRGBA(const SFImage &img) {
  const int w = img.width, h = img.height, nc = img.numComponents;
  if (w <= 0 || h <= 0 || nc < 1 || nc > 4) return {};
  if (img.data.size() < static_cast<std::size_t>(w) * h * nc) return {};
  std::vector<unsigned char> rgba(static_cast<std::size_t>(w) * h * 4);
  for (int i = 0; i < w * h; ++i) {
    const unsigned char *p = img.data.data() + static_cast<std::size_t>(i) * nc;
    unsigned char r = 0, g = 0, b = 0, a = 255;
    switch (nc) {
      case 1: r = g = b = p[0]; break;                 // intensity
      case 2: r = g = b = p[0]; a = p[1]; break;        // intensity + alpha
      case 3: r = p[0]; g = p[1]; b = p[2]; break;      // RGB
      case 4: r = p[0]; g = p[1]; b = p[2]; a = p[3]; break; // RGBA
    }
    rgba[i * 4 + 0] = r; rgba[i * 4 + 1] = g;
    rgba[i * 4 + 2] = b; rgba[i * 4 + 3] = a;
  }
  return rgba;
}

GLuint uploadInlineSFImage(const SFImage &img, bool repeatS, bool repeatT,
                           bool mipmap) {
  const std::vector<unsigned char> rgba = sfImageRGBA(img);
  if (rgba.empty()) return 0;
  return uploadGlTexture(rgba.data(), img.width, img.height, repeatS, repeatT, mipmap);
}

// A GL texture cache. Source::Url entries key on the URL string (upload-once
// across every material that references it); Source::Inline entries key on the
// PixelTexture node pointer (no URL). A value of 0 means "tried and failed"
// (cached so we do not retry a missing file every frame); a missing key on the
// Url path means "not yet attempted" (a Pending resolver leaves it absent so a
// later frame retries — contract A).
// One upload per image *and* format: Phong samples colour textures in display
// space (GL_RGBA) while PhysicalMaterial samples them sRGB-decoded
// (GL_SRGB8_ALPHA8), so the same url can need both (ADR-0027).
struct TextureCacheFormat {
  std::unordered_map<std::string, GLuint> byUrl;          // 0 = failed (no retry).
  std::unordered_map<const void *, GLuint> byInlineNode;  // keyed by SFImage addr.
  std::unordered_map<std::string, GLuint> byCube;         // keyed by face keys.
};
struct TextureCache {
  TextureCacheFormat linear, srgb;
  TextureCacheFormat &format(bool isSrgb) { return isSrgb ? srgb : linear; }
};

// Find the first TextureRef matching any of the given slots; returns nullptr if
// none. Used by the per-program bind helpers to locate a specific map.
const ex::TextureRef *findTexSlot(const ex::MaterialDesc &mat,
                                  std::initializer_list<ex::TextureRef::Slot> slots) {
  for (const ex::TextureRef &t : mat.textures)
    for (ex::TextureRef::Slot s : slots)
      if (t.slot == s) return &t;
  return nullptr;
}

// TXF-2: the material's §18.4.8 TextureCoordinateGenerator (the first texture
// that carries one), or null. Every slot samples its coordinates.
const ex::TextureRef *texCoordGenRef(const ex::MaterialDesc &mat) {
  for (const ex::TextureRef &t : mat.textures)
    if (t.hasTexCoordGen) return &t;
  return nullptr;
}

// The shaders' uTexCoordGenMode: TexCoordGenMode + 1 (Table 18.6 order, see
// texgen.glsl), 0 when no generator applies.
int texCoordGenModeUniform(const ex::MaterialDesc &mat) {
  const ex::TextureRef *t = texCoordGenRef(mat);
  return t ? static_cast<int>(t->texCoordGen.mode) + 1 : 0;
}

// §18.4.3 MultiTexture: the base-colour stages in authored order. The SDK
// expands a MultiTexture into one TextureRef per stage (channel = stage UV
// set); a plain texture is one stage. Same rule as cpu_raster buildTextures.
std::vector<const ex::TextureRef *> baseStageRefs(const ex::MaterialDesc &mat) {
  std::vector<const ex::TextureRef *> out;
  for (const ex::TextureRef &t : mat.textures)
    if (t.slot == ex::TextureRef::Slot::BaseColor ||
        t.slot == ex::TextureRef::Slot::Diffuse ||
        (mat.model == ex::MaterialModel::Unlit && t.slot == ex::TextureRef::Slot::Emissive))
      out.push_back(&t);
  return out;
}

// True when the stages need multitexture.glsl's combiner. A single default
// MODULATE stage on UV set 0 is the shaders' plain texture multiply.
bool needsStageCombiner(const std::vector<const ex::TextureRef *> &stages) {
  if (stages.size() > 1) return true;
  if (stages.empty()) return false;
  const ex::TextureRef &t = *stages.front();
  return t.multiMode != "MODULATE" || !t.multiSource.empty() ||
         !t.multiFunction.empty() || t.channel != 0 ||
         t.source == ex::TextureRef::Source::Cube; // §34: cube stages sample there.
}

// multitexture.glsl mode code for one §18.4.3 mode token (Table 18.3).
int multiTextureModeCode(std::string token) {
  const auto b = token.find_first_not_of(' ');
  token = b == std::string::npos ? "" : token.substr(b, token.find_last_not_of(' ') - b + 1);
  static const char *const names[] = {
      "MODULATE", "REPLACE", "SELECTARG2", "MODULATE2X", "MODULATE4X", "ADD",
      "ADDSIGNED", "ADDSIGNED2X", "SUBTRACT", "ADDSMOOTH", "BLENDDIFFUSEALPHA",
      "BLENDTEXTUREALPHA", "BLENDFACTORALPHA", "BLENDCURRENTALPHA",
      "MODULATEALPHA_ADDCOLOR", "MODULATEINVALPHA_ADDCOLOR",
      "MODULATEINVCOLOR_ADDALPHA", "DOTPRODUCT3", "OFF"};
  if (token == "SELECTARG1") return 1;
  for (int i = 0; i < static_cast<int>(std::size(names)); ++i)
    if (token == names[i]) return i;
  return 0; // MODULATE, the default (and any unknown token).
}

// MovieTexture decode state (ADR-0041). The MovieDecoder owns per-URL codec
// contexts; here we own ONE persistent GL texture per movie URL that we RE-UPLOAD
// each frame — unlike a still image, a movie frame changes every tick, so the
// upload-once Url cache does not apply.
struct MovieState {
  ex::MovieDecoder decoder;
  double mediaTime = 0.0;                            // seconds into the clip, this frame.
  std::unordered_map<std::string, GLuint> texByUrl;  // 0 = not yet created.
};

// Create-or-update a GL texture from a decoded movie frame (bottom-left RGBA8 per
// the seam contract, so no flip). Frame dimensions are constant per clip: allocate
// on the first frame, then glTexSubImage2D-update on every subsequent frame.
GLuint uploadMovieFrame(GLuint tex, const ex::VideoFrame &f, bool repeatS,
                        bool repeatT, bool srgb) {
  const GLint internalFmt = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (tex == 0) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, static_cast<GLsizei>(f.width),
                 static_cast<GLsizei>(f.height), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 f.rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    repeatS ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    repeatT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(f.width),
                    static_cast<GLsizei>(f.height), GL_RGBA, GL_UNSIGNED_BYTE,
                    f.rgba.data());
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

// Resolve+upload (or fetch cached) a SINGLE TextureRef. Returns a GL texture id,
// or 0 when there is no usable texture this frame (Pending / Failed / no slot).
// This is the shared resolution core; per-program bind helpers call it with a
// specific TextureRef they already located (via findTexSlot).
// Phase 5.5: srgb=true → GL_SRGB8_ALPHA8 (color textures: BaseColor/Diffuse/
// Emissive); srgb=false → GL_RGBA (data textures: Normal/MetallicRoughness/
// Occlusion/Specular — linear space, no driver decode).
// `movie` (when non-null) routes Source::Movie refs through the MovieDecoder seam.
GLuint resolveTexRef(const ex::TextureRef *pick, TextureCache &caches,
                     const ex::AssetResolver &resolver, bool srgb = false,
                     MovieState *movie = nullptr) {
  if (!pick) return 0;
  TextureCacheFormat &cache = caches.format(srgb);

  // §33 3D texture (T3D-1): the descriptor carries voxel bytes / slice refs, but
  // this renderer has no sampler3D path — return 0 so the caller uses the flat
  // material color (same as the white fallback). 3D sampling is NOT wired here.
  if (pick->source == ex::TextureRef::Source::Tex3D) return 0;

  // MovieTexture (ADR-0041): decode this frame's image via the MovieDecoder seam
  // and (re)upload it. Pending holds the previously uploaded frame; Failed falls
  // through the url fallback list (and ultimately to the white fallback).
  if (pick->source == ex::TextureRef::Source::Movie) {
    if (!movie || !movie->decoder) return 0;
    for (const std::string &url : pick->url) {
      if (url.empty()) continue;
      ex::FrameResult fr = movie->decoder(url, movie->mediaTime);
      if (fr.pending()) {
        auto it = movie->texByUrl.find(url);
        return it != movie->texByUrl.end() ? it->second : 0;
      }
      if (fr.failed()) continue;
      GLuint &tex = movie->texByUrl[url];
      tex = uploadMovieFrame(tex, fr.frame, pick->repeatS, pick->repeatT, srgb);
      return tex;
    }
    return 0;
  }

  if (pick->source == ex::TextureRef::Source::Inline) {
    const void *key = &pick->inlinePixels;
    auto it = cache.byInlineNode.find(key);
    if (it != cache.byInlineNode.end()) return it->second;
    GLuint tex = uploadInlineSFImage(pick->inlinePixels, pick->sampler.repeatS,
                                     pick->sampler.repeatT,
                                     pick->sampler.generateMipmaps);
    cache.byInlineNode[key] = tex; // cache even 0 (a bad inline image won't fix).
    return tex;
  }

  if (pick->source != ex::TextureRef::Source::Url || pick->url.empty()) return 0;

  // v1 T-TEX: PREFER the SDK-threaded decoded pixels (TextureRef.resolvedPixels)
  // from the embedder's TextureResolver seam. The SDK already decoded the bytes
  // for us; the consumer just uploads them. Key the GL upload by the first url so
  // it is shared/upload-once like the legacy path. A Pending resolve leaves the
  // cache key absent (retry next frame, contract A). Only when the seam yielded
  // nothing (Failed/no resolver wired) do we fall through to the legacy
  // AssetResolver bytes path below (PoC-side stb_image decode).
  if (pick->resolvedPixels.ready() && !pick->resolvedPixels.pixels->rgba.empty()) {
    const std::string &key = pick->url.front();
    auto it = cache.byUrl.find(key);
    if (it != cache.byUrl.end()) return it->second;
    const ex::TexturePixels &p = *pick->resolvedPixels.pixels;
    GLuint tex = uploadGlTexture(p.rgba.data(), static_cast<int>(p.width),
                                 static_cast<int>(p.height), pick->repeatS,
                                 pick->repeatT, pick->sampler.generateMipmaps, srgb);
    cache.byUrl[key] = tex;
    return tex;
  }
  if (pick->resolvedPixels.pending())
    return 0; // seam not ready this frame; leave UNCACHED -> retry (contract A).

  // Source::Url: try each url in the list as an ordered fallback.
  for (const std::string &url : pick->url) {
    if (url.empty()) continue;
    auto it = cache.byUrl.find(url);
    if (it != cache.byUrl.end()) return it->second; // hit (incl. cached-failed 0).

    if (!resolver) { cache.byUrl[url] = 0; continue; }
    ex::AssetResult res = resolver(url, ex::AssetKind::Texture);
    if (res.pending()) return 0; // contract A: leave UNCACHED -> retry next frame.
    if (res.failed() || res.bytes.empty()) { cache.byUrl[url] = 0; continue; }

    int w = 0, h = 0, comp = 0;
    // No vertical flip: X3D texcoords are bottom-left = GL; a reflexive flip
    // would double-flip and break every texture (B8 invariant).
    stbi_set_flip_vertically_on_load(0);
    unsigned char *px = stbi_load_from_memory(res.bytes.data(),
                                              static_cast<int>(res.bytes.size()),
                                              &w, &h, &comp, 4);
    if (!px) { cache.byUrl[url] = 0; continue; }
    GLuint tex = uploadGlTexture(px, w, h, pick->repeatS, pick->repeatT,
                                 pick->sampler.generateMipmaps, srgb);
    stbi_image_free(px);
    cache.byUrl[url] = tex;
    return tex;
  }
  return 0;
}

// Decoded RGBA8 for one 2D face of a cube map: inline pixels, the SDK's
// resolved pixels, or the asset resolver plus stb_image. Sets `pending` when
// the resolver has not delivered yet.
bool decodeFacePixels(const ex::TextureRef &ref, const ex::AssetResolver &resolver,
                      int &w, int &h, std::vector<unsigned char> &rgba, bool &pending) {
  if (ref.source == ex::TextureRef::Source::Inline) {
    rgba = sfImageRGBA(ref.inlinePixels);
    w = ref.inlinePixels.width;
    h = ref.inlinePixels.height;
    return !rgba.empty();
  }
  if (ref.source != ex::TextureRef::Source::Url) return false;
  if (ref.resolvedPixels.ready() && !ref.resolvedPixels.pixels->rgba.empty()) {
    const ex::TexturePixels &p = *ref.resolvedPixels.pixels;
    w = static_cast<int>(p.width);
    h = static_cast<int>(p.height);
    rgba.assign(p.rgba.begin(), p.rgba.end());
    return true;
  }
  if (ref.resolvedPixels.pending()) { pending = true; return false; }
  for (const std::string &url : ref.url) {
    if (url.empty() || !resolver) continue;
    ex::AssetResult res = resolver(url, ex::AssetKind::Texture);
    if (res.pending()) { pending = true; return false; }
    if (res.failed() || res.bytes.empty()) continue;
    int comp = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *px = stbi_load_from_memory(res.bytes.data(),
                                              static_cast<int>(res.bytes.size()),
                                              &w, &h, &comp, 4);
    if (!px) continue;
    rgba.assign(px, px + static_cast<std::size_t>(w) * h * 4);
    stbi_image_free(px);
    return true;
  }
  return false;
}

// §34.4.1 ComposedCubeMapTexture as a six-layer GL_TEXTURE_2D_ARRAY in
// TextureRef::cubeFaces order (front, back, left, right, top, bottom), which
// multitexture.glsl sampleCubeFaces indexes by direction. §34.2.2 requires
// equal square faces; a face of another size is resampled (nearest) to the
// largest, and a missing or undecodable face is white. Faces are clamped
// (§34.4.1 ignores repeatS/T). Returns 0 while a face is pending.
GLuint resolveCubeTex(const ex::TextureRef &cube, TextureCache &caches,
                      const ex::AssetResolver &resolver, bool srgb) {
  if (cube.source != ex::TextureRef::Source::Cube) return 0;
  TextureCacheFormat &cache = caches.format(srgb);
  std::string key;
  for (const ex::TextureRef &face : cube.cubeFaces) {
    if (face.source == ex::TextureRef::Source::Inline)
      key += std::to_string(reinterpret_cast<std::uintptr_t>(&face.inlinePixels));
    else if (!face.url.empty())
      key += face.url.front();
    key += '|';
  }
  if (auto it = cache.byCube.find(key); it != cache.byCube.end()) return it->second;
  struct Face { int w = 0, h = 0; std::vector<unsigned char> rgba; };
  Face faces[6];
  int size = 1;
  for (std::size_t i = 0; i < 6 && i < cube.cubeFaces.size(); ++i) {
    bool pending = false;
    if (!decodeFacePixels(cube.cubeFaces[i], resolver, faces[i].w, faces[i].h,
                          faces[i].rgba, pending)) {
      if (pending) return 0; // retry next frame, uncached.
      faces[i] = {};
    }
    size = std::max({size, faces[i].w, faces[i].h});
  }
  std::vector<unsigned char> layers(static_cast<std::size_t>(size) * size * 4 * 6, 255);
  for (int f = 0; f < 6; ++f) {
    if (faces[f].rgba.empty()) continue;
    for (int y = 0; y < size; ++y)
      for (int x = 0; x < size; ++x) {
        const int sx = x * faces[f].w / size, sy = y * faces[f].h / size;
        std::copy_n(&faces[f].rgba[(static_cast<std::size_t>(sy) * faces[f].w + sx) * 4], 4,
                    &layers[((static_cast<std::size_t>(f) * size + y) * size + x) * 4]);
      }
  }
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, size, size,
               6, 0, GL_RGBA, GL_UNSIGNED_BYTE, layers.data());
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  cache.byCube[key] = tex;
  return tex;
}

// Read the current framebuffer back and write a binary PPM (P6). Used by the
// --screenshot acceptance path so the GL output is captured directly from the
// driver (glReadPixels), independent of the window system's root-pixmap readback
// — which a GPU swap chain under Xvfb does not expose. GL origin is bottom-left,
// so flip rows to top-down for the PPM.
bool writeFramebufferPPM(const std::string &path, int w, int h) {
  if (w <= 0 || h <= 0) return false;
  std::vector<unsigned char> px(static_cast<std::size_t>(w) * h * 3);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << "P6\n" << w << " " << h << "\n255\n";
  for (int row = h - 1; row >= 0; --row) // flip bottom-up GL -> top-down PPM.
    out.write(reinterpret_cast<const char *>(
                  px.data() + static_cast<std::size_t>(row) * w * 3),
              static_cast<std::streamsize>(w) * 3);
  return static_cast<bool>(out);
}

} // namespace

int main(int argc, char **argv) {
  // ----------------------------------------------------------------------
  // 0. Argument parsing. `--headless` (anywhere) runs the no-GL extraction
  //    probe; the first non-flag argument is the scene path.
  // ----------------------------------------------------------------------
  bool headless = false;
  std::string scenePath;
  std::string screenshotPath; // --screenshot <path>: render a few frames, dump a
                              // PPM via glReadPixels, then exit (GL acceptance).
  // --animate <dir>: drive the event graph at a FIXED dt (frame/fps) and dump
  // every frame as <dir>/frame_NNNN.ppm, then exit. Deterministic capture of the
  // realtime GL loop (interpolators/sequencers/ROUTEs) for headless demo reels.
  bool animate = false;
  int animFps = 30;
  double animDuration = 4.0;
  std::string framesDir;
  // --front: ignore any authored Viewpoint and frame the scene from a canonical
  // straight-on front camera (eye on +Z, looking down -Z, bounding-sphere fit).
  // Mirrors how the Web3D/NIST conformance set generates its `*-front.jpg`
  // reference views, so screenshots line up apples-to-apples for the visual sweep.
  bool frontView = false;
  // --pbr-shader <name> (or X3D_POC_PBR_SHADER env var): swap the PBR fragment
  // program's source file. "pbr.frag" (the default, also what an unset/empty
  // value resolves to) loads poc's native shaders/pbr.frag exactly as before —
  // byte-identical to pre-flag behavior. Any other name is looked up in the
  // sibling cpu_raster example's shaders dir (X3D_POC_PORTABLE_SHADER_DIR), so
  // the SAME usd_preview_surface.frag source that runs in the CPU rasterizer's
  // GLSL interpreter can also be swap-tested on real desktop GL here.
  std::string pbrShaderName;
  if (const char *e = std::getenv("X3D_POC_PBR_SHADER"); e && *e) pbrShaderName = e;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--headless") headless = true;
    else if (a == "--front") frontView = true;
    else if (a == "--screenshot" && i + 1 < argc) screenshotPath = argv[++i];
    else if (a == "--animate" && i + 1 < argc) { animate = true; framesDir = argv[++i]; }
    else if (a == "--fps" && i + 1 < argc) animFps = std::atoi(argv[++i]);
    else if (a == "--duration" && i + 1 < argc) animDuration = std::atof(argv[++i]);
    else if (a == "--pbr-shader" && i + 1 < argc) pbrShaderName = argv[++i];
    else if (scenePath.empty()) scenePath = a;
  }
  if (scenePath.empty())
    scenePath = std::string(X3D_POC_ASSET_DIR) + "/multishape.x3d";

  // ----------------------------------------------------------------------
  // 1. Parse the scene and bring up the runtime context (SDK side).
  // ----------------------------------------------------------------------
  std::fprintf(stderr, "[poc] loading %s\n", scenePath.c_str());

  x3d::runtime::X3DDocument doc;
  try {
    doc = x3d::codec::parseFile(scenePath);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "[poc] parse failed: %s\n", e.what());
    return 1;
  }

  x3d::runtime::Scene &scene = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  // CHECK the BridgeResult and bail/log on failure (unresolved EXTERNPROTO,
  // rejected ROUTEs, ...). A bad bridge is a load-time error, not a draw-time
  // surprise.
  x3d::runtime::BridgeResult bridge = ctx.buildFrom(scene);
  if (!bridge.ok()) {
    std::fprintf(stderr,
                 "[poc] scene bridge reported %zu rejected route(s):\n",
                 bridge.rejected.size());
    for (const auto &r : bridge.rejected)
      std::fprintf(stderr, "  - %s\n", r.reason.c_str());
    // Non-fatal for the PoC: rejected ROUTEs only drop animation, geometry
    // still renders. Log loudly and continue.
  } else {
    std::fprintf(stderr, "[poc] scene bridge ok (%zu route(s) added)\n",
                 bridge.routesAdded);
  }

  ctx.tick(0.0); // resolve bindings + initial transforms/bounds.

  // v1 T-TEX: hand the SDK a real TextureResolver so it decodes Source::Url
  // ImageTexture bytes (PoC-owned, relative to the scene dir) and threads the
  // RGBA8 surface onto each TextureRef.resolvedPixels. The draw loop then binds
  // resolvedPixels directly — no per-frame re-decode. The default ctor's null
  // resolver always-Failed => white fallback; this replaces it end-to-end.
  // (FontMetrics for Text uses the SDK's default monospace stub: glyph cells
  // render flat-colored until an embedder supplies an atlas via the seam.)
  ex::TextureResolver textureResolver =
      makeLocalTextureResolver(dirOf(scenePath));

  // T-TEXT: bake a glyph atlas (stb_truetype, via the SDK's x3d_stbtt io module)
  // and wire it into the FontMetrics seam so Text renders real letterforms. The
  // CPU bake needs no GL; the atlas texture is uploaded later (GL path only).
  // If no font is found, fontMetrics is null and the SDK's monospace stub stands.
  auto glyphAtlas = x3d::runtime::io::stbtt::makeStbttGlyphAtlas(resolveFontFaces());
  ex::MeshBuildOptions meshOptions;
  if (glyphAtlas.fontMetrics) meshOptions.fontMetrics = glyphAtlas.fontMetrics;
  // §31: authored shaders are selected against this GL context's compiler.
  auto glReady = std::make_shared<bool>(false);
  meshOptions.shaders.resolver = x3d::runtime::io::file::makeFileResolver(dirOf(scenePath));
  meshOptions.shaders.validator = makeGlShaderValidator(glReady);
  ex::SceneExtractor extractor(ctx, scene, meshOptions, textureResolver);

  // ----------------------------------------------------------------------
  // 1b. Headless probe (T10 build-side acceptance). Extract once and report
  //     item/vertex counts, then exit WITHOUT touching GLFW/glad/GL. This path
  //     never opens a display, so it runs in CI / over SSH. Exit 0 only when at
  //     least one RenderItem was produced.
  // ----------------------------------------------------------------------
  if (headless) {
    ex::RenderDelta snap = extractor.fullSnapshot();
    std::size_t items = extractor.itemCount();
    std::size_t firstVerts =
        items ? extractor.item(snap.added.front()).mesh->positions.size() : 0;
    std::fprintf(stderr,
                 "[poc] headless: %zu render item(s); item[0] has %zu vertices\n",
                 items, firstVerts);
    for (ex::RenderItemId id : snap.added) {
      const auto &it = extractor.item(id);
      if (it.skin)
        std::fprintf(stderr, "[poc] headless skin %u: corners=%zu normal indices=%zu influences=%zu displacers=%zu\n",
                     static_cast<unsigned>(id), it.skin->sourceCoordIndex.size(),
                     it.skin->sourceNormalIndex.size(), it.skin->binding->influences.size(),
                     it.skin->binding->displacers.size());
    }

    // T11 acceptance probe: count items that SHARE a GeomId node with another
    // item but carry a DISTINCT worldTransform. A DEF'd Shape USE'd under two
    // Transforms is exactly this: one geometry node, two per-path world matrices.
    // This proves the per-PATH transform model end-to-end at the seam.
    std::unordered_map<const X3DNode *, std::vector<ex::RenderItemId>> byGeomNode;
    for (ex::RenderItemId id = 0; id < items; ++id)
      byGeomNode[extractor.item(id).geometry.node].push_back(id);
    std::size_t sharedDistinct = 0; // geom nodes with >=2 placements at distinct xforms.
    for (const auto &kv : byGeomNode) {
      if (kv.second.size() < 2) continue;
      const Mat4 &m0 = extractor.item(kv.second.front()).worldTransform;
      for (std::size_t i = 1; i < kv.second.size(); ++i) {
        if (extractor.item(kv.second[i]).worldTransform.m !=
            m0.m) { // column-major array compare
          ++sharedDistinct;
          break;
        }
      }
    }

    // B8 texture self-check (no GL): count items whose MaterialDesc carries a
    // BaseColor/Diffuse texture AND whose mesh carries texcoords — exactly the
    // items the lit draw would resolve+sample. Proves the texture DESCRIPTOR path
    // reaches the consumer (the GL upload visual is the user's check).
    std::size_t texturedItems = 0;
    for (ex::RenderItemId id = 0; id < items; ++id) {
      const ex::RenderItem &it = extractor.item(id);
      if (it.mesh->texcoords.empty()) continue;
      for (const ex::TextureRef &t : it.material.textures) {
        if (t.slot == ex::TextureRef::Slot::BaseColor ||
            t.slot == ex::TextureRef::Slot::Diffuse) {
          ++texturedItems;
          break;
        }
      }
    }
    std::fprintf(stderr, "[poc] headless b8: textured_items=%zu\n", texturedItems);

    // v1 T-TEX self-check (no GL): count BaseColor/Diffuse Source::Url textures
    // the embedder's TextureResolver seam decoded onto TextureRef.resolvedPixels
    // (status Ready, non-empty RGBA). This is the seam the PoC binds in the draw
    // loop; reporting it here proves the decoded-pixel path reaches the consumer
    // independent of GL. Source::Inline textures are NOT counted (they carry
    // inlinePixels, never go through the resolver).
    std::size_t resolvedTextures = 0;
    for (ex::RenderItemId id = 0; id < items; ++id) {
      for (const ex::TextureRef &t : extractor.item(id).material.textures) {
        if (t.source == ex::TextureRef::Source::Url &&
            t.resolvedPixels.ready() && !t.resolvedPixels.pixels->rgba.empty())
          ++resolvedTextures;
      }
    }
    std::fprintf(stderr, "[poc] headless t-tex: resolved_textures=%zu\n",
                 resolvedTextures);

    // v1 T-TEXT self-check (no GL): count glyph-quad meshes the Text extraction
    // path emitted (MeshData.isGlyphMesh). Each carries +Z normals + texcoords so
    // it routes through the lit/textured draw path; the atlas (when a font
    // resolver supplies one) binds via MaterialDesc.textures like any texture.
    std::size_t glyphMeshItems = 0;
    for (ex::RenderItemId id = 0; id < items; ++id)
      if (extractor.item(id).mesh->isGlyphMesh) ++glyphMeshItems;
    std::fprintf(stderr, "[poc] headless t-text: glyph_mesh_items=%zu\n",
                 glyphMeshItems);
    // T12 lit self-check (no GL): confirm the shading inputs the lit path needs
    // are present at the seam — the bound NavigationInfo headlight flag, the
    // active world-resolved light count, and the first item's material color +
    // whether its mesh carries normals (a lit draw needs a shading normal). Per
    // §23.4.4 the headlight is added on top of authored lights when TRUE.
    // The effective count caps all authored light types and reserves one
    // of the kMaxLights slots.
    bool headlightOn = true;
    if (const X3DNode *nav = ctx.boundNavigationInfo())
      headlightOn =
          x3d::runtime::geombounds::getField<bool>(*nav, "headlight", true);
    std::vector<ex::LightDesc> lights = extractor.lights();
    std::size_t directional = 0;
    for (const auto &L : lights)
      if (L.type == ex::LightDesc::Type::Directional) ++directional;
    const std::size_t lightCap =
        headlightOn ? static_cast<std::size_t>(kMaxLights - 1)
                    : static_cast<std::size_t>(kMaxLights);
    const std::size_t litLights =
        std::min(lights.size(), lightCap) + (headlightOn ? 1u : 0u);
    bool firstHasNormals =
        items ? extractor.item(snap.added.front()).mesh->hasNormals : false;
    SFColorRGBA c0 =
        items ? extractor.item(snap.added.front()).material.toRGBA()
              : SFColorRGBA{0, 0, 0, 0};

    // B7 transparency self-check (no GL): count items the renderer would route
    // to the BLEND pass — material alphaMode == Blend OR transparency > 0. These
    // are the items the draw loop alpha-blends back-to-front; reporting the count
    // here makes the transparency partition observable in the headless probe.
    std::size_t transparentItems = 0;
    for (ex::RenderItemId id = 0; id < items; ++id) {
      const ex::MaterialDesc &mat = extractor.item(id).material;
      if (mat.alphaMode == ex::AlphaMode::Blend || mat.transparency > 0.0f)
        ++transparentItems;
    }
    std::fprintf(stderr,
                 "[poc] headless lit: headlight=%s directional_lights=%zu authored_lights=%zu "
                 "effective_lit_lights=%zu item[0] hasNormals=%s "
                 "diffuse=rgba(%.2f,%.2f,%.2f,%.2f) transparent_items=%zu\n",
                 headlightOn ? "true" : "false", directional, lights.size(), litLights,
                 firstHasNormals ? "true" : "false", c0.r, c0.g, c0.b, c0.a,
                 transparentItems);

    std::printf("render_items=%zu first_item_vertices=%zu shared_geom_distinct_xform=%zu "
                "lit_lights=%zu first_item_normals=%d transparent_items=%zu "
                "resolved_textures=%zu glyph_mesh_items=%zu\n",
                items, firstVerts, sharedDistinct, litLights,
                firstHasNormals ? 1 : 0, transparentItems, resolvedTextures,
                glyphMeshItems);
    if (items == 0) {
      std::fprintf(stderr, "[poc] headless: NO render items extracted\n");
      return 1;
    }
    return 0;
  }

  // ----------------------------------------------------------------------
  // 2. GLFW + GL core-3.3 context (Wayland-native, X11/XWayland fallback).
  // ----------------------------------------------------------------------
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) {
    std::fprintf(stderr, "[poc] glfwInit failed\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  GLFWwindow *win =
      glfwCreateWindow(1280, 720, "x3d-cpp PoC renderer", nullptr, nullptr);
  if (!win) {
    std::fprintf(stderr,
                 "[poc] glfwCreateWindow failed. On Wayland this usually means "
                 "missing wayland/libxkbcommon/EGL dev libs — see README.\n");
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(win);
  // vsync on for interactive use; OFF for the --screenshot acceptance path so a
  // GPU swap chain under a headless X server (Xvfb) does not block on vblank.
  glfwSwapInterval((screenshotPath.empty() && !animate) ? 1 : 0);
  const bool uiEnabled = screenshotPath.empty() && !animate;

  if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
    std::fprintf(stderr, "[poc] gladLoadGL failed\n");
    glfwDestroyWindow(win);
    glfwTerminate();
    return 1;
  }
  *glReady = true; // author shaders now validate by compiling.
  std::fprintf(stderr, "[poc] GL_VERSION  : %s\n", glGetString(GL_VERSION));
  std::fprintf(stderr, "[poc] GL_RENDERER : %s\n", glGetString(GL_RENDERER));

  // GL_KHR_debug callback (if the driver granted a debug context).
  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                          GL_TRUE);
    std::fprintf(stderr, "[poc] GL_KHR_debug callback installed\n");
  }

  // Interaction: attach the pointer-driven systems (PointingSensorSystem +
  // NavigationSystem) and bridge glfw input into the SDK seam. The bridge needs
  // the window (callbacks) and the nav system (TAB mode-cycle). lastView/lastProj
  // carry the previous frame's camera so the cursor ray is unprojected against
  // the image the user is actually looking at.
  // Light up the full behavior runtime (TimeSensor clock, interpolators,
  // followers, event utilities, view-dependent, key sensors, viewpoint bind) so
  // authored animation/sensors actually run — then add the interactive systems.
  x3d::runtime::attachStandardRuntime(scene, ctx, nullptr, {}, {}, meshOptions.shaders);
  auto navSys = x3d::runtime::attachInteractive(scene, ctx);
  InputBridge input(ctx, win, navSys);
  Mat4 lastView = Mat4::identity();
  Mat4 lastProj = Mat4::identity();

  if (uiEnabled) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");
  }

  // ----------------------------------------------------------------------
  // 3. Shaders.
  //   phongProg (lit.vert / lit.frag) — Blinn-Phong + textures + normal-map.
  //   unlitProg (unlit.vert / unlit.frag) — B4: lines/points/normal-less.
  //   pbrProg (lit.vert / pbr.frag) — metallic-roughness analytic BRDF.
  //   authorProg — compiled on demand per ComposedShader (cached by source hash).
  // ----------------------------------------------------------------------
  const std::string shaderDir = X3D_POC_SHADER_DIR;

  // ---- Phong / lit program (was `prog`; renamed phongProg for clarity) -----
  GLuint vs = compileShader(GL_VERTEX_SHADER,
                            readTextFile(shaderDir + "/lit.vert"), "lit.vert");
  GLuint fs = compileShader(GL_FRAGMENT_SHADER,
                            readTextFile(shaderDir + "/lit.frag"), "lit.frag");
  GLuint phongProg = (vs && fs) ? linkProgram(vs, fs) : 0;
  if (vs) glDeleteShader(vs);
  if (fs) glDeleteShader(fs);
  if (!phongProg) {
    std::fprintf(stderr, "[poc] Phong shader program unavailable; clear-only mode\n");
  }
  const GLint uModel = phongProg ? glGetUniformLocation(phongProg, "uModel") : -1;
  const GLint uView = phongProg ? glGetUniformLocation(phongProg, "uView") : -1;
  const GLint uProj = phongProg ? glGetUniformLocation(phongProg, "uProjection") : -1;
  const GLint uNormalMat = phongProg ? glGetUniformLocation(phongProg, "uNormalMatrix") : -1;
  const GLint uDiffuse = phongProg ? glGetUniformLocation(phongProg, "uDiffuse") : -1;
  const GLint uEmissive = phongProg ? glGetUniformLocation(phongProg, "uEmissive") : -1;
  const GLint uAmbientColor = phongProg ? glGetUniformLocation(phongProg, "uAmbientColor") : -1;
  const GLint uHasColors = phongProg ? glGetUniformLocation(phongProg, "uHasColors") : -1;
  const GLint uNumLights = phongProg ? glGetUniformLocation(phongProg, "uNumLights") : -1;
  const GLint uLightDirEye = phongProg ? glGetUniformLocation(phongProg, "uLightDirEye") : -1;
  const GLint uLightColor = phongProg ? glGetUniformLocation(phongProg, "uLightColor") : -1;
  const GLint uLightAmbient = phongProg ? glGetUniformLocation(phongProg, "uLightAmbient") : -1;
  // B7 Blinn-Phong specular + alpha-mask uniforms.
  const GLint uSpecular = phongProg ? glGetUniformLocation(phongProg, "uSpecular") : -1;
  const GLint uShininess = phongProg ? glGetUniformLocation(phongProg, "uShininess") : -1;
  const GLint uAlphaMode = phongProg ? glGetUniformLocation(phongProg, "uAlphaMode") : -1;
  const GLint uAlphaCutoff = phongProg ? glGetUniformLocation(phongProg, "uAlphaCutoff") : -1;
  // B8 + Phase 5.2 texture uniforms (diffuse/normal/emissive/specular).
  const GLint uHasTexture = phongProg ? glGetUniformLocation(phongProg, "uHasTexture") : -1;
  const GLint uTexture = phongProg ? glGetUniformLocation(phongProg, "uTexture") : -1;
  // T-TEXT: when set, uTexture is the font coverage atlas (sample .r as alpha).
  const GLint uGlyphAtlas = phongProg ? glGetUniformLocation(phongProg, "uGlyphAtlas") : -1;

  // T-TEXT: upload the baked glyph coverage atlas as an R8 texture (rows already
  // bottom-up, so no flip). Bound on unit 0 only for glyph-mesh draws.
  GLuint glyphAtlasTex = 0;
  if (glyphAtlas.atlas.ok) {
    glGenTextures(1, &glyphAtlasTex);
    glBindTexture(GL_TEXTURE_2D, glyphAtlasTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, glyphAtlas.atlas.width,
                 glyphAtlas.atlas.height, 0, GL_RED, GL_UNSIGNED_BYTE,
                 glyphAtlas.atlas.coverage.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::fprintf(stderr, "[poc] glyph atlas %dx%d uploaded\n",
                 glyphAtlas.atlas.width, glyphAtlas.atlas.height);
  }
  // Phase 5.2 extended texture slots.
  const GLint uHasNormalTex   = phongProg ? glGetUniformLocation(phongProg, "uHasNormalTex") : -1;
  const GLint uNormalTex      = phongProg ? glGetUniformLocation(phongProg, "uNormalTex") : -1;
  const GLint uNormalScale    = phongProg ? glGetUniformLocation(phongProg, "uNormalScale") : -1;
  const GLint uHasEmissiveTex = phongProg ? glGetUniformLocation(phongProg, "uHasEmissiveTex") : -1;
  const GLint uEmissiveTex    = phongProg ? glGetUniformLocation(phongProg, "uEmissiveTex") : -1;
  const GLint uHasSpecularTex = phongProg ? glGetUniformLocation(phongProg, "uHasSpecularTex") : -1;
  const GLint uSpecularTex    = phongProg ? glGetUniformLocation(phongProg, "uSpecularTex") : -1;
  // Phase 5.5 gamma toggle for Phong.
  const GLint uGammaOutput    = phongProg ? glGetUniformLocation(phongProg, "uGammaOutput") : -1;
  // Fog (§24.4.2): the bound Fog's colour/type/world-scaled visibilityRange.
  const GLint uFogColor       = phongProg ? glGetUniformLocation(phongProg, "uFogColor") : -1;
  const GLint uFogType        = phongProg ? glGetUniformLocation(phongProg, "uFogType") : -1;
  const GLint uFogRange       = phongProg ? glGetUniformLocation(phongProg, "uFogVisibilityRange") : -1;

  // ---- B4 UNLIT program — lines/points/normal-less meshes ------------------
  GLuint uvs = compileShader(GL_VERTEX_SHADER,
                             readTextFile(shaderDir + "/unlit.vert"), "unlit.vert");
  GLuint ufs = compileShader(GL_FRAGMENT_SHADER,
                             readTextFile(shaderDir + "/unlit.frag"), "unlit.frag");
  GLuint unlitProg = (uvs && ufs) ? linkProgram(uvs, ufs) : 0;
  if (uvs) glDeleteShader(uvs);
  if (ufs) glDeleteShader(ufs);
  const GLint uUnlitModel = unlitProg ? glGetUniformLocation(unlitProg, "uModel") : -1;
  const GLint uUnlitView = unlitProg ? glGetUniformLocation(unlitProg, "uView") : -1;
  const GLint uUnlitProj = unlitProg ? glGetUniformLocation(unlitProg, "uProjection") : -1;
  const GLint uUnlitBaseColor = unlitProg ? glGetUniformLocation(unlitProg, "uBaseColor") : -1;
  const GLint uUnlitHasColors = unlitProg ? glGetUniformLocation(unlitProg, "uHasColors") : -1;
  const GLint uUnlitTexture = unlitProg ? glGetUniformLocation(unlitProg, "uTexture") : -1;
  const GLint uUnlitHasTexture = unlitProg ? glGetUniformLocation(unlitProg, "uHasTexture") : -1;
  // Fog (§24.4.2) for the unlit path.
  const GLint uUnlitFogColor = unlitProg ? glGetUniformLocation(unlitProg, "uFogColor") : -1;
  const GLint uUnlitFogType  = unlitProg ? glGetUniformLocation(unlitProg, "uFogType") : -1;
  const GLint uUnlitFogRange = unlitProg ? glGetUniformLocation(unlitProg, "uFogVisibilityRange") : -1;
  // SEAM-LINEPOINT: §12.4.8 PointProperties → gl_PointSize (unlit.vert).
  const GLint uUnlitPointScale = unlitProg ? glGetUniformLocation(unlitProg, "uPointSizeScale") : -1;
  const GLint uUnlitPointAtten = unlitProg ? glGetUniformLocation(unlitProg, "uPointAttenuation") : -1;
  const GLint uUnlitPointMin   = unlitProg ? glGetUniformLocation(unlitProg, "uPointSizeMin") : -1;
  const GLint uUnlitPointMax   = unlitProg ? glGetUniformLocation(unlitProg, "uPointSizeMax") : -1;

  // ---- Phase 5.3 PBR program (lit.vert / pbr.frag) -------------------------
  // Swap-test seam: --pbr-shader / X3D_POC_PBR_SHADER selects an alternate
  // fragment source (e.g. the portable usd_preview_surface.frag) from the
  // sibling cpu_raster shaders dir; empty/"pbr.frag" resolves to the native
  // path below, unchanged.
  const bool usePortablePbrShader = !pbrShaderName.empty() && pbrShaderName != "pbr.frag";
  const std::string pbrFragPath = usePortablePbrShader
      ? std::string(X3D_POC_PORTABLE_SHADER_DIR) + "/" + pbrShaderName
      : shaderDir + "/pbr.frag";
  if (usePortablePbrShader)
    std::fprintf(stderr, "[poc] PBR fragment shader override: %s\n", pbrFragPath.c_str());
  GLuint pvs = compileShader(GL_VERTEX_SHADER,
                             readTextFile(shaderDir + "/lit.vert"), "lit.vert(pbr)");
  const std::string pbrFragLabel = pbrShaderName.empty() ? "pbr.frag" : pbrShaderName;
  GLuint pfs = compileShader(GL_FRAGMENT_SHADER,
                             readTextFile(pbrFragPath), pbrFragLabel.c_str());
  GLuint pbrProg = (pvs && pfs) ? linkProgram(pvs, pfs) : 0;
  if (pvs) glDeleteShader(pvs);
  if (pfs) glDeleteShader(pfs);
  if (!pbrProg) {
    std::fprintf(stderr, "[poc] PBR shader unavailable; PhysicalMaterial will "
                         "fall back to Phong\n");
  }

  // §17.3.1 shadows (shadow.glsl): the light depth pass reuses unlit.vert
  // (skinning, model/view/projection) with a depth-only fragment stage. The
  // depth array texture grows on demand. The lit programs sample it on unit 5,
  // set once so the sampler never aliases unit 0's sampler2D.
  GLuint shadowProg = 0;
  {
    GLuint svs = compileShader(GL_VERTEX_SHADER, readTextFile(shaderDir + "/unlit.vert"),
                               "unlit.vert(shadow)");
    GLuint sfs = compileShader(GL_FRAGMENT_SHADER,
                               readTextFile(shaderDir + "/shadow_depth.frag"),
                               "shadow_depth.frag");
    shadowProg = (svs && sfs) ? linkProgram(svs, sfs) : 0;
    if (svs) glDeleteShader(svs);
    if (sfs) glDeleteShader(sfs);
  }
  for (GLuint p : {phongProg, pbrProg}) {
    const GLint loc = p ? glGetUniformLocation(p, "uShadowMaps") : -1;
    if (loc < 0) continue;
    glUseProgram(p);
    glUniform1i(loc, 5);
  }
  for (GLuint p : {unlitProg, phongProg, pbrProg}) {
    const GLint loc = p ? glGetUniformLocation(p, "uStageCube") : -1;
    if (loc < 0) continue;
    glUseProgram(p);
    glUniform1i(loc, 12); // multitexture.glsl's cube-map stage.
  }
  glUseProgram(0);
  constexpr int kShadowSize = 1024;
  constexpr int kMaxShadowLayers = 24; // shadow.glsl
  GLuint shadowTex = 0, shadowFbo = 0;
  int shadowLayersAllocated = 0;
  const GLint uPbrModel        = pbrProg ? glGetUniformLocation(pbrProg, "uModel") : -1;
  const GLint uPbrView         = pbrProg ? glGetUniformLocation(pbrProg, "uView") : -1;
  const GLint uPbrProj         = pbrProg ? glGetUniformLocation(pbrProg, "uProjection") : -1;
  const GLint uPbrNormalMat    = pbrProg ? glGetUniformLocation(pbrProg, "uNormalMatrix") : -1;
  const GLint uPbrBaseColor    = pbrProg ? glGetUniformLocation(pbrProg, "uBaseColor") : -1;
  const GLint uPbrMetallic     = pbrProg ? glGetUniformLocation(pbrProg, "uMetallic") : -1;
  const GLint uPbrRoughness    = pbrProg ? glGetUniformLocation(pbrProg, "uRoughness") : -1;
  const GLint uPbrEmissive     = pbrProg ? glGetUniformLocation(pbrProg, "uEmissive") : -1;
  const GLint uPbrAlphaMode    = pbrProg ? glGetUniformLocation(pbrProg, "uAlphaMode") : -1;
  const GLint uPbrAlphaCutoff  = pbrProg ? glGetUniformLocation(pbrProg, "uAlphaCutoff") : -1;
  const GLint uPbrNumLights    = pbrProg ? glGetUniformLocation(pbrProg, "uNumLights") : -1;
  const GLint uPbrLightDirEye  = pbrProg ? glGetUniformLocation(pbrProg, "uLightDirEye") : -1;
  const GLint uPbrLightColor   = pbrProg ? glGetUniformLocation(pbrProg, "uLightColor") : -1;
  const GLint uPbrLightAmbient = pbrProg ? glGetUniformLocation(pbrProg, "uLightAmbient") : -1;
  const GLint uPbrHasColors    = pbrProg ? glGetUniformLocation(pbrProg, "uHasColors") : -1;
  // Fog (§24.4.2) for the PBR path.
  const GLint uPbrFogColor     = pbrProg ? glGetUniformLocation(pbrProg, "uFogColor") : -1;
  const GLint uPbrFogType      = pbrProg ? glGetUniformLocation(pbrProg, "uFogType") : -1;
  const GLint uPbrFogRange     = pbrProg ? glGetUniformLocation(pbrProg, "uFogVisibilityRange") : -1;
  // PBR texture slots (unit 0=baseColor, 1=normal, 2=emissive, 3=metallicRoughness, 4=occlusion).
  const GLint uPbrBaseColorTex = pbrProg ? glGetUniformLocation(pbrProg, "uBaseColorTex") : -1;
  const GLint uPbrNormalTex    = pbrProg ? glGetUniformLocation(pbrProg, "uNormalTex") : -1;
  const GLint uPbrEmissiveTex  = pbrProg ? glGetUniformLocation(pbrProg, "uEmissiveTex") : -1;
  const GLint uPbrMRTex        = pbrProg ? glGetUniformLocation(pbrProg, "uMetallicRoughnessTex") : -1;
  const GLint uPbrOcclusionTex = pbrProg ? glGetUniformLocation(pbrProg, "uOcclusionTex") : -1;
  const GLint uPbrHasBaseColorTex = pbrProg ? glGetUniformLocation(pbrProg, "uHasBaseColorTex") : -1;
  const GLint uPbrHasNormalTex    = pbrProg ? glGetUniformLocation(pbrProg, "uHasNormalTex") : -1;
  const GLint uPbrHasEmissiveTex  = pbrProg ? glGetUniformLocation(pbrProg, "uHasEmissiveTex") : -1;
  const GLint uPbrHasMRTex        = pbrProg ? glGetUniformLocation(pbrProg, "uHasMetallicRoughnessTex") : -1;
  const GLint uPbrHasOcclusionTex = pbrProg ? glGetUniformLocation(pbrProg, "uHasOcclusionTex") : -1;
  const GLint uPbrNormalScale     = pbrProg ? glGetUniformLocation(pbrProg, "uNormalScale") : -1;
  const GLint uPbrOcclusionStrength = pbrProg ? glGetUniformLocation(pbrProg, "uOcclusionStrength") : -1;
  // UsdPreviewSurface fidelity uniforms (swap-test seam host contract): pbr.frag
  // doesn't declare these, so the locations resolve to -1 there and every
  // glUniform* call below is a no-op guarded by `>= 0`. When the portable
  // usd_preview_surface.frag is active, binding these to the spec defaults is
  // what keeps it bit-parity with the native metallic-roughness path (an
  // unbound sampler/uniform on real GL defaults to 0, which would otherwise
  // e.g. turn uIor=0 into a mirror F0 of 1.0 — see uUsdIor default below).
  const GLint uUsdUseSpecularWorkflow = pbrProg ? glGetUniformLocation(pbrProg, "uUseSpecularWorkflow") : -1;
  const GLint uUsdSpecularColor       = pbrProg ? glGetUniformLocation(pbrProg, "uSpecularColor") : -1;
  const GLint uUsdIor                 = pbrProg ? glGetUniformLocation(pbrProg, "uIor") : -1;
  const GLint uUsdClearcoat           = pbrProg ? glGetUniformLocation(pbrProg, "uClearcoat") : -1;
  const GLint uUsdClearcoatRoughness  = pbrProg ? glGetUniformLocation(pbrProg, "uClearcoatRoughness") : -1;
  const GLint uUsdOpacityMode         = pbrProg ? glGetUniformLocation(pbrProg, "uOpacityMode") : -1;
  const GLint uUsdOpacityThreshold    = pbrProg ? glGetUniformLocation(pbrProg, "uOpacityThreshold") : -1;

  // ---- #39 Background gradient program (bg.vert/bg.frag) --------------------
  // Paints the bound Background's full sky/ground gradient as a fullscreen
  // dome BEFORE scene geometry (see the render loop below). No VBO — the
  // vertex shader synthesizes a covering triangle from gl_VertexID, so a bare
  // VAO is enough to issue the draw call.
  GLuint bvs = compileShader(GL_VERTEX_SHADER, readTextFile(shaderDir + "/bg.vert"), "bg.vert");
  GLuint bfs = compileShader(GL_FRAGMENT_SHADER, readTextFile(shaderDir + "/bg.frag"), "bg.frag");
  GLuint bgProg = (bvs && bfs) ? linkProgram(bvs, bfs) : 0;
  if (bvs) glDeleteShader(bvs);
  if (bfs) glDeleteShader(bfs);
  if (!bgProg) {
    std::fprintf(stderr, "[poc] Background gradient shader unavailable; "
                         "flat first-skyColor clear only\n");
  }
  const GLint uBgInvView         = bgProg ? glGetUniformLocation(bgProg, "uInvView") : -1;
  const GLint uBgInvP0           = bgProg ? glGetUniformLocation(bgProg, "uInvP0") : -1;
  const GLint uBgInvP5           = bgProg ? glGetUniformLocation(bgProg, "uInvP5") : -1;
  const GLint uBgSkyColor        = bgProg ? glGetUniformLocation(bgProg, "uSkyColor") : -1;
  const GLint uBgSkyAngle        = bgProg ? glGetUniformLocation(bgProg, "uSkyAngle") : -1;
  const GLint uBgSkyColorCount   = bgProg ? glGetUniformLocation(bgProg, "uSkyColorCount") : -1;
  const GLint uBgSkyAngleCount   = bgProg ? glGetUniformLocation(bgProg, "uSkyAngleCount") : -1;
  const GLint uBgGroundColor      = bgProg ? glGetUniformLocation(bgProg, "uGroundColor") : -1;
  const GLint uBgGroundAngle      = bgProg ? glGetUniformLocation(bgProg, "uGroundAngle") : -1;
  const GLint uBgGroundColorCount = bgProg ? glGetUniformLocation(bgProg, "uGroundColorCount") : -1;
  const GLint uBgGroundAngleCount = bgProg ? glGetUniformLocation(bgProg, "uGroundAngleCount") : -1;
  GLuint bgVao = 0;
  glGenVertexArrays(1, &bgVao);

  // ---- Phase 5.4 author-shader cache (ComposedShader, compiled on demand) ---
  // Key: combined VS+FS source string. Value: linked GL program (or 0 = failed).
  std::unordered_map<std::string, GLuint> authorProgCache;

  // ----------------------------------------------------------------------
  // 4. Extract once (frame-0 full snapshot) and upload each GeomId once.
  //
  // The GpuMesh cache is keyed by GeomId (node* + contentVersion). Several
  // RenderItems (e.g. a DEF'd Shape USE'd under two Transforms) legitimately
  // share ONE GeomId — they upload once and instance N times, each with its own
  // per-path model uniform. We refcount by GeomId so a GpuMesh is GC'd only when
  // the LAST item referencing it leaves the scene (delta.removed), and a geometry
  // CONTENT change (delta.updatedGeometry bumps contentVersion -> a NEW GeomId
  // key) re-extracts + re-uploads under the new key while the stale key is GC'd.
  // ----------------------------------------------------------------------
  std::unordered_map<ex::GeomId, GpuMesh, ex::GeomIdHash> gpuMeshes;
  std::unordered_map<ex::GeomId, int, ex::GeomIdHash> meshRefs;
  std::unordered_map<ex::RenderItemId, GpuSkin> gpuSkins;

  // Ensure a GpuMesh exists for an item's GeomId, uploading on first reference
  // and bumping its refcount. Returns nothing; gpuMeshes/meshRefs are mutated.
  auto acquireMesh = [&](const ex::RenderItem &it) {
    if (gpuMeshes.find(it.geometry) == gpuMeshes.end())
      gpuMeshes.emplace(it.geometry, uploadMesh(*it.mesh));
    ++meshRefs[it.geometry];
  };
  auto acquireSkin = [&](ex::RenderItemId id) {
    const auto &it = extractor.item(id);
    if (!it.skin) return;
    // Joint displacers add offsets after skinning; those skins use the
    // reference CPU mesh. Everything else skins on the GPU.
    bool fallback = !it.skin->binding->displacers.empty() ||
                    it.skin->sourceCoordIndex.size() != it.mesh->positions.size();
    ex::MeshData bindMesh = *it.mesh;
    if (!fallback) {
      const auto &binding = *it.skin->binding;
      for (std::size_t i = 0; i < bindMesh.positions.size(); ++i) {
        auto source = it.skin->sourceCoordIndex[i];
        if (source < binding.bindPositions.size())
          bindMesh.positions[i] = binding.bindPositions[source];
        // Authored bind normal when present; otherwise keep the normal the
        // mesh builder generated for the bind pose.
        if (i < it.skin->sourceNormalIndex.size() && i < bindMesh.normals.size()) {
          auto normal = it.skin->sourceNormalIndex[i];
          if (normal < binding.bindNormals.size())
            bindMesh.normals[i] = binding.bindNormals[normal];
        }
      }
    }
    GpuSkin skin = uploadSkin(it, fallback ? extractor.deformedMesh(id) : bindMesh, fallback);
    if (!fallback) updateSkinPose(skin, it, ctx.geoProjection());
    gpuSkins.emplace(id, std::move(skin));
    std::fprintf(stderr, "[poc] skin item %u: %s, %zu influences\n",
                 static_cast<unsigned>(id), fallback ? "CPU reference" : "GPU palette",
                 it.skin->binding->influences.size());
  };
  // Drop one reference to a GeomId; delete the GpuMesh when it hits zero.
  auto releaseMesh = [&](const ex::GeomId &gid) {
    auto rit = meshRefs.find(gid);
    if (rit == meshRefs.end()) return;
    if (--rit->second <= 0) {
      meshRefs.erase(rit);
      auto mit = gpuMeshes.find(gid);
      if (mit != gpuMeshes.end()) {
        glDeleteVertexArrays(1, &mit->second.vao);
        glDeleteBuffers(1, &mit->second.vbo);
        glDeleteBuffers(1, &mit->second.ebo);
        gpuMeshes.erase(mit);
      }
    }
  };

  // Per-item GeomId shadow: the GeomId each RenderItemId currently references on
  // the GPU. delta() reports CHANGED ids, not their previous GeomId, so we track
  // it consumer-side to know which GpuMesh to release on a content bump / removal.
  std::unordered_map<ex::RenderItemId, ex::GeomId> itemGeom;

  ex::RenderDelta snapshot = extractor.fullSnapshot();
  for (ex::RenderItemId id : snapshot.added) {
    const ex::RenderItem &it = extractor.item(id);
    acquireMesh(it);
    acquireSkin(id);
    itemGeom[id] = it.geometry;
  }
  std::fprintf(stderr, "[poc] extracted %zu render item(s), %zu unique mesh(es)\n",
               extractor.itemCount(), gpuMeshes.size());

  // near/far fit from the per-path scene world bounds (NOT ctx.worldBounds()).
  Aabb sceneBounds = extractor.sceneWorldBounds();

  // "View all" default camera: when the scene authors NO Viewpoint, viewMatrix()
  // is identity (eye at origin), which mis-frames any large/offset model (e.g. the
  // SanCarlos cathedral spans 97 units centered at z=-23.5 — most of it would sit
  // behind/far from an origin camera). Frame the bounding sphere from a 3/4 angle.
  // --front forces the canonical front fit camera even when a Viewpoint IS
  // authored, so every scene lines up with the NIST `*-front.jpg` reference.
  const bool useFitCamera = (frontView || ctx.boundViewpoint() == nullptr);
  Mat4 fitView = Mat4::identity();
  if (useFitCamera && !sceneBounds.empty) {
    SFVec3f c = {(sceneBounds.min.x + sceneBounds.max.x) * 0.5f,
                 (sceneBounds.min.y + sceneBounds.max.y) * 0.5f,
                 (sceneBounds.min.z + sceneBounds.max.z) * 0.5f};
    SFVec3f sz = sceneBounds.size();
    float radius = 0.5f * std::sqrt(sz.x*sz.x + sz.y*sz.y + sz.z*sz.z);
    float fov = extractor.camera().fieldOfView;            // X3D min-dimension FOV
    float dist = radius / std::sin((std::max)(0.1f, fov) * 0.5f) * 1.25f; // margin
    // Straight-on front for --front (matches NIST front view); otherwise a 3/4
    // angle that reveals depth for the default "no Viewpoint" framing.
    SFVec3f dir = frontView ? SFVec3f{0.0f, 0.0f, 1.0f}
                            : v3norm({0.45f, 0.35f, 1.0f}); // front, slightly up/right
    SFVec3f eye = v3add(c, v3mul(dir, dist));
    fitView = lookAt(eye, c, {0.0f, 1.0f, 0.0f});
    std::fprintf(stderr, "[poc] %s -> view-all camera "
                 "(center %.1f %.1f %.1f, dist %.1f)\n",
                 frontView ? "--front" : "no Viewpoint authored",
                 c.x, c.y, c.z, dist);
  }

  // Background clear color from the bound Background (first skyColor), else a
  // visible default so a successful clear is unmistakable.
  float clearR = 0.10f, clearG = 0.12f, clearB = 0.18f;
  if (const X3DNode *bg = ctx.boundBackground()) {
    auto sky = x3d::runtime::geombounds::getField<std::vector<SFColor>>(
        *bg, "skyColor", {});
    if (!sky.empty()) {
      clearR = sky[0].r;
      clearG = sky[0].g;
      clearB = sky[0].b;
    }
  }

  // A 1x1 opaque-white texture bound to unit 0 whenever a draw has NO real
  // texture. The lit shader statically samples uTexture (guarded at runtime by
  // uHasTexture), and GL's debug validation flags a sampler whose bound texture
  // is incomplete/zero EVEN when that branch is skipped — so keep unit 0 always
  // complete to silence the "no defined base level" warning flood. White means a
  // stray sample (shouldn't happen) is a no-op multiply.
  GLuint whiteTex = 0;
  glGenTextures(1, &whiteTex);
  glBindTexture(GL_TEXTURE_2D, whiteTex);
  const unsigned char kWhitePixel[4] = {255, 255, 255, 255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kWhitePixel);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);

  glEnable(GL_DEPTH_TEST);
  // B4: let the unlit vertex shader size GL_POINTS (PointSet) via gl_PointSize.
  glEnable(GL_PROGRAM_POINT_SIZE);
  // M3: culling is now per-draw, driven by MeshData.ccw/solid (set in the draw
  // loop). Leave it disabled here as the default state; each draw enables/disables
  // and picks the front-face winding for its own mesh.
  glDisable(GL_CULL_FACE);

  // NavigationInfo headlight flag (default true). §23.4.4: headlight TRUE adds
  // a camera-space headlight regardless of the scene's own lights.
  bool headlightOn = true;
  if (const X3DNode *nav = ctx.boundNavigationInfo()) {
    headlightOn =
        x3d::runtime::geombounds::getField<bool>(*nav, "headlight", true);
  }
  std::fprintf(stderr, "[poc] NavigationInfo headlight=%s\n",
               headlightOn ? "true" : "false");

  // B8: the CONSUMER's asset resolver (local-file, relative to the scene dir)
  // and a URL/inline-keyed GL texture cache. Both live OUTSIDE the SDK; the SDK
  // surfaced only URLs/inline-pixels. A future async/http backend would swap in
  // a resolver that returns Pending without blocking the frame.
  ex::AssetResolver assetResolver = makeLocalFileResolver(dirOf(scenePath));
  TextureCache texCache;
  // MovieTexture decode (ADR-0041): Backend A (pl_mpeg) wired to the same
  // local-file resolver. mediaTime is set each frame from the render clock below.
  MovieState movieState;
  movieState.decoder = x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(assetResolver);

  // ----------------------------------------------------------------------
  // 5. Render loop.
  // ----------------------------------------------------------------------
  double t0 = glfwGetTime();
  double lastFrameClock = t0;
  int frameNo = 0;
  int animFrame = 0;
  const int animTotal = static_cast<int>(animDuration * animFps + 0.5);
  bool showDiagnostics = true;
  bool showImGuiDemo = false;
  bool wireframe = false;
  while (!glfwWindowShouldClose(win)) {
    // --animate advances time at a FIXED dt so the capture is deterministic and
    // loop-seamless (independent of the software-GL frame rate under Xvfb). Start
    // at +dt (not 0): the bring-up tick(0.0) already consumed t=0, and delta()
    // enforces a strictly-increasing time per tick (as the interactive path gets
    // for free from glfwGetTime()).
    double now = animate ? static_cast<double>(animFrame + 1) / animFps
                         : glfwGetTime() - t0;
    const double frameClock = glfwGetTime();
    const double frameDt = animate ? (1.0 / static_cast<double>(animFps))
                                   : (frameClock - lastFrameClock);
    lastFrameClock = frameClock;
    // MovieTexture media time: the PoC render clock drives decode (loop wrap / the
    // X3DTimeDependentSystem lifecycle is a follow-up; the decoder clamps to the
    // last frame at EOF so a non-looping clip holds, not blanks). --screenshot
    // captures ~t=0 -> the clip's first frame.
    movieState.mediaTime = now;
    // Feed the cursor ray into the SDK seam BEFORE tick, unprojected against the
    // PREVIOUS frame's camera (the image the user is pointing at). Keys + mouse
    // buttons arrive via glfw callbacks (fired during glfwPollEvents below).
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    input.feedPointerRay(lastView, lastProj, fbw, fbh);
    ctx.tick(now); // exactly one tick per frame (seam contract).

    // ------------------------------------------------------------------
    // INCREMENTAL UPDATE: exactly ONE delta() per tick (the seam contract —
    // tick() clears the dirty set at its end, so delta() reads it once). Apply
    // the buckets to the GPU caches before drawing:
    //   * updatedGeometry: the GeomId.contentVersion bumped, so the item now
    //       references a NEW GeomId. Release the stale GeomId, acquire the new
    //       one (re-extract+re-upload via uploadMesh; glBufferData orphans the
    //       old buffer store implicitly under the new VBO).
    //   * updatedTransform: the per-path worldTransform changed — no GPU upload,
    //       the model uniform is re-read from extractor.item() at draw time, so
    //       nothing to do here beyond the (free) re-read in the draw loop.
    //   * added: a new placement — acquire its mesh, shadow its GeomId.
    //   * removed: a placement left — release its mesh, drop the shadow.
    // ------------------------------------------------------------------
    ex::RenderDelta d = extractor.delta();

    for (ex::RenderItemId id : d.removed) {
      auto skin = gpuSkins.find(id);
      if (skin != gpuSkins.end()) { destroySkin(skin->second); gpuSkins.erase(skin); }
      auto sit = itemGeom.find(id);
      if (sit != itemGeom.end()) { releaseMesh(sit->second); itemGeom.erase(sit); }
    }
    for (ex::RenderItemId id : d.added) {
      const ex::RenderItem &it = extractor.item(id);
      acquireMesh(it);
      acquireSkin(id);
      itemGeom[id] = it.geometry;
    }
    for (ex::RenderItemId id : d.updatedGeometry) {
      const ex::RenderItem &it = extractor.item(id);
      auto sit = itemGeom.find(id);
      if (sit != itemGeom.end() && sit->second != it.geometry)
        releaseMesh(sit->second);  // drop the stale-contentVersion GeomId.
      acquireMesh(it);             // re-extract + re-upload under the new GeomId.
      auto skin = gpuSkins.find(id);
      if (skin != gpuSkins.end()) { destroySkin(skin->second); gpuSkins.erase(skin); }
      acquireSkin(id);
      itemGeom[id] = it.geometry;
    }
    for (ex::RenderItemId id : d.updatedSkinPose) {
      auto skin = gpuSkins.find(id);
      if (skin == gpuSkins.end()) continue;
      if (skin->second.cpuFallback) {
        GpuSkin next = uploadSkin(extractor.item(id), extractor.deformedMesh(id), true);
        destroySkin(skin->second);
        skin->second = std::move(next);
      } else {
        updateSkinPose(skin->second, extractor.item(id), ctx.geoProjection());
      }
    }
    // updatedTransform needs no GPU work: the model uniform is sourced from
    // extractor.item(id).worldTransform in the draw loop every frame.

    int w = 0, h = 0;
    glfwGetFramebufferSize(win, &w, &h);
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);

    // Camera: the bound Viewpoint's view matrix, else the view-all fit framed
    // once above. Computed every frame regardless of gpuMeshes — #39: a
    // geometry-less scene (e.g. a bare Background, as in the NIST
    // verify_lastground.x3d conformance case) still needs a real camera for
    // the background gradient pass below, which used to be skipped entirely
    // because it lived inside the "has meshes" gate.
    ex::CameraDesc cam = extractor.camera();
    const Mat4 view = (useFitCamera && !sceneBounds.empty)
                          ? fitView : cam.viewMatrix;
    const float aspect = static_cast<float>(w) / static_cast<float>(h);

    // near/far from scene bounds; sane defaults when empty.
    float zNear = 0.1f, zFar = 10000.0f;
    if (!sceneBounds.empty) {
      SFVec3f sz = sceneBounds.size();
      float diag = std::sqrt(sz.x * sz.x + sz.y * sz.y + sz.z * sz.z);
      if (diag > 0.0f) {
        zFar = diag * 100.0f;
        zNear = (std::max)(0.001f, diag * 0.001f);
      }
    }
    const Mat4 proj = perspective(cam.fieldOfView, aspect, zNear, zFar);
    // Stash for next frame's cursor-ray unprojection (feedPointerRay above).
    lastView = view;
    lastProj = proj;

    // Forced full-screen clear sanity step: if NOTHING else draws, the window
    // is still the Background color, proving context + swap work (M0 gate).
    glClearColor(clearR, clearG, clearB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // #39: paint the bound Background's full sky/ground gradient as a
    // depth-test-disabled fullscreen dome, BEFORE scene geometry. A single
    // skyColor (or no Background at all) is already exactly the flat clear
    // above, so the shader pass only runs when there's a real ramp to paint
    // (matches examples/cpu_raster's same "stay flat" fast path).
    if (bgProg) {
      if (const X3DNode *bg = ctx.boundBackground()) {
        auto skyC = x3d::runtime::geombounds::getField<std::vector<SFColor>>(
            *bg, "skyColor", {});
        auto skyA = x3d::runtime::geombounds::getField<std::vector<float>>(
            *bg, "skyAngle", {});
        auto grC = x3d::runtime::geombounds::getField<std::vector<SFColor>>(
            *bg, "groundColor", {});
        auto grA = x3d::runtime::geombounds::getField<std::vector<float>>(
            *bg, "groundAngle", {});
        if (skyC.size() > 1 || !grC.empty()) {
          glDisable(GL_DEPTH_TEST);
          glUseProgram(bgProg);
          const Mat4 invView = view.inverse();
          glUniformMatrix4fv(uBgInvView, 1, GL_FALSE, invView.m.data());
          glUniform1f(uBgInvP0, 1.0f / proj.m[0]);
          glUniform1f(uBgInvP5, 1.0f / proj.m[5]);
          auto uploadBand = [&](GLint locColor, GLint locAngle,
                                GLint locColorCount, GLint locAngleCount,
                                const std::vector<SFColor> &cols,
                                const std::vector<float> &angs) {
            float colBuf[kMaxBgBands * 3] = {0};
            float angBuf[kMaxBgBands - 1] = {0};
            const int cn = (std::min)(static_cast<int>(cols.size()), kMaxBgBands);
            const int an = (std::min)(static_cast<int>(angs.size()), kMaxBgBands - 1);
            for (int i = 0; i < cn; ++i) {
              colBuf[i * 3 + 0] = cols[i].r;
              colBuf[i * 3 + 1] = cols[i].g;
              colBuf[i * 3 + 2] = cols[i].b;
            }
            for (int i = 0; i < an; ++i) angBuf[i] = angs[i];
            if (locColor >= 0) glUniform3fv(locColor, kMaxBgBands, colBuf);
            if (locAngle >= 0) glUniform1fv(locAngle, kMaxBgBands - 1, angBuf);
            if (locColorCount >= 0) glUniform1i(locColorCount, cn);
            if (locAngleCount >= 0) glUniform1i(locAngleCount, an);
          };
          uploadBand(uBgSkyColor, uBgSkyAngle, uBgSkyColorCount,
                     uBgSkyAngleCount, skyC, skyA);
          uploadBand(uBgGroundColor, uBgGroundAngle, uBgGroundColorCount,
                     uBgGroundAngleCount, grC, grA);
          glBindVertexArray(bgVao);
          glDrawArrays(GL_TRIANGLES, 0, 3);
          glBindVertexArray(0);
          glEnable(GL_DEPTH_TEST);
        }
      }
    }

    if ((phongProg || unlitProg || pbrProg) && !gpuMeshes.empty()) {
      // Resolve the active lights to eye space (world-resolved LightDescs from
      // the extractor + the §23.4.4 NavigationInfo headlight when headlight is on).
      std::vector<ex::LightDesc> lights = extractor.lights();
      std::vector<EyeLight> eyeLights =
          buildEyeLights(lights, view, headlightOn);
      const int numLights = static_cast<int>(eyeLights.size());
      // Flatten into contiguous arrays for the uniform array upload.
      float lightDir[kMaxLights * 3] = {0};
      float lightCol[kMaxLights * 3] = {0};
      float lightAmb[kMaxLights] = {0};
      int lightType[kMaxLights] = {0}; // 0 directional, 1 point, 2 spot.
      float lightPos[kMaxLights * 3] = {0};
      float lightAtt[kMaxLights * 3] = {0};
      float lightRadius[kMaxLights] = {0};
      float lightCone[kMaxLights * 2] = {0};
      for (int i = 0; i < numLights && i < kMaxLights; ++i) {
        lightDir[i * 3 + 0] = eyeLights[i].dirEye.x;
        lightDir[i * 3 + 1] = eyeLights[i].dirEye.y;
        lightDir[i * 3 + 2] = eyeLights[i].dirEye.z;
        lightCol[i * 3 + 0] = eyeLights[i].color.r;
        lightCol[i * 3 + 1] = eyeLights[i].color.g;
        lightCol[i * 3 + 2] = eyeLights[i].color.b;
        lightAmb[i] = eyeLights[i].ambientIntensity;
        lightType[i] = eyeLights[i].positional ? (eyeLights[i].spot ? 2 : 1) : 0;
        lightPos[i * 3 + 0] = eyeLights[i].posEye.x;
        lightPos[i * 3 + 1] = eyeLights[i].posEye.y;
        lightPos[i * 3 + 2] = eyeLights[i].posEye.z;
        lightAtt[i * 3 + 0] = eyeLights[i].attenuation.x;
        lightAtt[i * 3 + 1] = eyeLights[i].attenuation.y;
        lightAtt[i * 3 + 2] = eyeLights[i].attenuation.z;
        lightRadius[i] = eyeLights[i].radius;
        lightCone[i * 2 + 0] = eyeLights[i].beamWidth;
        lightCone[i * 2 + 1] = eyeLights[i].cutOffAngle;
      }

      // §17.3.1 shadows: render each shadow-enabled scene light's depth
      // layers (shadowLayers) before the scene pass. Shapes with castShadow
      // TRUE occlude, as in the CPU host; the headlight casts none.
      int shadowBase[kMaxLights], shadowCount[kMaxLights];
      float shadowIntensity[kMaxLights];
      std::fill(std::begin(shadowBase), std::end(shadowBase), -1);
      std::fill(std::begin(shadowCount), std::end(shadowCount), 0);
      std::fill(std::begin(shadowIntensity), std::end(shadowIntensity), 0.0f);
      std::vector<float> shadowMatrices; // 16 per layer: eye -> light clip.
      const Aabb shadowBounds = extractor.sceneWorldBounds();
      if (shadowProg && !shadowBounds.empty) {
        const SFVec3f c{(shadowBounds.min.x + shadowBounds.max.x) * 0.5f,
                        (shadowBounds.min.y + shadowBounds.max.y) * 0.5f,
                        (shadowBounds.min.z + shadowBounds.max.z) * 0.5f};
        const SFVec3f sz = shadowBounds.size();
        const float radius =
            (std::max)(0.5f * std::sqrt(v3dot(sz, sz)), 1e-3f) * 1.01f;
        const int sceneLights = numLights - (headlightOn ? 1 : 0);
        std::vector<Mat4> layers;
        for (int i = 0; i < sceneLights && i < static_cast<int>(lights.size()); ++i) {
          if (!lights[i].shadows) continue;
          std::vector<Mat4> mine = shadowLayers(lights[i], c, radius);
          if (layers.size() + mine.size() > kMaxShadowLayers) break;
          shadowBase[i] = static_cast<int>(layers.size());
          shadowCount[i] = static_cast<int>(mine.size());
          shadowIntensity[i] = std::clamp(lights[i].shadowIntensity, 0.0f, 1.0f);
          layers.insert(layers.end(), mine.begin(), mine.end());
        }
        if (!layers.empty()) {
          const int n = static_cast<int>(layers.size());
          if (n > shadowLayersAllocated) {
            if (shadowTex) glDeleteTextures(1, &shadowTex);
            glGenTextures(1, &shadowTex);
            glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTex);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, kShadowSize,
                         kShadowSize, n, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE,
                            GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            shadowLayersAllocated = n;
          }
          if (!shadowFbo) glGenFramebuffers(1, &shadowFbo);
          GLint previousFbo = 0;
          glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
          glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
          glDrawBuffer(GL_NONE);
          glReadBuffer(GL_NONE);
          glViewport(0, 0, kShadowSize, kShadowSize);
          glUseProgram(shadowProg);
          glEnable(GL_DEPTH_TEST);
          glDepthMask(GL_TRUE);
          glDisable(GL_BLEND);
          glDisable(GL_CULL_FACE);
          glEnable(GL_POLYGON_OFFSET_FILL);
          glPolygonOffset(2.0f, 4.0f);
          for (int ci = 0; ci < static_cast<int>(ex::ClipPlaneList::kMaxClipPlanes); ++ci)
            glDisable(GL_CLIP_DISTANCE0 + ci);
          glUniform1i(glGetUniformLocation(shadowProg, "uNumClipPlanes"), 0);
          glUniform1i(glGetUniformLocation(shadowProg, "uInfluences"), 6);
          glUniform1i(glGetUniformLocation(shadowProg, "uPalette"), 7);
          glUniform1i(glGetUniformLocation(shadowProg, "uTexture"), 0);
          const GLint locModel = glGetUniformLocation(shadowProg, "uModel");
          const GLint locView = glGetUniformLocation(shadowProg, "uView");
          const GLint locProj = glGetUniformLocation(shadowProg, "uProjection");
          const GLint locSkin = glGetUniformLocation(shadowProg, "uSkinEnabled");
          const Mat4 identity = Mat4::identity();
          const Mat4 invView = view.inverse();
          for (int layer = 0; layer < n; ++layer) {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTex, 0,
                                      layer);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUniformMatrix4fv(locView, 1, GL_FALSE, identity.m.data());
            glUniformMatrix4fv(locProj, 1, GL_FALSE, layers[layer].m.data());
            const Mat4 eyeToLight = layers[layer] * invView;
            shadowMatrices.insert(shadowMatrices.end(), eyeToLight.m.begin(),
                                  eyeToLight.m.end());
            for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
              const ex::RenderItem &item = extractor.item(id);
              if (!item.castShadow) continue;
              auto mit = gpuMeshes.find(item.geometry);
              if (mit == gpuMeshes.end()) continue;
              auto sit = gpuSkins.find(id);
              const GpuMesh &mesh = sit == gpuSkins.end() ? mit->second : sit->second.mesh;
              if (mesh.topology != ex::Topology::Triangles) continue;
              const bool skinned = sit != gpuSkins.end() && !sit->second.cpuFallback;
              if (locSkin >= 0) glUniform1i(locSkin, skinned ? 1 : 0);
              if (skinned) {
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_BUFFER, sit->second.influences);
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_BUFFER, sit->second.palette);
                glActiveTexture(GL_TEXTURE0);
              }
              glUniformMatrix4fv(locModel, 1, GL_FALSE, item.worldTransform.m.data());
              glBindVertexArray(mesh.vao);
              glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            }
          }
          glDisable(GL_POLYGON_OFFSET_FILL);
          glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
          glViewport(0, 0, w, h);
          glUseProgram(0);
          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTex);
          glActiveTexture(GL_TEXTURE0);
        }
      }

      // Helper: bind a texture on the given unit; fall back to whiteTex if tex==0.
      // Keeps every sampler unit complete (no "no base level" GL warnings).
      auto bindTex = [&](int unit, GLint samplerLoc, GLint hasLoc, GLuint tex) {
        glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
        glBindTexture(GL_TEXTURE_2D, tex ? tex : whiteTex);
        if (samplerLoc >= 0) glUniform1i(samplerLoc, unit);
        if (hasLoc >= 0) glUniform1i(hasLoc, tex ? 1 : 0);
      };

      // §18.4.3 MultiTexture (multitexture.glsl): bind each base-colour stage
      // on unit 8+i (6/7 hold the skin buffers) with its mode/source/function/
      // factor/channel. Sets
      // uNumStages 0 (the plain single-texture path) when no combiner is
      // needed, the mesh has no UVs, or the program lacks the combiner.
      auto uploadStages = [&](GLuint program, const ex::RenderItem &item,
                              const GpuMesh &mesh, bool srgb) {
        const GLint locNum = glGetUniformLocation(program, "uNumStages");
        if (locNum < 0) return;
        const ex::MaterialDesc &m = item.material;
        std::vector<const ex::TextureRef *> stages = baseStageRefs(m);
        const bool anyCube = std::any_of(stages.begin(), stages.end(), [](auto *t) {
          return t->source == ex::TextureRef::Source::Cube;
        });
        if ((!mesh.hasTexcoords && !anyCube) || mesh.isGlyphMesh ||
            !needsStageCombiner(stages))
          stages.clear();
        constexpr int kMaxStages = 4; // multitexture.glsl; extra stages are ignored.
        const int n = std::min(static_cast<int>(stages.size()), kMaxStages);
        glUniform1i(locNum, n);
        // texgen.glsl evaluates the material's first generator only.
        const ex::TextureRef *generated = nullptr;
        for (const ex::TextureRef &t : m.textures)
          if (t.hasTexCoordGen) { generated = &t; break; }
        const bool genActive = generated && texCoordGenModeUniform(m) != 0;
        GLuint cubeTex = 0;
        for (int i = 0; i < n; ++i) {
          const ex::TextureRef &t = *stages[i];
          auto loc = [&](const char *name) {
            return glGetUniformLocation(
                program, (std::string(name) + "[" + std::to_string(i) + "]").c_str());
          };
          const bool isCube = t.source == ex::TextureRef::Source::Cube;
          bindTex(8 + i, loc("uStageTex"), loc("uStageHasTex"),
                  isCube ? 0 : resolveTexRef(&t, texCache, assetResolver, srgb, &movieState));
          if (isCube && !cubeTex) { // one cube map per material (unit 12).
            cubeTex = resolveCubeTex(t, texCache, assetResolver, srgb);
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D_ARRAY, cubeTex);
          }
          // A pending cube samples white, like any unresolved stage.
          glUniform1i(loc("uStageIsCube"), isCube && cubeTex ? 1 : 0);
          const auto comma = t.multiMode.find(',');
          const int rgbMode = multiTextureModeCode(t.multiMode.substr(0, comma));
          const int alphaMode = comma == std::string::npos
                                    ? rgbMode
                                    : multiTextureModeCode(t.multiMode.substr(comma + 1));
          glUniform2i(loc("uStageMode"), rgbMode, alphaMode);
          glUniform1i(loc("uStageSource"), t.multiSource == "DIFFUSE"    ? 1
                                            : t.multiSource == "SPECULAR" ? 2
                                            : t.multiSource == "FACTOR"   ? 3
                                                                          : 0);
          glUniform1i(loc("uStageFunction"), t.multiFunction == "COMPLEMENT"       ? 1
                                              : t.multiFunction == "ALPHAREPLICATE" ? 2
                                                                                    : 0);
          glUniform4f(loc("uStageFactor"), t.multiColor.r, t.multiColor.g,
                      t.multiColor.b, t.multiAlpha);
          glUniform1i(loc("uStageChannel"),
                      genActive && &t == generated ? -1 : std::max(t.channel, 0));
        }
        glActiveTexture(GL_TEXTURE0);
      };

      // TXF-2 (texgen.glsl): the generator's mode, parameters and the
      // TextureTransform applied to generated coordinates.
      auto uploadTexGen = [&](GLuint program, const ex::MaterialDesc &m) {
        const GLint locMode = glGetUniformLocation(program, "uTexCoordGenMode");
        if (locMode < 0) return;
        glUniform1i(locMode, texCoordGenModeUniform(m));
        const ex::TextureRef *t = texCoordGenRef(m);
        if (!t) return;
        const auto &param = t->texCoordGen.parameter;
        const int count = static_cast<int>(std::min<std::size_t>(param.size(), 6));
        if (count > 0)
          glUniform1fv(glGetUniformLocation(program, "uTexGenParam"), count, param.data());
        glUniform1i(glGetUniformLocation(program, "uTexGenParamCount"), count);
        const std::array<float, 9> tt = ex::makeTextureTransform3x3(t->generatedTransform);
        glUniformMatrix3fv(glGetUniformLocation(program, "uTexGenTransform"), 1,
                           GL_TRUE, tt.data()); // row-major
      };

      // Helper: upload standard eye-space lights to a program (already bound).
      auto uploadLights = [&](GLuint program, GLint locNum, GLint locDir, GLint locCol,
                              GLint locAmb) {
        if (locNum >= 0) glUniform1i(locNum, numLights);
        if (numLights > 0) {
          if (locDir >= 0) glUniform3fv(locDir, numLights, lightDir);
          if (locCol >= 0) glUniform3fv(locCol, numLights, lightCol);
          if (locAmb >= 0) glUniform1fv(locAmb, numLights, lightAmb);
          glUniform1iv(glGetUniformLocation(program, "uLightType"), numLights, lightType);
          glUniform3fv(glGetUniformLocation(program, "uLightPosEye"), numLights, lightPos);
          glUniform3fv(glGetUniformLocation(program, "uLightAttenuation"), numLights, lightAtt);
          glUniform1fv(glGetUniformLocation(program, "uLightRadius"), numLights, lightRadius);
          glUniform2fv(glGetUniformLocation(program, "uLightCone"), numLights, lightCone);
        }
        // shadow.glsl: every slot is written, so unused ones read base -1.
        const GLint locBase = glGetUniformLocation(program, "uLightShadowBase");
        if (locBase < 0) return;
        glUniform1iv(locBase, kMaxLights, shadowBase);
        glUniform1iv(glGetUniformLocation(program, "uLightShadowCount"), kMaxLights, shadowCount);
        glUniform1fv(glGetUniformLocation(program, "uLightShadowIntensity"), kMaxLights,
                     shadowIntensity);
        if (!shadowMatrices.empty())
          glUniformMatrix4fv(glGetUniformLocation(program, "uShadowMatrix"),
                             static_cast<GLsizei>(shadowMatrices.size() / 16), GL_FALSE,
                             shadowMatrices.data());
      };

      // §24.4.2: the bound Fog, world-scaled by the extractor. visibilityRange
      // 0 disables fog (the shaders no-op). §24.4.3: an item inside a
      // LocalFog's grouping scope is fogged by that LocalFog instead (the
      // extractor tags the nearest enabled one), so fog is uploaded per draw.
      const ex::FogDesc fogDesc = extractor.fog();
      auto uploadFog = [&](const ex::RenderItem &item, GLint locColor, GLint locType,
                           GLint locRange) {
        SFColor color = fogDesc.color;
        ex::FogDesc::Type type = fogDesc.fogType;
        float range = fogDesc.visibilityRange;
        const auto &locals = extractor.snapshotLocalFogs();
        if (item.localFog >= 0 && item.localFog < static_cast<int>(locals.size())) {
          const ex::LocalFogDesc &lf = locals[item.localFog];
          color = lf.color;
          type = lf.fogType;
          range = lf.visibilityRange;
        }
        if (locColor >= 0) glUniform3f(locColor, color.r, color.g, color.b);
        if (locType >= 0)
          glUniform1i(locType, type == ex::FogDesc::Type::Exponential ? 1 : 0);
        if (locRange >= 0) glUniform1f(locRange, range);
      };

      // Helper: per-draw culling from mesh winding/solidity.
      auto applyCull = [&](const GpuMesh &g, const x3d::runtime::Mat4 &model) {
        if (g.solid) {
          glEnable(GL_CULL_FACE);
          glCullFace(GL_BACK);
          glFrontFace(poc::frontFaceCCW(g.ccw, model) ? GL_CCW : GL_CW);
        } else {
          glDisable(GL_CULL_FACE);
        }
      };

      // Track which program is currently bound so we only re-upload the shared
      // view/proj uniforms (and re-glUseProgram) when the path actually changes.
      GLuint boundProg = 0;

      // ------------------------------------------------------------------
      // Phase 5.1: Per-program dispatch selector.
      //   UNLIT  — topology!=Triangles OR !hasNormals OR model==Unlit.
      //   PHONG  — model == Phong (or Physical fallback if pbrProg unavailable).
      //   PBR    — model == Physical AND pbrProg compiled.
      //   AUTHOR — it.shaderProgram.has_value() (overrides material model).
      // ------------------------------------------------------------------

      // Draw one render item under the current GL/pass state. Factored out of the
      // loop so the B7 opaque pass and the back-to-front BLEND pass share exactly
      // one code path; the only per-pass difference is GL_BLEND + depth-mask state
      // set by the caller around each pass.
      auto drawItem = [&](ex::RenderItemId id) {
        const ex::RenderItem &it = extractor.item(id);
        auto mit = gpuMeshes.find(it.geometry);
        if (mit == gpuMeshes.end()) return;
        auto skinIt = gpuSkins.find(id);
        const GpuMesh &g = skinIt == gpuSkins.end() ? mit->second : skinIt->second.mesh;
        const ex::MaterialDesc &mat = it.material;
        // REQ-CLIP (§11.4.1): the extractor carries WORLD planes; the shaders
        // and the vocabulary's `clipPlane` uniform take them in eye space.
        const int numClips = static_cast<int>(it.clipPlanes.size);
        std::array<float, 4 * ex::ClipPlaneList::kMaxClipPlanes> clipEye{};
        for (int ci = 0; ci < numClips; ++ci) {
          const SFVec4f q = x3d::runtime::transformPlane(view, it.clipPlanes.items[ci].planeWorld);
          clipEye[4 * ci] = q.x; clipEye[4 * ci + 1] = q.y;
          clipEye[4 * ci + 2] = q.z; clipEye[4 * ci + 3] = q.w;
        }
        // FillProperties covers polygonal areas. With neither component enabled,
        // issue no draw so neither color nor depth is written, including on the
        // author-shader path.
        if (g.topology == ex::Topology::Triangles &&
            !mat.fill.filled && !mat.fill.hatched) return;
        SFColorRGBA c = mat.toRGBA();
        if (g.topology != ex::Topology::Triangles && !g.hasNormals)
          c = mat.unlitGeometryRGBA();

        // ----------------------------------------------------------------
        // Determine which shader path to take.
        // ----------------------------------------------------------------
        const bool forceUnlit = !g.hasNormals
                                 || (mat.model == ex::MaterialModel::Unlit);
        const bool hasAuthor = it.shaderProgram.has_value()
                                && it.shaderProgram->isValid;
        const bool wantPbr   = !forceUnlit && !hasAuthor
                                && (mat.model == ex::MaterialModel::Physical)
                                && pbrProg;
        const bool wantPhong = !forceUnlit && !hasAuthor && !wantPbr && phongProg;
        const auto uploadLitPointSize = [&](GLuint program) {
          if (g.topology != ex::Topology::Points) return;
          const auto set1 = [&](const char *name, float value) {
            const GLint loc = glGetUniformLocation(program, name);
            if (loc >= 0) glUniform1f(loc, value);
          };
          set1("uPointSizeScale", mat.point.pointSizeScaleFactor);
          set1("uPointSizeMin", mat.point.pointSizeMinValue);
          set1("uPointSizeMax", mat.point.pointSizeMaxValue);
          const GLint loc = glGetUniformLocation(program, "uPointAttenuation");
          if (loc >= 0)
            glUniform3f(loc, mat.point.attenuation.x, mat.point.attenuation.y,
                        mat.point.attenuation.z);
        };

        // ----------------------------------------------------------------
        // PATH 1: UNLIT — normal-less geometry / UnlitMaterial.
        // ----------------------------------------------------------------
        if ((forceUnlit || (!wantPbr && !wantPhong && !hasAuthor)) && unlitProg) {
          if (boundProg != unlitProg) {
            glUseProgram(unlitProg);
            glUniformMatrix4fv(uUnlitView, 1, GL_FALSE, view.m.data());
            glUniformMatrix4fv(uUnlitProj, 1, GL_FALSE, proj.m.data());
            boundProg = unlitProg;
          }
          glUniformMatrix4fv(uUnlitModel, 1, GL_FALSE, it.worldTransform.m.data());
          uploadFog(it, uUnlitFogColor, uUnlitFogType, uUnlitFogRange);
          glUniform4f(uUnlitBaseColor, c.r, c.g, c.b, c.a);
          glUniform1i(uUnlitHasColors, g.hasColors ? 1 : 0);
          // SEAM-LINEPOINT: §12.4.8 PointProperties → gl_PointSize (unlit.vert).
          if (uUnlitPointScale >= 0) glUniform1f(uUnlitPointScale, mat.point.pointSizeScaleFactor);
          if (uUnlitPointAtten >= 0)
            glUniform3f(uUnlitPointAtten, mat.point.attenuation.x,
                        mat.point.attenuation.y, mat.point.attenuation.z);
          if (uUnlitPointMin >= 0) glUniform1f(uUnlitPointMin, mat.point.pointSizeMinValue);
          if (uUnlitPointMax >= 0) glUniform1f(uUnlitPointMax, mat.point.pointSizeMaxValue);
          // A textured Appearance with NO Material is Unlit with the image on the
          // Emissive slot (§12.2.5); also covers UnlitMaterial.emissiveTexture and
          // any Diffuse/BaseColor texture that lands on the unlit path. srgb=false:
          // raw passthrough to match unlit's no-gamma direct color output.
          if (g.hasTexcoords && !g.isGlyphMesh) {
            GLuint ut = resolveTexRef(
                findTexSlot(mat, {ex::TextureRef::Slot::Emissive,
                                  ex::TextureRef::Slot::BaseColor,
                                  ex::TextureRef::Slot::Diffuse}),
                texCache, assetResolver, /*srgb=*/false, &movieState);
            bindTex(0, uUnlitTexture, uUnlitHasTexture, ut);
          } else {
            bindTex(0, uUnlitTexture, uUnlitHasTexture, 0);
          }
          uploadStages(unlitProg, it, g, /*srgb=*/false);
          uploadTexGen(unlitProg, mat);
          glDisable(GL_CULL_FACE); // lines/points/normal-less always double-sided.

        // ----------------------------------------------------------------
        // PATH 2: PHONG (Blinn-Phong + textures + normal-map).
        // ----------------------------------------------------------------
        } else if (wantPhong) {
          if (boundProg != phongProg) {
            glUseProgram(phongProg);
            glUniformMatrix4fv(uView, 1, GL_FALSE, view.m.data());
            glUniformMatrix4fv(uProj, 1, GL_FALSE, proj.m.data());
            uploadLights(phongProg, uNumLights, uLightDirEye, uLightColor, uLightAmbient);
            boundProg = phongProg;
          }
          // Per-path model + eye-space normal matrix.
          glUniformMatrix4fv(uModel, 1, GL_FALSE, it.worldTransform.m.data());
          uploadFog(it, uFogColor, uFogType, uFogRange);
          std::array<float, 9> nrm = poc::normalMatrix3(view, it.worldTransform);
          glUniformMatrix3fv(uNormalMat, 1, GL_FALSE, nrm.data());
          uploadLitPointSize(phongProg);

          // Material: diffuse(rgb)+alpha, emissive, ambient.
          glUniform4f(uDiffuse, c.r, c.g, c.b, c.a);
          glUniform3f(uEmissive, mat.emissive.r, mat.emissive.g, mat.emissive.b);
          const float ai = mat.phong.ambientIntensity;
          // §17: ambientParameter = ambientIntensity × diffuseParameter; the
          // shader multiplies by the textured/vertex-coloured base itself.
          glUniform3f(uAmbientColor, ai, ai, ai);
          glUniform1i(uHasColors, g.hasColors ? 1 : 0);
          // TXF-2: §18.4.8 TextureCoordinateGenerator (texgen.glsl).
          uploadTexGen(phongProg, mat);

          // Blinn-Phong specular + alpha-mask.
          glUniform3f(uSpecular, mat.phong.specular.r, mat.phong.specular.g,
                      mat.phong.specular.b);
          glUniform1f(uShininess, mat.phong.shininess);
          glUniform1i(uAlphaMode, static_cast<int>(mat.alphaMode));
          glUniform1f(uAlphaCutoff, mat.alphaCutoff);
          // ADR-0027: Phong shades in display space — no sRGB output encode.
          if (uGammaOutput >= 0) glUniform1i(uGammaOutput, 0);
          // Normal scale.
          if (uNormalScale >= 0) glUniform1f(uNormalScale, mat.normalScale);

          if (g.isGlyphMesh && glyphAtlasTex) {
            // T-TEXT: texcoords index the font coverage atlas; bind it on unit 0
            // and flag the shader to read .r as alpha-tested glyph coverage with
            // the material color. No material texture slots apply to glyph meshes.
            bindTex(0, uTexture, uHasTexture, glyphAtlasTex);
            if (uGlyphAtlas >= 0) glUniform1i(uGlyphAtlas, 1);
            bindTex(1, uNormalTex,   uHasNormalTex,   0);
            bindTex(2, uEmissiveTex, uHasEmissiveTex, 0);
            bindTex(3, uSpecularTex, uHasSpecularTex, 0);
          } else if (g.hasTexcoords) {
            if (uGlyphAtlas >= 0) glUniform1i(uGlyphAtlas, 0);
            // Unit 0: diffuse/base color — display space for Phong (ADR-0027):
            // uploaded as GL_RGBA so the driver does not decode it.
            GLuint t0 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Diffuse,
                                                         ex::TextureRef::Slot::BaseColor}),
                                      texCache, assetResolver, /*srgb=*/false, &movieState);
            bindTex(0, uTexture, uHasTexture, t0);
            // Unit 1: normal map (linear — data texture, no sRGB decode).
            GLuint t1 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Normal}),
                                      texCache, assetResolver, /*srgb=*/false);
            bindTex(1, uNormalTex, uHasNormalTex, t1);
            // Unit 2: emissive texture (display space for Phong).
            GLuint t2 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Emissive}),
                                      texCache, assetResolver, /*srgb=*/false, &movieState);
            bindTex(2, uEmissiveTex, uHasEmissiveTex, t2);
            // Unit 3: specular texture (display space for Phong).
            GLuint t3 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Specular}),
                                      texCache, assetResolver, /*srgb=*/false);
            bindTex(3, uSpecularTex, uHasSpecularTex, t3);
          } else {
            if (uGlyphAtlas >= 0) glUniform1i(uGlyphAtlas, 0);
            bindTex(0, uTexture,     uHasTexture,     0);
            bindTex(1, uNormalTex,   uHasNormalTex,   0);
            bindTex(2, uEmissiveTex, uHasEmissiveTex, 0);
            bindTex(3, uSpecularTex, uHasSpecularTex, 0);
          }
          uploadStages(phongProg, it, g, /*srgb=*/false);
          applyCull(g, it.worldTransform);

        // ----------------------------------------------------------------
        // PATH 3: PBR — metallic-roughness analytic BRDF (no IBL: Phase 4 deferred).
        // ----------------------------------------------------------------
        } else if (wantPbr) {
          if (boundProg != pbrProg) {
            glUseProgram(pbrProg);
            glUniformMatrix4fv(uPbrView, 1, GL_FALSE, view.m.data());
            glUniformMatrix4fv(uPbrProj, 1, GL_FALSE, proj.m.data());
            uploadLights(pbrProg, uPbrNumLights, uPbrLightDirEye, uPbrLightColor, uPbrLightAmbient);
            boundProg = pbrProg;
          }
          glUniformMatrix4fv(uPbrModel, 1, GL_FALSE, it.worldTransform.m.data());
          uploadFog(it, uPbrFogColor, uPbrFogType, uPbrFogRange);
          std::array<float, 9> nrm = poc::normalMatrix3(view, it.worldTransform);
          glUniformMatrix3fv(uPbrNormalMat, 1, GL_FALSE, nrm.data());
          uploadLitPointSize(pbrProg);

          // PBR material params.
          const auto &ph = mat.physical;
          if (uPbrBaseColor >= 0)
            glUniform4f(uPbrBaseColor, ph.baseColor.r, ph.baseColor.g,
                        ph.baseColor.b, 1.0f - mat.transparency);
          if (uPbrMetallic  >= 0) glUniform1f(uPbrMetallic,  ph.metallic);
          if (uPbrRoughness >= 0) glUniform1f(uPbrRoughness, ph.roughness);
          if (uPbrEmissive  >= 0)
            glUniform3f(uPbrEmissive,
                        mat.emissive.r, mat.emissive.g, mat.emissive.b);
          if (uPbrAlphaMode   >= 0) glUniform1i(uPbrAlphaMode, static_cast<int>(mat.alphaMode));
          if (uPbrAlphaCutoff >= 0) glUniform1f(uPbrAlphaCutoff, mat.alphaCutoff);
          if (uPbrHasColors   >= 0) glUniform1i(uPbrHasColors, g.hasColors ? 1 : 0);
          uploadTexGen(pbrProg, mat);
          if (uPbrNormalScale >= 0) glUniform1f(uPbrNormalScale, mat.normalScale);
          if (uPbrOcclusionStrength >= 0)
            glUniform1f(uPbrOcclusionStrength, ph.occlusionStrength);

          // UsdPreviewSurface fidelity defaults (host contract — see uUsdIor
          // comment above). X3D PhysicalMaterial carries none of these, so we
          // always bind the spec fallbacks: metallic workflow, ior=1.5 (->
          // dielectric F0=0.04, matching pbr.frag's hardcoded 0.04), no
          // clearcoat, opacityMode "transparent" (matches pbr.frag's plain
          // alpha-out behavior). No-ops on pbr.frag (locations are -1).
          if (uUsdUseSpecularWorkflow >= 0) glUniform1i(uUsdUseSpecularWorkflow, 0);
          if (uUsdSpecularColor       >= 0) glUniform3f(uUsdSpecularColor, 0.0f, 0.0f, 0.0f);
          if (uUsdIor                 >= 0) glUniform1f(uUsdIor, 1.5f);
          if (uUsdClearcoat           >= 0) glUniform1f(uUsdClearcoat, 0.0f);
          if (uUsdClearcoatRoughness  >= 0) glUniform1f(uUsdClearcoatRoughness, 0.01f);
          if (uUsdOpacityMode         >= 0) glUniform1i(uUsdOpacityMode, 0);
          if (uUsdOpacityThreshold    >= 0) glUniform1f(uUsdOpacityThreshold, mat.alphaCutoff);

          if (g.hasTexcoords) {
            // Unit 0: base color (sRGB; uploaded as GL_SRGB8_ALPHA8 so the
            // driver linearises on sample; pbr.frag reads the linearised value
            // directly — no double-decode).
            GLuint t0 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::BaseColor}),
                                      texCache, assetResolver, /*srgb=*/true, &movieState);
            bindTex(0, uPbrBaseColorTex, uPbrHasBaseColorTex, t0);
            // Unit 1: normal map (linear — data texture, no sRGB decode).
            GLuint t1 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Normal}),
                                      texCache, assetResolver, /*srgb=*/false);
            bindTex(1, uPbrNormalTex, uPbrHasNormalTex, t1);
            // Unit 2: emissive (sRGB).
            GLuint t2 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Emissive}),
                                      texCache, assetResolver, /*srgb=*/true, &movieState);
            bindTex(2, uPbrEmissiveTex, uPbrHasEmissiveTex, t2);
            // Unit 3: ORM metallic-roughness (linear — no sRGB decode).
            GLuint t3 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::MetallicRoughness}),
                                      texCache, assetResolver, /*srgb=*/false);
            bindTex(3, uPbrMRTex, uPbrHasMRTex, t3);
            // Unit 4: occlusion (linear).
            GLuint t4 = resolveTexRef(findTexSlot(mat, {ex::TextureRef::Slot::Occlusion}),
                                      texCache, assetResolver, /*srgb=*/false);
            bindTex(4, uPbrOcclusionTex, uPbrHasOcclusionTex, t4);
          } else {
            bindTex(0, uPbrBaseColorTex,   uPbrHasBaseColorTex,   0);
            bindTex(1, uPbrNormalTex,      uPbrHasNormalTex,      0);
            bindTex(2, uPbrEmissiveTex,    uPbrHasEmissiveTex,    0);
            bindTex(3, uPbrMRTex,          uPbrHasMRTex,          0);
            bindTex(4, uPbrOcclusionTex,   uPbrHasOcclusionTex,   0);
          }
          uploadStages(pbrProg, it, g, /*srgb=*/true);
          applyCull(g, it.worldTransform);

        // ----------------------------------------------------------------
        // PATH 4: AUTHOR-SHADER (ComposedShader/ProgramShader via
        // ShaderBindingPlan). The SDK selects each Appearance's shader
        // (§31.2.2.3) against makeGlShaderValidator, so a program reaching here
        // compiled in this context; an invalid one was skipped for the next
        // shader or the material. A 0 entry in authorProgCache (a link that
        // still failed) skips the draw.
        } else if (hasAuthor) {
          // Build a cache key from the combined stage sources.
          std::string cacheKey;
          for (const auto &stage : it.shaderProgram->stages)
            cacheKey += stage.source + "\n---\n";

          auto ait = authorProgCache.find(cacheKey);
          if (ait == authorProgCache.end()) {
            // Compile the author stages.
            GLuint avs = 0, afs = 0;
            for (const auto &stage : it.shaderProgram->stages) {
              if (stage.stage == ex::ShaderStageDesc::Stage::Vertex)
                avs = compileShader(GL_VERTEX_SHADER, stage.source, "author.vert");
              else if (stage.stage == ex::ShaderStageDesc::Stage::Fragment)
                afs = compileShader(GL_FRAGMENT_SHADER, stage.source, "author.frag");
            }
            GLuint ap = (avs && afs) ? linkProgram(avs, afs) : 0;
            if (avs) glDeleteShader(avs);
            if (afs) glDeleteShader(afs);
            authorProgCache[cacheKey] = ap;
            ait = authorProgCache.find(cacheKey);
          }

          GLuint ap = ait->second;
          if (!ap) {
            // Author shader failed to link; draw nothing for this item.
            return;
          }

          if (boundProg != ap) {
            glUseProgram(ap);
            boundProg = ap;
          }

          // Collect declared uniforms via glGetActiveUniform + buildBindingPlan.
          GLint numUniforms = 0;
          glGetProgramiv(ap, GL_ACTIVE_UNIFORMS, &numUniforms);
          std::vector<std::pair<std::string, int>> declaredUniforms;
          for (GLint ui = 0; ui < numUniforms; ++ui) {
            char nameBuf[256] = {};
            GLsizei len = 0; GLint usize = 0; GLenum utype = 0;
            glGetActiveUniform(ap, static_cast<GLuint>(ui), sizeof(nameBuf),
                               &len, &usize, &utype, nameBuf);
            int loc = glGetUniformLocation(ap, nameBuf);
            if (loc >= 0)
              declaredUniforms.emplace_back(std::string(nameBuf), loc);
          }

          ex::ShaderBindingPlan plan = ex::buildBindingPlan(
              declaredUniforms, *it.shaderProgram);

          // Log diagnostics (once, on first use — keyed by ap).
          static std::unordered_map<GLuint, bool> diagLogged;
          if (!diagLogged[ap]) {
            diagLogged[ap] = true;
            for (const std::string &d : plan.diagnostics)
              std::fprintf(stderr, "[poc] author-shader diag: %s\n", d.c_str());
          }

          // Upload each vocab entry the author declared.
          Mat4 viewModel = view * it.worldTransform;
          std::array<float, 9> nrm = poc::normalMatrix3(view, it.worldTransform);
          for (const auto &e : plan.entries) {
            if (e.unrecognized) continue; // already diagnosed above
            const int loc = e.location;
            using S = ex::vocab::UniformSource;
            switch (e.source) {
              case S::ModelViewMatrix: {
                Mat4 mv = viewModel;
                glUniformMatrix4fv(loc, 1, GL_FALSE, mv.m.data()); break; }
              case S::ProjectionMatrix:
                glUniformMatrix4fv(loc, 1, GL_FALSE, proj.m.data()); break;
              case S::NormalMatrix:
                glUniformMatrix3fv(loc, 1, GL_FALSE, nrm.data()); break;
              case S::ModelMatrix:
                glUniformMatrix4fv(loc, 1, GL_FALSE,
                                   it.worldTransform.m.data()); break;
              case S::ViewMatrix:
                glUniformMatrix4fv(loc, 1, GL_FALSE, view.m.data()); break;
              case S::NumLights:
                glUniform1i(loc, numLights); break;
              case S::LightDirection:
                if (numLights > 0) glUniform3fv(loc, numLights, lightDir);
                break;
              case S::LightColor:
                if (numLights > 0) glUniform3fv(loc, numLights, lightCol);
                break;
              case S::DiffuseColor:
                glUniform3f(loc, mat.phong.diffuse.r, mat.phong.diffuse.g,
                             mat.phong.diffuse.b); break;
              case S::EmissiveColor:
                glUniform3f(loc, mat.emissive.r, mat.emissive.g,
                             mat.emissive.b); break;
              case S::BaseColor:
                glUniform3f(loc, mat.physical.baseColor.r,
                             mat.physical.baseColor.g,
                             mat.physical.baseColor.b); break;
              case S::Metallic:
                glUniform1f(loc, mat.physical.metallic); break;
              case S::Roughness:
                glUniform1f(loc, mat.physical.roughness); break;
              case S::Shininess:
                glUniform1f(loc, mat.phong.shininess); break;
              case S::AmbientIntensity:
                glUniform1f(loc, mat.phong.ambientIntensity); break;
              case S::Transparency:
                glUniform1f(loc, mat.transparency); break;
              case S::AlphaMode:
                glUniform1i(loc, static_cast<int>(mat.alphaMode)); break;
              case S::AlphaCutoff:
                glUniform1f(loc, mat.alphaCutoff); break;
              case S::NumClipPlanes:
                glUniform1i(loc, numClips); break;
              case S::ClipPlane:
                if (numClips > 0) glUniform4fv(loc, numClips, clipEye.data());
                break;
              default: break; // EnvDiffuse/IBL etc. left unbound (Phase 4 deferred)
            }
            // Author <field> values.
            if (e.isAuthorField) {
              for (const auto &f : it.shaderProgram->fields) {
                if (f.name != e.declaredName) continue;
                // Every SF value the descriptor carries (SFNode/MF fields
                // arrive empty and stay unbound).
                std::visit([&](const auto &x) {
                  using T = std::decay_t<decltype(x)>;
                  if constexpr (std::is_same_v<T, float>) glUniform1f(loc, x);
                  else if constexpr (std::is_same_v<T, int>) glUniform1i(loc, x);
                  else if constexpr (std::is_same_v<T, bool>) glUniform1i(loc, x ? 1 : 0);
                  else if constexpr (std::is_same_v<T, SFColor>) glUniform3f(loc, x.r, x.g, x.b);
                  else if constexpr (std::is_same_v<T, SFColorRGBA>) glUniform4f(loc, x.r, x.g, x.b, x.a);
                  else if constexpr (std::is_same_v<T, SFVec2f>) glUniform2f(loc, x.x, x.y);
                  else if constexpr (std::is_same_v<T, SFVec3f>) glUniform3f(loc, x.x, x.y, x.z);
                  else if constexpr (std::is_same_v<T, SFVec4f>) glUniform4f(loc, x.x, x.y, x.z, x.w);
                  else if constexpr (std::is_same_v<T, SFMatrix3f>) glUniformMatrix3fv(loc, 1, GL_FALSE, &x.matrix[0][0]);
                  else if constexpr (std::is_same_v<T, SFMatrix4f>) glUniformMatrix4fv(loc, 1, GL_FALSE, &x.matrix[0][0]);
                }, f.value.value);
                break;
              }
            }
          }
          applyCull(g, it.worldTransform);
        }

        // The built-in fragment programs share the same FillProperties uniform
        // contract. Always upload per draw: adjacent items can use different
        // appearances while sharing a program. Lines and points stay filled.
        if (boundProg != 0 &&
            (boundProg == unlitProg || boundProg == phongProg || boundProg == pbrProg)) {
          const int mode = g.topology == ex::Topology::Triangles
              ? (mat.fill.filled ? 1 : 0) | (mat.fill.hatched ? 2 : 0)
              : 1;
          glUniform1i(glGetUniformLocation(boundProg, "uFillMode"), mode);
          glUniform1i(glGetUniformLocation(boundProg, "uHatchStyle"), mat.fill.hatchStyle);
          glUniform3f(glGetUniformLocation(boundProg, "uHatchColor"),
                      mat.fill.hatchColor.r, mat.fill.hatchColor.g,
                      mat.fill.hatchColor.b);
        }

        // §12.4.6: line width applies on both lit and unlit paths.
        const float lineWidth = g.topology == ex::Topology::Lines &&
                                        mat.line.applied &&
                                        mat.line.linewidthScaleFactor > 0.0f
                                    ? mat.line.linewidthScaleFactor : 1.0f;
        glLineWidth(lineWidth);
        // B4: branch the draw-call primitive on topology.
        GLenum mode = (g.topology == ex::Topology::Lines)    ? GL_LINES
                      : (g.topology == ex::Topology::Points) ? GL_POINTS
                                                             : GL_TRIANGLES;
        GLint activeProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &activeProgram);
        GLint skinLoc = glGetUniformLocation(static_cast<GLuint>(activeProgram), "uSkinEnabled");
        if (skinLoc >= 0) {
          bool enabled = skinIt != gpuSkins.end() && !skinIt->second.cpuFallback;
          glUniform1i(skinLoc, enabled ? 1 : 0);
          // Active sampler types must not alias the same texture unit, even
          // when this draw takes the unskinned shader branch.
          glUniform1i(glGetUniformLocation(static_cast<GLuint>(activeProgram), "uInfluences"), 6);
          glUniform1i(glGetUniformLocation(static_cast<GLuint>(activeProgram), "uPalette"), 7);
          if (enabled) {
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_BUFFER, skinIt->second.influences);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_BUFFER, skinIt->second.palette);
          }
        }
        // REQ-CLIP: the built-in vertex shaders write gl_ClipDistance; an
        // author program receives the planes as `clipPlane`/`numClipPlanes`
        // and clips itself, so its clip distances stay disabled.
        const bool builtinProgram =
            activeProgram != 0 &&
            (static_cast<GLuint>(activeProgram) == unlitProg ||
             static_cast<GLuint>(activeProgram) == phongProg ||
             static_cast<GLuint>(activeProgram) == pbrProg);
        if (builtinProgram) {
          const GLuint prog = static_cast<GLuint>(activeProgram);
          glUniform1i(glGetUniformLocation(prog, "uNumClipPlanes"), numClips);
          if (numClips > 0)
            glUniform4fv(glGetUniformLocation(prog, "uClipPlane"), numClips, clipEye.data());
        }
        for (int ci = 0; ci < static_cast<int>(ex::ClipPlaneList::kMaxClipPlanes); ++ci) {
          if (builtinProgram && ci < numClips) glEnable(GL_CLIP_DISTANCE0 + ci);
          else glDisable(GL_CLIP_DISTANCE0 + ci);
        }
        glBindVertexArray(g.vao);
        glDrawElements(mode, g.indexCount, GL_UNSIGNED_INT, nullptr);
      };

      // ----------------------------------------------------------------------
      // B7 TRANSPARENCY: partition items into OPAQUE vs BLENDED, draw opaque
      // first (depth writes on, no blend), then sort the blended set back-to-
      // front by world-space centroid depth and draw it with GL_BLEND (srcAlpha,
      // oneMinusSrcAlpha) and the depth MASK off (depth TEST still on, so opaque
      // geometry still occludes transparent surfaces behind it).
      //
      // An item is BLENDED when its material alphaMode == Blend OR transparency
      // > 0. alphaMode == Mask stays in the OPAQUE pass — its cutoff discard in
      // the fragment shader yields hard edges that need no blending or sorting.
      //
      // The centroid sort is exact for SEPARABLE/convex meshes; interpenetrating
      // transparent surfaces (or large concave ones) show the usual sort
      // artifacts — this is browser-parity (X3DOM/X_ITE behave the same), NOT
      // order-independent transparency (OIT is out of scope for the PoC).
      // ----------------------------------------------------------------------
      std::vector<ex::RenderItemId> opaqueItems;
      std::vector<std::pair<float, ex::RenderItemId>> blendedItems; // (depthEye, id)
      for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
        const ex::RenderItem &it = extractor.item(id);
        auto mit = gpuMeshes.find(it.geometry);
        if (mit == gpuMeshes.end()) continue;
        const ex::MaterialDesc &mat = it.material;
        const bool blended = (mat.alphaMode == ex::AlphaMode::Blend) ||
                             (mat.transparency > 0.0f);
        if (!blended) {
          opaqueItems.push_back(id);
        } else {
          // World-space centroid -> eye space; sort key is its eye-space Z
          // (more negative = farther down -Z in eye space). Back-to-front means
          // farthest first => ascending Z (most negative first).
          SFVec3f wc = it.worldTransform.transformPoint(mit->second.localCentroid);
          SFVec3f ec = view.transformPoint(wc);
          blendedItems.emplace_back(ec.z, id);
        }
      }
      // Farthest (most negative eye Z) first.
      std::sort(blendedItems.begin(), blendedItems.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });

      // OPAQUE pass: depth writes on, blending off.
      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
      glDisable(GL_BLEND);
      glDepthMask(GL_TRUE);
      for (ex::RenderItemId id : opaqueItems) drawItem(id);

      // BLENDED pass: back-to-front, GL_BLEND on, depth-MASK off (test still on).
      if (!blendedItems.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (const auto &kv : blendedItems) drawItem(kv.second);
        glDepthMask(GL_TRUE);   // restore for the next frame's depth clear+opaque.
        glDisable(GL_BLEND);
      }

      glBindVertexArray(0);
      glDisable(GL_CULL_FACE); // leave a clean default for the next frame.
      for (int ci = 0; ci < static_cast<int>(ex::ClipPlaneList::kMaxClipPlanes); ++ci)
        glDisable(GL_CLIP_DISTANCE0 + ci); // background/ImGui draw unclipped.
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // keep ImGui/textured quads solid.
    }

    if (uiEnabled) {
      // F1 toggles the diagnostics panel (edge-triggered). F1 is not a nav key,
      // so polling it here never collides with the InputBridge key mapping.
      static bool f1Prev = false;
      const bool f1Now = glfwGetKey(win, GLFW_KEY_F1) == GLFW_PRESS;
      if (f1Now && !f1Prev) showDiagnostics = !showDiagnostics;
      f1Prev = f1Now;

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if (showDiagnostics) {
        ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("x3d-cpp PoC", &showDiagnostics,
                         ImGuiWindowFlags_AlwaysAutoResize)) {
          ImGui::TextUnformatted(scenePath.c_str());
          ImGui::Separator();
          ImGui::Text("frame %.2f ms (%.1f fps)", frameDt * 1000.0,
                      frameDt > 0.0 ? 1.0 / frameDt : 0.0);
          ImGui::Text("render items %zu", extractor.itemCount());
          ImGui::Text("gpu meshes %zu", gpuMeshes.size());
          ImGui::Text("textures %zu",
                      texCache.linear.byUrl.size() + texCache.linear.byInlineNode.size() +
                          texCache.srgb.byUrl.size() + texCache.srgb.byInlineNode.size());
          ImGui::Text("time %.3f", now);
          ImGui::Separator();
          ImGui::Checkbox("Wireframe", &wireframe);
          ImGui::Checkbox("ImGui demo", &showImGuiDemo);
          ImGui::Text("TAB cycles navigation mode");
          ImGui::Text("F1 toggles this panel");
        }
        ImGui::End();
      }
      if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      // Hand this frame's capture intent to the input bridge so the camera/nav
      // and the overlay don't both consume the same mouse/keyboard event. The
      // callbacks fire in glfwPollEvents below (after this), so they see it.
      const ImGuiIO &io = ImGui::GetIO();
      input.setUiCapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
    }

    // --animate: dump every frame as <dir>/frame_NNNN.ppm at the fixed dt, then
    // exit after duration*fps frames. The realtime GL loop (interpolators,
    // sequencers, ROUTEs) captured headlessly into a demo reel.
    if (animate) {
      int sw = 0, sh = 0;
      glfwGetFramebufferSize(win, &sw, &sh);
      char tail[24];
      std::snprintf(tail, sizeof tail, "/frame_%04d.ppm", animFrame + 1);
      writeFramebufferPPM(framesDir + tail, sw, sh);
      glfwSwapBuffers(win);
      glfwPollEvents();
      if (++animFrame >= animTotal) break;
      continue;
    }

    // --screenshot: render a few warm-up frames (so a Pending texture/atlas has
    // resolved on a later frame, contract A), capture the GL framebuffer via
    // glReadPixels, then exit. Captures the real driver output regardless of the
    // window system (a GPU swap chain under Xvfb is invisible to root readback).
    if (!screenshotPath.empty() && ++frameNo >= 3) {
      int sw = 0, sh = 0;
      glfwGetFramebufferSize(win, &sw, &sh);
      if (writeFramebufferPPM(screenshotPath, sw, sh))
        std::fprintf(stderr, "[poc] wrote screenshot %s (%dx%d)\n",
                     screenshotPath.c_str(), sw, sh);
      else
        std::fprintf(stderr, "[poc] screenshot write FAILED: %s\n",
                     screenshotPath.c_str());
      glfwSwapBuffers(win);
      break;
    }

    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  // ----------------------------------------------------------------------
  // 6. Teardown.
  // ----------------------------------------------------------------------
  if (uiEnabled) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
  for (auto &kv : gpuMeshes) {
    glDeleteVertexArrays(1, &kv.second.vao);
    glDeleteBuffers(1, &kv.second.vbo);
    glDeleteBuffers(1, &kv.second.ebo);
  }
  for (auto &kv : gpuSkins) destroySkin(kv.second);
  if (phongProg) glDeleteProgram(phongProg);
  if (unlitProg) glDeleteProgram(unlitProg);
  if (pbrProg)   glDeleteProgram(pbrProg);
  for (auto &kv : authorProgCache) if (kv.second) glDeleteProgram(kv.second);
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
