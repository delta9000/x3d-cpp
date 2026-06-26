// tools/x3d-cli/gate_baseurl_roundtrip_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Regression for the cli-gate Inline/EXTERNPROTO baseUrl asymmetry (BUG-1).
//
// Background: the convert-roundtrip gate parses the source file with the file's
// directory as baseUrl (parseFile derives it), so relative Inline/EXTERNPROTO
// urls resolve and expand into rootNodes/children. The reparse of the serialized
// text used an EMPTY baseUrl, so the same externals stayed as unexpanded stubs
// (Inline node / scene-root ProtoInstance). The comparator then reported false
// "not equivalent" verdicts:
//   - Group vs Inline nodeTypeName mismatch
//   - rootNodes count mismatch (scene-root EXTERNPROTO instances dropped)
//   - nested children count mismatch
//
// The fix (cli_gate.cpp testConvert): reparse with the SAME baseUrl as the
// source parse, so external expansion is symmetric on both sides.
//
// This test reproduces both shapes (a load=TRUE Inline + a scene-root
// EXTERNPROTO instance) on disk and asserts that the gate's flow — parse(base)
// -> serialize -> reparse(base) -> sceneEquivalent — holds for xml and json.
// Without a baseUrl on the reparse, these assertions fail (the historical bug).
//
// Exit 0 on success; nonzero (with a message) on any failed assertion.
// ─────────────────────────────────────────────────────────────────────────────
#include "scene_equiv.hpp"
#include "x3d/sdk.hpp"
#include "Encoding.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace sdk = x3d::sdk;

static int failures = 0;

static void check(bool cond, const std::string &name) {
    if (!cond) {
        std::cerr << "FAIL: " << name << "\n";
        ++failures;
    } else {
        std::cout << "ok:   " << name << "\n";
    }
}

static void writeFile(const fs::path &p, const std::string &body) {
    std::ofstream f(p);
    f << body;
}

static std::string serialize(const sdk::X3DDocument &doc, const std::string &enc) {
    if (enc == "xml")  { sdk::XmlWriter  w; return w.writeDocument(doc); }
    if (enc == "json") { sdk::JsonWriter w; return w.writeDocument(doc); }
    throw std::runtime_error("unknown enc");
}

static x3d::codec::Encoding hint(const std::string &enc) {
    if (enc == "xml")  return x3d::codec::Encoding::XML;
    if (enc == "json") return x3d::codec::Encoding::JSON;
    return x3d::codec::Encoding::Unknown;
}

// Mirror testConvert(): parse(base) -> serialize(enc) -> reparse(enc, base) ->
// sceneEquivalent. Returns true when equivalent.
static bool roundtripEquiv(const std::string &srcPath, const std::string &enc,
                           std::string &why) {
    const std::string base = fs::path(srcPath).parent_path().string();
    sdk::X3DDocument src = sdk::parseFile(srcPath);
    const std::string serialized = serialize(src, enc);
    sdk::X3DDocument rt = sdk::parseDocument(serialized, hint(enc), base);
    return x3d_cli::sceneEquivalent(src.scene, rt.scene, why);
}

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path dir = fs::temp_directory_path() /
                   ("x3d_gate_baseurl_" + std::to_string(stamp));
    fs::create_directories(dir);

    // ── Fixture A: load=TRUE Inline pointing at a relative sibling. ──────────
    // The source parse expands the Inline's content into a synthetic Group;
    // the reparse must do the same (needs the baseUrl) for equivalence.
    writeFile(dir / "child.x3d", R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Shape><Box size="2 2 2"/></Shape>
  </Scene>
</X3D>
)");
    writeFile(dir / "inline_parent.x3d", R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Group>
      <Shape><Sphere radius="1"/></Shape>
    </Group>
    <Inline url='"child.x3d"' load="true"/>
  </Scene>
</X3D>
)");

    // ── Fixture B: a scene-root EXTERNPROTO instance + a sibling proto file. ──
    writeFile(dir / "proto_lib.x3d", R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <ProtoDeclare name="MyBox">
      <ProtoBody>
        <Shape><Box size="3 3 3"/></Shape>
      </ProtoBody>
    </ProtoDeclare>
  </Scene>
</X3D>
)");
    writeFile(dir / "extern_parent.x3d", R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <ExternProtoDeclare name="MyBox" url='"proto_lib.x3d#MyBox"'/>
    <ProtoInstance name="MyBox"/>
    <Shape><Cone bottomRadius="1" height="2"/></Shape>
  </Scene>
</X3D>
)");

    for (const std::string enc : {"xml", "json"}) {
        std::string why;
        bool a = roundtripEquiv((dir / "inline_parent.x3d").string(), enc, why);
        check(a, "Inline(load=TRUE) sibling roundtrip equivalent (" + enc + ")" +
                     (a ? "" : ": " + why));

        why.clear();
        bool b = roundtripEquiv((dir / "extern_parent.x3d").string(), enc, why);
        check(b, "scene-root EXTERNPROTO sibling roundtrip equivalent (" + enc +
                     ")" + (b ? "" : ": " + why));
    }

    std::error_code ec;
    fs::remove_all(dir, ec);

    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All gate baseUrl roundtrip checks passed.\n";
    return 0;
}
