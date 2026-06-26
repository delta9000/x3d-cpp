// external_geometry_roundtrip_test.cpp
// Stage 2: EXTERNPROTO resolver + round-trip for ExternalGeometry.
// Gated on X3D_CPP_BUILD_EXT; registered in CMakeLists.txt ext block.
//
// What this tests:
//   1. Parse an X3D doc with <ExternProtoDeclare> + <ProtoInstance name="ExternalGeometry">
//      supplying install() as the resolver. After parseDocument, the ProtoInstance
//      should be expanded: an ExternalGeometry node in the graph with url=["chair.stl"].
//   2. The expanded node has nodeTypeName()=="ExternalGeometry" and url=["chair.stl"]
//      readable via the url field reflection.
//   3. scene.expandedSources maps the primary node back to the ProtoInstance.
//   4. Round-trip: re-serialize to XML and confirm the output contains
//      <ExternProtoDeclare name='ExternalGeometry'> and
//      <ProtoInstance name='ExternalGeometry'> with url fieldValue "chair.stl",
//      NOT a raw <ExternalGeometry> element (the expandedSources redirect).

#include "ExtResolver.hpp"        // x3d::runtime::ext::install()
#include "ExternalGeometry.hpp"   // x3d::runtime::ext::ExternalGeometry

// Core parse/write surface (reachable via x3d_cpp include path)
#include "X3DParse.hpp"           // x3d::codec::parseDocument, Encoding
#include "X3DCodecs.hpp"          // x3d::codec::XmlWriter

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

static const char* kDoc = R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile='Interchange' version='4.0'
     xmlns:xsd='http://www.w3.org/2001/XMLSchema-instance'
     xsd:noNamespaceSchemaLocation='https://www.web3d.org/specifications/x3d-4.0.xsd'>
  <Scene>
    <ExternProtoDeclare name='ExternalGeometry'
        url='"urn:x3d-cpp-gen:ext:ExternalGeometry" "ExternalGeometry.x3d"'>
      <field name='url'         type='MFString' accessType='initializeOnly'/>
      <field name='bboxCenter'  type='SFVec3f'  accessType='initializeOnly'/>
      <field name='bboxSize'    type='SFVec3f'  accessType='initializeOnly'/>
      <field name='contentType' type='SFString'  accessType='initializeOnly'/>
    </ExternProtoDeclare>
    <Shape>
      <ProtoInstance name='ExternalGeometry' containerField='geometry'>
        <fieldValue name='url' value='"chair.stl"'/>
      </ProtoInstance>
    </Shape>
  </Scene>
</X3D>
)";

int main() {
    using namespace x3d::runtime;
    using namespace x3d::runtime::ext;
    using namespace x3d::codec;

    // =========================================================================
    // TEST 1: parse with install() resolver → ProtoInstance expands
    // =========================================================================
    auto resolver = install();
    X3DDocument doc = parseDocument(kDoc, Encoding::XML, "", resolver);

    if (!doc.protoWarnings.empty()) {
        std::fprintf(stderr, "proto warnings (%zu):\n", doc.protoWarnings.size());
        for (const auto& w : doc.protoWarnings) {
            std::fprintf(stderr, "  [%s] %s\n", w.instanceName.c_str(), w.detail.c_str());
        }
    }
    assert(doc.protoWarnings.empty() && "Expected zero proto warnings after expansion with install()");

    // Scene root should have at least one node (Shape)
    assert(!doc.scene.rootNodes.empty() && "Expected at least one root node (Shape)");

    // =========================================================================
    // TEST 2: ExternalGeometry node in graph — find it in expandedSources
    // =========================================================================
    ExternalGeometry* eg_node = nullptr;
    for (const auto& kv : doc.scene.expandedSources) {
        X3DNode* primary = kv.first;
        if (primary && primary->nodeTypeName() == "ExternalGeometry") {
            eg_node = dynamic_cast<ExternalGeometry*>(primary);
            break;
        }
    }

    if (!eg_node) {
        std::fprintf(stderr, "expandedSources size: %zu\n", doc.scene.expandedSources.size());
        for (const auto& kv : doc.scene.expandedSources) {
            if (kv.first)
                std::fprintf(stderr, "  expanded: %s\n", kv.first->nodeTypeName().c_str());
        }
    }
    assert(eg_node != nullptr && "Expected an ExternalGeometry node in expandedSources");

    // url must carry the fieldValue override
    if (eg_node->getUrl().size() != 1) {
        std::fprintf(stderr, "url size: %zu\n", eg_node->getUrl().size());
        for (const auto& u : eg_node->getUrl())
            std::fprintf(stderr, "  url: [%s]\n", u.c_str());
    }
    assert(eg_node->getUrl().size() == 1 && "url must have exactly one entry");
    assert(eg_node->getUrl()[0] == "chair.stl" && "url[0] must be 'chair.stl'");

    // =========================================================================
    // TEST 3: expandedSources maps the primary back to its ProtoInstance
    // =========================================================================
    {
        auto it = doc.scene.expandedSources.find(eg_node);
        assert(it != doc.scene.expandedSources.end() && "expandedSources must map the EG node");
        assert(it->second.name == "ExternalGeometry" && "Instance name must be 'ExternalGeometry'");
    }

    // =========================================================================
    // TEST 4: round-trip via XmlWriter re-emits ProtoInstance, NOT raw node
    // =========================================================================
    {
        XmlWriter writer;
        std::string xml = writer.writeDocument(doc);

        // Must re-emit ExternProtoDeclare (preserved in scene.externProtoDeclarations)
        assert(xml.find("ExternProtoDeclare") != std::string::npos &&
               "Round-trip must re-emit <ExternProtoDeclare>");
        assert(xml.find("urn:x3d-cpp-gen:ext:ExternalGeometry") != std::string::npos &&
               "Round-trip must include the URN in ExternProtoDeclare url");

        // Must re-emit ProtoInstance (via expandedSources redirect)
        assert(xml.find("ProtoInstance") != std::string::npos &&
               "Round-trip must re-emit <ProtoInstance> (not raw <ExternalGeometry>)");

        // Must re-emit fieldValue for url
        assert(xml.find("fieldValue") != std::string::npos &&
               "Round-trip must re-emit <fieldValue name='url'>");

        // Raw ExternalGeometry opening tag would look like "<ExternalGeometry "
        // "<ExternalGeometry/>" or "<ExternalGeometry\n". This should NOT appear
        // (expandedSources redirects to ProtoInstance).
        // Note: closing tags ("</ExternalGeometry>") start with "</" so
        // find("<ExternalGeometry") already excludes them. We additionally
        // require that the character immediately after the tag name is a
        // delimiter (space, '/', '\n', '\t', or '>') so we don't accidentally
        // match a hypothetical "<ExternalGeometryFoo>" element name.
        {
            bool has_raw_tag = false;
            // "<ExternalGeometry" is 17 chars; the 18th must be a tag delimiter.
            static constexpr std::string_view kTag = "<ExternalGeometry";
            std::size_t pos = 0;
            while ((pos = xml.find(kTag, pos)) != std::string::npos) {
                char next = (pos + kTag.size() < xml.size()) ? xml[pos + kTag.size()] : '\0';
                if (next == ' ' || next == '/' || next == '\n' || next == '\t' || next == '>') {
                    has_raw_tag = true;
                    break;
                }
                pos += kTag.size();
            }
            if (has_raw_tag) {
                std::fprintf(stderr, "Round-trip XML:\n%s\n", xml.c_str());
            }
            assert(!has_raw_tag && "Round-trip must NOT emit raw <ExternalGeometry> element");
        }
    }

    // =========================================================================
    // TEST 5a: FIREWALL-DEGRADATION — parse the SAME doc WITHOUT install().
    //   The ExternProto must stay unexpanded, an UnresolvedExtern warning must
    //   be recorded, and the round-trip must still emit <ExternProtoDeclare> +
    //   <ProtoInstance> (not a raw <ExternalGeometry> element).
    // =========================================================================
    {
        // Parse with a no-op resolver (returns nullptr for everything) so the
        // URN is unresolvable — simulates the "no ext module installed" case.
        ProtoDeclarationResolver no_resolver =
            [](const std::vector<std::string>&, const std::string&)
                -> std::shared_ptr<x3d::runtime::ProtoDeclaration> { return nullptr; };
        X3DDocument doc2 = parseDocument(kDoc, Encoding::XML, "", no_resolver);

        // Must record at least one UnresolvedExtern warning.
        bool has_unresolved = false;
        for (const auto& w : doc2.protoWarnings) {
            if (w.kind == ProtoWarning::Kind::UnresolvedExtern &&
                w.instanceName == "ExternalGeometry") {
                has_unresolved = true;
                break;
            }
        }
        if (!has_unresolved) {
            std::fprintf(stderr, "DEGRADATION: expected UnresolvedExtern warning; got %zu warnings:\n",
                         doc2.protoWarnings.size());
            for (const auto& w : doc2.protoWarnings)
                std::fprintf(stderr, "  [%s] %s\n", w.instanceName.c_str(), w.detail.c_str());
        }
        assert(has_unresolved &&
               "Degradation (no install): must record UnresolvedExtern for 'ExternalGeometry'");

        // Must NOT crash; round-trip must preserve ExternProtoDeclare + ProtoInstance.
        XmlWriter writer2;
        std::string xml2 = writer2.writeDocument(doc2);

        assert(xml2.find("ExternProtoDeclare") != std::string::npos &&
               "Degradation round-trip must re-emit <ExternProtoDeclare>");
        assert(xml2.find("ProtoInstance") != std::string::npos &&
               "Degradation round-trip must re-emit <ProtoInstance>");

        // Must NOT emit a raw <ExternalGeometry> opening tag.
        {
            bool raw2 = false;
            static constexpr std::string_view kTag2 = "<ExternalGeometry";
            std::size_t pos2 = 0;
            while ((pos2 = xml2.find(kTag2, pos2)) != std::string::npos) {
                char next2 = (pos2 + kTag2.size() < xml2.size()) ? xml2[pos2 + kTag2.size()] : '\0';
                if (next2 == ' ' || next2 == '/' || next2 == '\n' || next2 == '\t' || next2 == '>') {
                    raw2 = true;
                    break;
                }
                pos2 += kTag2.size();
            }
            if (raw2) std::fprintf(stderr, "Degradation round-trip XML:\n%s\n", xml2.c_str());
            assert(!raw2 && "Degradation round-trip must NOT emit raw <ExternalGeometry> element");
        }

        std::fprintf(stderr, "TEST 5a PASS: no-install path degraded gracefully (UnresolvedExtern + clean round-trip)\n");
    }

    // =========================================================================
    // TEST 5b: DELEGATION — install() resolver delegates non-matching URNs to base.
    //   Use a stub base resolver that records whether it was called, then confirm
    //   a non-ExternalGeometry URN reaches the stub.
    // =========================================================================
    {
        bool stub_called = false;
        // Stub resolver: records invocations, never returns a real decl.
        auto stub = [&stub_called](const std::vector<std::string>& /*urls*/,
                                   const std::string& /*baseUrl*/)
            -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
            stub_called = true;
            return nullptr;
        };

        auto delegating_resolver = install(stub);

        // Call with a non-matching URN — must reach the stub.
        std::vector<std::string> other_urls = {"urn:some:other:proto", "OtherProto.x3d"};
        auto result = delegating_resolver(other_urls, "");
        assert(stub_called && "Delegation: non-matching URN must reach the stub base resolver");
        assert(result == nullptr && "Delegation: stub returns nullptr for unknown URN");

        // Call with our matching URN — must NOT reach the stub.
        stub_called = false;
        std::vector<std::string> our_urls = {kExternalGeometryUrn};
        auto eg_decl = delegating_resolver(our_urls, "");
        assert(!stub_called && "Delegation: matching URN must be intercepted, not forwarded to stub");
        assert(eg_decl != nullptr && "Delegation: matching URN must return the EG ProtoDeclaration");
        assert(eg_decl->name == "ExternalGeometry" &&
               "Delegation: returned decl must be 'ExternalGeometry'");

        std::fprintf(stderr, "TEST 5b PASS: install() delegates non-matching URN to base; intercepts own URN\n");
    }

    return 0;
}
