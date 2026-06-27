// reader_test.cpp
// WS-A integration test for the unified parsing front-end:
//   1. Encoding sniffing — extension and content classify the four encodings;
//      content overrides a wrong extension; gzip magic is detected.
//   2. VrmlTokenizer — punctuation, quoted-string unescape, comments,
//      comma==whitespace, header-line skip.
//   3. NodeBuilder::collectFieldValue — the SF/MF token-gathering that hands a
//      wire string to FieldValueIO::parseValue, round-tripped through
//      parseValue to prove the value semantics survive.
//   4. XmlReaderAdapter — satisfies X3DReader and produces a document identical
//      to the raw XmlReader (the adapter adds no behavior of its own).
//   5. parseDocument — with no hint, sniffs XML content and produces a document
//      equal to the explicitly-dispatched XmlReaderAdapter.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DParse.hpp"

#include "FieldValueIO.hpp"
#include "Inflate.hpp" // inflateGzip (gzip fixture round-trip)
#include "X3DCodecs.hpp" // XmlWriter (to produce a known XML doc to read back)
#include "X3DRuntime.hpp"

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
using x3d::codec::Encoding;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

// --- A tiny known document, built programmatically (mirrors roundtrip_test).
// --
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

void testSniffing() {
  using x3d::codec::sniff;
  using x3d::codec::sniffByContent;
  using x3d::codec::sniffByExtension;

  // Extension sniffing.
  check(sniffByExtension("scene.x3d") == Encoding::XML, "ext .x3d -> XML");
  check(sniffByExtension("scene.x3dv") == Encoding::ClassicVRML,
        "ext .x3dv -> ClassicVRML");
  check(sniffByExtension("scene.wrl") == Encoding::VRML97,
        "ext .wrl -> VRML97");
  check(sniffByExtension("scene.json") == Encoding::JSON, "ext .json -> JSON");
  check(sniffByExtension("/a/b/SCENE.X3DV") == Encoding::ClassicVRML,
        "ext is case-insensitive");
  check(sniffByExtension("model.wrl.gz") == Encoding::VRML97,
        "ext strips .gz suffix");
  check(sniffByExtension("noext") == Encoding::Unknown,
        "ext unknown -> Unknown");

  // Content sniffing.
  check(sniffByContent("#VRML V2.0 utf8\n") == Encoding::VRML97,
        "content #VRML V2.0 -> VRML97");
  check(sniffByContent("#VRML V1.0 ascii\n") == Encoding::VRML97,
        "content #VRML V1.0 -> VRML97 (best-effort)");
  check(sniffByContent("#X3D V4.0 utf8\n") == Encoding::ClassicVRML,
        "content #X3D V4 -> ClassicVRML");
  check(sniffByContent("#X3D V3.3 utf8\n") == Encoding::ClassicVRML,
        "content #X3D V3 -> ClassicVRML");
  check(sniffByContent("<?xml version=\"1.0\"?><X3D/>") == Encoding::XML,
        "content <?xml -> XML");
  check(sniffByContent("<X3D profile='Interchange'></X3D>") == Encoding::XML,
        "content <X3D -> XML");
  check(sniffByContent("  \n  {\"X3D\":{}}") == Encoding::JSON,
        "content { ... \"X3D\" -> JSON (after whitespace)");
  check(sniffByContent("\xEF\xBB\xBF<?xml ?>") == Encoding::XML,
        "content skips UTF-8 BOM");
  check(sniffByContent("{ \"other\": 1 }") == Encoding::Unknown,
        "content bare {} without X3D key -> Unknown");

  // gzip detection.
  std::string gz = std::string("\x1f\x8b\x08\x00", 4);
  check(x3d::codec::isGzip(gz), "gzip magic detected");
  check(sniffByContent(gz) == Encoding::Unknown,
        "gzip content -> Unknown (caller inflates)");

  // Combined: content overrides a wrong extension.
  check(sniff("mislabeled.wrl", "#X3D V4.0 utf8\n") == Encoding::ClassicVRML,
        "content overrides wrong .wrl extension (really ClassicVRML)");
  check(sniff("noext.dat", "<?xml ?><X3D/>") == Encoding::XML,
        "content classifies an extensionless file");
  // Combined: extension is the fallback when content is inconclusive.
  check(sniff("scene.json", "{\"unrecognized\":true}") == Encoding::JSON,
        "extension is the fallback when content is inconclusive");
}

void testTokenizer() {
  using x3d::codec::VrmlToken;
  using x3d::codec::VrmlTokenizer;

  // Punctuation, identifiers, numbers, comma==whitespace, comments.
  VrmlTokenizer t("Shape { # a comment\n geometry Box { size 2, 2 2 } }");
  check(t.next().isWord("Shape"), "tok: Shape identifier");
  check(t.next().isPunct('{'), "tok: { punct");
  check(t.next().isWord("geometry"), "tok: geometry (comment skipped)");
  check(t.next().isWord("Box"), "tok: Box identifier");
  check(t.next().isPunct('{'), "tok: nested { punct");
  check(t.next().isWord("size"), "tok: size identifier");
  VrmlToken n1 = t.next();
  check(n1.kind == VrmlToken::Kind::Number && n1.text == "2",
        "tok: number 2 (comma is whitespace)");
  check(t.next().text == "2", "tok: number 2 again");
  check(t.next().text == "2", "tok: number 2 third");
  check(t.next().isPunct('}'), "tok: } punct");
  check(t.next().isPunct('}'), "tok: outer } punct");
  check(t.atEnd(), "tok: stream exhausted");

  // Quoted string unescape + isString flag.
  VrmlTokenizer s(R"(name "he said \"hi\" \\ end")");
  check(s.next().isWord("name"), "tok: bare word before string");
  VrmlToken str = s.next();
  check(str.kind == VrmlToken::Kind::String && str.isString,
        "tok: quoted string flagged isString");
  check(str.text == "he said \"hi\" \\ end",
        "tok: string escapes unescaped to: he said \"hi\" \\ end");

  // Header-line skip.
  VrmlTokenizer h("#X3D V4.0 utf8\nPROFILE Interchange\n",
                  /*firstLineIsHeader=*/true);
  check(h.next().isWord("PROFILE"), "tok: first header line skipped");
  check(h.next().isWord("Interchange"), "tok: PROFILE arg");
  check(h.atEnd(), "tok: body exhausted after header skip");
}

void testCollectFieldValue() {
  namespace b = x3d::codec::build;
  using x3d::codec::VrmlTokenizer;

  // SF scalar: one token.
  {
    VrmlTokenizer t("2.5 rest");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFFloat);
    check(w == "2.5", "collect SFFloat: one token");
    check(t.peek().isWord("rest"), "collect SFFloat: stops after one token");
    auto v = x3d::codec::parseValue(X3DFieldType::SFFloat, w);
    check(std::any_cast<float>(v) == 2.5f, "collect SFFloat: parses to 2.5");
  }
  // SF struct: N component tokens.
  {
    VrmlTokenizer t("1 2 3 next");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFVec3f);
    check(w == "1 2 3", "collect SFVec3f: three tokens joined");
    auto v = std::any_cast<SFVec3f>(
        x3d::codec::parseValue(X3DFieldType::SFVec3f, w));
    check(v.x == 1 && v.y == 2 && v.z == 3,
          "collect SFVec3f: parses to (1,2,3)");
    check(t.peek().isWord("next"), "collect SFVec3f: stops after 3 tokens");
  }
  // SFString: one quoted token, re-quoted for parseValue.
  {
    VrmlTokenizer t(R"("hello world" tail)");
    std::string w = b::collectFieldValue(t, X3DFieldType::SFString);
    auto v = std::any_cast<std::string>(
        x3d::codec::parseValue(X3DFieldType::SFString, w));
    check(v == "hello world", "collect SFString: 'hello world'");
    check(t.peek().isWord("tail"), "collect SFString: stops after the string");
  }
  // MFInt32: bracketed run.
  {
    VrmlTokenizer t("[ 1 2 3 4 ] after");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFInt32);
    auto v = std::any_cast<std::vector<int>>(
        x3d::codec::parseValue(X3DFieldType::MFInt32, w));
    check(v.size() == 4 && v[0] == 1 && v[3] == 4,
          "collect MFInt32: [1 2 3 4]");
    check(t.peek().isWord("after"), "collect MFInt32: consumes the brackets");
  }
  // MF*: a single bare element without brackets (VRML one-element MF).
  {
    VrmlTokenizer t("0 0 1 0 done");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFRotation);
    auto v = std::any_cast<std::vector<SFRotation>>(
        x3d::codec::parseValue(X3DFieldType::MFRotation, w));
    check(v.size() == 1 && v[0].z == 1, "collect MFRotation: bare 4-tuple");
    check(t.peek().isWord("done"),
          "collect MFRotation: bare element consumes exactly 4 tokens");
  }
  // MFString: quotes preserved per element.
  {
    VrmlTokenizer t(R"([ "a b" "c" ] x)");
    std::string w = b::collectFieldValue(t, X3DFieldType::MFString);
    auto v = std::any_cast<std::vector<std::string>>(
        x3d::codec::parseValue(X3DFieldType::MFString, w));
    check(v.size() == 2 && v[0] == "a b" && v[1] == "c",
          "collect MFString: preserves per-element quoting");
  }
}

void testXmlAdapterAndFrontDoor() {
  runtime::X3DDocument doc = buildSampleDocument();
  codec::XmlWriter writer;
  std::string xml = writer.writeDocument(doc);

  // Raw XmlReader (the baseline the adapter must reproduce).
  codec::XmlReader rawReader;
  runtime::X3DDocument viaRaw = rawReader.readDocument(xml);

  // Through the adapter.
  codec::XmlReaderAdapter adapter;
  check(adapter.encoding() == Encoding::XML, "adapter reports XML encoding");
  runtime::X3DDocument viaAdapter = adapter.readDocument(xml);

  // Through the front door with no hint (must sniff XML and pick the adapter).
  runtime::X3DDocument viaFrontDoor = codec::parseDocument(xml);

  // Re-serialize all three and compare against the baseline serialization.
  std::string sRaw = writer.writeDocument(viaRaw);
  std::string sAdapter = writer.writeDocument(viaAdapter);
  std::string sFront = writer.writeDocument(viaFrontDoor);
  check(sAdapter == sRaw, "XmlReaderAdapter == raw XmlReader (serialized)");
  check(sFront == sRaw, "parseDocument(no hint) == raw XmlReader (serialized)");

  // Structural spot checks on the adapter's output.
  check(viaAdapter.scene.rootNodes.size() == 1, "adapter: one root node");
  check(viaAdapter.scene.routes.size() == 1, "adapter: ROUTE survived");
  check(viaAdapter.head.meta.size() == 1 &&
            viaAdapter.head.meta[0].name == "title",
        "adapter: head meta survived");

  // DEF/USE identity: both shapes share one Appearance shared_ptr.
  auto root =
      std::dynamic_pointer_cast<Transform>(viaAdapter.scene.rootNodes[0]);
  check(static_cast<bool>(root), "adapter: root is a Transform");
  if (root) {
    const auto &kids = root->getChildren();
    check(kids.size() == 2, "adapter: transform has two children");
    if (kids.size() == 2) {
      auto s1 = std::dynamic_pointer_cast<Shape>(kids[0]);
      auto s2 = std::dynamic_pointer_cast<Shape>(kids[1]);
      check(s1 && s2 && s1->getAppearance() && s2->getAppearance() &&
                s1->getAppearance().get() == s2->getAppearance().get(),
            "adapter: DEF/USE Appearance is one shared_ptr");
    }
  }

  // The JSON reader has landed (WS-D): makeReader returns it, reporting JSON.
  {
    auto jsonReader = codec::makeReader(Encoding::JSON);
    check(jsonReader && jsonReader->encoding() == Encoding::JSON,
          "makeReader(JSON) returns a JSON reader (WS-D landed)");
  }

  // The VRML97 reader has landed (WS-C): makeReader returns it, reporting
  // VRML97.
  {
    auto vrmlReader = codec::makeReader(Encoding::VRML97);
    check(vrmlReader && vrmlReader->encoding() == Encoding::VRML97,
          "makeReader(VRML97) returns a VRML97 reader (WS-C landed)");
  }

  // Unknown-after-sniff throws.
  bool threw = false;
  try {
    codec::parseDocument("this is not any X3D encoding at all");
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "parseDocument throws on unrecognizable content");
}

// --- Lenient-read recovery fixes (BOM at the front door, nested same-quote
//     attribute recovery, set-only MFNode field guard). Each is exercised with
//     a hand-authored snippet (no external corpus file needed).
void testLenientReadFixes() {
  // Fix 1: a UTF-8-BOM-prefixed XML document parses (the front door strips the
  // BOM before sniffing/dispatch; without the fix the byte-level XmlLite parser
  // throws "expected root element" because the first byte is 0xEF, not '<').
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
    check(!threw, "BOM: UTF-8 BOM-prefixed XML parses without throwing");
    check(doc.scene.rootNodes.size() == 1,
          "BOM: BOM-prefixed document yields the one root node");
    check(doc.version == "3.0", "BOM: version attribute read past the BOM");

    // And the same content without a BOM is byte-identical in outcome (the
    // strip is a no-op when no BOM is present).
    runtime::X3DDocument plain = codec::parseDocument(xml.substr(bom.size()));
    check(plain.scene.rootNodes.size() == doc.scene.rootNodes.size(),
          "BOM: with/without BOM produce the same root count");
  }

  // Fix 3: an attribute value carrying an inner same-kind quote recovers. Here
  // a single-quoted meta content contains an apostrophe; the parser widens the
  // value to the last apostrophe before the tag close instead of throwing on a
  // bogus "missing =". (Warning is emitted to stderr; the document survives.)
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
    check(!threw, "attr-recovery: inner same-quote value does not throw");
    check(doc.scene.rootNodes.size() == 1,
          "attr-recovery: document past the bad attribute still parses");
    bool gotMeta = doc.head.meta.size() == 1 &&
                   doc.head.meta[0].name == "description" &&
                   doc.head.meta[0].content == "it's a test";
    check(gotMeta,
          "attr-recovery: widened value captured the full \"it's a test\"");

    // A well-formed single-quoted value (no inner quote) is byte-identical: the
    // recovery path must not fire for valid XML.
    const std::string ok =
        "<X3D profile='Interchange'><head>"
        "<meta name='description' content='plain text' />"
        "</head><Scene/></X3D>";
    runtime::X3DDocument okDoc = codec::parseDocument(ok, Encoding::XML);
    check(okDoc.head.meta.size() == 1 &&
              okDoc.head.meta[0].content == "plain text",
          "attr-recovery: well-formed value is unchanged (no false trigger)");
  }

  // Fix 2: the set-only MFNode field guard. A <Contour2D> with NO containerField
  // attribute under <NurbsTrimmedSurface> would route (by the no-slot fallback)
  // to the InputOnly sink "addTrimmingContour" — an MFNode with a setter but no
  // getter. Before the guard, attachChild called the empty get() thunk and
  // crashed with std::bad_function_call. Now it must parse (warn + skip / route
  // to a storage field) and yield a NurbsTrimmedSurface root.
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
    check(!threw,
          "NURBS guard: Contour2D w/o containerField does not crash/throw");
    check(doc.scene.rootNodes.size() == 1,
          "NURBS guard: the Shape root still parses");
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

// Fix 1 (parser-gap plan): gzip inflate before sniffing. Two committed gzip
// fixtures (each a gzip of an uncompressed sibling already exercised by the
// VRML97 / ClassicVRML reader tests) prove:
//   1. inflateGzip() returns text whose first line is a #VRML / #X3D header.
//   2. parseFile(".../x.gz") yields a document that round-trips to the same
//      serialization as parseFile(".../x") on the uncompressed sibling — i.e.
//      the gzip path is transparent to the rest of the front end.
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
    check(x3d::codec::isGzip(gz), "gzip: in-memory fixture has gzip magic");
    std::string text = x3d::codec::inflateGzip(gz);
    check(text == "#VRML V2.0 utf8\n",
          "gzip: inflateGzip recovers exact #VRML header line");
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
    check(!raw.empty() && x3d::codec::isGzip(raw),
          "gzip: " + c.label + " fixture present and gzip-magic");
    if (raw.empty())
      continue;

    // (1) inflateGzip header check.
    std::string inflated = x3d::codec::inflateGzip(raw);
    check(inflated.compare(0, c.headPfx.size(), c.headPfx) == 0,
          "gzip: " + c.label + " inflates to a " + c.headPfx + " header");

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
    check(!threw, "gzip: " + c.label + " parseFile did not throw");
    check(!sCompressed.empty(),
          "gzip: " + c.label + " parseFile produced a document");
    check(sCompressed == sPlain,
          "gzip: " + c.label +
              " parseFile(.gz) == parseFile(uncompressed sibling)");
  }
}

// A no-containerField child must attach to the parent field named by the
// CHILD's default containerField, not be dropped onto an arbitrary slot. The
// canonical case: <Contour2D/> under <NurbsTrimmedSurface> -> trimmingContour.
void testDefaultContainerFieldRouting() {
  std::cout << "----- default containerField routing -----\n";
  const std::string xml =
      "<X3D profile='Immersive' version='3.3'><Scene>"
      "<Shape><NurbsTrimmedSurface><Contour2D/></NurbsTrimmedSurface></Shape>"
      "</Scene></X3D>";
  runtime::X3DDocument doc = codec::parseDocument(xml, Encoding::XML);
  check(!doc.scene.rootNodes.empty(), "routing: document has a root");
  auto shape = doc.scene.rootNodes.empty()
                   ? nullptr
                   : std::dynamic_pointer_cast<Shape>(doc.scene.rootNodes[0]);
  check(shape != nullptr, "routing: root is a Shape");
  auto nts = shape ? std::dynamic_pointer_cast<NurbsTrimmedSurface>(
                         shape->getGeometry())
                   : nullptr;
  check(nts != nullptr, "routing: Shape.geometry is a NurbsTrimmedSurface");
  if (nts) {
    MFNode contours = nts->getTrimmingContour();
    check(contours.size() == 1,
          "routing: Contour2D landed in trimmingContour (size==1)");
    if (contours.size() == 1)
      check(std::dynamic_pointer_cast<Contour2D>(contours[0]) != nullptr,
            "routing: the trimmingContour child is a Contour2D");
  }
}

} // namespace

int main(int argc, char **argv) {
  const std::string dataDir = (argc > 1) ? argv[1] : "";
  std::cout << "===== WS-A: sniffing =====\n";
  testSniffing();
  std::cout << "===== WS-A: VrmlTokenizer =====\n";
  testTokenizer();
  std::cout << "===== WS-A: collectFieldValue =====\n";
  testCollectFieldValue();
  std::cout << "===== WS-A: XmlReaderAdapter + parseDocument =====\n";
  testXmlAdapterAndFrontDoor();
  std::cout << "===== Fix 1: gzip inflate before sniffing =====\n";
  testGzipInflate(dataDir);
  std::cout << "===== Lenient read: BOM + nested-quote + set-only guard =====\n";
  testLenientReadFixes();
  std::cout << "===== Default containerField routing =====\n";
  testDefaultContainerFieldRouting();

  if (failures == 0) {
    std::cout << "\nALL WS-A PARSE FRONT-END CHECKS PASSED\n";
    return 0;
  }
  std::cerr << "\n" << failures << " CHECK(S) FAILED\n";
  return 1;
}
