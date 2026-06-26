// runtime/parse/tests/proto_front_door_test.cpp
//
// Task 10: front-door PROTO expansion. Asserts that parseDocument/parseFile
// drive expandScene with the default local-file resolver, so:
//   - a local PROTO instance expands end-to-end through the in-memory front door
//   - an EXTERNPROTO instance resolves a sibling file relative to the parsed
//     file's directory (baseUrl derivation in parseFile)
//   - a mutually-referential EXTERN cycle terminates (thread_local file guard)
//     rather than recursing without bound.
//
// The fixtures dir (runtime/parse/tests/data/proto) is passed as argv[1] so the
// test runs from any working directory.
#include "X3DParse.hpp"

#include <cassert>
#include <string>

static std::string g_dataDir = "runtime/parse/tests/data/proto";

static bool hasRootNamed(const x3d::runtime::X3DDocument &doc,
                         const std::string &typeName) {
  for (const auto &n : doc.getScene().rootNodes)
    if (n && n->nodeTypeName() == typeName)
      return true;
  return false;
}

// Local PROTO expands end-to-end through the in-memory front door.
static void frontDoorLocalExpandTest() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P'/></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  assert(hasRootNamed(doc, "Box"));
  assert(doc.protoWarnings.empty());
}

// EXTERN resolves a sibling file via the default local-file resolver, with the
// base directory derived from the parsed file's path.
static void frontDoorExternResolveTest() {
  auto doc = x3d::codec::parseFile(g_dataDir + "/main.x3d");
  assert(hasRootNamed(doc, "Box"));
}

// A mutually-referential EXTERN cycle must terminate (bounded, no hang).
static void frontDoorExternCycleTest() {
  auto doc = x3d::codec::parseFile(g_dataDir + "/cycleA.x3d");
  // We only require that parsing returns at all. The cycle resolves to nothing
  // concrete, so no Box is spliced; the key property is termination.
  (void)doc;
}

int main(int argc, char **argv) {
  if (argc > 1)
    g_dataDir = argv[1];
  frontDoorLocalExpandTest();
  frontDoorExternResolveTest();
  frontDoorExternCycleTest();
  return 0;
}
