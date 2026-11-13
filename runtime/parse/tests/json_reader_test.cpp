// json_reader_test.cpp
// Unit test for the X3D-JSON (.json) reader (runtime/parse/JsonReader), the
// inverse of codecs/JsonWriter — completing read/write symmetry for .json.
//
// Three layers of coverage:
//   1. Corpus fixtures (runtime/parse/tests/data/json/): X3D-JSON derived from
//      corpus X3D content (HelloWorld + a spinning-cube animation) via the
//      committed XML/ClassicVRML readers + JsonWriter. Each is parsed into an
//      X3DDocument and asserted on: profile/version, head metadata, node types
//      (via reflection), representative field values, DEF/USE shared identity,
//      and ROUTEs.
//   2. The JsonWriter -> JsonReader -> JsonWriter round-trip: re-serializing
//   the
//      parsed document must reproduce the fixture byte-for-byte (structural +
//      value fidelity, the strongest symmetry assertion).
//   3. Hand-authored snippets exercising specific shapes in isolation: SF/MF
//      numeric arrays, SFString vs MFString (with embedded-quote escaping),
//      bool/enum scalars, DEF/USE identity, head component/unit/meta, the
//      "ROUTE"/"IMPORT"/"EXPORT" scene members, and the parseDocument
//      front-door content-sniff dispatch (no extension).
//
// Driven entirely through reflection getters + the runtime model; no per-node
// codec code. Exit 0 on success, nonzero on any failed assertion.

#include "doctest/doctest.h"
#include "JsonReader.hpp"
#include "JsonWriter.hpp" // codecs/JsonWriter (round-trip symmetry)
#include "X3DParse.hpp"   // parseDocument front door (content-sniffs JSON)
#include "X3DRuntime.hpp"

// Concrete node types asserted on.
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace x3d;
using x3d::codec::Encoding;
using x3d::codec::JsonReader;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {
bool approx(double a, double b, double eps = 1e-4) {
  return std::fabs(a - b) <= eps;
}

// The fixtures dir is passed as X3D_TEST_DATA_DIR (CMake sets it).
std::string g_dataDir = X3D_TEST_DATA_DIR;

std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    FAIL_CHECK("cannot open fixture: " << path);
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}

// Recursively find the first node of a given concrete type in a subtree (over
// readable SFNode/MFNode fields). Used to reach deep nodes without hard-coding
// the exact containerField chain.
template <class T>
std::shared_ptr<T> findFirst(const std::shared_ptr<X3DNode> &n) {
  if (!n)
    return nullptr;
  if (auto hit = as<T>(n))
    return hit;
  for (const FieldInfo &f : n->fields()) {
    if (!f.isNode() || !f.isReadable())
      continue;
    std::any v = f.get(*n);
    if (f.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(v);
      if (auto hit = findFirst<T>(c))
        return hit;
    } else {
      auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
      for (const auto &c : vec)
        if (auto hit = findFirst<T>(c))
          return hit;
    }
  }
  return nullptr;
}

// Collect every node of a concrete type reachable from a subtree (so DEF/USE
// shared identity can be asserted by pointer equality).
template <class T>
void collect(const std::shared_ptr<X3DNode> &n,
             std::vector<std::shared_ptr<T>> &out) {
  if (!n)
    return;
  if (auto hit = as<T>(n))
    out.push_back(hit);
  for (const FieldInfo &f : n->fields()) {
    if (!f.isNode() || !f.isReadable())
      continue;
    std::any v = f.get(*n);
    if (f.type == X3DFieldType::SFNode) {
      collect<T>(std::any_cast<std::shared_ptr<X3DNode>>(v), out);
    } else {
      for (const auto &c :
           std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v))
        collect<T>(c, out);
    }
  }
}
// Corpus fixture 1: HelloWorld.json — header metadata, MFString, nested
// SF/MFNode children, DEF/USE shared Material identity.
TEST_CASE("hello world fixture") {
  std::string text = readFile(g_dataDir + "/HelloWorld.json");
  JsonReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  CHECK_MESSAGE((doc.profile == runtime::Profile::Immersive), "hello: profile Immersive");
  CHECK_MESSAGE((doc.version == "3.3"), "hello: version 3.3");
  CHECK_MESSAGE((doc.head.meta.size() >= 10), "hello: many <meta> recovered");

  // The first meta is title=HelloWorld.x3d (preserved order).
  bool sawTitle = false;
  bool sawEmbeddedQuote = false;
  for (const auto &m : doc.head.meta) {
    if (m.name == "title" && m.content == "HelloWorld.x3d")
      sawTitle = true;
    // One reference contains an embedded double-quote: it must survive JSON
    // string escaping intact through JsonLite.
    if (m.content.find("\"Hello,_World!\"") != std::string::npos)
      sawEmbeddedQuote = true;
  }
  CHECK_MESSAGE((sawTitle), "hello: meta title round-trips");
  CHECK_MESSAGE((sawEmbeddedQuote), "hello: meta content embedded-quote survives escaping");

  CHECK_MESSAGE((doc.scene.rootNodes.size() == 3), "hello: three root nodes");

  // DEF/USE: the DEF'd Material and the USE'd Material must be the same object.
  std::vector<std::shared_ptr<Material>> mats;
  for (const auto &root : doc.scene.rootNodes)
    collect<Material>(root, mats);
  CHECK_MESSAGE((mats.size() >= 2), "hello: at least two Material references reachable");
  if (mats.size() >= 2) {
    // All references named MaterialOffWhite resolve to one shared pointer.
    std::shared_ptr<Material> defd;
    for (const auto &m : mats)
      if (m->getDEF() == "MaterialOffWhite") {
        defd = m;
        break;
      }
    CHECK_MESSAGE((static_cast<bool>(defd)), "hello: DEF'd Material found");
    int sharing = 0;
    for (const auto &m : mats)
      if (m == defd)
        ++sharing;
    CHECK_MESSAGE((sharing >= 2), "hello: USE shares the DEF'd Material pointer (identity)");
  }

  // A representative value field on the DEF'd Material (diffuseColor).
  for (const auto &m : mats)
    if (m->getDEF() == "MaterialOffWhite") {
      auto c = m->getDiffuseColor();
      CHECK_MESSAGE((approx(c.r, 0.980392) && approx(c.g, 0.976471) &&
                approx(c.b, 0.964706)), "hello: Material.diffuseColor value parsed (SFColor array)");
      break;
    }
}
// Corpus fixture 2: spinning_cube.json — DEF/USE, ROUTEs, enums/bools, MF
// arrays (key/keyValue), TimeSensor + OrientationInterpolator animation wiring.
TEST_CASE("spinning cube fixture") {
  std::string text = readFile(g_dataDir + "/spinning_cube.json");
  JsonReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  CHECK_MESSAGE((doc.version == "4.0"), "cube: version 4.0");
  CHECK_MESSAGE((doc.scene.rootNodes.size() == 6), "cube: six root nodes");

  CHECK_MESSAGE((doc.scene.routes.size() == 2), "cube: two ROUTEs recovered");
  if (doc.scene.routes.size() == 2) {
    const auto &r0 = doc.scene.routes[0];
    CHECK_MESSAGE((r0.fromNode == "Clock" && r0.fromField == "fraction_changed" &&
              r0.toNode == "Spinner" && r0.toField == "set_fraction"), "cube: ROUTE 0 endpoints (Clock.fraction_changed -> Spinner)");
    // resolveRoutes() (run inside readDocument) bound the endpoints.
    CHECK_MESSAGE((!doc.scene.routes[0].from.expired()), "cube: ROUTE 0 source DEF resolved to a node");
    CHECK_MESSAGE((!doc.scene.routes[1].to.expired()), "cube: ROUTE 1 sink DEF (CubeTransform) resolved to a node");
  }

  // Bool + SFTime scalar on the TimeSensor.
  std::shared_ptr<TimeSensor> clock;
  for (const auto &root : doc.scene.rootNodes)
    if (auto t = findFirst<TimeSensor>(root)) {
      clock = t;
      break;
    }
  CHECK_MESSAGE((static_cast<bool>(clock)), "cube: TimeSensor reached");
  if (clock) {
    CHECK_MESSAGE((clock->getLoop() == true), "cube: TimeSensor.loop=true (SFBool)");
    CHECK_MESSAGE((approx(clock->getCycleInterval(), 4.0)), "cube: TimeSensor.cycleInterval=4 (SFTime)");
  }

  // MF arrays on the interpolator: key (MFFloat, 3) and keyValue (MFRotation,
  // 12 floats = 3 rotations).
  std::shared_ptr<OrientationInterpolator> spinner;
  for (const auto &root : doc.scene.rootNodes)
    if (auto s = findFirst<OrientationInterpolator>(root)) {
      spinner = s;
      break;
    }
  CHECK_MESSAGE((static_cast<bool>(spinner)), "cube: OrientationInterpolator reached");
  if (spinner) {
    CHECK_MESSAGE((spinner->getKey().size() == 3), "cube: key MFFloat has 3 entries");
    if (spinner->getKey().size() == 3)
      CHECK_MESSAGE((approx(spinner->getKey()[1], 0.5)), "cube: key[1]=0.5 (MF numeric array element)");
    CHECK_MESSAGE((spinner->getKeyValue().size() == 3), "cube: keyValue MFRotation has 3 rotations");
    if (spinner->getKeyValue().size() == 3)
      CHECK_MESSAGE((approx(spinner->getKeyValue()[2].angle, 6.28318, 1e-3)), "cube: keyValue[2].angle ~ 2pi (MFRotation element)");
  }
}
// Round-trip symmetry: JsonWriter(JsonReader(text)) == text, byte-for-byte,
// for both fixtures. This is the read/write symmetry assertion.
void testRoundTripByteStable(const std::string &name) {
  std::string text = readFile(g_dataDir + "/" + name);
  JsonReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);
  codec::JsonWriter writer;
  std::string again = writer.writeDocument(doc);
  CHECK_MESSAGE((again == text), ("round-trip: JsonWriter(JsonReader(" + name + ")) is byte-stable"));
}
// Hand-authored snippets: isolate specific value shapes and scene members.
TEST_CASE("hand authored snippets") {
  // SFString vs MFString (with an embedded escaped quote), bool, MF numeric.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "WorldInfo": { "@title": "a \"quoted\" title",
                         "@info": [ "line one", "two \"q\"" ] } }
      ]}}})";
    JsonReader reader;
    runtime::X3DDocument doc = reader.readDocument(j);
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "snippet: one WorldInfo");
    auto wi = as<WorldInfo>(doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(wi)), "snippet: root is WorldInfo");
    if (wi) {
      CHECK_MESSAGE((wi->getTitle() == "a \"quoted\" title"), "snippet: SFString with embedded quotes round-trips");
      auto info = wi->getInfo();
      CHECK_MESSAGE((info.size() == 2), "snippet: MFString has 2 elements");
      if (info.size() == 2) {
        CHECK_MESSAGE((info[0] == "line one"), "snippet: MFString[0]");
        CHECK_MESSAGE((info[1] == "two \"q\""), "snippet: MFString[1] embedded quote preserved");
      }
    }
  }

  // DEF/USE identity in hand-authored JSON.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "Shape": { "-appearance": [ { "Appearance": {
            "-material": [ { "Material": { "@DEF": "M",
                            "@diffuseColor": [1,0,0] } } ] } } ] } },
        { "Shape": { "-appearance": [ { "Appearance": {
            "-material": [ { "Material": { "@USE": "M" } } ] } } ] } }
      ]}}})";
    JsonReader reader;
    runtime::X3DDocument doc = reader.readDocument(j);
    std::vector<std::shared_ptr<Material>> mats;
    for (const auto &r : doc.scene.rootNodes)
      collect<Material>(r, mats);
    CHECK_MESSAGE((mats.size() == 2), "snippet: two Material references");
    if (mats.size() == 2) {
      CHECK_MESSAGE((mats[0] == mats[1]), "snippet: USE shares the DEF'd Material pointer");
      CHECK_MESSAGE((approx(mats[0]->getDiffuseColor().r, 1.0)), "snippet: DEF Material value visible through USE alias");
    }
  }

  // Head component/unit/meta + scene-scope ROUTE/IMPORT/EXPORT members.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "head":{
        "component":[ { "@name":"NURBS", "@level":1 } ],
        "unit":[ { "@category":"length", "@name":"km", "@conversionFactor":1000 } ],
        "meta":[ { "@name":"author", "@content":"corpus" } ] },
      "Scene":{
        "-children":[
          { "TimeSensor": { "@DEF":"T", "@loop": true } },
          { "Transform": { "@DEF":"X" } } ],
        "ROUTE":[ { "@fromNode":"T", "@fromField":"fraction_changed",
                    "@toNode":"X", "@toField":"rotation" } ],
        "IMPORT":[ { "@inlineDEF":"I", "@importedDEF":"Foo", "@AS":"Bar" } ],
        "EXPORT":[ { "@localDEF":"X", "@AS":"World" } ]
      }}})";
    JsonReader reader;
    runtime::X3DDocument doc = reader.readDocument(j);
    CHECK_MESSAGE((doc.head.components.size() == 1 &&
              doc.head.components[0].name == "NURBS"), "snippet: head component recovered");
    CHECK_MESSAGE((doc.head.units.size() == 1 &&
              approx(doc.head.units[0].conversionFactor, 1000.0)), "snippet: head unit conversionFactor recovered");
    CHECK_MESSAGE((doc.head.meta.size() == 1 && doc.head.meta[0].content == "corpus"), "snippet: head meta recovered");
    CHECK_MESSAGE((doc.scene.routes.size() == 1 && doc.scene.routes[0].fromNode == "T"), "snippet: scene ROUTE member recovered");
    CHECK_MESSAGE((doc.scene.imports.size() == 1 && doc.scene.imports[0].as == "Bar"), "snippet: scene IMPORT member recovered");
    CHECK_MESSAGE((doc.scene.exports.size() == 1 && doc.scene.exports[0].localDEF == "X"), "snippet: scene EXPORT member recovered");
  }
}
// Front-door dispatch: parseDocument with no hint must sniff JSON by content
// and produce the same document the explicit JsonReader does.
TEST_CASE("front door dispatch") {
  std::string text = readFile(g_dataDir + "/spinning_cube.json");
  CHECK_MESSAGE((codec::sniffByContent(text) == Encoding::JSON), "frontdoor: content sniffs as JSON");
  runtime::X3DDocument viaFront = codec::parseDocument(text);
  JsonReader reader;
  runtime::X3DDocument viaReader = reader.readDocument(text);
  CHECK_MESSAGE((viaFront.scene.rootNodes.size() == viaReader.scene.rootNodes.size() &&
            viaFront.scene.routes.size() == viaReader.scene.routes.size()), "frontdoor: parseDocument(JSON) matches explicit JsonReader");
}

} // namespace

TEST_CASE("round trip byte stable: HelloWorld.json") {
  testRoundTripByteStable("HelloWorld.json");
}

TEST_CASE("round trip byte stable: spinning_cube.json") {
  testRoundTripByteStable("spinning_cube.json");
}
