// Encoding detection, tokenization, reflected value collection, and reader dispatch.

#include "doctest/doctest.h"
#include "X3DParse.hpp"

#include "FieldValueIO.hpp"
#include "Inflate.hpp" // inflateGzip (gzip fixture round-trip)
#include "NodeBuilder.hpp"
#include "VrmlTokenizer.hpp"
#include "X3DCodecs.hpp" // XmlWriter (to produce a known XML doc to read back)
#include "X3DRuntime.hpp"
#include "XmlReaderAdapter.hpp"

// Concrete node types used by the sample scene / collectFieldValue checks.
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Contour2D.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/NurbsTrimmedSurface.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::codec::Encoding;

namespace {
runtime::X3DDocument buildSampleDocument() {
  runtime::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = runtime::Profile::Interchange;
  doc.head.meta.push_back({"title", "WS-A sample", "", "", "", ""});

  auto transform = std::make_shared<Transform>();
  transform->setDEF("Root");
  transform->setTranslation(SFVec3f{0, 1, 0});

  auto sharedApp = std::make_shared<Appearance>();
  sharedApp->setDEF("SharedApp");
  auto material = std::make_shared<Material>();
  material->setDiffuseColor(SFColor{1, 0, 0});
  sharedApp->setMaterial(material);

  auto shape1 = std::make_shared<Shape>();
  shape1->setDEF("Box1");
  shape1->setAppearance(sharedApp);
  shape1->setGeometry(std::make_shared<Box>());

  auto shape2 = std::make_shared<Shape>();
  shape2->setAppearance(sharedApp); // same shared_ptr -> emits USE
  shape2->setGeometry(std::make_shared<Box>());

  transform->setChildren({shape1, shape2});
  doc.scene.addRootNode(transform);
  doc.scene.routes.emplace_back("Clock", "fraction_changed", "Root",
                                "set_translation");
  return doc;
}

TEST_CASE("sniffing") {
  using x3d::codec::sniff;
  using x3d::codec::sniffByContent;
  using x3d::codec::sniffByExtension;

  // Extension sniffing.
  CHECK_MESSAGE((sniffByExtension("scene.x3d") == Encoding::XML), "ext .x3d -> XML");
  CHECK_MESSAGE((sniffByExtension("scene.x3dv") == Encoding::ClassicVRML), "ext .x3dv -> ClassicVRML");
  CHECK_MESSAGE((sniffByExtension("scene.wrl") == Encoding::VRML97), "ext .wrl -> VRML97");
  CHECK_MESSAGE((sniffByExtension("scene.json") == Encoding::JSON), "ext .json -> JSON");
  CHECK_MESSAGE((sniffByExtension("/a/b/SCENE.X3DV") == Encoding::ClassicVRML), "ext is case-insensitive");
  CHECK_MESSAGE((sniffByExtension("model.wrl.gz") == Encoding::VRML97), "ext strips .gz suffix");
  CHECK_MESSAGE((sniffByExtension("noext") == Encoding::Unknown), "ext unknown -> Unknown");

  // Content sniffing.
  CHECK_MESSAGE((sniffByContent("#VRML V2.0 utf8\n") == Encoding::VRML97), "content #VRML V2.0 -> VRML97");
  CHECK_MESSAGE((sniffByContent("#VRML V1.0 ascii\n") == Encoding::VRML97), "content #VRML V1.0 -> VRML97 (best-effort)");
  CHECK_MESSAGE((sniffByContent("#X3D V4.0 utf8\n") == Encoding::ClassicVRML), "content #X3D V4 -> ClassicVRML");
  CHECK_MESSAGE((sniffByContent("#X3D V3.3 utf8\n") == Encoding::ClassicVRML), "content #X3D V3 -> ClassicVRML");
  CHECK_MESSAGE((sniffByContent("<?xml version=\"1.0\"?><X3D/>") == Encoding::XML), ("content <?xml -> XML"));
  CHECK_MESSAGE((sniffByContent("<X3D profile='Interchange'></X3D>") == Encoding::XML), "content <X3D -> XML");
  CHECK_MESSAGE((sniffByContent("  \n  {\"X3D\":{}}") == Encoding::JSON), "content { ... \"X3D\" -> JSON (after whitespace)");
  CHECK_MESSAGE((sniffByContent("\xEF\xBB\xBF<?xml ?>") == Encoding::XML), "content skips UTF-8 BOM");
  CHECK_MESSAGE((sniffByContent("{ \"other\": 1 }") == Encoding::Unknown), "content bare {} without X3D key -> Unknown");

  // gzip detection.
  std::string gz = std::string("\x1f\x8b\x08\x00", 4);
  CHECK_MESSAGE((x3d::codec::isGzip(gz)), "gzip magic detected");
  CHECK_MESSAGE((sniffByContent(gz) == Encoding::Unknown), "gzip content -> Unknown (caller inflates)");

  // Combined: content overrides a wrong extension.
  CHECK_MESSAGE((sniff("mislabeled.wrl", "#X3D V4.0 utf8\n") == Encoding::ClassicVRML), "content overrides wrong .wrl extension (really ClassicVRML)");
  CHECK_MESSAGE((sniff("noext.dat", "<?xml ?><X3D/>") == Encoding::XML), "content classifies an extensionless file");
  // Combined: extension is the fallback when content is inconclusive.
  CHECK_MESSAGE((sniff("scene.json", "{\"unrecognized\":true}") == Encoding::JSON), "extension is the fallback when content is inconclusive");
}

TEST_CASE("tokenizer") {
  using x3d::codec::VrmlToken;
  using x3d::codec::VrmlTokenizer;

  // Punctuation, identifiers, numbers, comma==whitespace, comments.
  VrmlTokenizer t("Shape { # a comment\n geometry Box { size 2, 2 2 } }");
  CHECK_MESSAGE((t.next().isWord("Shape")), "tok: Shape identifier");
  CHECK_MESSAGE((t.next().isPunct('{')), "tok: { punct");
  CHECK_MESSAGE((t.next().isWord("geometry")), "tok: geometry (comment skipped)");
  CHECK_MESSAGE((t.next().isWord("Box")), "tok: Box identifier");
  CHECK_MESSAGE((t.next().isPunct('{')), "tok: nested { punct");
  CHECK_MESSAGE((t.next().isWord("size")), "tok: size identifier");
  VrmlToken n1 = t.next();
  CHECK_MESSAGE((n1.kind == VrmlToken::Kind::Number && n1.text == "2"), "tok: number 2 (comma is whitespace)");
  CHECK_MESSAGE((t.next().text == "2"), "tok: number 2 again");
  CHECK_MESSAGE((t.next().text == "2"), "tok: number 2 third");
  CHECK_MESSAGE((t.next().isPunct('}')), "tok: } punct");
  CHECK_MESSAGE((t.next().isPunct('}')), "tok: outer } punct");
  CHECK_MESSAGE((t.atEnd()), "tok: stream exhausted");

  // Quoted string unescape + isString flag.
  VrmlTokenizer s(R"(name "he said \"hi\" \\ end")");
  CHECK_MESSAGE((s.next().isWord("name")), "tok: bare word before string");
  VrmlToken str = s.next();
  CHECK_MESSAGE((str.kind == VrmlToken::Kind::String && str.isString), "tok: quoted string flagged isString");
  CHECK_MESSAGE((str.text == "he said \"hi\" \\ end"), "tok: string escapes unescaped to: he said \"hi\" \\ end");

  // Header-line skip.
  VrmlTokenizer h("#X3D V4.0 utf8\nPROFILE Interchange\n",
                  /*firstLineIsHeader=*/true);
  CHECK_MESSAGE((h.next().isWord("PROFILE")), "tok: first header line skipped");
  CHECK_MESSAGE((h.next().isWord("Interchange")), "tok: PROFILE arg");
  CHECK_MESSAGE((h.atEnd()), "tok: body exhausted after header skip");
}

TEST_CASE("collect field value") {
  namespace b = x3d::codec::build;
  using x3d::codec::VrmlTokenizer;

  // SF scalar: one token.
  {
    VrmlTokenizer t("2.5 rest");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFFloat);
    CHECK_MESSAGE((w == "2.5"), "collect SFFloat: one token");
    CHECK_MESSAGE((t.peek().isWord("rest")), "collect SFFloat: stops after one token");
    auto v = x3d::codec::parseValue(X3DFieldType::SFFloat, w);
    CHECK_MESSAGE((std::any_cast<float>(v) == 2.5f), "collect SFFloat: parses to 2.5");
  }
  // SF struct: N component tokens.
  {
    VrmlTokenizer t("1 2 3 next");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFVec3f);
    CHECK_MESSAGE((w == "1 2 3"), "collect SFVec3f: three tokens joined");
    auto v = std::any_cast<SFVec3f>(
        x3d::codec::parseValue(X3DFieldType::SFVec3f, w));
    CHECK_MESSAGE((v.x == 1 && v.y == 2 && v.z == 3), "collect SFVec3f: parses to (1,2,3)");
    CHECK_MESSAGE((t.peek().isWord("next")), "collect SFVec3f: stops after 3 tokens");
  }
  // SFString: one quoted token, re-quoted for parseValue.
  {
    VrmlTokenizer t(R"("hello world" tail)");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFString);
    auto v = std::any_cast<std::string>(
        x3d::codec::parseValue(X3DFieldType::SFString, w));
    CHECK_MESSAGE((v == "hello world"), "collect SFString: 'hello world'");
    CHECK_MESSAGE((t.peek().isWord("tail")), "collect SFString: stops after the string");
  }
  // MFInt32: bracketed run.
  {
    VrmlTokenizer t("[ 1 2 3 4 ] after");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFInt32);
    auto v = std::any_cast<std::vector<int>>(
        x3d::codec::parseValue(X3DFieldType::MFInt32, w));
    CHECK_MESSAGE((v.size() == 4 && v[0] == 1 && v[3] == 4), "collect MFInt32: [1 2 3 4]");
    CHECK_MESSAGE((t.peek().isWord("after")), "collect MFInt32: consumes the brackets");
  }
  // MF*: a single bare element without brackets (VRML one-element MF).
  {
    VrmlTokenizer t("0 0 1 0 done");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFRotation);
    auto v = std::any_cast<std::vector<SFRotation>>(
        x3d::codec::parseValue(X3DFieldType::MFRotation, w));
    CHECK_MESSAGE((v.size() == 1 && v[0].z == 1), "collect MFRotation: bare 4-tuple");
    CHECK_MESSAGE((t.peek().isWord("done")), "collect MFRotation: bare element consumes exactly 4 tokens");
  }
  // MFString: quotes preserved per element.
  {
    VrmlTokenizer t(R"([ "a b" "c" ] x)");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFString);
    auto v = std::any_cast<std::vector<std::string>>(
        x3d::codec::parseValue(X3DFieldType::MFString, w));
    CHECK_MESSAGE((v.size() == 2 && v[0] == "a b" && v[1] == "c"), "collect MFString: preserves per-element quoting");
  }
}

TEST_CASE("xml adapter and front door") {
  runtime::X3DDocument doc = buildSampleDocument();
  codec::XmlWriter writer;
  std::string xml = writer.writeDocument(doc);

  // Raw XmlReader (the baseline the adapter must reproduce).
  codec::XmlReader rawReader;
  runtime::X3DDocument viaRaw = rawReader.readDocument(xml);

  // Through the adapter.
  codec::XmlReaderAdapter adapter;
  CHECK_MESSAGE((adapter.encoding() == Encoding::XML), "adapter reports XML encoding");
  runtime::X3DDocument viaAdapter = adapter.readDocument(xml);

  // Through the front door with no hint (must sniff XML and pick the adapter).
  runtime::X3DDocument viaFrontDoor = codec::parseDocument(xml);

  // Re-serialize all three and compare against the baseline serialization.
  std::string sRaw = writer.writeDocument(viaRaw);
  std::string sAdapter = writer.writeDocument(viaAdapter);
  std::string sFront = writer.writeDocument(viaFrontDoor);
  CHECK_MESSAGE((sAdapter == sRaw), "XmlReaderAdapter == raw XmlReader (serialized)");
  CHECK_MESSAGE((sFront == sRaw), "parseDocument(no hint) == raw XmlReader (serialized)");

  // Structural spot checks on the adapter's output.
  CHECK_MESSAGE((viaAdapter.scene.rootNodes.size() == 1), "adapter: one root node");
  CHECK_MESSAGE((viaAdapter.scene.routes.size() == 1), "adapter: ROUTE survived");
  CHECK_MESSAGE((viaAdapter.head.meta.size() == 1 &&
            viaAdapter.head.meta[0].name == "title"), "adapter: head meta survived");

  // DEF/USE identity: both shapes share one Appearance shared_ptr.
  auto root =
      std::dynamic_pointer_cast<Transform>(viaAdapter.scene.rootNodes[0]);
  CHECK_MESSAGE((static_cast<bool>(root)), "adapter: root is a Transform");
  if (root) {
    const auto &kids = root->getChildren();
    CHECK_MESSAGE((kids.size() == 2), "adapter: transform has two children");
    if (kids.size() == 2) {
      auto s1 = std::dynamic_pointer_cast<Shape>(kids[0]);
      auto s2 = std::dynamic_pointer_cast<Shape>(kids[1]);
      CHECK_MESSAGE((s1 && s2 && s1->getAppearance() && s2->getAppearance() &&
                s1->getAppearance().get() == s2->getAppearance().get()), "adapter: DEF/USE Appearance is one shared_ptr");
    }
  }

  {
    auto jsonReader = codec::makeReader(Encoding::JSON);
    CHECK_MESSAGE((jsonReader && jsonReader->encoding() == Encoding::JSON), "makeReader(JSON) returns a JSON reader (WS-D landed)");
  }

  {
    auto vrmlReader = codec::makeReader(Encoding::VRML97);
    CHECK_MESSAGE((vrmlReader && vrmlReader->encoding() == Encoding::VRML97), "makeReader(VRML97) returns a VRML97 reader (WS-C landed)");
  }

  // Unknown-after-sniff throws.
  bool threw = false;
  try {
    codec::parseDocument("this is not any X3D encoding at all");
  } catch (const std::runtime_error &) {
    threw = true;
  }
  CHECK_MESSAGE((threw), "parseDocument throws on unrecognizable content");
}
// Lenient recovery for BOM, nested quotes, and write-only MFNode fields.
TEST_CASE("lenient read fixes") {
  // Strip the UTF-8 BOM before XML sniffing and parsing.
  {
    const std::string bom = "\xEF\xBB\xBF";
    const std::string xml =
        bom + "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              "<X3D profile='Interchange' version='3.0'>"
              "<Scene><Shape><Box/></Shape></Scene></X3D>";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = codec::parseDocument(xml); // no hint: must sniff XML after BOM strip
    } catch (const std::exception &e) {
      threw = true;
      std::cerr << "  (BOM parse threw: " << e.what() << ")\n";
    }
    CHECK_MESSAGE((!threw), "BOM: UTF-8 BOM-prefixed XML parses without throwing");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "BOM: BOM-prefixed document yields the one root node");
    CHECK_MESSAGE((doc.version == "3.0"), "BOM: version attribute read past the BOM");

    // And the same content without a BOM is byte-identical in outcome (the
    // strip is a no-op when no BOM is present).
    runtime::X3DDocument plain = codec::parseDocument(xml.substr(bom.size()));
    CHECK_MESSAGE((plain.scene.rootNodes.size() == doc.scene.rootNodes.size()), "BOM: with/without BOM produce the same root count");
  }

  // An inner same-kind quote extends the value to the last quote before tag close.
  {
    const std::string xml =
        "<X3D profile='Interchange'><head>"
        "<meta name='description' content='it's a test' />"
        "</head><Scene><Shape><Box/></Shape></Scene></X3D>";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = codec::parseDocument(xml, Encoding::XML);
    } catch (const std::exception &e) {
      threw = true;
      std::cerr << "  (nested-quote parse threw: " << e.what() << ")\n";
    }
    CHECK_MESSAGE((!threw), "attr-recovery: inner same-quote value does not throw");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "attr-recovery: document past the bad attribute still parses");
    bool gotMeta = doc.head.meta.size() == 1 &&
                   doc.head.meta[0].name == "description" &&
                   doc.head.meta[0].content == "it's a test";
    CHECK_MESSAGE((gotMeta), "attr-recovery: widened value captured the full \"it's a test\"");

    // A well-formed single-quoted value (no inner quote) is byte-identical: the
    // recovery path must not fire for valid XML.
    const std::string ok =
        "<X3D profile='Interchange'><head>"
        "<meta name='description' content='plain text' />"
        "</head><Scene/></X3D>";
    runtime::X3DDocument okDoc = codec::parseDocument(ok, Encoding::XML);
    CHECK_MESSAGE((okDoc.head.meta.size() == 1 &&
              okDoc.head.meta[0].content == "plain text"), "attr-recovery: well-formed value is unchanged (no false trigger)");
  }

  // A child without containerField must not call an inputOnly sink's missing getter.
  {
    const std::string xml =
        "<X3D profile='Full'><Scene>"
        "<Shape><NurbsTrimmedSurface>"
        "<Contour2D/>"
        "</NurbsTrimmedSurface></Shape>"
        "</Scene></X3D>";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = codec::parseDocument(xml, Encoding::XML);
    } catch (const std::exception &e) {
      threw = true;
      std::cerr << "  (NURBS guard parse threw: " << e.what() << ")\n";
    }
    CHECK_MESSAGE((!threw), "NURBS guard: Contour2D w/o containerField does not crash/throw");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "NURBS guard: the Shape root still parses");
  }
}

std::string readBinaryFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Gzip input and its uncompressed sibling produce identical serialization.
// dataDir points at runtime/parse/tests/data; "" disables the file-backed
// checks (the in-memory inflateGzip check below still runs unconditionally).
void testGzipInflate(const std::string &dataDir) {
  // In-memory: a tiny gzip stream produced offline (gzip -n of "#VRML V2.0
  // utf8\n") decompresses to exactly that header line, with CRC32/ISIZE
  // verified inside inflateGzip. This keeps one assertion independent of the
  // on-disk fixtures.
  {
    // printf '#VRML V2.0 utf8\n' | gzip -n -c  (16 payload bytes incl. \n).
    static const unsigned char kGz[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x53, 0x0e,
        0x0b, 0xf2, 0xf5, 0x51, 0x08, 0x33, 0xd2, 0x33, 0x50, 0x28, 0x2d, 0x49,
        0xb3, 0xe0, 0x02, 0x00, 0x01, 0x4b, 0x9f, 0x68, 0x10, 0x00, 0x00, 0x00};
    std::string gz(reinterpret_cast<const char *>(kGz), sizeof(kGz));
    CHECK_MESSAGE((x3d::codec::isGzip(gz)), "gzip: in-memory fixture has gzip magic");
    std::string text = x3d::codec::inflateGzip(gz);
    CHECK_MESSAGE((text == "#VRML V2.0 utf8\n"), "gzip: inflateGzip recovers exact #VRML header line");
  }

  if (dataDir.empty()) {
    std::cout << "ok: gzip: (no data dir given; file-backed checks skipped)\n";
    return;
  }

  codec::XmlWriter writer; // a stable, content-equality serialization.

  struct Case {
    std::string gz;       // compressed fixture
    std::string plain;    // uncompressed sibling
    std::string headPfx;  // expected inflated header prefix
    std::string label;
  };
  const Case cases[] = {
      {dataDir + "/gzip/NewShape.wrl.gz", dataDir + "/wrl/NewShape.wrl",
       "#VRML", "VRML97 .wrl.gz"},
      {dataDir + "/gzip/HelloWorld.x3dv.gz", dataDir + "/x3dv/HelloWorld.x3dv",
       "#X3D", "ClassicVRML .x3dv.gz"},
  };

  for (const auto &c : cases) {
    std::string raw = readBinaryFile(c.gz);
    CHECK_MESSAGE((!raw.empty() && x3d::codec::isGzip(raw)), ("gzip: " + c.label + " fixture present and gzip-magic"));
    if (raw.empty())
      continue;

    // (1) inflateGzip header check.
    std::string inflated = x3d::codec::inflateGzip(raw);
    CHECK_MESSAGE((inflated.compare(0, c.headPfx.size(), c.headPfx) == 0), ("gzip: " + c.label + " inflates to a " + c.headPfx + " header"));

    // (2) parseFile(.gz) parity with parseFile(uncompressed sibling).
    bool threw = false;
    std::string sCompressed, sPlain;
    try {
      runtime::X3DDocument viaGz = codec::parseFile(c.gz);
      runtime::X3DDocument viaPlain = codec::parseFile(c.plain);
      sCompressed = writer.writeDocument(viaGz);
      sPlain = writer.writeDocument(viaPlain);
    } catch (const std::exception &e) {
      threw = true;
      std::cerr << "  (exception: " << e.what() << ")\n";
    }
    CHECK_MESSAGE((!threw), ("gzip: " + c.label + " parseFile did not throw"));
    CHECK_MESSAGE((!sCompressed.empty()), ("gzip: " + c.label + " parseFile produced a document"));
    CHECK_MESSAGE((sCompressed == sPlain), ("gzip: " + c.label +
              " parseFile(.gz) == parseFile(uncompressed sibling)"));
  }
}

// A no-containerField child must attach to the parent field named by the
// CHILD's default containerField, not be dropped onto an arbitrary slot. The
// canonical case: <Contour2D/> under <NurbsTrimmedSurface> -> trimmingContour.
TEST_CASE("default container field routing") {
  const std::string xml =
      "<X3D profile='Immersive' version='3.3'><Scene>"
      "<Shape><NurbsTrimmedSurface><Contour2D/></NurbsTrimmedSurface></Shape>"
      "</Scene></X3D>";
  runtime::X3DDocument doc = codec::parseDocument(xml, Encoding::XML);
  CHECK_MESSAGE((!doc.scene.rootNodes.empty()), "routing: document has a root");
  auto shape = doc.scene.rootNodes.empty()
                   ? nullptr
                   : std::dynamic_pointer_cast<Shape>(doc.scene.rootNodes[0]);
  CHECK_MESSAGE((shape != nullptr), "routing: root is a Shape");
  auto nts = shape ? std::dynamic_pointer_cast<NurbsTrimmedSurface>(
                         shape->getGeometry())
                   : nullptr;
  CHECK_MESSAGE((nts != nullptr), "routing: Shape.geometry is a NurbsTrimmedSurface");
  if (nts) {
    MFNode contours = nts->getTrimmingContour();
    CHECK_MESSAGE((contours.size() == 1), "routing: Contour2D landed in trimmingContour (size==1)");
    if (contours.size() == 1)
      CHECK_MESSAGE((std::dynamic_pointer_cast<Contour2D>(contours[0]) != nullptr), "routing: the trimmingContour child is a Contour2D");
  }
}

} // namespace

TEST_CASE("gzip inflate") {
  testGzipInflate(X3D_TEST_DATA_DIR);
}
