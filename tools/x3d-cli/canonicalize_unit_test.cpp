// tools/x3d-cli/canonicalize_unit_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Unit tests for CanonicalXmlWriter (X3DC14N).
//
// Tests:
//   1.  XML prolog present.
//   2.  DOCTYPE line present (version-specific).
//   3.  Single-quote attribute delimiters.
//   4.  Attributes sorted alphabetically within each element.
//   5.  Minimal float format (0.8 not 0.800000, 0 not 0.0).
//   6.  Integer attrs without .0 (height='3' not height='3.0').
//   7.  xmlns:xsd and xsd:noNamespaceSchemaLocation on <X3D>.
//   8.  DEF/USE preserved across round-trip.
//   9.  Idempotence: canonicalize(canonicalize(x)) == canonicalize(x).
//  10.  2-space-per-level indentation (no tabs).
//  11.  Default writer (XmlWriter) output is byte-identical before/after
//       CanonicalXmlWriter is used (isolation proof).
//  12.  meta content/name attrs appear in alphabetical order (content < name).
//  13.  &apos; escaping in single-quote context.
// ─────────────────────────────────────────────────────────────────────────────
#include "CanonicalXmlWriter.hpp"
#include "XmlWriter.hpp"
#include "x3d/sdk.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace sdk = x3d::sdk;
using x3d::codec::CanonicalXmlWriter;
using x3d::codec::XmlWriter;

// ── helpers ───────────────────────────────────────────────────────────────────

static int failures = 0;

static void check(const char *name, bool cond) {
    if (cond) {
        std::cout << "ok:   " << name << "\n";
    } else {
        std::cout << "FAIL: " << name << "\n";
        ++failures;
    }
}

static void checkContains(const char *name, const std::string &haystack,
                          const std::string &needle) {
    check(name, haystack.find(needle) != std::string::npos);
}

static void checkNotContains(const char *name, const std::string &haystack,
                              const std::string &needle) {
    check(name, haystack.find(needle) == std::string::npos);
}

// Build a minimal X3DDocument for testing.
static sdk::X3DDocument makeMinimalDoc(const std::string &version = "4.0") {
    sdk::X3DDocument doc;
    doc.version = version;
    doc.profile = sdk::Profile::Interchange;
    x3d::runtime::Meta m1, m2;
    m1.name = "title"; m1.content = "test.x3d";
    m2.name = "description"; m2.content = "Unit test fixture.";
    doc.head.meta.push_back(m1);
    doc.head.meta.push_back(m2);
    return doc;
}

// Parse a small X3D-XML string into a document.
static sdk::X3DDocument parseXml(const std::string &xml) {
    return sdk::parseDocument(xml, sdk::Encoding::XML);
}

// Write a temporary file and return its path.
static std::string writeTempFile(const std::string &content, const std::string &suffix) {
    std::string path = (fs::temp_directory_path() /
                        ("x3d_canon_test_" + suffix)).string();
    std::ofstream f(path);
    f << content;
    return path;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

// T1: XML prolog.
static void testXmlProlog() {
    auto doc = makeMinimalDoc("3.3");
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    checkContains("T1-prolog", out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
}

// T2: DOCTYPE line is present with correct version.
static void testDoctype() {
    auto doc32 = makeMinimalDoc("3.2");
    CanonicalXmlWriter w;
    std::string out32 = w.writeDocument(doc32);
    checkContains("T2-doctype-3.2", out32,
                  "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 3.2//EN\"");

    auto doc40 = makeMinimalDoc("4.0");
    std::string out40 = w.writeDocument(doc40);
    checkContains("T2-doctype-4.0", out40,
                  "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 4.0//EN\"");
}

// T3: Single-quote attribute delimiters (not double-quotes).
static void testSingleQuotes() {
    auto doc = makeMinimalDoc("3.3");
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    // <X3D profile='Interchange' ...> — should use single quotes in attrs.
    checkContains("T3-single-quote-profile", out, "profile='Interchange'");
    checkNotContains("T3-no-double-quote-profile", out, "profile=\"Interchange\"");
}

// T4: Alphabetical attribute order.
static void testAttrOrder() {
    // <meta content='...' name='...'/>  — content < name alphabetically.
    auto doc = makeMinimalDoc("3.3");
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    // Find the meta line and check content comes before name.
    std::size_t contentPos = out.find("content='test.x3d'");
    std::size_t namePos    = out.find("name='title'");
    check("T4-meta-content-before-name", contentPos != std::string::npos &&
                                          namePos    != std::string::npos &&
                                          contentPos < namePos);
    // <X3D profile version xmlns:xsd xsd:noNamespace...> — profile < version < xmlns < xsd.
    std::size_t profPos    = out.find("profile='");
    std::size_t versionPos = out.find("version='");
    std::size_t xmlnsPos   = out.find("xmlns:xsd=");
    std::size_t xsdPos     = out.find("xsd:noNamespace");
    check("T4-X3D-attrs-sorted", profPos < versionPos &&
                                  versionPos < xmlnsPos &&
                                  xmlnsPos < xsdPos);
}

// T5: Minimal float format (use shortest representation).
static void testMinimalFloats() {
    // Parse a small X3D with a known float value (0.8 should stay as "0.8").
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head/><Scene>)"
        R"(<Shape><Appearance><Material diffuseColor='0.8 0 0'/></Appearance></Shape>)"
        R"(</Scene></X3D>)";
    auto doc = parseXml(xml);
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    // 0.8 in float binary IS exactly representable as 0.8, so shortest = "0.8".
    checkContains("T5-minimal-float-0.8", out, "diffuseColor='0.8 0 0'");
    // 0 should be "0" not "0.0".
    checkNotContains("T5-no-trailing-zero", out, "0.0 0.0");
}

// T6: Integer values without .0 (e.g. size='3 4 5' not '3.0 4.0 5.0').
// Use non-default size (default Box size is 2 2 2 so we use 3 4 5).
static void testIntegerAttrs() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head/><Scene>)"
        R"(<Shape><Box size='3 4 5'/></Shape>)"
        R"(</Scene></X3D>)";
    auto doc = parseXml(xml);
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    checkContains("T6-integer-size", out, "size='3 4 5'");
    checkNotContains("T6-no-float-size", out, "size='3.0");
}

// T7: xmlns:xsd and xsd:noNamespaceSchemaLocation on <X3D>.
static void testSchemaAttrs() {
    auto doc = makeMinimalDoc("4.0");
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    checkContains("T7-xmlns-xsd", out, "xmlns:xsd='http://www.w3.org/2001/XMLSchema-instance'");
    checkContains("T7-schema-loc", out, "xsd:noNamespaceSchemaLocation='https://www.web3d.org/specifications/x3d-4.0.xsd'");
}

// T8: DEF/USE preserved.
static void testDefUse() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head/><Scene>)"
        R"(<Shape DEF='S1'><Appearance><Material diffuseColor='1 0 0'/></Appearance></Shape>)"
        R"(<Shape USE='S1'/>)"
        R"(</Scene></X3D>)";
    auto doc = parseXml(xml);
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    checkContains("T8-DEF", out, "DEF='S1'");
    checkContains("T8-USE", out, "USE='S1'");
}

// T9: Idempotence — canonicalize(canonicalize(x)) == canonicalize(x).
static void testIdempotence() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head>)"
        R"(<meta content='test' name='title'/>)"
        R"(</head><Scene>)"
        R"(<Shape><Appearance><Material diffuseColor='0.8 0.2 0.5'/></Appearance><Box/></Shape>)"
        R"(</Scene></X3D>)";
    auto doc1 = parseXml(xml);
    CanonicalXmlWriter w;
    std::string canon1 = w.writeDocument(doc1);

    // Re-parse the canonical output and canonicalize again.
    auto doc2 = parseXml(canon1);
    std::string canon2 = w.writeDocument(doc2);

    check("T9-idempotence", canon1 == canon2);
    if (canon1 != canon2) {
        // Print the first diff for debugging.
        std::istringstream s1(canon1), s2(canon2);
        std::string l1, l2;
        int lineno = 0;
        while (std::getline(s1, l1) && std::getline(s2, l2)) {
            ++lineno;
            if (l1 != l2) {
                std::cout << "  first diff at line " << lineno << ":\n";
                std::cout << "    c1: " << l1 << "\n";
                std::cout << "    c2: " << l2 << "\n";
                break;
            }
        }
    }
}

// T10: 2-space indentation (no tabs).
static void testIndentation() {
    auto doc = makeMinimalDoc("3.3");
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    checkNotContains("T10-no-tabs", out, "\t");
    // head is at depth 1 → 2 spaces.
    checkContains("T10-2space-head", out, "  <head>");
    // meta is at depth 2 → 4 spaces.
    checkContains("T10-4space-meta", out, "    <meta ");
}

// T11: XmlWriter (default) output byte-identical before vs after CanonicalXmlWriter use.
// This proves the canonical mode does NOT modify the default writer's code path.
static void testDefaultWriterUnchanged() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head/><Scene>)"
        R"(<Shape><Box size='1 2 3'/></Shape>)"
        R"(</Scene></X3D>)";
    auto doc = parseXml(xml);

    // Run default writer before canonical.
    XmlWriter defaultWriter;
    std::string before = defaultWriter.writeDocument(doc);

    // Run canonical.
    CanonicalXmlWriter canonWriter;
    std::string canon = canonWriter.writeDocument(doc);
    (void)canon;

    // Run default writer after canonical.
    std::string after = defaultWriter.writeDocument(doc);

    check("T11-default-writer-unchanged", before == after);
}

// T12: meta attrs in canonical order (content < name).
static void testMetaOrder() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head>)"
        R"(<meta name='generator' content='X3D-Edit'/>)"
        R"(</head><Scene/></X3D>)";
    auto doc = parseXml(xml);
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    // canonical: <meta content='X3D-Edit' name='generator'/>
    std::size_t contentPos = out.find("content='X3D-Edit'");
    std::size_t namePos    = out.find("name='generator'");
    check("T12-meta-content-before-name",
          contentPos != std::string::npos &&
          namePos    != std::string::npos &&
          contentPos < namePos);
}

// T13: &apos; escaping for apostrophes in attribute values.
static void testAposEscaping() {
    const std::string xml =
        R"(<?xml version="1.0" encoding="UTF-8"?>)"
        R"(<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.3//EN" "https://www.web3d.org/specifications/x3d-3.3.dtd">)"
        R"(<X3D profile='Interchange' version='3.3'><head>)"
        R"(<meta name="apostrophe-test" content="it&apos;s here"/>)"
        R"(</head><Scene/></X3D>)";
    auto doc = parseXml(xml);
    CanonicalXmlWriter w;
    std::string out = w.writeDocument(doc);
    // The apostrophe in "it's here" must be escaped as &apos; (single-quote context).
    checkContains("T13-apos-escape", out, "&apos;");
    checkNotContains("T13-no-bare-apos-in-attr", out, "content='it's here'");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== CanonicalXmlWriter unit tests ===\n\n";

    testXmlProlog();
    testDoctype();
    testSingleQuotes();
    testAttrOrder();
    testMinimalFloats();
    testIntegerAttrs();
    testSchemaAttrs();
    testDefUse();
    testIdempotence();
    testIndentation();
    testDefaultWriterUnchanged();
    testMetaOrder();
    testAposEscaping();

    std::cout << "\n";
    if (failures == 0) {
        std::cout << "All canonical unit tests passed.\n";
        return 0;
    } else {
        std::cout << failures << " test(s) FAILED.\n";
        return 1;
    }
}
