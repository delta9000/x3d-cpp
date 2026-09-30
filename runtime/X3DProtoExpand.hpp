// runtime/X3DProtoExpand.hpp
#ifndef X3D_RUNTIME_PROTO_EXPAND_HPP
#define X3D_RUNTIME_PROTO_EXPAND_HPP

#include "DynamicField.hpp"
#include "X3DProtoClone.hpp"
#include "X3DProtoFieldOrder.hpp"
#include "X3DScene.hpp"
#include "parse/X3DProtoResolver.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <algorithm>
#include <any>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
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

// Lives for one expansion transaction. Owning keys keep caller-authored USE
// targets alive and prevent address reuse while multiple outer instances share
// the same caller fieldValue graph.
struct ExpandContext {
  std::map<std::shared_ptr<X3DNode>, std::shared_ptr<X3DNode>,
           std::owner_less<std::shared_ptr<X3DNode>>> callerResults;
  std::set<std::shared_ptr<X3DNode>,
           std::owner_less<std::shared_ptr<X3DNode>>> activeCallerTemplates;
  std::set<std::shared_ptr<X3DNode>,
           std::owner_less<std::shared_ptr<X3DNode>>> visitedCallerNodes;
};
} // namespace proto_detail

namespace proto_detail {

inline const FieldInfo *findField(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

inline const FieldInfo *findIsBodyField(const X3DNode &n,
                                        const std::string &name,
                                        AccessType interfaceAccess) {
  if (auto exact = findField(n, name)) return exact;
  std::string base;
  if (interfaceAccess == AccessType::InputOnly && name.rfind("set_", 0) == 0)
    base = name.substr(4);
  else if (interfaceAccess == AccessType::OutputOnly && name.size() > 8 &&
           name.compare(name.size() - 8, 8, "_changed") == 0)
    base = name.substr(0, name.size() - 8);
  if (base.empty()) return nullptr;
  auto field = findField(n, base);
  return field && field->access == AccessType::InputOutput ? field : nullptr;
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

/// Resolve the effective forwarded value for interface field `pf`. A present
/// node fieldValue is an override even when its node list is empty (NULL/[]).
/// Returns false when a scalar/event field has no supplied value.
inline bool resolveForwardedValue(const ProtoFieldValue *override_,
                                  const ProtoField &pf, ProtoFieldValue &out) {
  if (override_) out.sourceUnits = override_->sourceUnits;
  if (pf.type == X3DFieldType::SFNode || pf.type == X3DFieldType::MFNode) {
    if (override_ && override_->value.has_value())
      out.value = override_->value; // mistyped input: let the setter diagnose it
    else
      out.nodeValue = override_ ? override_->nodeValue : pf.nodeDefault;
    return true;
  }
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

inline std::shared_ptr<X3DNode>
expandInstance(ProtoInstance &inst, Scene &scene,
               const x3d::codec::ProtoDeclarationResolver &resolver,
               const std::string &baseUrl,
               ExpandGuard &guard, std::vector<ProtoWarning> &warnings,
               proto_detail::ExpandContext *context = nullptr);

namespace proto_detail {

// Caller-owned ordinary nodes keep their identity. Replace only successful
// wrapper children; an unresolved wrapper stays in the graph for round-trip.
inline std::shared_ptr<X3DNode> materializeCallerNode(
    const std::shared_ptr<X3DNode> &source, Scene &scene,
    const x3d::codec::ProtoDeclarationResolver &resolver,
    const std::string &baseUrl, ExpandGuard &guard,
    std::vector<ProtoWarning> &warnings, ExpandContext &context) {
  if (!source) return nullptr;
  if (auto wrapper = std::dynamic_pointer_cast<ProtoInstanceTemplate>(source)) {
    if (auto found = context.callerResults.find(source);
        found != context.callerResults.end())
      return found->second;
    if (!context.activeCallerTemplates.insert(source).second) {
      warnings.push_back({ProtoWarning::Kind::RecursionLimit,
                          wrapper->instance.name, "cyclic node fieldValue"});
      return source;
    }
    try {
      ProtoInstance nested = wrapper->instance;
      nested.DEF = wrapper->getDEF();
      if (!nested.declaration && !nested.externDeclaration)
        nested.declaration = scene.findProto(nested.name);
      for (auto &value : nested.fieldValues)
        for (auto &node : value.nodeValue)
          node = materializeCallerNode(node, scene, resolver, baseUrl, guard,
                                       warnings, context);
      std::shared_ptr<X3DNode> primary;
      {
        DepthScope ds(guard);
        primary = expandInstance(nested, scene, resolver, baseUrl, guard,
                                 warnings, &context);
      }
      // A failed EXTERN or missing declaration remains a serializable template.
      // The transaction still remembers the attempt, so USE aliases warn once.
      auto result = primary ? primary : source;
      context.callerResults[source] = result;
      if (primary) {
        scene.expandedSources[primary.get()] = std::move(nested);
        if (!wrapper->getDEF().empty()) {
          auto def = scene.defs.find(wrapper->getDEF());
          if (def != scene.defs.end() && def->second == source)
            def->second = primary;
        }
      }
      context.activeCallerTemplates.erase(source);
      return result;
    } catch (...) {
      context.callerResults[source] = source;
      context.activeCallerTemplates.erase(source);
      throw;
    }
  }
  if (!context.visitedCallerNodes.insert(source).second) return source;
  for (const FieldInfo &field : source->fields()) {
    if (!field.get || !field.set) continue;
    if (field.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(field.get(*source));
      auto result = materializeCallerNode(child, scene, resolver, baseUrl,
                                          guard, warnings, context);
      if (result != child) field.set(*source, std::any(result));
    } else if (field.type == X3DFieldType::MFNode) {
      auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
          field.get(*source));
      bool changed = false;
      for (auto &child : children) {
        auto result = materializeCallerNode(child, scene, resolver, baseUrl,
                                            guard, warnings, context);
        changed |= result != child;
        child = std::move(result);
      }
      if (changed) field.set(*source, std::any(std::move(children)));
    }
  }
  return source;
}

} // namespace proto_detail

/// Expand a single instance into its primary cloned node. Local declarations
/// only at this stage. Returns null if no declaration is available.
inline std::shared_ptr<X3DNode>
expandInstance(ProtoInstance &inst, Scene &scene,
               const x3d::codec::ProtoDeclarationResolver &resolver,
               const std::string &baseUrl,
               ExpandGuard &guard, std::vector<ProtoWarning> &warnings,
               proto_detail::ExpandContext *context) {
  proto_detail::ExpandContext localContext;
  if (!context) context = &localContext;
  if (guard.depth >= guard.maxDepth) {
    warnings.push_back(
        {ProtoWarning::Kind::RecursionLimit, inst.name, "max expansion depth"});
    return nullptr;
  }
  std::shared_ptr<ProtoDeclaration> decl = inst.declaration;
  if (inst.externDeclaration) {
    // An EXTERN always consults the supplied resolver for this expansion
    // attempt, even when a prior attempt retained its selected declaration.
    inst.declaration.reset();
    decl = resolver(inst.externDeclaration->url, baseUrl);
    if (!decl) {
      warnings.push_back(
          {ProtoWarning::Kind::UnresolvedExtern, inst.name,
           inst.externDeclaration->url.empty()
               ? "no url"
               : inst.externDeclaration->url.front()});
      return nullptr;
    }
    // Retain the selected external source on the expanded ProtoInstance. The
    // instance is copied into Scene::expandedSources after this call.
    inst.declaration = decl;
  }
  if (!decl) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "no declaration"});
    return nullptr;
  }
  const auto bodyStatements = decl->body.orderedStatements();
  const auto firstNode = std::find_if(bodyStatements.begin(), bodyStatements.end(),
      [](const ProtoBodyStatement &s) {
        return s.kind == ProtoBodyStatement::Kind::Node ||
               s.kind == ProtoBodyStatement::Kind::Instance;
      });
  if (firstNode == bodyStatements.end()) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "empty proto body"});
    return nullptr;
  }

  for (auto &value : inst.fieldValues)
    for (auto &node : value.nodeValue)
      node = proto_detail::materializeCallerNode(
          node, scene, resolver, baseUrl, guard, warnings, *context);

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
  auto isCompatibleIsType = [](const FieldInfo &body,
                               const ProtoField &iface) {
    return body.type == iface.type ||
           (body.isEnum() && iface.type == X3DFieldType::SFString);
  };
  auto findNestedIsField = [](const std::vector<ProtoField> &fields,
                              const std::string &name,
                              AccessType outerAccess) -> const ProtoField * {
    if (outerAccess == AccessType::InputOnly)
      return findProtoRouteField(fields, name, false);
    if (outerAccess == AccessType::OutputOnly)
      return findProtoRouteField(fields, name, true);
    for (const auto &field : fields)
      if (field.name == name) return &field;
    return nullptr;
  };

  // Materialize template instances before deepClone sees them. All authored
  // graphs in this outer instance share one map, including DEF/USE references
  // between interface defaults, body nodes, and literal fieldValue graphs.
  using InterfaceTargets =
      std::unordered_map<std::string, std::vector<ProtoRedirect>>;
  struct InterfaceSnapshot {
    std::shared_ptr<ProtoDeclaration> declaration;
    std::shared_ptr<ExternProtoDeclaration> externDeclaration;
    InterfaceTargets targets;
  };
  auto captureInterface = [&](const ProtoInstance &source,
                              const std::shared_ptr<X3DNode> &primary) {
    InterfaceSnapshot snapshot{source.declaration, source.externDeclaration, {}};
    if (primary) {
      auto found = scene.protoRedirects.find(primary.get());
      if (found != scene.protoRedirects.end()) snapshot.targets = found->second;
    }
    return snapshot;
  };
  std::unordered_map<const X3DNode *, InterfaceSnapshot> wrapperInterfaces;
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
  std::unordered_set<const X3DNode *> visited;
  std::unordered_set<const X3DNode *> activeTemplates;
  std::function<std::shared_ptr<X3DNode>(const std::shared_ptr<X3DNode> &)> cloneGraph;
  std::function<void(ProtoInstance &)> forwardNestedIs;
  std::function<void(const std::shared_ptr<X3DNode> &)> materialize =
      [&](const std::shared_ptr<X3DNode> &source) {
        if (!source || cloneMap.contains(source.get())) return;
        if (auto wrapper = std::dynamic_pointer_cast<ProtoInstanceTemplate>(source)) {
          if (!activeTemplates.insert(source.get()).second) {
            warnings.push_back({ProtoWarning::Kind::RecursionLimit,
                                wrapper->instance.name, "cyclic node default"});
            cloneMap[source.get()] = nullptr;
            return;
          }
          ProtoInstance nested = wrapper->instance;
          nested.DEF = wrapper->getDEF();
          if (!nested.declaration && !nested.externDeclaration)
            nested.declaration = scene.findProto(nested.name);
          for (auto &value : nested.fieldValues)
            for (auto &node : value.nodeValue)
              node = cloneGraph(node);
          forwardNestedIs(nested);
          std::shared_ptr<X3DNode> primary;
          {
            proto_detail::DepthScope ds(guard);
            primary = expandInstance(nested, scene, resolver, baseUrl, guard,
                                     warnings, context);
          }
          cloneMap[source.get()] = primary;
          if (primary)
            wrapperInterfaces.emplace(source.get(),
                                      captureInterface(nested, primary));
          if (primary) scene.expandedSources[primary.get()] = std::move(nested);
          activeTemplates.erase(source.get());
          return;
        }
        if (!visited.insert(source.get()).second) return;
        for (const FieldInfo &field : source->fields()) {
          if (!field.get || !field.set) continue;
          if (field.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(field.get(*source));
            materialize(child);
          } else if (field.type == X3DFieldType::MFNode) {
            const auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
                field.get(*source));
            for (const auto &child : children) materialize(child);
          }
        }
      };
  cloneGraph = [&](const std::shared_ptr<X3DNode> &source) {
    materialize(source);
    return deepClone(source, cloneMap);
  };
  auto cloneNodes = [&cloneGraph](
      const std::vector<std::shared_ptr<X3DNode>> &sources) {
    std::vector<std::shared_ptr<X3DNode>> copies;
    copies.reserve(sources.size());
    for (const auto &source : sources)
      copies.push_back(cloneGraph(source));
    return copies;
  };
  forwardNestedIs = [&](ProtoInstance &nested) {
    for (const auto &is : nested.isConnections) {
      if (is.nodeField.empty() || is.protoField.empty()) continue;
      const ProtoField *outerPf = proto_detail::interfaceField(*decl, is.protoField);
      if (!outerPf) {
        warnings.push_back(
            {ProtoWarning::Kind::UnknownField, inst.name, is.protoField});
        continue;
      }
      const std::vector<ProtoField> *innerFields = nullptr;
      if (nested.externDeclaration)
        innerFields = &nested.externDeclaration->interface;
      else if (nested.declaration)
        innerFields = &nested.declaration->interface;
      const ProtoField *nestedPf = innerFields
          ? findNestedIsField(*innerFields, is.nodeField, outerPf->access)
          : nullptr;
      if (innerFields && !nestedPf) {
        warnings.push_back({ProtoWarning::Kind::UnknownField, inst.name,
                            is.nodeField + " IS " + is.protoField});
        continue;
      }
      if (nestedPf &&
          (!isValidIsMapping(nestedPf->access, outerPf->access) ||
           nestedPf->type != outerPf->type)) {
        warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                            is.nodeField + " IS " + is.protoField +
                                " (access/type mismatch per Table 4.4)"});
        continue;
      }
      const ProtoFieldValue *outerOverride =
          proto_detail::instanceValue(inst, outerPf->name);
      ProtoFieldValue forwarded;
      forwarded.name = nestedPf ? nestedPf->name : is.nodeField;
      if (!proto_detail::resolveForwardedValue(
              outerOverride, *outerPf, forwarded))
        continue;
      if (!outerOverride) forwarded.sourceUnits = decl->sourceUnits;
      if (!outerOverride && (outerPf->type == X3DFieldType::SFNode ||
                             outerPf->type == X3DFieldType::MFNode))
        forwarded.nodeValue = cloneNodes(forwarded.nodeValue);
      proto_detail::setInstanceFieldValue(nested, std::move(forwarded));
    }
  };
  // A present override suppresses an unused direct default, including an
  // explicit NULL/[] value. Body USE aliases are traversed independently.
  for (const auto &bn : decl->body.nodes) materialize(bn);
  for (const auto &field : decl->interface)
    if (!proto_detail::instanceValue(inst, field.name))
      for (const auto &node : field.nodeDefault) materialize(node);
  for (const auto &bn : decl->body.nodes) cloneGraph(bn);
  for (const auto &field : decl->interface)
    if (!proto_detail::instanceValue(inst, field.name))
      cloneNodes(field.nodeDefault);
  auto nestedInstances = decl->body.nestedInstances;
  std::vector<bool> clonedNestedValues(nestedInstances.size(), false);
  bool foundNewParent;
  do {
    foundNewParent = false;
    for (std::size_t index = 0; index < nestedInstances.size(); ++index) {
      if (clonedNestedValues[index]) continue;
      auto &nested = nestedInstances[index];
      if (auto parent = nested.parent.lock();
          parent && (!cloneMap.contains(parent.get()) || !cloneMap.at(parent.get())))
        continue;
      for (auto &value : nested.fieldValues)
        value.nodeValue = cloneNodes(value.nodeValue);
      clonedNestedValues[index] = true;
      foundNewParent = true;
    }
  } while (foundNewParent);
  decl->authoredScalarFields.copyClonesTo(cloneMap, scene.authoredScalarFields);
  for (const auto &[source, clone] : cloneMap) {
    if (!clone) continue;
    for (const FieldInfo &field : clone->fields())
      if (scene.authoredScalarFields.contains(clone, field.x3dName))
        scene.unitFieldSources[clone].try_emplace(field.x3dName, decl->sourceUnits);
  }

  // Set a scalar (non-node) body field from a forwarded interface value. X3D
  // has no enum field type, so a bounded SimpleType the bindings emit as a C++
  // enum is declared SFString in the proto interface; the override therefore
  // arrives as a string and must go through the enum-string setter (mirroring
  // the reader's enum path) rather than the typed `set`, which would
  // bad_any_cast on a string.
  auto setScalar = [](const FieldInfo *fi, X3DNode &cloned,
                      const std::any &val) -> bool {
    if (fi->isEnum() && fi->setEnumString && val.type() == typeid(std::string)) {
      const auto &token = std::any_cast<const std::string &>(val);
      fi->setEnumString(cloned, token);
      return fi->getEnumString && fi->getEnumString(cloned) == token;
    }
    fi->set(cloned, val);
    return true;
  };

  // Value-forward initializeOnly / inputOutput interface fields onto the
  // cloned body fields named by each IS connection.
  for (const IsConnection &is : decl->body.isConnections) {
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end() || !cit->second) continue;
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
    const FieldInfo *fi =
        proto_detail::findIsBodyField(cloned, is.nodeField, pf->access);
    if (!fi || !fi->set) continue;
    if (!isValidIsMapping(fi->access, pf->access) ||
        !isCompatibleIsType(*fi, *pf)) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          is.nodeField + " IS " + is.protoField +
                              " (access/type mismatch per Table 4.4)"});
      continue;
    }

    const ProtoFieldValue *override_ =
        proto_detail::instanceValue(inst, pf->name);
    ProtoFieldValue eff;
    if (!proto_detail::resolveForwardedValue(
            override_, *pf, eff))
      continue;
    if (!override_ && (pf->type == X3DFieldType::SFNode ||
                       pf->type == X3DFieldType::MFNode))
      eff.nodeValue = cloneNodes(eff.nodeValue);
    // The override value's stored type may not match the body field's setter —
    // e.g. an EXTERN instance whose <fieldValue> the reader could only type as
    // the SFString fallback (no local decl to resolve the real type) being
    // forwarded to a typed setter. A type-erased set then throws bad_any_cast;
    // keep the read lenient by recording an InterfaceMismatch and moving on.
    try {
      if (eff.value.has_value()) {
        if (setScalar(fi, cloned, eff.value)) {
          scene.authoredScalarFields.record(cit->second, is.nodeField);
          scene.unitFieldSources[cit->second][is.nodeField] =
              eff.sourceUnits.value_or(override_ ? scene.sourceUnits : decl->sourceUnits);
        }
      } else if (fi->type == X3DFieldType::SFNode ||
                 fi->type == X3DFieldType::MFNode) {
        if (fi->type == X3DFieldType::SFNode)
          fi->set(cloned, std::any(eff.nodeValue.empty()
                                       ? std::shared_ptr<X3DNode>{}
                                       : eff.nodeValue.front()));
        else if (fi->type == X3DFieldType::MFNode)
          fi->set(cloned, std::any(eff.nodeValue));
      }
    } catch (const std::exception &) {
      warnings.push_back(
          {ProtoWarning::Kind::InterfaceMismatch, inst.name, is.protoField});
    }
  }

  // Expand ProtoInstances captured inside this body, once per outer
  // instantiation. Each was recorded against an ORIGINAL body node (a cloneMap
  // key) or with an empty parent (a direct body-root child). Recurse through the
  // shared guard — increment/decrement depth so deep/cyclic nesting terminates
  // with a RecursionLimit warning rather than overflowing.
  std::vector<std::shared_ptr<X3DNode>> nestedPrimaries(nestedInstances.size());
  std::vector<std::optional<InterfaceSnapshot>> nestedInterfaces(
      nestedInstances.size());
  for (std::size_t index = 0; index < nestedInstances.size(); ++index) {
    x3d::runtime::ProtoInstance nested = nestedInstances[index];
    if (!clonedNestedValues[index]) continue;
    if (!nested.declaration && !nested.externDeclaration) {
      if (auto d = scene.findProto(nested.name)) nested.declaration = d;
    }
    // Forward outer overrides/defaults across the proto boundary: each
    // `nodeField IS protoField` on the nested instance wires the nested
    // interface field to the enclosing proto interface field.
    forwardNestedIs(nested);
    std::shared_ptr<X3DNode> nestedPrimary;
    {
      proto_detail::DepthScope ds(guard);
      nestedPrimary =
          expandInstance(nested, scene, resolver, baseUrl, guard, warnings,
                         context);
    }
    if (!nestedPrimary) continue;
    nestedPrimaries[index] = nestedPrimary;
    nestedInterfaces[index] = captureInterface(nested, nestedPrimary);
    if (auto origParent = nested.parent.lock()) {
      // Case A: nested under a real body node — splice into that node's clone.
      auto pit = cloneMap.find(origParent.get());
      if (pit != cloneMap.end() && pit->second)
        proto_detail::attachToParent(pit->second, nested.parentField,
                                     nestedPrimary);
      else
        warnings.push_back({ProtoWarning::Kind::UnknownField, nested.name,
                            "nested parent not found in body clone"});
    }
    scene.expandedSources[nestedPrimary.get()] = nested;
  }

  // Body DEFs remain private to this instantiation. A nested PROTO DEF names
  // its interface, whose IS targets were captured before an enclosing PROTO
  // can reuse the same concrete primary pointer for its own interface.
  struct LocalDef {
    std::shared_ptr<X3DNode> node;
    const InterfaceSnapshot *interface = nullptr;
  };
  std::unordered_map<std::string, LocalDef> localDefs;
  for (const auto &[source, clone] : cloneMap) {
    if (!clone || source->getDEF().empty()) continue;
    auto wrapper = wrapperInterfaces.find(source);
    localDefs[source->getDEF()] =
        {clone, wrapper == wrapperInterfaces.end() ? nullptr : &wrapper->second};
  }
  for (std::size_t index = 0; index < nestedInstances.size(); ++index) {
    if (!nestedPrimaries[index] || nestedInstances[index].DEF.empty()) continue;
    localDefs[nestedInstances[index].DEF] =
        {nestedPrimaries[index], &*nestedInterfaces[index]};
  }
  struct RouteEndpoint {
    std::vector<ProtoRedirect> targets;
    std::optional<X3DFieldType> nominalType;
  };
  auto resolveBodyEndpoint = [&](const std::string &def,
                                 const std::string &field,
                                 bool asSource,
                                 const std::string &routeName) -> RouteEndpoint {
    auto found = localDefs.find(def);
    if (found == localDefs.end()) return {};
    const auto &[node, iface] = found->second;
    if (!iface) {
      const FieldInfo *reflected = proto_detail::findField(*node, field);
      if (!reflected) {
        std::string base;
        if (!asSource && field.rfind("set_", 0) == 0)
          base = field.substr(4);
        else if (asSource && field.size() > 8 &&
                 field.compare(field.size() - 8, 8, "_changed") == 0)
          base = field.substr(0, field.size() - 8);
        if (!base.empty()) {
          auto alias = proto_detail::findField(*node, base);
          if (alias && alias->access == AccessType::InputOutput)
            reflected = alias;
        }
      }
      if (!reflected)
        return {{{node, field}}, std::nullopt}; // dynamic field: bridge checks it
      if ((asSource && reflected->access != AccessType::OutputOnly &&
           reflected->access != AccessType::InputOutput) ||
          (!asSource && reflected->access != AccessType::InputOnly &&
           reflected->access != AccessType::InputOutput)) {
        warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                            routeName + ": " + def + "." + field +
                                " is not routable as " +
                                (asSource ? "source" : "sink")});
        return {};
      }
      return {{{node, reflected->x3dName}}, reflected->type};
    }
    const std::vector<ProtoField> *fields = nullptr;
    if (iface->externDeclaration)
      fields = &iface->externDeclaration->interface;
    else if (iface->declaration)
      fields = &iface->declaration->interface;
    if (!fields) return {};
    const ProtoField *pf = findProtoRouteField(*fields, field, asSource);
    if (!pf) {
      const bool implicitMetadata =
          field == "metadata" ||
          (asSource && field == "metadata_changed") ||
          (!asSource && field == "set_metadata");
      if (implicitMetadata) {
        const FieldInfo *metadata = proto_detail::findField(*node, "metadata");
        if (metadata && metadata->type == X3DFieldType::SFNode &&
            metadata->access == AccessType::InputOutput)
          return {{{node, "metadata"}}, X3DFieldType::SFNode};
      }
      warnings.push_back({ProtoWarning::Kind::UnknownField, inst.name,
                          routeName + ": unknown interface endpoint " +
                              def + "." + field});
      return {};
    }
    if ((asSource && pf->access != AccessType::OutputOnly &&
         pf->access != AccessType::InputOutput) ||
        (!asSource && pf->access != AccessType::InputOnly &&
         pf->access != AccessType::InputOutput)) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          routeName + ": " + def + "." + field +
                              " is not routable as " +
                              (asSource ? "source" : "sink")});
      return {};
    }
    auto targets = iface->targets.find(pf->name);
    if (targets == iface->targets.end()) return {};
    return {targets->second, pf->type};
  };
  for (const Route &route : decl->body.routes) {
    const std::string routeName =
        route.fromNode + "." + route.fromField + " TO " +
        route.toNode + "." + route.toField;
    auto sources = resolveBodyEndpoint(route.fromNode, route.fromField,
                                       true, routeName);
    auto sinks = resolveBodyEndpoint(route.toNode, route.toField,
                                     false, routeName);
    if (sources.targets.empty() || sinks.targets.empty()) continue;
    if (sources.nominalType && sinks.nominalType &&
        *sources.nominalType != *sinks.nominalType) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          routeName + ": type mismatch"});
      continue;
    }
    for (const auto &source : sources.targets)
      for (const auto &sink : sinks.targets)
        scene.resolvedProtoRoutes.push_back(
            {source.targetNode, source.targetField,
             sink.targetNode, sink.targetField});
  }

  // Cloning copies ordinary children first, while nested instances are spliced
  // later. Rebuild only MFNode slots containing instances in the authored
  // sequence, retaining the clone map's shared nodes for DEF/USE aliases.
  std::unordered_set<const X3DNode *> reorderedParents;
  for (const auto &nested : nestedInstances) {
    auto originalOwner = nested.parent.lock();
    if (!originalOwner || !reorderedParents.insert(originalOwner.get()).second)
      continue;
    auto cloneIt = cloneMap.find(originalOwner.get());
    if (cloneIt == cloneMap.end() || !cloneIt->second) continue;
    const auto &cloned = cloneIt->second;
    const auto statements = orderedNodeStatements(decl->body, originalOwner);
    std::unordered_map<std::string, std::vector<std::shared_ptr<X3DNode>>> ordered;
    std::unordered_set<std::string> slotsWithInstances;
    for (const auto &statement : statements) {
      if (statement.kind == ProtoBodyStatement::Kind::Node) {
        auto it = cloneMap.find(statement.node.get());
        if (it != cloneMap.end()) ordered[statement.field].push_back(it->second);
      } else if (statement.kind == ProtoBodyStatement::Kind::Instance &&
                 statement.instanceIndex < nestedPrimaries.size()) {
        slotsWithInstances.insert(statement.field);
        if (auto &node = nestedPrimaries[statement.instanceIndex])
          ordered[statement.field].push_back(node);
      }
    }
    for (const auto &slot : slotsWithInstances) {
      const FieldInfo *fi = proto_detail::findField(*cloned, slot);
      if (fi && fi->type == X3DFieldType::MFNode && fi->set)
        fi->set(*cloned, std::any(std::move(ordered[slot])));
    }
  }

  // The first authored node or instance fixes this PROTO's primary type.
  // A failed first instance cannot be replaced by a later peer.
  std::shared_ptr<X3DNode> primary;
  for (const auto &statement : bodyStatements) {
    std::shared_ptr<X3DNode> node;
    if (statement.kind == ProtoBodyStatement::Kind::Node) {
      auto it = cloneMap.find(statement.node.get());
      if (it != cloneMap.end()) node = it->second;
    } else if (statement.kind == ProtoBodyStatement::Kind::Instance &&
               statement.instanceIndex < nestedPrimaries.size()) {
      node = nestedPrimaries[statement.instanceIndex];
    } else {
      continue;
    }
    if (!primary) {
      if (!node) return nullptr;
      primary = node;
    } else if (node) {
      scene.protoPeerNodes.push_back(node);
    }
  }
  if (!primary) return nullptr;
  primary->setDEF(inst.DEF);
  // Interface event redirects: an exposed event interface field maps to the
  // IS-connected body field(s). initializeOnly is value-only (handled above).
  InterfaceTargets outerTargets;
  for (const IsConnection &is : decl->body.isConnections) {
    const ProtoField *pf = proto_detail::interfaceField(*decl, is.protoField);
    if (!pf) continue;
    if (pf->access == AccessType::InitializeOnly) continue;
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end() || !cit->second) continue;
    const FieldInfo *fi = proto_detail::findIsBodyField(
        *cit->second, is.nodeField, pf->access);
    if (!fi) continue;
    if (!isValidIsMapping(fi->access, pf->access) ||
        !isCompatibleIsType(*fi, *pf)) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name,
                          is.nodeField + " IS " + is.protoField +
                              " (access/type mismatch per Table 4.4)"});
      continue;
    }
    outerTargets[is.protoField].push_back({cit->second, fi->x3dName});
  }

  auto forwardNestedEvents = [&](const InterfaceSnapshot &child,
                                 const std::vector<ProtoInstanceIsConnection>
                                     &connections) {
    const std::vector<ProtoField> *innerFields = nullptr;
    if (child.externDeclaration)
      innerFields = &child.externDeclaration->interface;
    else if (child.declaration)
      innerFields = &child.declaration->interface;
    if (!innerFields) return;
    for (const auto &is : connections) {
      const ProtoField *outerPf =
          proto_detail::interfaceField(*decl, is.protoField);
      if (!outerPf || outerPf->access == AccessType::InitializeOnly) continue;
      const ProtoField *innerPf =
          findNestedIsField(*innerFields, is.nodeField, outerPf->access);
      if (!innerPf) continue;
      if (!isValidIsMapping(innerPf->access, outerPf->access) ||
          innerPf->type != outerPf->type)
        continue;
      auto targets = child.targets.find(innerPf->name);
      if (targets == child.targets.end()) continue;
      auto &outer = outerTargets[outerPf->name];
      outer.insert(outer.end(), targets->second.begin(), targets->second.end());
    }
  };
  for (std::size_t index = 0; index < nestedInstances.size(); ++index)
    if (nestedInterfaces[index])
      forwardNestedEvents(*nestedInterfaces[index],
                          nestedInstances[index].isConnections);
  for (const auto &[source, snapshot] : wrapperInterfaces) {
    if (auto wrapper = dynamic_cast<const ProtoInstanceTemplate *>(source))
      forwardNestedEvents(snapshot, wrapper->instance.isConnections);
  }
  // Every declared interface field exists on the instance as a real field with
  // its initial value, whether or not the body IS-connects it (§4.4.2.2,
  // §4.4.4.2). A field with an IS mapping is backed by its body clone; the rest
  // keep independent storage and event endpoints on the primary (the
  // dynamic-field store, the same abstraction Script author fields use), so a
  // ROUTE may set an inputOnly/inputOutput field and fan out its value_changed
  // even with no inner node to map onto. Scalar fields only: node-valued
  // interface defaults stay body-owned.
  {
    std::unordered_set<std::string> isMapped;
    for (const IsConnection &is : decl->body.isConnections)
      isMapped.insert(is.protoField);
    for (const ProtoInstance &nested : nestedInstances)
      for (const auto &is : nested.isConnections)
        isMapped.insert(is.protoField);
    for (const auto &[name, targets] : outerTargets)
      if (!targets.empty()) isMapped.insert(name);
    std::vector<AuthorFieldDecl> decls;
    for (const ProtoField &field : decl->interface) {
      if (isMapped.count(field.name)) continue;
      if (field.type == X3DFieldType::SFNode ||
          field.type == X3DFieldType::MFNode)
        continue;
      AuthorFieldDecl d;
      d.x3dName = field.name;
      d.type = field.type;
      d.access = field.access;
      ProtoFieldValue eff;
      if (proto_detail::resolveForwardedValue(
              proto_detail::instanceValue(inst, field.name), field, eff))
        d.initialValue = eff.value;
      decls.push_back(std::move(d));
    }
    if (!decls.empty())
      dynamicFieldStore().addAuthorFields(
          std::static_pointer_cast<const X3DNode>(primary), decls);
  }

  // The outer instance may use the nested instance's concrete primary. Its
  // visible interface replaces the nested interface at that same node pointer.
  scene.protoRedirects[primary.get()] = std::move(outerTargets);

  return primary;
}

/// ADR-0033 fallback: an instance of a quarantined built-in-shadowing PROTO
/// becomes the built-in node, its fieldValues applied to same-named fields.
inline std::shared_ptr<X3DNode> instantiateBuiltin(const ProtoInstance &inst,
                                                   std::vector<ProtoWarning> &warnings,
                                                   AuthoredScalarFields *authored = nullptr) {
  auto node = x3d::nodes::X3DNodeFactory::create(inst.name);
  if (!node) return nullptr;
  node->setDEF(inst.DEF);
  for (const ProtoFieldValue &fv : inst.fieldValues) {
    const FieldInfo *fi = nullptr;
    for (const auto &f : node->fields())
      if (f.x3dName == fv.name) { fi = &f; break; }
    if (!fi || !fi->set) {
      warnings.push_back({ProtoWarning::Kind::UnknownField, inst.name, fv.name});
      continue;
    }
    try {
      if (fi->type == X3DFieldType::SFNode)
        fi->set(*node, std::any(fv.nodeValue.empty() ? std::shared_ptr<X3DNode>{}
                                                     : fv.nodeValue.front()));
      else if (fi->type == X3DFieldType::MFNode)
        fi->set(*node, std::any(fv.nodeValue));
      else if (fi->isEnum() && fi->setEnumString && fv.value.type() == typeid(std::string)) {
        const auto &token = std::any_cast<const std::string &>(fv.value);
        fi->setEnumString(*node, token);
        if (authored && fi->getEnumString && fi->getEnumString(*node) == token)
          authored->record(node, fv.name);
      } else if (fv.value.has_value()) {
        fi->set(*node, fv.value);
        if (authored) authored->record(node, fv.name);
      }
    } catch (const std::exception &) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch, inst.name, fv.name});
    }
  }
  return node;
}

/// Expand every captured ProtoInstance in `scene`, splicing primaries into
/// place. Front-door entry point. Collects diagnostics into `warnings`.
inline void expandScene(Scene &scene,
                        const x3d::codec::ProtoDeclarationResolver &resolver,
                        const std::string &baseUrl,
                        std::vector<ProtoWarning> &warnings) {
  proto_detail::ExpandContext context;
  for (ProtoInstance &inst : scene.protoInstances) {
    // Backstop: no expansion failure of one instance may abort the whole parse
    // (lenient-read policy). Any escaping exception becomes a ProtoWarning and
    // the remaining instances still expand.
    const bool linkedPlacement = inst.hasPlacementTemplate();
    auto placementTemplate = inst.placementTemplate.lock();
    // A removed authored placeholder must not be recreated by expansion.
    if (linkedPlacement && !placementTemplate) continue;
    if (placementTemplate)
      context.activeCallerTemplates.insert(placementTemplate);
    try {
      std::shared_ptr<X3DNode> primary;
      if (inst.builtinFallback) {
        // Written back as the plain built-in node, not re-emitted as an instance.
        primary = instantiateBuiltin(inst, warnings, &scene.authoredScalarFields);
      } else {
        ExpandGuard guard;
        primary = expandInstance(inst, scene, resolver, baseUrl, guard, warnings,
                                 &context);
      }
      if (primary) {
        auto parent = inst.parent.lock();
        if (linkedPlacement) {
          // The reader placed this instance at its authored root or child slot.
          // The final graph traversal replaces that exact wrapper and its USEs.
          const std::string oldDef = placementTemplate->getDEF();
          const std::string newDef = primary->getDEF();
          if (!oldDef.empty()) {
            auto def = scene.defs.find(oldDef);
            if (def != scene.defs.end() && def->second == placementTemplate) {
              if (oldDef == newDef)
                def->second = primary;
              else
                scene.defs.erase(def);
            }
          }
          if (!newDef.empty()) scene.defs[newDef] = primary;
        } else if (parent) {
          proto_detail::attachToParent(parent, inst.parentField, primary);
        } else {
          scene.addRootNode(primary);
        }
        if (!inst.builtinFallback) scene.expandedSources[primary.get()] = inst;
        inst.expanded = true; // writers re-emit via expandedSources
      }
      if (placementTemplate) {
        context.callerResults[placementTemplate] =
            primary ? primary : placementTemplate;
        context.activeCallerTemplates.erase(placementTemplate);
      }
    } catch (const std::exception &e) {
      if (placementTemplate) {
        context.callerResults[placementTemplate] = placementTemplate;
        context.activeCallerTemplates.erase(placementTemplate);
      }
      warnings.push_back(
          {ProtoWarning::Kind::InterfaceMismatch, inst.name, e.what()});
    }
  }
  ExpandGuard rootGuard;
  context.visitedCallerNodes.clear();
  for (auto &node : scene.rootNodes) {
    try {
      node = proto_detail::materializeCallerNode(
          node, scene, resolver, baseUrl, rootGuard, warnings, context);
    } catch (const std::exception &e) {
      warnings.push_back({ProtoWarning::Kind::InterfaceMismatch,
                          node ? node->nodeTypeName() : std::string{}, e.what()});
    }
  }
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_EXPAND_HPP
