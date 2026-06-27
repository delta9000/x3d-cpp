// CanonicalXmlWriter.hpp
// X3D Canonical Form (X3DC14N) serializer.
//
// Produces the deterministic X3D Canonical Form as defined by X3DJSAIL
// (-canonical mode / X3dCanonicalizer). Rules:
//   1. XML prolog: <?xml version="1.0" encoding="UTF-8"?>
//   2. X3D DOCTYPE line (version-appropriate).
//   3. Attributes sorted alphabetically by name within each element.
//   4. Single-quote attribute delimiters.
//   5. X3DC14N escaping in attribute values: &amp; &lt; &gt; &apos;
//      (NOT &quot; — single-quoted attrs need &apos; for the apostrophe).
//   6. 2-space-per-level indentation; one element per line.
//   7. Minimal number formatting: shortest round-trip float/double
//      representation (0.8 not 0.800000, 0 not 0.0, integer via to_string).
//   8. DEF/USE, ProtoDeclare/field/connect, ROUTE, IMPORT/EXPORT, head meta,
//      xmlns:xsd and xsd:noNamespaceSchemaLocation on <X3D> all preserved.
//
// The DEFAULT XmlWriter path is COMPLETELY UNCHANGED — this is a separate
// class that makes no modifications to any shared code paths.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_CANONICAL_XML_WRITER_HPP
#define X3D_CANONICAL_XML_WRITER_HPP

#include "DynamicField.hpp"
#include "FieldValueIO.hpp"
#include "ProtoNameMaps.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"
#include "XmlLite.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::codec {

// ---------------------------------------------------------------------------
// X3DC14N attribute escaping (single-quote context):
//   &amp; for &, &lt; for <, &gt; for >, &apos; for ' (the delimiter).
//   Double-quote does NOT need escaping in single-quote context.
// ---------------------------------------------------------------------------
inline std::string canonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '\'': out += "&apos;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// X3DC14N minimal number formatting.
//
// Uses std::to_chars with chars_format::general to produce the shortest
// decimal string that round-trips back to the same float/double value.
// Integers are emitted without a decimal point (0, 1, 2 not 0.0, 1.0).
// This matches X3DJSAIL's behaviour: `0.9607843` not `0.960784316`.
// ---------------------------------------------------------------------------

/// Shortest round-trip float representation, X3DC14N style.
inline std::string canonFmtFloat(float v) {
    // Integer fast-path (no decimal needed).
    if (v == static_cast<float>(static_cast<long long>(v)) &&
        std::fabs(v) < 1e15f) {
        return std::to_string(static_cast<long long>(v));
    }
    // Shortest representation via to_chars general (Ryu/Dragonfly algo).
    std::array<char, 32> buf{};
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(),
                                   v, std::chars_format::general);
    if (ec == std::errc{}) {
        return std::string(buf.data(), ptr);
    }
    // Fallback: use 7 significant digits (enough for float round-trip).
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(7);
    os << v;
    return os.str();
}

/// Shortest round-trip double representation, X3DC14N style.
///
/// Unlike floats, doubles in X3D (SFDouble, SFTime) MUST retain a decimal
/// point even for whole-number values — `0.0` not `0`, `1.0` not `1`.
/// X3DJSAIL always produces `0.0` for double zero; dropping the decimal
/// would cause a type mismatch when re-parsed (XmlReader treats `0` as
/// SFInt32 context; the spec says SFDouble fields serialise with a decimal).
/// This is also required for Tier-1 idempotence: `0` re-parses as `0` then
/// emits as `0` (OK), but some readers would treat `0.0` and `0` differently
/// in MF contexts. The safest and X3DC14N-correct choice is always decimal.
inline std::string canonFmtDouble(double v) {
    // Always use to_chars for doubles (no integer fast-path).
    // This ensures `0.0` is emitted as `0.0` to match X3DJSAIL.
    std::array<char, 32> buf{};
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(),
                                   v, std::chars_format::general);
    if (ec == std::errc{}) {
        std::string s(buf.data(), ptr);
        // If to_chars produced an integer-looking string (no '.', no 'e', no 'E'),
        // append ".0" to distinguish from integer types.
        if (s.find('.') == std::string::npos &&
            s.find('e') == std::string::npos &&
            s.find('E') == std::string::npos) {
            s += ".0";
        }
        return s;
    }
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(17);
    os << v;
    return os.str();
}

// ---------------------------------------------------------------------------
// X3DC14N formatValue: like FieldValueIO::formatValue but uses canonical
// (shortest round-trip) number formatting.
// ---------------------------------------------------------------------------

inline std::string canonFormatValue(X3DFieldType type, const std::any &v) {
    switch (type) {
    case X3DFieldType::SFBool:
        return fmtBool(std::any_cast<SFBool>(v));
    case X3DFieldType::SFInt32:
        return std::to_string(std::any_cast<int>(v));
    case X3DFieldType::SFFloat:
        return canonFmtFloat(std::any_cast<float>(v));
    case X3DFieldType::SFDouble:
    case X3DFieldType::SFTime:
        return canonFmtDouble(std::any_cast<double>(v));
    case X3DFieldType::SFString:
        return std::any_cast<std::string>(v);
    case X3DFieldType::SFColor: {
        auto c = std::any_cast<SFColor>(v);
        return canonFmtFloat(c.r) + " " + canonFmtFloat(c.g) + " " + canonFmtFloat(c.b);
    }
    case X3DFieldType::SFColorRGBA: {
        auto c = std::any_cast<SFColorRGBA>(v);
        return canonFmtFloat(c.r) + " " + canonFmtFloat(c.g) + " " +
               canonFmtFloat(c.b) + " " + canonFmtFloat(c.a);
    }
    case X3DFieldType::SFVec2f: {
        auto p = std::any_cast<SFVec2f>(v);
        return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y);
    }
    case X3DFieldType::SFVec2d: {
        auto p = std::any_cast<SFVec2d>(v);
        return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y);
    }
    case X3DFieldType::SFVec3f: {
        auto p = std::any_cast<SFVec3f>(v);
        return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y) + " " + canonFmtFloat(p.z);
    }
    case X3DFieldType::SFVec3d: {
        auto p = std::any_cast<SFVec3d>(v);
        return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y) + " " + canonFmtDouble(p.z);
    }
    case X3DFieldType::SFVec4f: {
        auto p = std::any_cast<SFVec4f>(v);
        return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y) + " " +
               canonFmtFloat(p.z) + " " + canonFmtFloat(p.w);
    }
    case X3DFieldType::SFVec4d: {
        auto p = std::any_cast<SFVec4d>(v);
        return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y) + " " +
               canonFmtDouble(p.z) + " " + canonFmtDouble(p.w);
    }
    case X3DFieldType::SFRotation: {
        auto r = std::any_cast<SFRotation>(v);
        return canonFmtFloat(r.x) + " " + canonFmtFloat(r.y) + " " +
               canonFmtFloat(r.z) + " " + canonFmtFloat(r.angle);
    }
    case X3DFieldType::MFBool: {
        const auto &vec = std::any_cast<std::vector<bool>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += fmtBool(vec[i]);
        }
        return out;
    }
    case X3DFieldType::MFInt32: {
        const auto &vec = std::any_cast<std::vector<int>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += std::to_string(vec[i]);
        }
        return out;
    }
    case X3DFieldType::MFFloat: {
        const auto &vec = std::any_cast<std::vector<float>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i]);
        }
        return out;
    }
    case X3DFieldType::MFDouble:
    case X3DFieldType::MFTime: {
        const auto &vec = std::any_cast<std::vector<double>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtDouble(vec[i]);
        }
        return out;
    }
    case X3DFieldType::MFString:
        return fmtMFString(std::any_cast<std::vector<std::string>>(v));
    case X3DFieldType::MFColor: {
        const auto &vec = std::any_cast<std::vector<SFColor>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].r) + " " + canonFmtFloat(vec[i].g) + " " +
                   canonFmtFloat(vec[i].b);
        }
        return out;
    }
    case X3DFieldType::MFColorRGBA: {
        const auto &vec = std::any_cast<std::vector<SFColorRGBA>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].r) + " " + canonFmtFloat(vec[i].g) + " " +
                   canonFmtFloat(vec[i].b) + " " + canonFmtFloat(vec[i].a);
        }
        return out;
    }
    case X3DFieldType::MFVec2f: {
        const auto &vec = std::any_cast<std::vector<SFVec2f>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y);
        }
        return out;
    }
    case X3DFieldType::MFVec3f: {
        const auto &vec = std::any_cast<std::vector<SFVec3f>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
                   canonFmtFloat(vec[i].z);
        }
        return out;
    }
    case X3DFieldType::MFVec2d: {
        const auto &vec = std::any_cast<std::vector<SFVec2d>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y);
        }
        return out;
    }
    case X3DFieldType::MFVec3d: {
        const auto &vec = std::any_cast<std::vector<SFVec3d>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y) + " " +
                   canonFmtDouble(vec[i].z);
        }
        return out;
    }
    case X3DFieldType::MFVec4f: {
        const auto &vec = std::any_cast<std::vector<SFVec4f>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
                   canonFmtFloat(vec[i].z) + " " + canonFmtFloat(vec[i].w);
        }
        return out;
    }
    case X3DFieldType::MFVec4d: {
        const auto &vec = std::any_cast<std::vector<SFVec4d>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y) + " " +
                   canonFmtDouble(vec[i].z) + " " + canonFmtDouble(vec[i].w);
        }
        return out;
    }
    case X3DFieldType::MFRotation: {
        const auto &vec = std::any_cast<std::vector<SFRotation>>(v);
        std::string out;
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i) out += ' ';
            out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
                   canonFmtFloat(vec[i].z) + " " + canonFmtFloat(vec[i].angle);
        }
        return out;
    }
    case X3DFieldType::SFMatrix3f:
        return fmtMatrixF<SFMatrix3f, 3>(std::any_cast<SFMatrix3f>(v));
    case X3DFieldType::SFMatrix4f:
        return fmtMatrixF<SFMatrix4f, 4>(std::any_cast<SFMatrix4f>(v));
    case X3DFieldType::SFMatrix3d:
        return fmtMatrixD<SFMatrix3d, 3>(std::any_cast<SFMatrix3d>(v));
    case X3DFieldType::SFMatrix4d:
        return fmtMatrixD<SFMatrix4d, 4>(std::any_cast<SFMatrix4d>(v));
    case X3DFieldType::SFImage:
        return fmtImage(std::any_cast<SFImage>(v));
    default:
        return "";
    }
}

// ---------------------------------------------------------------------------
// X3DC14N DOCTYPE line for a given version string.
// Covers the X3D 3.x and 4.x versions encountered in the wild.
// ---------------------------------------------------------------------------
inline std::string x3dDoctype(const std::string &version) {
    // Public/system identifier pattern:
    //   PUBLIC "ISO//Web3D//DTD X3D <ver>//EN"
    //          "https://www.web3d.org/specifications/x3d-<ver>.dtd"
    return "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D " + version +
           "//EN\" \"https://www.web3d.org/specifications/x3d-" + version + ".dtd\">";
}

// ---------------------------------------------------------------------------
// X3DC14N schema location for a given version.
// ---------------------------------------------------------------------------
inline std::string x3dSchemaLocation(const std::string &version) {
    return "https://www.web3d.org/specifications/x3d-" + version + ".xsd";
}

/// Serializes the runtime document model to X3D Canonical Form (X3DC14N).
///
/// Design: build an xml::Element tree (same as XmlWriter) then render it with
/// canonical rules: sorted attrs, single-quote, 2-space indent. The tree build
/// reuses the same logic as XmlWriter but is fully self-contained so the default
/// writer path has zero risk of change.
class CanonicalXmlWriter {
public:
    /// Serialize a full document to its X3D Canonical Form string.
    std::string writeDocument(const runtime::X3DDocument &doc) {
        seen_.clear();
        auto root = std::make_unique<xml::Element>();
        root->name = "X3D";
        // X3DC14N <X3D> attrs (sorted alphabetically, so profile < version <
        // xmlns:xsd < xsd:noNamespaceSchemaLocation):
        root->setAttr("profile", doc.profileName());
        root->setAttr("version", doc.version);
        root->setAttr("xmlns:xsd", "http://www.w3.org/2001/XMLSchema-instance");
        root->setAttr("xsd:noNamespaceSchemaLocation", x3dSchemaLocation(doc.version));

        // <head>
        if (!doc.head.empty()) {
            xml::Element *head = root->addChild("head");
            for (const auto &c : doc.head.components) {
                xml::Element *e = head->addChild("component");
                e->setAttr("level", std::to_string(c.level));
                e->setAttr("name", c.name);
            }
            for (const auto &u : doc.head.units) {
                xml::Element *e = head->addChild("unit");
                e->setAttr("category", u.category);
                e->setAttr("conversionFactor", canonFmtDouble(u.conversionFactor));
                e->setAttr("name", u.name);
            }
            for (const auto &m : doc.head.meta) {
                xml::Element *e = head->addChild("meta");
                e->setAttr("content", m.content);
                if (!m.dir.empty())       e->setAttr("dir", m.dir);
                if (!m.httpEquiv.empty()) e->setAttr("http-equiv", m.httpEquiv);
                if (!m.lang.empty())      e->setAttr("lang", m.lang);
                e->setAttr("name", m.name);
                if (!m.scheme.empty())    e->setAttr("scheme", m.scheme);
            }
        }

        // <Scene>
        xml::Element *scene = root->addChild("Scene");
        writeSceneInto(scene, doc.scene);

        std::ostringstream os;
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        os << x3dDoctype(doc.version) << "\n";
        renderCanonical(*root, os, 0);
        return os.str();
    }

private:
    std::unordered_set<const X3DNode *> seen_;
    std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
    const runtime::Scene *scene_ = nullptr;
    const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;

    void writeSceneInto(xml::Element *scene, const runtime::Scene &s) {
        scene_ = &s;
        for (const auto &d : s.protoDeclarations)
            if (d) scene->children.push_back(writeProtoDeclareElement(*d));
        for (const auto &e : s.externProtoDeclarations)
            if (e) scene->children.push_back(writeExternProtoDeclareElement(*e));
        for (const auto &n : s.rootNodes) {
            auto child = writeNodeElement(n, "");
            if (child) scene->children.push_back(std::move(child));
        }
        // Un-expanded ProtoInstances at scene root.
        for (const auto &inst : s.protoInstances)
            if (!inst.expanded && inst.parent.expired())
                scene->children.push_back(writeProtoInstanceElement(inst, ""));
        for (const auto &r : s.routes) {
            xml::Element *e = scene->addChild("ROUTE");
            e->setAttr("fromField", r.fromField);
            e->setAttr("fromNode", r.fromNode);
            e->setAttr("toField", r.toField);
            e->setAttr("toNode", r.toNode);
        }
        for (const auto &imp : s.imports) {
            xml::Element *e = scene->addChild("IMPORT");
            if (!imp.as.empty())      e->setAttr("AS", imp.as);
            e->setAttr("importedDEF", imp.importedDEF);
            e->setAttr("inlineDEF",   imp.inlineDEF);
        }
        for (const auto &exp : s.exports) {
            xml::Element *e = scene->addChild("EXPORT");
            if (!exp.as.empty()) e->setAttr("AS", exp.as);
            e->setAttr("localDEF", exp.localDEF);
        }
    }

    X3DNode *defaultFor(const std::string &typeName) {
        auto it = defaults_.find(typeName);
        if (it != defaults_.end()) return it->second.get();
        auto fresh = X3DNodeFactory::create(typeName);
        X3DNode *raw = fresh.get();
        defaults_[typeName] = std::move(fresh);
        return raw;
    }

    std::unique_ptr<xml::Element>
    writeNodeElement(const std::shared_ptr<X3DNode> &node,
                     const std::string &containerOverride) {
        if (!node) return nullptr;

        // Inline round-trip.
        if (scene_) {
            auto il = scene_->expandedInlines.find(node.get());
            if (il != scene_->expandedInlines.end())
                return writeNodeElement(il->second, containerOverride);
        }
        // PROTO round-trip.
        if (scene_) {
            auto it = scene_->expandedSources.find(node.get());
            if (it != scene_->expandedSources.end())
                return writeProtoInstanceElement(it->second, containerOverride);
        }

        const std::string typeName = node->nodeTypeName();
        auto el = std::make_unique<xml::Element>();
        el->name = typeName;

        // DEF/USE.
        const std::string def = node->getDEF();
        if (seen_.count(node.get())) {
            el->setAttr("USE", def);
            if (!containerOverride.empty())
                el->setAttr("containerField", containerOverride);
            return el;
        }
        if (!def.empty()) {
            el->setAttr("DEF", def);
            seen_.insert(node.get());
        }

        X3DNode *defaults = defaultFor(typeName);

        // Value attributes (canonically sorted later in renderCanonical).
        const FieldTable &table = node->fields();
        for (const FieldInfo &f : table) {
            if (!f.isReadable()) continue;
            if (f.x3dName == "DEF" || f.x3dName == "USE") continue;
            if (f.x3dName == "IS") continue;
            if (f.x3dName == "sourceCode" && dynamic_cast<const Script *>(node.get()))
                continue;

            if (f.isNode()) {
                writeNodeField(*el, node, f);
                continue;
            }

            std::string text;
            std::string defText;
            if (f.isEnum()) {
                if (!f.getEnumString) continue;
                text = f.getEnumString(*node);
                if (defaults && f.getEnumString)
                    defText = f.getEnumString(*defaults);
            } else {
                std::any v = f.get(*node);
                text = canonFormatValue(f.type, v);
                if (defaults) {
                    for (const FieldInfo &df : defaults->fields()) {
                        if (df.x3dName == f.x3dName && df.isReadable()) {
                            defText = canonFormatValue(df.type, df.get(*defaults));
                            break;
                        }
                    }
                }
            }
            if (defaults && text == defText) continue;
            el->setAttr(f.x3dName, text);
        }

        // Nested un-expanded ProtoInstances for this node.
        if (scene_) {
            for (const auto &inst : scene_->protoInstances) {
                if (inst.expanded) continue;
                auto p = inst.parent.lock();
                if (!p || p.get() != node.get()) continue;
                const std::string slot =
                    inst.parentField.empty() ? inst.containerField : inst.parentField;
                el->children.push_back(writeProtoInstanceElement(inst, slot));
            }
        }

        if (!containerOverride.empty())
            el->setAttr("containerField", containerOverride);

        if (bodyIsc_) attachIsBlocks(*el, node, *bodyIsc_);

        if (const auto *script = dynamic_cast<const Script *>(node.get()))
            writeScriptAuthorFields(*el, *script);

        return el;
    }

    void writeScriptAuthorFields(xml::Element &el, const Script &script) {
        const std::size_t staticCount = script.fields().size();
        FieldTable eff = runtime::effectiveFields(script);
        for (std::size_t i = staticCount; i < eff.size(); ++i) {
            const FieldInfo &f = eff[i];
            xml::Element *fe = el.addChild("field");
            fe->setAttr("accessType", accessTypeName(f.access));
            fe->setAttr("name", f.x3dName);
            fe->setAttr("type", fieldTypeName(f.type));
            if (f.isReadable() && (f.access == AccessType::InitializeOnly ||
                                   f.access == AccessType::InputOutput)) {
                std::any v = f.get(script);
                if (v.has_value())
                    fe->setAttr("value", canonFormatValue(f.type, v));
            }
        }
        const std::string src = script.getSourceCode();
        if (!src.empty()) el.text = src;
    }

    void writeNodeField(xml::Element &parent,
                        const std::shared_ptr<X3DNode> &owner,
                        const FieldInfo &f) {
        const std::string slot = f.containerField;
        std::any v = f.get(*owner);
        if (f.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
            if (!child) return;
            auto el = writeNodeElement(child, slot);
            if (el) parent.children.push_back(std::move(el));
        } else {
            auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
            for (const auto &child : vec) {
                if (!child) continue;
                auto el = writeNodeElement(child, slot);
                if (el) parent.children.push_back(std::move(el));
            }
        }
    }

    std::unique_ptr<xml::Element>
    writeProtoInstanceElement(const runtime::ProtoInstance &src,
                              const std::string &containerOverride) {
        auto el = std::make_unique<xml::Element>();
        el->name = "ProtoInstance";
        if (!src.DEF.empty()) el->setAttr("DEF", src.DEF);
        if (!containerOverride.empty())
            el->setAttr("containerField", containerOverride);
        else if (!src.containerField.empty() && src.containerField != "children")
            el->setAttr("containerField", src.containerField);
        el->setAttr("name", src.name);

        for (const runtime::ProtoFieldValue &fv : src.fieldValues) {
            xml::Element *fe = el->addChild("fieldValue");
            fe->setAttr("name", fv.name);
            if (!fv.nodeValue.empty()) {
                for (const auto &child : fv.nodeValue) {
                    auto ce = writeNodeElement(child, "");
                    if (ce) fe->children.push_back(std::move(ce));
                }
            } else if (fv.value.has_value()) {
                if (const runtime::ProtoField *pf = interfaceField(src, fv.name))
                    fe->setAttr("value", canonFormatValue(pf->type, fv.value));
            }
        }
        return el;
    }

    std::unique_ptr<xml::Element>
    writeProtoFieldElement(const runtime::ProtoField &f) {
        auto fe = std::make_unique<xml::Element>();
        fe->name = "field";
        fe->setAttr("accessType", accessTypeName(f.access));
        if (!f.nodeDefault.empty()) {
            for (const auto &n : f.nodeDefault) {
                auto ce = writeNodeElement(n, "");
                if (ce) fe->children.push_back(std::move(ce));
            }
        } else if (f.value.has_value() &&
                   (f.access == AccessType::InitializeOnly ||
                    f.access == AccessType::InputOutput)) {
            fe->setAttr("value", canonFormatValue(f.type, f.value));
        }
        fe->setAttr("name", f.name);
        fe->setAttr("type", fieldTypeName(f.type));
        return fe;
    }

    std::unique_ptr<xml::Element>
    writeProtoDeclareElement(const runtime::ProtoDeclaration &d) {
        auto el = std::make_unique<xml::Element>();
        el->name = "ProtoDeclare";
        if (!d.appinfo.empty())       el->setAttr("appinfo", d.appinfo);
        if (!d.documentation.empty()) el->setAttr("documentation", d.documentation);
        el->setAttr("name", d.name);

        auto iface = std::make_unique<xml::Element>();
        iface->name = "ProtoInterface";
        for (const auto &f : d.interface)
            iface->children.push_back(writeProtoFieldElement(f));
        if (!iface->children.empty()) el->children.push_back(std::move(iface));

        auto body = std::make_unique<xml::Element>();
        body->name = "ProtoBody";
        CanonicalXmlWriter bodyWriter;
        bodyWriter.bodyIsc_ = &d.body.isConnections;
        for (const auto &n : d.body.nodes) {
            auto ne = bodyWriter.writeNodeElement(n, "");
            if (!ne) continue;
            for (const auto &ni : d.body.nestedInstances) {
                if (ni.parent.lock().get() == n.get()) {
                    auto ie = writeProtoInstanceElement(
                        ni, ni.parentField.empty() ? std::string() : ni.parentField);
                    if (ie) ne->children.push_back(std::move(ie));
                }
            }
            body->children.push_back(std::move(ne));
        }
        for (const auto &r : d.body.routes) {
            xml::Element *re = body->addChild("ROUTE");
            re->setAttr("fromField", r.fromField);
            re->setAttr("fromNode", r.fromNode);
            re->setAttr("toField", r.toField);
            re->setAttr("toNode", r.toNode);
        }
        for (const auto &ni : d.body.nestedInstances) {
            if (!ni.parent.lock()) {
                auto ie = writeProtoInstanceElement(ni, std::string());
                if (ie) body->children.push_back(std::move(ie));
            }
        }
        el->children.push_back(std::move(body));
        return el;
    }

    std::unique_ptr<xml::Element>
    writeExternProtoDeclareElement(const runtime::ExternProtoDeclaration &d) {
        auto el = std::make_unique<xml::Element>();
        el->name = "ExternProtoDeclare";
        if (!d.appinfo.empty())       el->setAttr("appinfo", d.appinfo);
        if (!d.documentation.empty()) el->setAttr("documentation", d.documentation);
        el->setAttr("name", d.name);
        if (!d.url.empty())           el->setAttr("url", fmtMFString(d.url));
        for (const auto &f : d.interface)
            el->children.push_back(writeProtoFieldElement(f));
        return el;
    }

    static void attachIsBlocks(xml::Element &nodeEl,
                               const std::shared_ptr<X3DNode> &node,
                               const std::vector<runtime::IsConnection> &isc) {
        xml::Element *is = nullptr;
        for (const auto &c : isc) {
            if (c.node.get() != node.get()) continue;
            if (!is) is = nodeEl.addChild("IS");
            xml::Element *conn = is->addChild("connect");
            conn->setAttr("nodeField", c.nodeField);
            conn->setAttr("protoField", c.protoField);
        }
    }

    static const runtime::ProtoField *
    interfaceField(const runtime::ProtoInstance &src, const std::string &name) {
        auto scan = [&](const std::vector<runtime::ProtoField> &iface)
            -> const runtime::ProtoField * {
            for (const runtime::ProtoField &f : iface)
                if (f.name == name) return &f;
            return nullptr;
        };
        if (src.declaration)      return scan(src.declaration->interface);
        if (src.externDeclaration) return scan(src.externDeclaration->interface);
        return nullptr;
    }

    // ── Canonical rendering ──────────────────────────────────────────────────

    static void indentCan(std::ostringstream &os, int depth) {
        for (int i = 0; i < depth; ++i) os << "  ";
    }

    /// Collect and sort attrs alphabetically (stable — attr names are unique per
    /// XML element; for canonical form identical-name attrs would be an error).
    static std::vector<std::pair<std::string, std::string>>
    sortedAttrs(const std::vector<std::pair<std::string, std::string>> &attrs) {
        auto sorted = attrs;
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const auto &a, const auto &b) {
                             return a.first < b.first;
                         });
        return sorted;
    }

    void renderCanonical(const xml::Element &el, std::ostringstream &os, int depth) {
        indentCan(os, depth);
        os << "<" << el.name;
        for (const auto &a : sortedAttrs(el.attributes)) {
            os << " " << a.first << "='" << canonEscape(a.second) << "'";
        }
        if (el.children.empty() && el.text.empty()) {
            os << "/>\n";
            return;
        }
        os << ">\n";
        for (const auto &c : el.children)
            renderCanonical(*c, os, depth + 1);
        if (!el.text.empty()) {
            indentCan(os, depth + 1);
            os << "<![CDATA[" << el.text << "]]>\n";
        }
        indentCan(os, depth);
        os << "</" << el.name << ">\n";
    }
};

} // namespace x3d::codec

#endif // X3D_CANONICAL_XML_WRITER_HPP
