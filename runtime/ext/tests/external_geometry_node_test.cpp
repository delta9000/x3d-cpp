// ExternalGeometry identity and reflection.
#include "doctest/doctest.h"
#include "ExternalGeometry.hpp"

#include <any>
#include <cmath>
#include <string>
#include <vector>

TEST_CASE("external_geometry_node_test") {
    using namespace x3d::runtime::ext;

    // basic construction + typed accessors

    {
        ExternalGeometry eg;

        // Default field values
        REQUIRE((eg.getUrl().empty()));
        REQUIRE((eg.getBboxCenter().x == 0.0f));
        REQUIRE((eg.getBboxCenter().y == 0.0f));
        REQUIRE((eg.getBboxCenter().z == 0.0f));
        REQUIRE((eg.getBboxSize().x == -1.0f));
        REQUIRE((eg.getBboxSize().y == -1.0f));
        REQUIRE((eg.getBboxSize().z == -1.0f));
        REQUIRE((eg.getContentType().empty()));

        // Typed setters
        eg.setUrl({"chair.stl", "chair.x3d"});
        eg.setBboxCenter({1.0f, 2.0f, 3.0f});
        eg.setBboxSize({10.0f, 5.0f, 8.0f});
        eg.setContentType("model/x.stl-binary");

        REQUIRE((eg.getUrl().size() == 2));
        REQUIRE((eg.getUrl()[0] == "chair.stl"));
        REQUIRE((eg.getUrl()[1] == "chair.x3d"));
        REQUIRE((eg.getBboxCenter().x == 1.0f));
        REQUIRE((eg.getBboxCenter().y == 2.0f));
        REQUIRE((eg.getBboxCenter().z == 3.0f));
        REQUIRE((eg.getBboxSize().x == 10.0f));
        REQUIRE((eg.getContentType() == "model/x.stl-binary"));
    }

    // nodeTypeName / defaultContainerField

    {
        ExternalGeometry eg;
        REQUIRE((eg.nodeTypeName() == "ExternalGeometry"));
        REQUIRE((eg.defaultContainerField() == "geometry"));
    }

    // fields() reflection — correct count and types

    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        bool has_url = false, has_bboxCenter = false, has_bboxSize = false, has_contentType = false;
        for (const auto& fi : ft) {
            if (fi.x3dName == "url") {
                REQUIRE((fi.type == X3DFieldType::MFString));
                REQUIRE((fi.access == AccessType::InitializeOnly));
                has_url = true;
            }
            if (fi.x3dName == "bboxCenter") {
                REQUIRE((fi.type == X3DFieldType::SFVec3f));
                REQUIRE((fi.access == AccessType::InitializeOnly));
                has_bboxCenter = true;
            }
            if (fi.x3dName == "bboxSize") {
                REQUIRE((fi.type == X3DFieldType::SFVec3f));
                REQUIRE((fi.access == AccessType::InitializeOnly));
                has_bboxSize = true;
            }
            if (fi.x3dName == "contentType") {
                REQUIRE((fi.type == X3DFieldType::SFString));
                REQUIRE((fi.access == AccessType::InitializeOnly));
                has_contentType = true;
            }
        }
        REQUIRE((has_url && "fields() must contain 'url'"));
        REQUIRE((has_bboxCenter && "fields() must contain 'bboxCenter'"));
        REQUIRE((has_bboxSize && "fields() must contain 'bboxSize'"));
        REQUIRE((has_contentType && "fields() must contain 'contentType'"));
    }

    // reflection get/set thunks — read/write url via FieldTable

    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        const FieldInfo* url_fi = nullptr;
        for (const auto& fi : ft) {
            if (fi.x3dName == "url") { url_fi = &fi; break; }
        }
        REQUIRE((url_fi != nullptr && "url FieldInfo must exist"));
        REQUIRE((url_fi->get && "url FieldInfo must have get thunk"));
        REQUIRE((url_fi->set && "url FieldInfo must have set thunk"));

        // Write via reflection
        MFString via_refl = {"refl.stl"};
        url_fi->set(eg, std::any(via_refl));
        // Read back via typed getter
        REQUIRE((eg.getUrl().size() == 1));
        REQUIRE((eg.getUrl()[0] == "refl.stl"));
        // Read back via reflection
        auto got = std::any_cast<MFString>(url_fi->get(eg));
        REQUIRE((got[0] == "refl.stl"));
    }

    // reflection get/set for bboxCenter and bboxSize

    {
        ExternalGeometry eg;
        const FieldTable& ft = eg.fields();

        auto find = [&](const std::string& name) -> const FieldInfo* {
            for (const auto& fi : ft) if (fi.x3dName == name) return &fi;
            return nullptr;
        };

        const FieldInfo* bc = find("bboxCenter");
        const FieldInfo* bs = find("bboxSize");
        REQUIRE((bc && bs));

        bc->set(eg, std::any(SFVec3f{4.0f, 5.0f, 6.0f}));
        bs->set(eg, std::any(SFVec3f{20.0f, 30.0f, 40.0f}));

        auto got_c = std::any_cast<SFVec3f>(bc->get(eg));
        auto got_s = std::any_cast<SFVec3f>(bs->get(eg));
        REQUIRE((got_c.x == 4.0f && got_c.y == 5.0f && got_c.z == 6.0f));
        REQUIRE((got_s.x == 20.0f && got_s.y == 30.0f && got_s.z == 40.0f));
    }

    // accept(NodeVisitor&) — enter/leave called in order

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
        REQUIRE((tv.enters == 1 && "accept must call enter once"));
        REQUIRE((tv.leaves == 1 && "accept must call leave once"));
    }

    // DEF/USE inherited from X3DNode base

    {
        ExternalGeometry eg;
        eg.setDEF("MyEG");
        REQUIRE((eg.getDEF() == "MyEG"));
    }
}
