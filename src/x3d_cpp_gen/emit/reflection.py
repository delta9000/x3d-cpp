"""Generation of the runtime reflection support header (``X3DReflection.hpp``).

This header is hand-written-shaped but DATA-DRIVEN: the ``X3DFieldType`` runtime
enum is emitted directly from :class:`~x3d_cpp_gen.model.types.X3DType` so the
two never drift. It also declares the type-erased ``FieldInfo`` / ``FieldTable``
records and the ``NodeVisitor`` interface that hand-written, node-agnostic codecs
walk. None of this header branches per node; every node simply *populates* a
``FieldTable`` (built in Python via the descriptors, rendered by the thin Jinja
template) and overrides ``fields()`` / ``nodeTypeName()`` / ``accept()``.

The reflection API a codec relies on:

* ``X3DNode::nodeTypeName()`` -> the node's X3D type name (e.g. ``"Box"``).
* ``X3DNode::fields()`` -> ``const FieldTable&`` covering OWN + INHERITED fields.
* ``X3DNode::accept(NodeVisitor&)`` -> double-dispatch entry point.
* ``FieldInfo::get(node)`` -> ``std::any`` holding the field's typed value.
* ``FieldInfo::set(node, any)`` -> writes the typed value back (no-op if absent).
* ``X3DFieldType`` -> switch on this to know how to format/parse the ``std::any``.
"""

from x3d_cpp_gen.model.types import X3DType, TypeRegistry


def _runtime_enumerators() -> list[str]:
    """Every distinct runtime field-type tag, in a stable order.

    Derived from :class:`X3DType` (so a newer spec type appears automatically),
    plus the two synthetic ``SFEnum`` / ``MFEnum`` tags used for bounded-enum
    fields whose C++ value is a generated ``enum class``.
    """
    seen: list[str] = []
    for member in X3DType:
        tag = TypeRegistry.runtime_tag(member)
        if tag not in seen:
            seen.append(tag)
    for tag in ("SFEnum", "MFEnum"):
        if tag not in seen:
            seen.append(tag)
    return seen


def gen_reflection_header() -> str:
    """Render the full text of ``X3DReflection.hpp``."""
    enumerators = _runtime_enumerators()

    lines: list[str] = []
    lines.append("// X3DReflection.hpp")
    lines.append("// Auto-generated runtime reflection support shared by every X3D node.")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <any>")
    lines.append("#include <functional>")
    lines.append("#include <string>")
    lines.append("#include <vector>")
    lines.append("")
    lines.append("namespace x3d::nodes { class X3DNode; }")
    lines.append("")
    lines.append("namespace x3d::core {")
    lines.append("")
    lines.append("using nodes::X3DNode;")
    lines.append("class NodeVisitor;")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Runtime tag for an X3D field's value type.")
    lines.append(" * @details Mirrors the model X3DType enum so a node-agnostic codec can")
    lines.append(" *          switch on a field's type to decide how to format/parse its")
    lines.append(" *          std::any value. SFEnum/MFEnum cover bounded enum-class fields.")
    lines.append(" */")
    lines.append("enum class X3DFieldType {")
    for tag in enumerators:
        lines.append(f"    {tag},")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief X3D field access category (mirrors the spec accessType).")
    lines.append(" */")
    lines.append("enum class AccessType {")
    lines.append("    InitializeOnly,  // \"initializeOnly\"")
    lines.append("    InputOnly,       // \"inputOnly\"")
    lines.append("    OutputOnly,      // \"outputOnly\"")
    lines.append("    InputOutput,     // \"inputOutput\"")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Type-erased, node-agnostic description of one X3D field.")
    lines.append(" * @details `get`/`set` are thunks bound by each node that read/write the")
    lines.append(" *          field's strongly-typed member through the node's existing")
    lines.append(" *          getX()/setX() accessors. The value is carried in a std::any whose")
    lines.append(" *          concrete type is the field's C++ type (e.g. SFVec3f, std::string,")
    lines.append(" *          std::vector<std::shared_ptr<X3DNode>>). Switch on `type` to know")
    lines.append(" *          how to interpret it. `get` is empty for write-only (inputOnly)")
    lines.append(" *          fields; `set` is empty for non-inputOutput (read-only) fields.")
    lines.append(" */")
    lines.append("struct FieldInfo {")
    lines.append("    std::string x3dName;")
    lines.append("    X3DFieldType type;")
    lines.append("    AccessType access;")
    lines.append("    // For SFNode/MFNode fields, the default containerField slot; empty otherwise.")
    lines.append("    std::string containerField;")
    lines.append("    // Reads the field's value from a node, boxed in std::any (empty fn if none).")
    lines.append("    std::function<std::any(const X3DNode&)> get;")
    lines.append("    // Writes a boxed value back into a node (empty fn if the field is read-only).")
    lines.append("    std::function<void(X3DNode&, const std::any&)> set;")
    lines.append("    // For SFEnum/MFEnum fields ONLY: token-string access so a node-agnostic")
    lines.append("    // codec can read/write the enum value without knowing its concrete")
    lines.append("    // enum-class type. getEnumString returns the field's X3D token(s)")
    lines.append("    // (single token for SFEnum; space-separated tokens for MFEnum).")
    lines.append("    // setEnumString parses token(s) back; it silently ignores unknown")
    lines.append("    // tokens. Both are empty for non-enum fields.")
    lines.append("    std::function<std::string(const X3DNode&)> getEnumString;")
    lines.append("    std::function<void(X3DNode&, const std::string&)> setEnumString;")
    lines.append("")
    lines.append("    bool isNode() const {")
    lines.append("        return type == X3DFieldType::SFNode || type == X3DFieldType::MFNode;")
    lines.append("    }")
    lines.append("    bool isEnum() const {")
    lines.append("        return type == X3DFieldType::SFEnum || type == X3DFieldType::MFEnum;")
    lines.append("    }")
    lines.append("    bool isReadable() const { return static_cast<bool>(get); }")
    lines.append("    bool isWritable() const { return static_cast<bool>(set); }")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief A node's full field table (own + inherited fields, declaration order).")
    lines.append(" */")
    lines.append("using FieldTable = std::vector<FieldInfo>;")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief One out-of-range value kept by the lenient read path.")
    lines.append(" * @details Range constraints (SFColor [0,1], numeric min/maxInclusive)")
    lines.append(" *          are enforced by the typed set<Name>() but NOT by the lenient")
    lines.append(" *          reflection write path, so out-of-range authored values are")
    lines.append(" *          kept. validateRanges()/collectRangeWarnings() recover them as")
    lines.append(" *          structured diagnostics. Range constraints ONLY (not enum/type).")
    lines.append(" */")
    lines.append("struct RangeDiagnostic {")
    lines.append("    std::string nodeType;   // e.g. \"Material\"")
    lines.append("    std::string defName;    // DEF name if known, else \"\"")
    lines.append("    std::string fieldName;  // e.g. \"specularColor\"")
    lines.append("    std::string detail;     // e.g. \"specularColor.r above maximum of 1\"")
    lines.append("    std::string message() const {")
    lines.append("        std::string who = defName.empty() ? nodeType")
    lines.append("                                          : (nodeType + \" DEF \" + defName);")
    lines.append("        return who + \".\" + fieldName + \": \" + detail;")
    lines.append("    }")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Visitor over X3D nodes for hand-written codecs.")
    lines.append(" * @details Nodes dispatch via X3DNode::accept(). The generic enter/leave")
    lines.append(" *          pair brackets a node and exposes its reflected field table so a")
    lines.append(" *          codec needs no per-node code.")
    lines.append(" */")
    lines.append("class NodeVisitor {")
    lines.append("public:")
    lines.append("    virtual ~NodeVisitor() = default;")
    lines.append("    // Called when entering a node. Return false to skip descending into it.")
    lines.append("    virtual bool enter(const X3DNode& node) { (void)node; return true; }")
    lines.append("    // Called when leaving a node (always paired with a true-returning enter).")
    lines.append("    virtual void leave(const X3DNode& node) { (void)node; }")
    lines.append("};")
    lines.append("")
    lines.append("// Whitespace/comma-delimited token split for MFEnum wire values")
    lines.append("// (e.g. \"OPAQUE MASK\" or \"OPAQUE,MASK\"). Shared by every")
    lines.append("// generated node's MFEnum reflection set-thunk so the split logic")
    lines.append("// lives in exactly one place instead of once per generated .cpp.")
    lines.append("inline std::vector<std::string> parseEnumTokens(const std::string& s) {")
    lines.append("    std::vector<std::string> out;")
    lines.append("    std::size_t i = 0;")
    lines.append("    while (i < s.size()) {")
    lines.append("        while (i < s.size() && (s[i] == ' ' || s[i] == '\\t' ||")
    lines.append("               s[i] == '\\n' || s[i] == '\\r' || s[i] == ',')) ++i;")
    lines.append("        std::size_t j = i;")
    lines.append("        while (j < s.size() && s[j] != ' ' && s[j] != '\\t' &&")
    lines.append("               s[j] != '\\n' && s[j] != '\\r' && s[j] != ',') ++j;")
    lines.append("        if (j > i) out.push_back(s.substr(i, j - i));")
    lines.append("        i = j;")
    lines.append("    }")
    lines.append("    return out;")
    lines.append("}")
    lines.append("")
    lines.append("} // namespace x3d::core")
    lines.append("")
    return "\n".join(lines)
