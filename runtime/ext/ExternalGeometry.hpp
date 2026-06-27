// runtime/ext/ExternalGeometry.hpp
// Hand-written X3DNode subclass for the ExternalGeometry extension node.
// Namespace: x3d::runtime::ext.
// Part of the runtime/ext/ quarantine (x3d_cpp_ext target, default OFF).
// Core (x3d_cpp, sdk.hpp) MUST NEVER include this file.
//
// ExternalGeometry is a pure-reference geometry leaf node:
//   url          — MFString, initializeOnly — external mesh url(s)
//   bboxCenter   — SFVec3f,  initializeOnly — pre-computed bounds center (0 0 0)
//   bboxSize     — SFVec3f,  initializeOnly — pre-computed bounds size (-1 -1 -1)
//   contentType  — SFString, initializeOnly — optional format hint ("")
//
// The node does NOT hold binary data / accessors — those live in PackedMesh,
// produced by the resolver/codec layer (lazy materialization, Round 2).
// defaultContainerField() == "geometry" so it attaches to Shape.geometry.
#ifndef X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_HPP
#define X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_HPP

#include "x3d/nodes/X3DNode.hpp"       // X3DNode, NodeVisitor (on x3d_cpp include path)
#include "x3d/core/X3DReflection.hpp" // FieldTable, FieldInfo, X3DFieldType, AccessType
#include "x3d/core/X3Dtypes.hpp"      // MFString, SFVec3f, SFString

#include <any>
#include <string>
#include <vector>

namespace x3d::runtime::ext {

/**
 * @class ExternalGeometry
 * @brief A pure-reference geometry leaf node for the x3d-cpp-gen extension layer.
 * @details Holds an external mesh url + pre-computed bounding box. Declared in
 *          X3D files via <ExternProtoDeclare url='"urn:x3d-cpp-gen:ext:ExternalGeometry"'/>.
 *          The EXTERNPROTO expansion wires fieldValues onto this node via IS connections
 *          inside the factory ProtoDeclaration (see ExtResolver.hpp).
 *          Lazy materialization (producing a PackedMesh) is Round 2.
 */
class ExternalGeometry : public X3DNode {
public:
    ExternalGeometry() = default;
    ~ExternalGeometry() override = default;

    // ── Typed accessors ────────────────────────────────────────────────────

    MFString getUrl() const { return _url; }
    void setUrl(const MFString& v) { _url = v; }
    void setUrl(MFString&& v) { _url = std::move(v); }

    SFVec3f getBboxCenter() const { return _bboxCenter; }
    void setBboxCenter(const SFVec3f& v) { _bboxCenter = v; }

    SFVec3f getBboxSize() const { return _bboxSize; }
    void setBboxSize(const SFVec3f& v) { _bboxSize = v; }

    SFString getContentType() const { return _contentType; }
    void setContentType(const SFString& v) { _contentType = v; }

    // ── X3DNode virtuals ──────────────────────────────────────────────────

    std::string nodeTypeName() const override { return "ExternalGeometry"; }

    std::string defaultContainerField() const override { return "geometry"; }

    const FieldTable& fields() const override {
        static const FieldTable table = [] {
            FieldTable t;

            // url — MFString initializeOnly
            t.push_back(FieldInfo{
                "url", X3DFieldType::MFString, AccessType::InitializeOnly, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).getUrl());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).setUrl(std::any_cast<MFString>(v));
                },
                nullptr, nullptr
            });

            // bboxCenter — SFVec3f initializeOnly
            t.push_back(FieldInfo{
                "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).getBboxCenter());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).setBboxCenter(std::any_cast<SFVec3f>(v));
                },
                nullptr, nullptr
            });

            // bboxSize — SFVec3f initializeOnly
            t.push_back(FieldInfo{
                "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).getBboxSize());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).setBboxSize(std::any_cast<SFVec3f>(v));
                },
                nullptr, nullptr
            });

            // contentType — SFString initializeOnly
            t.push_back(FieldInfo{
                "contentType", X3DFieldType::SFString, AccessType::InitializeOnly, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).getContentType());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).setContentType(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            // ── Inherited base fields (mirrors WorldInfo.cpp) ───────────────

            t.push_back(FieldInfo{
                "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getIS());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getMetadata());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setMetadata(std::any_cast<SFNode>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getDEF());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "USE", X3DFieldType::SFString, AccessType::InputOutput, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getUSE());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "class", X3DFieldType::SFString, AccessType::InputOutput, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getClass_());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setClass_(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "id", X3DFieldType::SFString, AccessType::InputOutput, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getId());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setId(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            t.push_back(FieldInfo{
                "style", X3DFieldType::SFString, AccessType::InputOutput, "",
                [](const X3DNode& n) -> std::any {
                    return std::any(dynamic_cast<const ExternalGeometry&>(n).X3DNode::getStyle());
                },
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<ExternalGeometry&>(n).X3DNode::setStyle(std::any_cast<SFString>(v));
                },
                nullptr, nullptr
            });

            return t;
        }();
        return table;
    }

    void accept(NodeVisitor& visitor) const override {
        if (!visitor.enter(*this)) return;
        visitor.leave(*this);
    }

private:
    MFString _url{};
    SFVec3f  _bboxCenter{0.0f, 0.0f, 0.0f};
    SFVec3f  _bboxSize{-1.0f, -1.0f, -1.0f};
    SFString _contentType{};
};

} // namespace x3d::runtime::ext
#endif // X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_HPP
