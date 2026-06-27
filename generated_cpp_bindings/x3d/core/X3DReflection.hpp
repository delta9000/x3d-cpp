// X3DReflection.hpp
// Auto-generated runtime reflection support shared by every X3D node.
#pragma once

#include <any>
#include <functional>
#include <string>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::core {

using nodes::X3DNode;
class NodeVisitor;

/**
 * @brief Runtime tag for an X3D field's value type.
 * @details Mirrors the model X3DType enum so a node-agnostic codec can
 *          switch on a field's type to decide how to format/parse its
 *          std::any value. SFEnum/MFEnum cover bounded enum-class fields.
 */
enum class X3DFieldType {
    SFBool,
    SFColor,
    SFColorRGBA,
    SFDouble,
    SFFloat,
    SFImage,
    SFInt32,
    SFMatrix3d,
    SFMatrix3f,
    SFMatrix4d,
    SFMatrix4f,
    SFNode,
    SFRotation,
    SFString,
    SFTime,
    SFVec2d,
    SFVec2f,
    SFVec3d,
    SFVec3f,
    SFVec4d,
    SFVec4f,
    MFBool,
    MFColor,
    MFColorRGBA,
    MFDouble,
    MFFloat,
    MFImage,
    MFInt32,
    MFMatrix3d,
    MFMatrix3f,
    MFMatrix4d,
    MFMatrix4f,
    MFNode,
    MFRotation,
    MFString,
    MFTime,
    MFVec2d,
    MFVec2f,
    MFVec3d,
    MFVec3f,
    MFVec4d,
    MFVec4f,
    SFEnum,
    MFEnum,
};

/**
 * @brief X3D field access category (mirrors the spec accessType).
 */
enum class AccessType {
    InitializeOnly,  // "initializeOnly"
    InputOnly,       // "inputOnly"
    OutputOnly,      // "outputOnly"
    InputOutput,     // "inputOutput"
};

/**
 * @brief Type-erased, node-agnostic description of one X3D field.
 * @details `get`/`set` are thunks bound by each node that read/write the
 *          field's strongly-typed member through the node's existing
 *          getX()/setX() accessors. The value is carried in a std::any whose
 *          concrete type is the field's C++ type (e.g. SFVec3f, std::string,
 *          std::vector<std::shared_ptr<X3DNode>>). Switch on `type` to know
 *          how to interpret it. `get` is empty for write-only (inputOnly)
 *          fields; `set` is empty for non-inputOutput (read-only) fields.
 */
struct FieldInfo {
    std::string x3dName;
    X3DFieldType type;
    AccessType access;
    // For SFNode/MFNode fields, the default containerField slot; empty otherwise.
    std::string containerField;
    // Reads the field's value from a node, boxed in std::any (empty fn if none).
    std::function<std::any(const X3DNode&)> get;
    // Writes a boxed value back into a node (empty fn if the field is read-only).
    std::function<void(X3DNode&, const std::any&)> set;
    // For SFEnum/MFEnum fields ONLY: token-string access so a node-agnostic
    // codec can read/write the enum value without knowing its concrete
    // enum-class type. getEnumString returns the field's X3D token(s)
    // (single token for SFEnum; space-separated tokens for MFEnum).
    // setEnumString parses token(s) back; it silently ignores unknown
    // tokens. Both are empty for non-enum fields.
    std::function<std::string(const X3DNode&)> getEnumString;
    std::function<void(X3DNode&, const std::string&)> setEnumString;

    bool isNode() const {
        return type == X3DFieldType::SFNode || type == X3DFieldType::MFNode;
    }
    bool isEnum() const {
        return type == X3DFieldType::SFEnum || type == X3DFieldType::MFEnum;
    }
    bool isReadable() const { return static_cast<bool>(get); }
    bool isWritable() const { return static_cast<bool>(set); }
};

/**
 * @brief A node's full field table (own + inherited fields, declaration order).
 */
using FieldTable = std::vector<FieldInfo>;

/**
 * @brief One out-of-range value kept by the lenient read path.
 * @details Range constraints (SFColor [0,1], numeric min/maxInclusive)
 *          are enforced by the typed set<Name>() but NOT by the lenient
 *          reflection write path, so out-of-range authored values are
 *          kept. validateRanges()/collectRangeWarnings() recover them as
 *          structured diagnostics. Range constraints ONLY (not enum/type).
 */
struct RangeDiagnostic {
    std::string nodeType;   // e.g. "Material"
    std::string defName;    // DEF name if known, else ""
    std::string fieldName;  // e.g. "specularColor"
    std::string detail;     // e.g. "specularColor.r above maximum of 1"
    std::string message() const {
        std::string who = defName.empty() ? nodeType
                                          : (nodeType + " DEF " + defName);
        return who + "." + fieldName + ": " + detail;
    }
};

/**
 * @brief Visitor over X3D nodes for hand-written codecs.
 * @details Nodes dispatch via X3DNode::accept(). The generic enter/leave
 *          pair brackets a node and exposes its reflected field table so a
 *          codec needs no per-node code.
 */
class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;
    // Called when entering a node. Return false to skip descending into it.
    virtual bool enter(const X3DNode& node) { (void)node; return true; }
    // Called when leaving a node (always paired with a true-returning enter).
    virtual void leave(const X3DNode& node) { (void)node; }
};

} // namespace x3d::core
