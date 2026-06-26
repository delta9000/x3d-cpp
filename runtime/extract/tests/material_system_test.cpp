// material_system_test.cpp — M2.5 T5 acceptance: extract/MaterialSystem.hpp.
//
// Builds real generated Appearance/Material nodes and asserts materialOf()
// resolves them to the right MaterialDesc per the locked spec cases:
//   * Material diffuse + transparency (default toRGBA().a == 1.0).
//   * PhysicalMaterial baseColor / metallic / roughness.
//   * null material under a present Appearance => Phong 0.8 grey.
//   * null Appearance entirely => Unlit white.
// Plus texturesOf material-slot precedence over legacy Appearance.texture (D6),
// PixelTexture->Inline, MultiTexture channel expansion.
#include "MaterialSystem.hpp"

#include "X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("material_system_test") {
  // --- null Appearance entirely => Unlit white ---------------------------
  {
    MaterialDesc m = materialOf(nullptr);
    CHECK((m.model == MaterialModel::Unlit));
    CHECK((feq(m.toRGBA().r, 1.0f) && feq(m.toRGBA().g, 1.0f) &&
           feq(m.toRGBA().b, 1.0f)));
    CHECK((feq(m.emissive.r, 1.0f)));
    CHECK((feq(m.toRGBA().a, 1.0f)));
  }

  // --- present Appearance, null material => Unlit white (MAT-001; §12.4.2,
  //     §12.2.5 rule 4: equivalent to a default UnlitMaterial, lighting off) ---
  {
    auto app = createX3DNode("Appearance");
    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Unlit));
    CHECK((feq(m.toRGBA().r, 1.0f) && feq(m.toRGBA().g, 1.0f) &&
           feq(m.toRGBA().b, 1.0f)));
    CHECK((feq(m.emissive.r, 1.0f) && feq(m.emissive.g, 1.0f) &&
           feq(m.emissive.b, 1.0f)));
  }
  // --- null material: Appearance.texture acts as the EMISSIVE (unlit) texture
  //     (MAT-001 part 2; §12.2.5 rule 4) ----------------------------------
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"u.png"}));
    auto app = createX3DNode("Appearance");
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(tex)));
    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Unlit));
    CHECK((m.textures.size() == 1 &&
           m.textures[0].slot == TextureRef::Slot::Emissive));
  }

  // --- Material diffuse + transparency -----------------------------------
  {
    auto mat = createX3DNode("Material");
    setF(mat, "diffuseColor", std::any(SFColor{0.2f, 0.4f, 0.6f}));
    setF(mat, "transparency", std::any(0.25f));
    setF(mat, "emissiveColor", std::any(SFColor{0.1f, 0.0f, 0.0f}));
    setF(mat, "specularColor", std::any(SFColor{0.5f, 0.5f, 0.5f}));
    setF(mat, "shininess", std::any(0.6f));
    setF(mat, "ambientIntensity", std::any(0.3f));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Phong));
    CHECK((feq(m.phong.diffuse.r, 0.2f) && feq(m.phong.diffuse.g, 0.4f) &&
           feq(m.phong.diffuse.b, 0.6f)));
    CHECK((feq(m.transparency, 0.25f)));
    CHECK((feq(m.toRGBA().a, 0.75f))); // alpha = 1 - transparency.
    CHECK((feq(m.emissive.r, 0.1f)));
    CHECK((feq(m.phong.specular.r, 0.5f)));
    CHECK((feq(m.phong.shininess, 0.6f)));
    CHECK((feq(m.phong.ambientIntensity, 0.3f)));
  }

  // --- Material default toRGBA().a == 1.0 (the locked assertion) ---------
  {
    auto mat = createX3DNode("Material");
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    MaterialDesc m = materialOf(app.get());
    CHECK((feq(m.toRGBA().a, 1.0f)));
  }

  // --- PhysicalMaterial baseColor / metallic / roughness -----------------
  {
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "baseColor", std::any(SFColor{0.1f, 0.2f, 0.3f}));
    setF(mat, "metallic", std::any(0.0f));
    setF(mat, "roughness", std::any(0.5f));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Physical));
    CHECK((feq(m.physical.baseColor.r, 0.1f) && feq(m.physical.baseColor.g, 0.2f) &&
           feq(m.physical.baseColor.b, 0.3f)));
    CHECK((feq(m.physical.metallic, 0.0f)));
    CHECK((feq(m.physical.roughness, 0.5f)));
    CHECK((feq(m.toRGBA().a, 1.0f))); // default transparency 0.
  }

  // --- UnlitMaterial: emissive IS the surface color ----------------------
  {
    auto mat = createX3DNode("UnlitMaterial");
    setF(mat, "emissiveColor", std::any(SFColor{0.9f, 0.1f, 0.2f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Unlit));
    CHECK((feq(m.toRGBA().r, 0.9f) && feq(m.toRGBA().g, 0.1f) &&
           feq(m.toRGBA().b, 0.2f)));
    CHECK((feq(m.emissive.r, 0.9f)));
  }

  // --- texturesOf: material-slot precedence over legacy Appearance.texture
  {
    // A Material with a diffuseTexture AND a legacy Appearance.texture: the
    // material slot must win (D6) and the legacy texture must be ignored.
    auto diffuseTex = createX3DNode("ImageTexture");
    setF(diffuseTex, "url", std::any(MFString{"diffuse.png"}));
    auto legacyTex = createX3DNode("ImageTexture");
    setF(legacyTex, "url", std::any(MFString{"legacy.png"}));

    auto mat = createX3DNode("Material");
    setF(mat, "diffuseTexture", std::any(std::shared_ptr<X3DNode>(diffuseTex)));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(legacyTex)));

    auto texs = texturesOf(app.get());
    CHECK((texs.size() == 1));
    CHECK((texs[0].slot == TextureRef::Slot::Diffuse));
    CHECK((texs[0].source == TextureRef::Source::Url));
    CHECK((texs[0].url.size() == 1 && texs[0].url[0] == "diffuse.png"));
  }

  // --- texturesOf: legacy Appearance.texture used when no material slot ---
  {
    auto legacyTex = createX3DNode("ImageTexture");
    setF(legacyTex, "url", std::any(MFString{"legacy.png"}));
    auto mat = createX3DNode("Material"); // no texture slots set.
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(legacyTex)));

    auto texs = texturesOf(app.get());
    CHECK((texs.size() == 1));
    CHECK((texs[0].slot == TextureRef::Slot::BaseColor));
    CHECK((texs[0].url.size() == 1 && texs[0].url[0] == "legacy.png"));
  }

  // --- texturesOf: PixelTexture -> Inline (pixels verbatim) ---------------
  {
    auto px = createX3DNode("PixelTexture");
    setF(px, "image",
         std::any(SFImage{2, 2, 3, std::vector<unsigned char>(12, 0xAB)}));
    auto app = createX3DNode("Appearance");
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(px)));

    auto texs = texturesOf(app.get());
    CHECK((texs.size() == 1));
    CHECK((texs[0].source == TextureRef::Source::Inline));
    CHECK((texs[0].inlinePixels.width == 2 && texs[0].inlinePixels.height == 2));
  }

  // --- texturesOf: MovieTexture -> Movie ----------------------------------
  {
    auto mv = createX3DNode("MovieTexture");
    setF(mv, "url", std::any(MFString{"clip.mp4"}));
    auto app = createX3DNode("Appearance");
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(mv)));
    auto texs = texturesOf(app.get());
    CHECK((texs.size() == 1));
    CHECK((texs[0].source == TextureRef::Source::Movie));
    CHECK((texs[0].url.size() == 1 && texs[0].url[0] == "clip.mp4"));
  }

  // --- texturesOf: MultiTexture -> one ref per channel --------------------
  {
    auto t0 = createX3DNode("ImageTexture");
    setF(t0, "url", std::any(MFString{"layer0.png"}));
    auto t1 = createX3DNode("ImageTexture");
    setF(t1, "url", std::any(MFString{"layer1.png"}));
    auto multi = createX3DNode("MultiTexture");
    setF(multi, "texture",
         std::any(std::vector<std::shared_ptr<X3DNode>>{t0, t1}));
    auto app = createX3DNode("Appearance");
    setF(app, "texture", std::any(std::shared_ptr<X3DNode>(multi)));

    auto texs = texturesOf(app.get());
    CHECK((texs.size() == 2));
    CHECK((texs[0].channel == 0 && texs[1].channel == 1));
    CHECK((texs[0].url[0] == "layer0.png" && texs[1].url[0] == "layer1.png"));
  }

  // --- MAT-002/003/004/005: Phong Material extra texture slots + params ----
  {
    auto sh = createX3DNode("ImageTexture"); setF(sh, "url", std::any(MFString{"shin.png"}));
    auto am = createX3DNode("ImageTexture"); setF(am, "url", std::any(MFString{"amb.png"}));
    auto mat = createX3DNode("Material");
    setF(mat, "shininessTexture", std::any(std::shared_ptr<X3DNode>(sh)));
    setF(mat, "ambientTexture", std::any(std::shared_ptr<X3DNode>(am)));
    setF(mat, "occlusionStrength", std::any(0.5f));
    setF(mat, "normalScale", std::any(2.0f));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    MaterialDesc m = materialOf(app.get());
    CHECK((feq(m.phong.occlusionStrength, 0.5f))); // MAT-004
    CHECK((feq(m.normalScale, 2.0f)));             // MAT-005
    bool hasShin = false, hasAmb = false;
    for (auto &t : m.textures) {
      if (t.slot == TextureRef::Slot::Shininess) hasShin = true;
      if (t.slot == TextureRef::Slot::Ambient) hasAmb = true;
    }
    CHECK((hasShin)); // MAT-002
    CHECK((hasAmb));  // MAT-003
  }

  // --- alphaMode / alphaCutoff read off the Appearance --------------------
  {
    auto app = createX3DNode("Appearance");
    setF(app, "alphaCutoff", std::any(0.3f));
    // alphaMode is an SFEnum; set via its enum token through reflection.
    for (auto &f : app->fields())
      if (f.x3dName == "alphaMode" && f.setEnumString)
        f.setEnumString(*app, "BLEND");
    MaterialDesc m = materialOf(app.get());
    CHECK((m.alphaMode == AlphaMode::Blend));
    CHECK((feq(m.alphaCutoff, 0.3f)));
  }

  // --- discriminated-union invariant: Phong model ignores physical block ----
  {
    auto mat = createX3DNode("Material");
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
    MaterialDesc m = materialOf(app.get());
    CHECK((m.model == MaterialModel::Phong));
    CHECK((feq(m.physical.metallic, 1.0f)));  // default, but meaningless for Phong
    CHECK((feq(m.phong.shininess, 0.2f)));    // the field that actually matters
  }

  // --- backMaterial (closes MAT-006) --------------------------------------
  {
    auto frontMat = createX3DNode("Material");
    setF(frontMat, "diffuseColor", std::any(SFColor{0.2f, 0.4f, 0.6f}));
    auto backMat = createX3DNode("Material");
    setF(backMat, "diffuseColor", std::any(SFColor{0.6f, 0.4f, 0.2f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(frontMat)));
    setF(app, "backMaterial", std::any(std::shared_ptr<X3DNode>(backMat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.backMaterial != nullptr));
    CHECK((m.doubleSided == true));
    CHECK((m.backMaterial->model == MaterialModel::Phong));
    CHECK((feq(m.backMaterial->phong.diffuse.r, 0.6f)));
  }

  // --- backMaterialConstraintMet: matching front/back (same model type)
  //     => constraint met (true) -------------------
  {
    auto frontMat = createX3DNode("Material");
    setF(frontMat, "diffuseColor", std::any(SFColor{0.3f, 0.3f, 0.3f}));
    auto backMat = createX3DNode("Material");
    setF(backMat, "diffuseColor", std::any(SFColor{0.5f, 0.5f, 0.5f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(frontMat)));
    setF(app, "backMaterial", std::any(std::shared_ptr<X3DNode>(backMat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.backMaterial != nullptr));
    CHECK((m.backMaterialConstraintMet == true));
  }

  // --- backMaterialConstraintMet: mismatched model (front=Phong,
  //     back=PhysicalMaterial) => constraint NOT met (false) ----------------
  {
    auto frontMat = createX3DNode("Material");
    setF(frontMat, "diffuseColor", std::any(SFColor{0.3f, 0.3f, 0.3f}));
    auto backMat = createX3DNode("PhysicalMaterial");
    setF(backMat, "baseColor", std::any(SFColor{0.5f, 0.5f, 0.5f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(frontMat)));
    setF(app, "backMaterial", std::any(std::shared_ptr<X3DNode>(backMat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.backMaterial != nullptr));
    CHECK((m.backMaterialConstraintMet == false));
  }

  // --- backMaterialConstraintMet: no backMaterial => trivially true --------
  {
    auto frontMat = createX3DNode("Material");
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(frontMat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((m.backMaterial == nullptr));
    CHECK((m.backMaterialConstraintMet == true));
  }

  return;
}
