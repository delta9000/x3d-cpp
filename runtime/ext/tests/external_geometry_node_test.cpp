// external_geometry_node_test.cpp
// Stage 1: X3DNode identity + reflection for ExternalGeometry.
// Gated on X3D_CPP_BUILD_EXT; registered in CMakeLists.txt ext block.
#include "ExternalGeometry.hpp"

#include <any>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

int main() {
    using namespace x3d::runtime::ext;

    // =========================================================================
    // TEST 1: basic construction + typed accessors
    // =========================================================================
    {
        ExternalGeometry eg;

        // Default field values
        assert(eg.getUrl().empty());
        assert(eg.getBboxCenter().x == 0.0f);
        assert(eg.getBboxCenter().y == 0.0f);
        assert(eg.getBboxCenter().z == 0.0f);
        assert(eg.getBboxSize().x == -1.0f);
        assert(eg.getBboxSize().y == -1.0f);
        assert(eg.getBboxSize().z == -1.0f);
        assert(eg.getContentType().empty());

        // Typed setters
        eg.setUrl({"chair.stl", "chair.x3d"});
        eg.setBboxCenter({1.0f, 2.0f, 3.0f});
        eg.setBboxSize({10.0f, 5.0f, 8.0f});
        eg.setContentType("model/x.stl-binary");

        assert(eg.getUrl().size() == 2);
        assert(eg.getUrl()[0] == "chair.stl");
        assert(eg.getUrl()[1] == "chair.x3d");
        assert(eg.getBboxCenter().x == 1.0f);
        assert(eg.getBboxCenter().y == 2.0f);
        assert(eg.getBboxCenter().z == 3.0f);
        assert(eg.getBboxSize().x == 10.0f);
        assert(eg.getContentType() == "model/x.stl-binary");
    }

    // =========================================================================
    // TEST 2: nodeTypeName / defaultContainerField
    // =========================================================================
    {
        ExternalGeometry eg;
        assert(eg.nodeTypeName() == "ExternalGeometry");
        assert(eg.defaultContainerField() == "geometry");
    }

    // =========================================================================
    // TEST 3: fields() reflection — correct count and types
    // =========================================================================
    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        bool has_url = false, has_bboxCenter = false, has_bboxSize = false, has_contentType = false;
        for (const auto& fi : ft) {
            if (fi.x3dName == "url") {
                assert(fi.type == X3DFieldType::MFString);
                assert(fi.access == AccessType::InitializeOnly);
                has_url = true;
            }
            if (fi.x3dName == "bboxCenter") {
                assert(fi.type == X3DFieldType::SFVec3f);
                assert(fi.access == AccessType::InitializeOnly);
                has_bboxCenter = true;
            }
            if (fi.x3dName == "bboxSize") {
                assert(fi.type == X3DFieldType::SFVec3f);
                assert(fi.access == AccessType::InitializeOnly);
                has_bboxSize = true;
            }
            if (fi.x3dName == "contentType") {
                assert(fi.type == X3DFieldType::SFString);
                assert(fi.access == AccessType::InitializeOnly);
                has_contentType = true;
            }
        }
        assert(has_url && "fields() must contain 'url'");
        assert(has_bboxCenter && "fields() must contain 'bboxCenter'");
        assert(has_bboxSize && "fields() must contain 'bboxSize'");
        assert(has_contentType && "fields() must contain 'contentType'");
    }

    // =========================================================================
    // TEST 4: reflection get/set thunks — read/write url via FieldTable
    // =========================================================================
    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        const FieldInfo* url_fi = nullptr;
        for (const auto& fi : ft) {
            if (fi.x3dName == "url") { url_fi = &fi; break; }
        }
        assert(url_fi != nullptr && "url FieldInfo must exist");
        assert(url_fi->get && "url FieldInfo must have get thunk");
        assert(url_fi->set && "url FieldInfo must have set thunk");

        // Write via reflection
        MFString via_refl = {"refl.stl"};
        url_fi->set(eg, std::any(via_refl));
        // Read back via typed getter
        assert(eg.getUrl().size() == 1);
        assert(eg.getUrl()[0] == "refl.stl");
        // Read back via reflection
        auto got = std::any_cast<MFString>(url_fi->get(eg));
        assert(got[0] == "refl.stl");
    }

    // =========================================================================
    // TEST 5: reflection get/set for bboxCenter and bboxSize
    // =========================================================================
    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        auto find = [&](const std::string& name) -> const FieldInfo* {
            for (const auto& fi : ft) if (fi.x3dName == name) return &fi;
            return nullptr;
        };

        const FieldInfo* bc = find("bboxCenter");
        const FieldInfo* bs = find("bboxSize");
        assert(bc && bs);

        bc->set(eg, std::any(SFVec3f{4.0f, 5.0f, 6.0f}));
        bs->set(eg, std::any(SFVec3f{20.0f, 30.0f, 40.0f}));

        auto got_c = std::any_cast<SFVec3f>(bc->get(eg));
        auto got_s = std::any_cast<SFVec3f>(bs->get(eg));
        assert(got_c.x == 4.0f && got_c.y == 5.0f && got_c.z == 6.0f);
        assert(got_s.x == 20.0f && got_s.y == 30.0f && got_s.z == 40.0f);
    }

    // =========================================================================
    // TEST 6: accept(NodeVisitor&) — enter/leave called in order
    // =========================================================================
    {
        class TrackVisitor : public NodeVisitor {
        public:
            int enters = 0, leaves = 0;
            bool enter(const X3DNode&) override { ++enters; return true; }
            void leave(const X3DNode&) override { ++leaves; }
        };

        ExternalGeometry eg;
        TrackVisitor tv;
        eg.accept(tv);
        assert(tv.enters == 1 && "accept must call enter once");
        assert(tv.leaves == 1 && "accept must call leave once");
    }

    // =========================================================================
    // TEST 7: DEF/USE inherited from X3DNode base
    // =========================================================================
    {
        ExternalGeometry eg;
        eg.setDEF("MyEG");
        assert(eg.getDEF() == "MyEG");
    }

    return 0;
}
