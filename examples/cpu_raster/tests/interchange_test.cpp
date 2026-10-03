// Annex B acceptance cases for the CPU reference host. These exercise profile
// minima and presentation, independently of descriptor/schema coverage.
#include "FieldValueIO.hpp"
#include "RuntimeSession.hpp"
#include "StbTextureResolver.hpp"
#include "X3DParse.hpp"
#include "cpuraster/ProceduralTexture.hpp"
#include "cpuraster/SceneRender.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace x3d::cpuraster;
namespace ex = x3d::runtime::extract;
namespace rt = x3d::runtime;
namespace g = x3d::cpuraster::glsl;
static int failures = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x);                     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)
static bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

static float shadowPixel(bool shadows, float intensity, bool casts,
                         bool visible, float scale = 1) {
  std::ostringstream s;
  s << "<X3D profile='Interchange' version='4.0'><Scene><Viewpoint position='0 "
       "0 "
    << 5 * scale
    << "'/><NavigationInfo headlight='false'/>"
       "<DirectionalLight direction='-1 0 -1' shadows='"
    << (shadows ? "true" : "false") << "' shadowIntensity='" << intensity
    << "'/><Transform scale='" << scale << " " << scale << " " << scale
    << "'>"
       "<Shape><Appearance><Material diffuseColor='1 1 1' specularColor='0 0 "
       "0'/></Appearance>"
       "<IndexedFaceSet coordIndex='0 1 2 3 -1'><Coordinate point='-2 -2 0 2 "
       "-2 0 2 2 0 -2 2 0'/>"
       "</IndexedFaceSet></Shape><Transform translation='1 0 1'><Shape "
       "castShadow='"
    << (casts ? "true" : "false") << "' visible='"
    << (visible ? "true" : "false")
    << "'><Box size='0.7 0.7 "
       "0.7'/></Shape></Transform></Transform></Scene></X3D>";
  auto session = rt::RuntimeSession::create(x3d::codec::parseDocument(s.str()));
  session->fullSnapshot();
  RenderOptions opt;
  opt.width = opt.height = 64;
  auto frame = renderScene(session->context(), session->extractor(), opt);
  return frame.colorAt(32, 32).x;
}

int main() {
  using Mode = ex::TexCoordGenMode;
  using detail::texCoordGenUv;
  auto local = texCoordGenUv(Mode::Coord, {9, 8, -7}, {1, 0, 0}, true,
                             {0.25f, 0.75f, 2}, {0, 1, 0});
  CHECK(near(local.x, 0.25f) && near(local.y, 0.75f));
  auto sphere = texCoordGenUv(Mode::SphereLocal, {9, 8, -7}, {1, 0, 0}, true,
                              {}, {0, 1, 0});
  CHECK(near(sphere.x, 0.5f) && near(sphere.y, 1));
  auto back = texCoordGenUv(Mode::SphereLocal, {9, 8, -7}, {1, 0, 0}, false, {},
                            {0, 1, 0});
  CHECK(near(back.y, 0));
  auto refract = texCoordGenUv(Mode::SphereReflect, {3, 0, -4}, {0, 0, 1}, true,
                               {}, {}, {0.5f});
  CHECK(near(refract.x, 0.3f) && near(refract.y, 0));
  auto localRefract =
      texCoordGenUv(Mode::SphereReflectLocal, {9, 8, 7}, {1, 0, 0}, true,
                    {4, 2, -1}, {0, 0, 1}, {0.5f, 1, 2, 3});
  CHECK(near(localRefract.x, 0.3f) && near(localRefract.y, 0));
  auto total = texCoordGenUv(Mode::SphereReflect, {3, 0, -4}, {0, 0, 1}, true,
                             {}, {}, {2});
  CHECK(near(total.x, 0) && near(total.y, 0));
  auto zero = texCoordGenUv(Mode::Noise, {9, 8, 7}, {1, 0, 0}, true, {1, 2, 3});
  CHECK(near(zero.x, 0) &&
        near(zero.y, 0)); // gradient noise vanishes at lattice points.
  auto a = texCoordGenUv(Mode::Noise, {}, {}, true, {0.21f, 0.32f, 0.43f}, {},
                         {2, 3, 4, 1, 2, 3});
  auto b = texCoordGenUv(Mode::NoiseEye, {1.42f, 2.96f, 4.72f}, {}, true, {});
  CHECK(near(a.x, b.x) && near(a.y, b.y));
  auto c = texCoordGenUv(Mode::Noise, {}, {}, true, {0.21001f, 0.32f, 0.43f},
                         {}, {2, 3, 4, 1, 2, 3});
  CHECK(near(a.x, c.x) && near(a.y, c.y));
  CHECK(std::fabs(a.x) > 0.001f || std::fabs(a.y) > 0.001f);

  // Generated coordinates reach unlit presentation and the authored transform.
  ex::MaterialDesc material;
  material.model = ex::MaterialModel::Unlit;
  material.emissive = {1, 1, 1};
  ex::TextureRef tex;
  tex.source = ex::TextureRef::Source::Inline;
  tex.slot = ex::TextureRef::Slot::BaseColor;
  tex.inlinePixels = SFImage{2, 1, 3, {255, 0, 0, 0, 255, 0}};
  tex.hasTexCoordGen = true;
  tex.texCoordGen.mode = Mode::Coord;
  tex.generatedTransform.translationS = 0.5f;
  material.textures.push_back(tex);
  FragmentInput f;
  f.posLocal = {0.25f, 0, 0};
  f.texcoord = {0, 0};
  g::vec4 out;
  CHECK(makeUnlitShader(material, false)(f, out));
  CHECK(out.y > 0.95f && out.x < 0.05f);

  std::vector<ex::LightDesc> lights(8);
  CHECK(render_detail::buildEyeLights(lights, rt::Mat4::identity(), true)
            .size() == 9);
  CHECK(render_detail::buildEyeLights(lights, rt::Mat4::identity(), false)
            .size() == 8);
  for (auto &light : lights)
    light.intensity = 0;
  lights.back().intensity = 0.5f;
  lights.back().color = {0, 1, 0};
  lights.back().worldDirection = {0, 0, -1};
  ex::MaterialDesc eightLightMaterial;
  eightLightMaterial.model = ex::MaterialModel::Phong;
  eightLightMaterial.phong.diffuse = {0.25f, 0.25f, 0.25f};
  eightLightMaterial.phong.specular = {0, 0, 0};
  FragmentInput eightLightFragment;
  eightLightFragment.posEye = {0, 0, -1};
  eightLightFragment.normalEye = {0, 0, 1};
  auto eightLightShader = makePhongShader(
      eightLightMaterial,
      render_detail::buildEyeLights(lights, rt::Mat4::identity(), true), false);
  CHECK(eightLightShader(eightLightFragment, out));
  CHECK(near(out.x, 0.25f) && near(out.y, 0.375f) && near(out.z, 0.25f));
  const float lit = shadowPixel(false, 1, true, true);
  CHECK(lit > 0.6f);
  CHECK(shadowPixel(true, 1, true, true) < 0.02f);
  CHECK(near(shadowPixel(true, 0.5f, true, true), lit * 0.5f));
  CHECK(near(shadowPixel(true, 1, false, true), lit));
  CHECK(near(shadowPixel(true, 1, true, false), lit));
  CHECK(shadowPixel(false, 1, true, true, 1e-5f) > 0.6f);
  CHECK(shadowPixel(true, 1, true, true, 1e-5f) < 0.02f);
  CHECK(shadowPixel(false, 1, true, true, 1e5f) > 0.6f);
  CHECK(shadowPixel(true, 1, true, true, 1e5f) < 0.02f);

  // Annex B.3/B.6: 65,535 coordinates, 5,000 planar ten-vertex faces,
  // 65,535 texture coordinates and 500 children in one Group.
  std::ostringstream scene;
  scene << "<X3D profile='Interchange' version='4.0'><Scene><Group>";
  scene << "<Shape><IndexedFaceSet coordIndex='";
  for (int face = 0; face < 5000; ++face) {
    for (int j = 0; j < 10; ++j)
      scene << (face == 0 ? 65525 + j : j) << " ";
    scene << "-1 ";
  }
  scene << "'><Coordinate point='";
  for (int i = 0; i < 65535; ++i) {
    float angle = float(i % 10) * 6.28318530718f / 10;
    scene << std::cos(angle) << ' ' << std::sin(angle) << " 0 ";
  }
  scene << "'/><TextureCoordinate point='";
  for (int i = 0; i < 65535; ++i)
    scene << "0.5 0.5 ";
  scene << "'/></IndexedFaceSet></Shape>";
  for (int i = 1; i < 500; ++i)
    scene << "<Shape><Box/></Shape>";
  scene << "</Group></Scene></X3D>";
  auto session =
      rt::RuntimeSession::create(x3d::codec::parseDocument(scene.str()));
  session->fullSnapshot();
  CHECK(session->extractor().itemCount() == 500);
  const auto &mesh = *session->extractor().item(0).mesh;
  CHECK(mesh.indices.size() == 5000 * 8 * 3);

  // Full parse/extract/raster path: channel 1 and its transform must win.
  const char *multiScene = R"(<X3D profile='Interchange' version='4.0'><Scene>
<Viewpoint position='0 0 5'/><Shape><Appearance>
<MultiTexture mode='"REPLACE" "REPLACE"'><PixelTexture DEF='Pixels' image='2 1 3 0xff0000 0x00ff00'/><PixelTexture USE='Pixels'/></MultiTexture>
<MultiTextureTransform><TextureTransform/><TextureTransform translation='0.5 0'/></MultiTextureTransform>
</Appearance><IndexedFaceSet coordIndex='0 1 2 3 -1'>
<Coordinate point='-2 -2 0 2 -2 0 2 2 0 -2 2 0'/>
<MultiTextureCoordinate><TextureCoordinate point='0.25 0 0.25 0 0.25 0 0.25 0'/><TextureCoordinate point='0.25 0 0.25 0 0.25 0 0.25 0'/></MultiTextureCoordinate>
</IndexedFaceSet></Shape></Scene></X3D>)";
  auto multiSession =
      rt::RuntimeSession::create(x3d::codec::parseDocument(multiScene));
  multiSession->fullSnapshot();
  RenderOptions multiOptions;
  multiOptions.width = multiOptions.height = 32;
  auto multiFrame = renderScene(multiSession->context(),
                                multiSession->extractor(), multiOptions);
  CHECK(multiFrame.colorAt(16, 16).y > 0.95f &&
        multiFrame.colorAt(16, 16).x < 0.05f);

  // Remaining Annex B geometry capacities: validate emitted primitives,
  // not just whether the parser accepts a long array.
  auto geometryCapacity = [](const std::string &node,
                             const std::string &attributes, int points,
                             int expectedIndices, ex::Topology topology) {
    std::ostringstream xml;
    xml << "<X3D profile='Interchange' version='4.0'><Scene><Shape><" << node
        << " " << attributes << "><Coordinate point='";
    for (int i = 0; i < points; ++i) {
      const int k = i % 3;
      if (node.find("Fan") != std::string::npos) {
        const int j = i % 5002;
        const double angle = 2 * 3.141592653589793 * (j - 1) / 5000;
        xml << (j ? std::cos(angle) : 0) << " " << (j ? std::sin(angle) : 0)
            << " 0 ";
      } else if (node.find("Strip") != std::string::npos) {
        xml << i / 2 << " " << i % 2 << " 0 ";
      } else
        xml << (k == 1 ? 1 : 0) << " " << (k == 2 ? 1 : 0) << " 0 ";
    }
    xml << "'/></" << node << "></Shape></Scene></X3D>";
    auto session =
        rt::RuntimeSession::create(x3d::codec::parseDocument(xml.str()));
    session->fullSnapshot();
    CHECK(session->extractor().itemCount() == 1);
    if (session->extractor().itemCount() == 1) {
      const auto &mesh = *session->extractor().item(0).mesh;
      CHECK(mesh.topology == topology);
      CHECK(mesh.indices.size() == static_cast<std::size_t>(expectedIndices));
    }
  };
  geometryCapacity("TriangleSet", "", 45000, 45000, ex::Topology::Triangles);
  geometryCapacity("TriangleFanSet", "fanCount='5002 5002 5002'", 15006, 45000,
                   ex::Topology::Triangles);
  geometryCapacity("TriangleStripSet", "stripCount='5002 5002 5002'", 15006,
                   45000, ex::Topology::Triangles);
  geometryCapacity("LineSet", "vertexCount='15000'", 15000, 29998,
                   ex::Topology::Lines);
  geometryCapacity("PointSet", "", 5000, 5000, ex::Topology::Points);
  std::ostringstream triangleIndices, chainIndices;
  for (int i = 0; i < 15000; ++i)
    triangleIndices << i % 3 << " ";
  for (int i = 0; i < 5002; ++i)
    chainIndices << i << " ";
  geometryCapacity("IndexedTriangleSet",
                   "index='" + triangleIndices.str() + "'", 3, 15000,
                   ex::Topology::Triangles);
  geometryCapacity("IndexedTriangleFanSet",
                   "index='" + chainIndices.str() + " -1'", 5002, 15000,
                   ex::Topology::Triangles);
  geometryCapacity("IndexedTriangleStripSet",
                   "index='" + chainIndices.str() + " -1'", 5002, 15000,
                   ex::Topology::Triangles);
  geometryCapacity("IndexedLineSet",
                   "coordIndex='" + triangleIndices.str() + "'", 3, 29998,
                   ex::Topology::Lines);

  // Field storage minima use the shared codec conversion path.
  auto arrayCapacity = [](X3DFieldType type, auto value, std::size_t count) {
    using T = decltype(value);
    std::vector<T> values(count, value);
    const auto wire = x3d::codec::formatValue(type, std::any(values));
    const auto parsed = x3d::codec::parseValue(type, wire);
    CHECK(std::any_cast<const std::vector<T> &>(parsed).size() == count);
    CHECK(x3d::codec::formatValue(type, parsed) == wire);
  };
  arrayCapacity(X3DFieldType::MFColor, SFColor{0.25f, 0.5f, 0.75f}, 15000);
  arrayCapacity(X3DFieldType::MFColorRGBA, SFColorRGBA{0.25f, 0.5f, 0.75f, 1},
                15000);
  arrayCapacity(X3DFieldType::MFDouble, 1.23456789123e12, 1000);
  arrayCapacity(X3DFieldType::MFFloat, 1.25f, 1000);
  arrayCapacity(X3DFieldType::MFInt32, -2147483647, 20000);
  arrayCapacity(X3DFieldType::MFRotation, SFRotation{0, 1, 0, 0.5f}, 1000);
  arrayCapacity(X3DFieldType::MFTime, 1.23456789123e12, 1000);
  arrayCapacity(X3DFieldType::MFVec2f, SFVec2f{0.25f, 0.75f}, 15000);
  arrayCapacity(X3DFieldType::MFVec2d, SFVec2d{1.23456789123e12, 1e-12}, 15000);
  arrayCapacity(X3DFieldType::MFVec3f, SFVec3f{0.25f, 0.75f, 1}, 15000);
  arrayCapacity(X3DFieldType::MFVec3d, SFVec3d{1.23456789123e12, 1e-12, -1e12},
                15000);
  const std::string longString(30000, 's');
  CHECK(std::any_cast<SFString>(x3d::codec::parseValue(
            X3DFieldType::SFString, longString)) == longString);
  arrayCapacity(X3DFieldType::MFString, longString, 10);
  const std::string defName(50, 'N');
  auto namedDocument = x3d::codec::parseDocument(
      "<X3D profile='Interchange' version='4.0'><Scene><WorldInfo DEF='" +
      defName + "' title='" + longString + "'/></Scene></X3D>");
  CHECK(namedDocument.scene.defs.count(defName) == 1);
  CHECK(rt::geombounds::getField<SFString>(
            *namedDocument.scene.defs.at(defName), "title", {}) == longString);

  // Named material mapping also selects its transform when children are
  // reordered.
  const std::string namedScene =
      R"(<X3D profile='Interchange' version='4.0'><Scene>
<Viewpoint position='0 0 5'/><Shape><Appearance>
<UnlitMaterial emissiveTextureMapping='second'><PixelTexture containerField='emissiveTexture' image='2 1 3 0xff0000 0x00ff00'/></UnlitMaterial>
<MultiTextureTransform><TextureTransform mapping='second' translation='0.5 0'/><TextureTransform mapping='first'/></MultiTextureTransform>
</Appearance><IndexedFaceSet coordIndex='0 1 2 3 -1'>
<Coordinate point='-2 -2 0 2 -2 0 2 2 0 -2 2 0'/>
<MultiTextureCoordinate><TextureCoordinate mapping='first' point='0.25 0 0.25 0 0.25 0 0.25 0'/><TextureCoordinate mapping='second' point='0.25 0 0.25 0 0.25 0 0.25 0'/></MultiTextureCoordinate>
</IndexedFaceSet></Shape></Scene></X3D>)";
  auto namedSession =
      rt::RuntimeSession::create(x3d::codec::parseDocument(namedScene));
  namedSession->fullSnapshot();
  auto namedFrame = renderScene(namedSession->context(),
                                namedSession->extractor(), multiOptions);
  CHECK(namedFrame.colorAt(16, 16).y > 0.95f &&
        namedFrame.colorAt(16, 16).x < 0.05f);

  // Required PixelTexture dimensions and fully transparent/opaque pixels.
  ex::TextureRef large;
  large.source = ex::TextureRef::Source::Inline;
  large.inlinePixels =
      SFImage{512, 512, 4, std::vector<std::uint8_t>(512 * 512 * 4, 255)};
  large.inlinePixels.data[3] = 0;
  auto image = Texture::fromRef(large, false);
  CHECK(image.valid());
  CHECK(image.sample({0.5f / 512, 0.5f / 512}).w < 0.01f);
  CHECK(image.sample({0.5f, 0.5f}).w > 0.99f);

  // Minification must use the authored filter, rather than magnification.
  ex::TextureRef filter;
  filter.source = ex::TextureRef::Source::Inline;
  filter.inlinePixels =
      SFImage{2, 2, 3, {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255}};
  filter.extSampler.generateMipmaps = true;
  filter.extSampler.minificationFilter = ex::MinFilter::AvgPixelAvgMipmap;
  auto mip = Texture::fromRef(filter, false);
  auto averaged = mip.sample({0.25f, 0.25f}, {1, 0}, {0, 1});
  CHECK(near(averaged.x, 0.5f) && near(averaged.y, 0.5f) &&
        near(averaged.z, 0.5f));
  filter.extSampler.generateMipmaps = false;
  filter.extSampler.minificationFilter = ex::MinFilter::NearestPixel;
  auto nearest =
      Texture::fromRef(filter, false).sample({0.4f, 0.4f}, {1, 0}, {0, 1});
  CHECK(near(nearest.x, 1) && near(nearest.y, 0) && near(nearest.z, 0));

  // Fetch/decode composition: URL extension is not a format oracle.
  auto read = [](const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {});
  };
  const auto png =
      read(std::string(X3D_CPURASTER_ASSET_DIR) +
           "/../../../runtime/io/tests/fixtures/texture/rgba_gradient.png");
  CHECK(!png.empty());
  auto decoder = x3d::runtime::io::stb::makeStbTextureResolver(
      [png](const std::string &url, ex::AssetKind kind) {
        CHECK(kind == ex::AssetKind::Texture);
        if (url == "pending")
          return ex::AssetResult::makePending();
        if (url == "missing")
          return ex::AssetResult::makeFailed();
        return ex::AssetResult::makeReady(png);
      });
  CHECK(decoder("pending").pending());
  CHECK(decoder("missing").failed());
  auto decoded = decoder("ftp://fixture/no-extension");
  CHECK(decoded.ready() && decoded.pixels->width == 16 &&
        decoded.pixels->height == 16);
  // All ten URL candidates remain available in authored fallback order.
  ex::TextureRef fallback;
  fallback.source = ex::TextureRef::Source::Url;
  for (int i = 0; i < 9; ++i)
    fallback.url.push_back("missing" + std::to_string(i));
  fallback.url.push_back("ready");
  int attempted = 0;
  auto fallbackDecoder = x3d::runtime::io::stb::makeStbTextureResolver(
      [&](const std::string &url, ex::AssetKind) {
        CHECK(url == fallback.url[static_cast<std::size_t>(attempted)]);
        ++attempted;
        return url == "ready" ? ex::AssetResult::makeReady(png)
                              : ex::AssetResult::makeFailed();
      });
  std::vector<ex::TextureRef> fallbacks{fallback};
  ex::resolveTextureRefs(fallbacks, fallbackDecoder);
  CHECK(attempted == 10 && fallbacks.front().resolvedPixels.ready());

  const auto jpeg = read(std::string(X3D_CPURASTER_ASSET_DIR) +
                         "/models/lion_head/lion_head_diff_1k.jpg");
  auto jpgDecoder = x3d::runtime::io::stb::makeStbTextureResolver(
      [jpeg](const std::string &, ex::AssetKind) {
        return ex::AssetResult::makeReady(jpeg);
      });
  CHECK(jpgDecoder("http://fixture/wrong.png").ready());
  CHECK(x3d::runtime::io::stb::makeStbTextureResolver(
            [](const std::string &, ex::AssetKind) {
              return ex::AssetResult::makeReady({1, 2, 3});
            })("corrupt")
            .failed());

  // Each stage samples its own channel; the second REPLACE must be green.
  tex.hasTexCoordGen = false;
  tex.inlinePixels = SFImage{2, 1, 3, {255, 0, 0, 0, 255, 0}};
  tex.multiMode = "REPLACE";
  tex.channel = 0;
  material.textures = {tex};
  tex.channel = 1;
  material.textures.push_back(tex);
  f.texcoordSets = {{0.25f, 0}, {0.75f, 0}};
  CHECK(makeUnlitShader(material, false)(f, out));
  CHECK(out.y > 0.95f && out.x < 0.05f);
  material.textures[1].slot = ex::TextureRef::Slot::Emissive;
  material.textures.erase(material.textures.begin());
  CHECK(makeUnlitShader(material, false)(f, out));
  CHECK(out.y > 0.95f && out.x < 0.05f);
  f.texcoordSets.clear();

  // Numeric operator oracles: Table 18.3, including separate RGB/alpha modes.
  g::vec4 arg1{0.2f, 0.4f, 0.6f, 0.25f}, arg2{0.8f, 0.6f, 0.4f, 0.75f};
  CHECK(near(detail::multiCombine("ADDSMOOTH", "", arg1, arg2).x, 0.84f));
  CHECK(
      near(detail::multiCombine("BLENDTEXTUREALPHA", "", arg1, arg2).x, 0.65f));
  CHECK(
      near(detail::multiCombine("BLENDCURRENTALPHA", "", arg1, arg2).x, 0.35f));
  CHECK(near(detail::multiCombine("BLENDDIFFUSEALPHA", "", arg1, arg2, 0.5f).x,
             0.5f));
  CHECK(
      near(detail::multiCombine("BLENDFACTORALPHA", "", arg1, arg2, 1, 0.5f).x,
           0.5f));
  CHECK(near(detail::multiCombine("MODULATEALPHA_ADDCOLOR", "", arg1, arg2).x,
             0.4f));
  CHECK(
      near(detail::multiCombine("MODULATEINVALPHA_ADDCOLOR", "", arg1, arg2).x,
           0.8f));
  CHECK(
      near(detail::multiCombine("MODULATEINVCOLOR_ADDALPHA", "", arg1, arg2).x,
           0.89f));
  CHECK(near(detail::multiCombine("DOTPRODUCT3", "", arg1, arg1).x, 0.44f));
  tex.hasTexCoordGen = false;
  tex.inlinePixels = SFImage{1, 1, 4, {128, 128, 128, 64}};
  tex.multiMode = "MODULATE, REPLACE";
  material.textures = {tex};
  CHECK(makeUnlitShader(material, false)(f, out));
  CHECK(near(out.x, 128.0f / 255) && near(out.w, 64.0f / 255));

  // Coordinate-channel selection must not reorder authored texture stages.
  ex::TextureRef firstStage;
  firstStage.source = ex::TextureRef::Source::Inline;
  firstStage.inlinePixels = SFImage{1, 1, 3, {0, 255, 0}};
  firstStage.channel = 1;
  firstStage.multiMode = "REPLACE";
  auto lastStage = firstStage;
  lastStage.inlinePixels = SFImage{1, 1, 3, {255, 0, 0}};
  lastStage.channel = 0;
  material.textures = {firstStage, lastStage};
  CHECK(makeUnlitShader(material, false)(f, out));
  CHECK(out.x > 0.95f && out.y < 0.05f);

  // Non-base material slots use their own coordinate channel.
  ex::MaterialDesc ambientMaterial;
  ambientMaterial.model = ex::MaterialModel::Phong;
  ambientMaterial.phong.diffuse = {1, 1, 1};
  ambientMaterial.phong.ambientIntensity = 1;
  ambientMaterial.phong.specular = {0, 0, 0};
  ex::TextureRef ambientTexture;
  ambientTexture.slot = ex::TextureRef::Slot::Ambient;
  ambientTexture.source = ex::TextureRef::Source::Inline;
  ambientTexture.channel = 1;
  ambientTexture.inlinePixels = SFImage{2, 1, 3, {255, 0, 0, 0, 255, 0}};
  ambientTexture.extSampler.magnificationFilter = ex::MagFilter::NearestPixel;
  ambientMaterial.textures = {ambientTexture};
  EyeLight ambientLight;
  ambientLight.ambientIntensity = 1;
  ambientLight.color = {1, 1, 1};
  ambientLight.dirEye = {0, 0,
                         1}; // diffuse faces away, only ambient contributes.
  FragmentInput ambientFragment;
  ambientFragment.normalEye = {0, 0, 1};
  ambientFragment.posEye = {0, 0, -1};
  ambientFragment.texcoord = {0.25f, 0};
  ambientFragment.texcoordSets = {{0.25f, 0}, {0.75f, 0}};
  glsl::vec4 ambientResult;
  CHECK(makePhongShader(ambientMaterial, {ambientLight}, false)(ambientFragment,
                                                                ambientResult));
  CHECK(ambientResult.y > 0.95f && ambientResult.x < 0.05f);

  std::printf("Interchange acceptance: %d failures\n", failures);
  return failures ? 1 : 0;
}
