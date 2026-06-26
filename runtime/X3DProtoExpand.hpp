// runtime/X3DProtoExpand.hpp
#ifndef X3D_RUNTIME_PROTO_EXPAND_HPP
#define X3D_RUNTIME_PROTO_EXPAND_HPP

#include "X3DProtoClone.hpp"
#include "X3DScene.hpp"
#include "parse/X3DProtoResolver.hpp"

#include <any>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

/// Recursion/cycle guard threaded through nested expansion.
struct ExpandGuard {
  int depth = 0;
  int maxDepth = 32;
};

namespace proto_detail {
/// RAII depth bump for a nested expandInstance call — decrements even if the
/// recursive call throws, so sibling instances are not falsely depth-capped.
struct DepthScope {
  ExpandGuard &g;
  explicit DepthScope(ExpandGuard &guard) : g(guard) { ++g.depth; }
  ~DepthScope() { --g.depth; }
};
} // namespace proto_detail

namespace proto_detail {

inline const FieldInfo *findField(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

inline const ProtoField *interfaceField(const ProtoDeclaration &d,
                                        const std::string &name) {
  for (const auto &p : d.interface) if (p.name == name) return &p;
  return nullptr;
}

inline const ProtoFieldValue *instanceValue(const ProtoInstance &inst,
                                             const std::string &name) {
  for (const auto &v : inst.fieldValues) if (v.name == name) return &v;
  return nullptr;
}

inline void setInstanceFieldValue(ProtoInstance &inst, ProtoFieldValue &&fv) {
  for (auto &v : inst.fieldValues) {
    if (v.name == fv.name) {
      v = std::move(fv);
      return;
    }
  }
  inst.fieldValues.push_back(std::move(fv));
}

/// Resolve the effective forwarded value for interface field `pf`: the instance
/// `override_` (scalar then node) when it carries one, else the interface
/// default (scalar then node). Exactly one of `out.value`/`out.nodeValue` is
/// populated. Returns false when no source supplies a value (e.g. an event-only
/// field with no default), so the caller can skip forwarding.
inline bool resolveForwardedValue(const ProtoFieldValue *override_,
                                  const ProtoField &pf, ProtoFieldValue &out) {
  if (override_ && override_->value.has_value()) {
    out.value = override_->value;
  } else if (override_ && !override_->nodeValue.empty()) {
    out.nodeValue = override_->nodeValue;
  } else if (pf.value.has_value()) {
    out.value = pf.value;
  } else if (!pf.nodeDefault.empty()) {
    out.nodeValue = pf.nodeDefault;
  } else {
    return false;
  }
  return true;
}

/// Splice `primary` into `parent`'s `field` slot (SFNode set / MFNode append).
/// Empty field => the node's default containerField.
inline void attachToParent(const std::shared_ptr<X3DNode> &parent,
                           const std::string &field,
                           const std::shared_ptr<X3DNode> &primary) {
  std::string slot = field.empty() ? primary->defaultContainerField() : field;
  const FieldInfo *fi = findField(*parent, slot);
  if (!fi || !fi->set) return;
  if (fi->type == X3DFieldType::SFNode) {
    fi->set(*parent, std::any(primary));
  } else if (fi->type == X3DFieldType::MFNode) {
    auto kids =
        std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi->get(*parent));
    kids.push_back(primary);
    fi->set(*parent, std::any(std::move(kids)));
  }
}

} // namespace proto_detail

/// Expand a single instance into its primary cloned node. Local declarations
/// only at this stage. Returns null if no declaration is available.
inline std::shared_ptr<X3DNode>
expandInstance(ProtoInstance &inst, Scene &scene,
               const x3d::codec::ProtoDeclarationResolver &resolver,
               const std::string &baseUrl,
               ExpandGuard &guard, std::vector<ProtoWarning> &warnings) {
  if (guard.depth >= guard.maxDepth) {
    warnings.push_back(
        {ProtoWarning::Kind::RecursionLimit, inst.name, "max expansion depth"});
    return nullptr;
  }
  std::shared_ptr<ProtoDeclaration> decl = inst.declaration;
  if (!decl && inst.externDeclaration) {
    decl = resolver(inst.externDeclaration->url, baseUrl);
    if (!decl) {
      warnings.push_back(
          {ProtoWarning::Kind::UnresolvedExtern, inst.name,
           inst.externDeclaration->url.empty()
               ? "no url"
               : inst.externDeclaration->url.front()});
      return nullptr;
    }
  }
  if (!decl) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "no declaration"});
    return nullptr;
  }
  if (decl->body.nodes.empty()) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "empty proto body"});
    return nullptr;
  }

  // Validate an IS access-type mapping per ISO/IEC 19775-1 Table 4.4.
  // inputOutput body fields may map to any interface type; all other body
  // fields must map to an interface of the SAME access type.
  auto isValidIsMapping = [](AccessType body, AccessType iface) -> bool {
    switch (body) {
      case AccessType::InputOutput:
        return true;
      case AccessType::InitializeOnly:
        return iface == AccessType::InitializeOnly;
      case AccessType::InputOnly:
        return iface == AccessType::InputOnly;
      case AccessType::OutputOnly:
        return iface == AccessType::OutputOnly;
    }
    return false;
  };

  // Clone the body; primary = clone of the first body node.
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
  std::shared_ptr<X3DNode> primary;
  for (const auto &bn : decl->body.nodes) {
    auto c = deepClone(bn, cloneMap);
    if (!primary) primary = c;
  }
  if (primary) primary->setDEF(inst.DEF);

  // Set a scalar (non-node) body field from a forwarded interface value. X3D
  // has no enum field type, so a bounded SimpleType the bindings emit as a C++
  // enum is declared SFString in the proto interface; the override therefore
  // arrives as a string and must go through the enum-string setter (mirroring
  // the reader's enum path) rather than the typed `set`, which would
  // bad_any_cast on a string.
  auto setScalar = [](const FieldInfo *fi, X3DNode &cloned,
                      const std::any &val) {
    if (fi->isEnum() && fi->setEnumString && val.type() == typeid(std::string))
      fi->setEnumString(cloned, std::any_cast<const std::string &>(val));
    else
      fi->set(cloned, val);
  };

  // Value-forward initializeOnly / inputOutput interface fields onto the
  // cloned body fields named by each IS connection.
  for (const IsConnection &is : decl->body.isConnections) {
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end()) continue;
    X3DNode &cloned = *cit->second;
    const ProtoField *pf = proto_detail::interfaceField(*decl, is.protoField);
    if (!pf) {
      warnings.push_back(
          {ProtoWarning::Kind::UnknownField, inst.name, is.protoField});
      continue;
    }
    if (pf->access != AccessType::InitializeOnly &&
        pf->access != AccessType::InputOutput)
      continue; // event fields handled by redirects in a later task
    const FieldInfo *fi = proto_detail::findField(cloned, is.nodeField);
    if (!fi || !fi->set) continue;
    if (!isValidIsMapping(fi->access, pf->access)) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          is.nodeField + " IS " + is.protoField +
                              " (invalid access-type mapping per Table 4.4)"});
      continue;
    }

    ProtoFieldValue eff;
    if (!proto_detail::resolveForwardedValue(
            proto_detail::instanceValue(inst, pf->name), *pf, eff))
      continue;
    // The override value's stored type may not match the body field's setter —
    // e.g. an EXTERN instance whose <fieldValue> the reader could only type as
    // the SFString fallback (no local decl to resolve the real type) being
    // forwarded to a typed setter. A type-erased set then throws bad_any_cast;
    // keep the read lenient by recording an InterfaceMismatch and moving on.
    try {
      if (eff.value.has_value()) {
        setScalar(fi, cloned, eff.value);
      } else if (!eff.nodeValue.empty()) {
        if (fi->type == X3DFieldType::SFNode)
          fi->set(cloned, std::any(eff.nodeValue.front()));
        else if (fi->type == X3DFieldType::MFNode)
          fi->set(cloned, std::any(eff.nodeValue));
      }
    } catch (const std::exception &) {
      warnings.push_back(
          {ProtoWarning::Kind::InterfaceMismatch, inst.name, is.protoField});
    }
  }

  // Build a body-local DEF table from the cloned nodes (proto-local scope; these
  // DEF names are NOT registered in scene.defs — they stay proto-private).
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> localDefs;
  for (const auto &kv : cloneMap) {
    const std::string def = kv.first->getDEF();
    if (!def.empty()) localDefs[def] = kv.second;
  }

  // Pre-resolve body-internal ROUTEs to concrete cloned endpoints.
  for (const Route &br : decl->body.routes) {
    auto fIt = localDefs.find(br.fromNode);
    auto tIt = localDefs.find(br.toNode);
    if (fIt == localDefs.end() || tIt == localDefs.end()) continue;
    scene.resolvedProtoRoutes.push_back(
        {fIt->second, br.fromField, tIt->second, br.toField});
  }

  // Interface event redirects: an exposed event interface field maps to the
  // IS-connected body field(s). initializeOnly is value-only (handled above).
  for (const IsConnection &is : decl->body.isConnections) {
    const ProtoField *pf = proto_detail::interfaceField(*decl, is.protoField);
    if (!pf) continue;
    if (pf->access == AccessType::InitializeOnly) continue;
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end()) continue;
    const FieldInfo *fi = proto_detail::findField(*cit->second, is.nodeField);
    if (fi && !isValidIsMapping(fi->access, pf->access)) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          is.nodeField + " IS " + is.protoField +
                              " (invalid access-type mapping per Table 4.4)"});
      continue;
    }
    scene.protoRedirects[primary.get()][is.protoField].push_back(
        {cit->second, is.nodeField});
  }

  // Expand ProtoInstances captured inside this body, once per outer
  // instantiation. Each was recorded against an ORIGINAL body node (a cloneMap
  // key) or with an empty parent (a direct body-root child). Recurse through the
  // shared guard — increment/decrement depth so deep/cyclic nesting terminates
  // with a RecursionLimit warning rather than overflowing.
  for (x3d::runtime::ProtoInstance nested : decl->body.nestedInstances) {
    if (!nested.declaration && !nested.externDeclaration) {
      if (auto d = scene.findProto(nested.name)) nested.declaration = d;
    }
    // Forward outer overrides/defaults across the proto boundary: each
    // `nodeField IS protoField` on the nested instance wires the nested
    // interface field to the enclosing proto interface field.
    for (const auto &is : nested.isConnections) {
      if (is.nodeField.empty() || is.protoField.empty()) continue;
      const ProtoField *outerPf = proto_detail::interfaceField(*decl, is.protoField);
      if (!outerPf) {
        warnings.push_back(
            {ProtoWarning::Kind::UnknownField, inst.name, is.protoField});
        continue;
      }
      // Validate the access-type mapping per Table 4.4 when the nested proto's
      // interface field type is resolvable (local decl); lenient otherwise —
      // mirrors the body IS-forwarding check above.
      if (nested.declaration) {
        if (const ProtoField *nestedPf = proto_detail::interfaceField(
                *nested.declaration, is.nodeField);
            nestedPf && !isValidIsMapping(nestedPf->access, outerPf->access)) {
          warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                              is.nodeField + " IS " + is.protoField +
                                  " (invalid access-type mapping per Table 4.4)"});
          continue;
        }
      }
      ProtoFieldValue forwarded;
      forwarded.name = is.nodeField;
      if (!proto_detail::resolveForwardedValue(
              proto_detail::instanceValue(inst, outerPf->name), *outerPf,
              forwarded))
        continue;
      proto_detail::setInstanceFieldValue(nested, std::move(forwarded));
    }
    std::shared_ptr<X3DNode> nestedPrimary;
    {
      proto_detail::DepthScope ds(guard);
      nestedPrimary =
          expandInstance(nested, scene, resolver, baseUrl, guard, warnings);
    }
    if (!nestedPrimary) continue;
    if (auto origParent = nested.parent.lock()) {
      // Case A: nested under a real body node — splice into that node's clone.
      auto pit = cloneMap.find(origParent.get());
      if (pit != cloneMap.end())
        proto_detail::attachToParent(pit->second, nested.parentField,
                                     nestedPrimary);
      else
        warnings.push_back({ProtoWarning::Kind::UnknownField, nested.name,
                            "nested parent not found in body clone"});
    } else {
      // Case B/C: direct body-root child — best-effort attach onto the outer
      // primary by the nested instance's containerField; lenient.
      const std::string slot =
          nested.parentField.empty() ? nested.containerField : nested.parentField;
      proto_detail::attachToParent(primary, slot, nestedPrimary);
    }
    scene.expandedSources[nestedPrimary.get()] = nested;
  }

  return primary;
}

/// Expand every captured ProtoInstance in `scene`, splicing primaries into
/// place. Front-door entry point. Collects diagnostics into `warnings`.
inline void expandScene(Scene &scene,
                        const x3d::codec::ProtoDeclarationResolver &resolver,
                        const std::string &baseUrl,
                        std::vector<ProtoWarning> &warnings) {
  for (ProtoInstance &inst : scene.protoInstances) {
    // Backstop: no expansion failure of one instance may abort the whole parse
    // (lenient-read policy). Any escaping exception becomes a ProtoWarning and
    // the remaining instances still expand.
    try {
      ExpandGuard guard;
      auto primary =
          expandInstance(inst, scene, resolver, baseUrl, guard, warnings);
      if (!primary) continue;
      auto parent = inst.parent.lock();
      if (!parent)
        scene.addRootNode(primary);
      else
        proto_detail::attachToParent(parent, inst.parentField, primary);
      scene.expandedSources[primary.get()] = inst;
      inst.expanded = true; // re-emitted via expandedSources (AUD-B: !expanded => writers re-emit directly)
    } catch (const std::exception &e) {
      warnings.push_back(
          {ProtoWarning::Kind::InterfaceMismatch, inst.name, e.what()});
    }
  }
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_EXPAND_HPP
