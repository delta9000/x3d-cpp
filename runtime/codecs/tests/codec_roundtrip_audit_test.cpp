// codec_roundtrip_audit_test.cpp
// AUD-CODEC-ROUNDTRIP (Unit 17) — comprehensive codec round-trip verification.
//
// For every encoding with read+write symmetry:
//   1. Parse fixture -> runtime model
//   2. Serialize back to the SAME encoding
//   3. Re-parse
//   4. Assert structural equivalence (node counts, DEFs, ROUTEs, Script sourceCode).
//
// Cross-format (VRML97 read-only):
//   VRML97 -> VrmlWriter (ClassicVRML) -> ClassicVrmlReader.
//
// Fixture directories are passed as argv[1]..argv[4] in order:
//   proto (XML .x3d), json (.json), x3dv (.x3dv), wrl (.wrl)
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "JsonReader.hpp"
#include "JsonWriter.hpp"
#include "Vrml97Reader.hpp"
#include "VrmlWriter.hpp"
#include "XmlReader.hpp"
#include "XmlWriter.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Script.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::runtime;

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

std::string g_protoDir;
std::string g_jsonDir;
std::string g_x3dvDir;
std::string g_wrlDir;

std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::cerr << "FAIL: cannot open fixture: " << path << "\n";
    ++failures;
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Recursively count every reachable node from the roots.
std::size_t countNodes(const std::vector<std::shared_ptr<X3DNode>> &roots) {
  std::size_t n = 0;
  std::vector<std::shared_ptr<X3DNode>> stack = roots;
  std::unordered_set<const X3DNode *> seen;
  while (!stack.empty()) {
    auto cur = stack.back();
    stack.pop_back();
    if (!cur || seen.count(cur.get())) continue;
    seen.insert(cur.get());
    ++n;
    for (const FieldInfo &f : cur->fields()) {
      if (!f.isReadable()) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto v = f.get(*cur);
        if (v.has_value()) {
          if (auto c = std::any_cast<std::shared_ptr<X3DNode>>(v))
            stack.push_back(c);
        }
      } else if (f.type == X3DFieldType::MFNode) {
        auto v = f.get(*cur);
        if (v.has_value()) {
          for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v))
            stack.push_back(c);
        }
      }
    }
  }
  return n;
}

std::size_t countScripts(const std::vector<std::shared_ptr<X3DNode>> &roots,
                         std::vector<std::string> *outSourceCode = nullptr) {
  std::size_t n = 0;
  std::vector<std::shared_ptr<X3DNode>> stack = roots;
  std::unordered_set<const X3DNode *> seen;
  while (!stack.empty()) {
    auto cur = stack.back();
    stack.pop_back();
    if (!cur || seen.count(cur.get())) continue;
    seen.insert(cur.get());
    if (cur->nodeTypeName() == "Script") {
      ++n;
      if (outSourceCode) {
        auto s = std::dynamic_pointer_cast<x3d::nodes::Script>(cur);
        if (s)
          outSourceCode->push_back(s->getSourceCode());
      }
    }
    for (const FieldInfo &f : cur->fields()) {
      if (!f.isReadable()) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto v = f.get(*cur);
        if (v.has_value()) {
          if (auto c = std::any_cast<std::shared_ptr<X3DNode>>(v))
            stack.push_back(c);
        }
      } else if (f.type == X3DFieldType::MFNode) {
        auto v = f.get(*cur);
        if (v.has_value()) {
          for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v))
            stack.push_back(c);
        }
      }
    }
  }
  return n;
}

std::size_t countDefs(const Scene &scene) { return scene.defs.size(); }
std::size_t countRoutes(const Scene &scene) { return scene.routes.size(); }

// Compare two documents for structural equivalence.
void compareDocs(const std::string &label, const X3DDocument &a,
                 const X3DDocument &b) {
  check(countNodes(a.scene.rootNodes) == countNodes(b.scene.rootNodes),
        label + ": total node count matches");
  check(a.scene.rootNodes.size() == b.scene.rootNodes.size(),
        label + ": root node count matches");
  check(countDefs(a.scene) == countDefs(b.scene),
        label + ": DEF count matches");
  check(countRoutes(a.scene) == countRoutes(b.scene),
        label + ": ROUTE count matches");

  // Script sourceCode comparison
  std::vector<std::string> scA, scB;
  countScripts(a.scene.rootNodes, &scA);
  countScripts(b.scene.rootNodes, &scB);
  check(scA.size() == scB.size(),
        label + ": Script node count matches");
  for (std::size_t i = 0; i < scA.size() && i < scB.size(); ++i) {
    check(scA[i] == scB[i],
          label + ": Script[" + std::to_string(i) + "] sourceCode matches");
  }
}

// DEF/USE identity check: every USE node must share its shared_ptr with the
// DEF node of the same name.
void checkDefUseIdentity(const std::string &label, const Scene &scene) {
  for (const auto &kv : scene.defs) {
    if (!kv.second) continue;
    // Walk every reachable node; if we find a node whose DEF name matches kv
    // but whose pointer differs, that's a clone instead of a USE.
    std::vector<std::shared_ptr<X3DNode>> stack = scene.rootNodes;
    std::unordered_set<const X3DNode *> seen;
    while (!stack.empty()) {
      auto cur = stack.back();
      stack.pop_back();
      if (!cur || seen.count(cur.get())) continue;
      seen.insert(cur.get());
      if (cur->getDEF() == kv.first && cur.get() != kv.second.get()) {
        check(false,
              label + ": DEF/USE identity broken for '" + kv.first + "'");
      }
      for (const FieldInfo &f : cur->fields()) {
        if (!f.isReadable()) continue;
        if (f.type == X3DFieldType::SFNode) {
          auto v = f.get(*cur);
          if (v.has_value()) {
            if (auto c = std::any_cast<std::shared_ptr<X3DNode>>(v))
              stack.push_back(c);
          }
        } else if (f.type == X3DFieldType::MFNode) {
          auto v = f.get(*cur);
          if (v.has_value()) {
            for (auto &c :
                 std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v))
              stack.push_back(c);
          }
        }
      }
    }
  }
}

} // namespace

int main(int argc, char **argv) {
  if (argc > 1) g_protoDir = argv[1];
  else          g_protoDir = "runtime/parse/tests/data/proto";
  if (argc > 2) g_jsonDir = argv[2];
  else          g_jsonDir = "runtime/parse/tests/data/json";
  if (argc > 3) g_x3dvDir = argv[3];
  else          g_x3dvDir = "runtime/parse/tests/data/x3dv";
  if (argc > 4) g_wrlDir  = argv[4];
  else          g_wrlDir  = "runtime/parse/tests/data/wrl";

  codec::XmlReader    xmlReader;
  codec::XmlWriter    xmlWriter;
  codec::JsonReader   jsonReader;
  codec::JsonWriter   jsonWriter;
  codec::ClassicVrmlReader cvReader;
  codec::VrmlWriter   vrmlWriter;
  codec::Vrml97Reader vrml97Reader;

  // ========================================================================
  // XML round-trip (proto fixtures)
  // ========================================================================
  {
    std::vector<std::string> fixtures = {
        g_protoDir + "/shapes.x3d",
        g_protoDir + "/main.x3d",
    };
    for (const auto &path : fixtures) {
      std::string name = path.substr(path.find_last_of("/\\") + 1);
      std::string text = readFile(path);
      if (text.empty()) continue;

      X3DDocument doc1 = xmlReader.readDocument(text);
      std::string out  = xmlWriter.writeDocument(doc1);
      X3DDocument doc2 = xmlReader.readDocument(out);

      compareDocs("XML-" + name, doc1, doc2);
      checkDefUseIdentity("XML-" + name, doc1.scene);
      checkDefUseIdentity("XML-" + name, doc2.scene);
    }
  }

  // ========================================================================
  // JSON round-trip
  // ========================================================================
  {
    std::vector<std::string> fixtures = {
        g_jsonDir + "/HelloWorld.json",
        g_jsonDir + "/spinning_cube.json",
    };
    for (const auto &path : fixtures) {
      std::string name = path.substr(path.find_last_of("/\\") + 1);
      std::string text = readFile(path);
      if (text.empty()) continue;

      X3DDocument doc1 = jsonReader.readDocument(text);
      std::string out  = jsonWriter.writeDocument(doc1);
      X3DDocument doc2 = jsonReader.readDocument(out);

      compareDocs("JSON-" + name, doc1, doc2);
      checkDefUseIdentity("JSON-" + name, doc1.scene);
      checkDefUseIdentity("JSON-" + name, doc2.scene);
    }
  }

  // ========================================================================
  // ClassicVRML round-trip (.x3dv fixtures)
  // ========================================================================
  {
    std::vector<std::string> fixtures = {
        g_x3dvDir + "/HelloWorld.x3dv",
        g_x3dvDir + "/ship.x3dv",
        g_x3dvDir + "/animated_transform.x3dv",
        g_x3dvDir + "/animated_patch.x3dv",
        g_x3dvDir + "/AddDynamicRoutes.x3dv",
    };
    for (const auto &path : fixtures) {
      std::string name = path.substr(path.find_last_of("/\\") + 1);
      std::string text = readFile(path);
      if (text.empty()) continue;

      X3DDocument doc1 = cvReader.readDocument(text);
      std::string out  = vrmlWriter.writeDocument(doc1);
      X3DDocument doc2 = cvReader.readDocument(out);

      compareDocs("X3DV-" + name, doc1, doc2);
      checkDefUseIdentity("X3DV-" + name, doc1.scene);
      checkDefUseIdentity("X3DV-" + name, doc2.scene);
    }
  }

  // ========================================================================
  // VRML97 -> ClassicVRML cross-format round-trip
  // ========================================================================
  {
    std::vector<std::string> fixtures = {
        g_wrlDir + "/NewShape.wrl",
        g_wrlDir + "/Rollers.wrl",
        g_wrlDir + "/TranslationTestScene.wrl",
    };
    for (const auto &path : fixtures) {
      std::string name = path.substr(path.find_last_of("/\\") + 1);
      std::string text = readFile(path);
      if (text.empty()) continue;

      X3DDocument doc1 = vrml97Reader.readDocument(text);
      std::string out  = vrmlWriter.writeDocument(doc1);
      X3DDocument doc2 = cvReader.readDocument(out);

      compareDocs("WRL->X3DV-" + name, doc1, doc2);
      checkDefUseIdentity("WRL->X3DV-" + name, doc1.scene);
      checkDefUseIdentity("WRL->X3DV-" + name, doc2.scene);
    }
  }

  // ========================================================================
  // Script sourceCode round-trip (XML path)
  // ========================================================================
  {
    std::string text = readFile(g_x3dvDir + "/AddDynamicRoutes.x3dv");
    if (!text.empty()) {
      X3DDocument doc1 = cvReader.readDocument(text);
      std::string xmlOut = xmlWriter.writeDocument(doc1);
      X3DDocument doc2 = xmlReader.readDocument(xmlOut);

      std::vector<std::string> sc1, sc2;
      countScripts(doc1.scene.rootNodes, &sc1);
      countScripts(doc2.scene.rootNodes, &sc2);
      check(sc1.size() == sc2.size(),
            "Script-x3dv->xml: Script count matches");
      for (std::size_t i = 0; i < sc1.size() && i < sc2.size(); ++i)
        check(sc1[i] == sc2[i],
              "Script-x3dv->xml: sourceCode[" + std::to_string(i) +
                  "] matches");
    }
  }

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all codec round-trip audit checks passed\n";
  return 0;
}
