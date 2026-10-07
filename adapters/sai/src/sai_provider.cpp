#include "x3d/sai_provider.hpp"
#include "X3DDocument.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime {
namespace sai = x3d::sai::experimental;
namespace {
// Existing reflection supplies the field facts; only Transform's field kinds
// are needed by this bounded implementation. Unknown kinds fail closed.
sai::result<sai::value_kind> kind(x3d::core::X3DFieldType type) {
  using native = x3d::core::X3DFieldType;
  switch (type) {
  case native::SFBool:
    return sai::value_kind::sf_bool;
  case native::SFVec3f:
    return sai::value_kind::sf_vec3f;
  case native::SFRotation:
    return sai::value_kind::sf_rotation;
  case native::SFNode:
    return sai::value_kind::sf_node;
  case native::MFNode:
    return sai::value_kind::mf_node;
  default:
    sai::sai_error error;
    error.code = sai::error_code::unsupported_field_type;
    error.operation = "cpp_provider.fields";
    error.message = "reflection kind is outside the Transform pilot";
    return sai::failure(std::move(error));
  }
}
sai::access_type access(x3d::core::AccessType type) {
  using native = x3d::core::AccessType;
  switch (type) {
  case native::InitializeOnly:
    return sai::access_type::initialize_only;
  case native::InputOnly:
    return sai::access_type::input_only;
  case native::OutputOnly:
    return sai::access_type::output_only;
  case native::InputOutput:
    return sai::access_type::input_output;
  }
  throw std::logic_error("unrecognized native field access category");
}
bool document_syntax(std::string_view name) {
  return name == "DEF" || name == "USE" || name == "IS" || name == "class" ||
         name == "id" || name == "style";
}
} // namespace

struct SaiOfflineProvider::state {
  std::shared_ptr<Scene> scene = std::make_shared<Scene>();
  // This is an identity/ownership registry only: values, DEF bindings and root
  // occurrences always live in the native Scene/X3DNode, never a mirrored
  // model.
  std::unordered_map<std::uint64_t, std::shared_ptr<x3d::nodes::X3DNode>> nodes;
  std::uint64_t next_id = 1;

  using node_ids = std::vector<std::optional<std::uint64_t>>;
  using graph = std::unordered_map<std::uint64_t, node_ids>;

  // Reconstruct the entire registered containment graph from authoritative
  // native pointers on each node-valued operation, including detached nodes.
  // A replacement is validated instead of the target's existing list so that
  // a valid candidate can repair an out-of-band mutation. Nothing is published
  // until this complete candidate graph has passed validation.
  sai::result<graph> children_graph(std::string_view operation,
                                   std::uint64_t replacement_id = 0,
                                   const x3d::core::MFNode *replacement = nullptr) const {
    std::unordered_map<const x3d::nodes::X3DNode *, std::uint64_t> identities;
    for (const auto &[id, node] : nodes)
      identities.emplace(node.get(), id);

    graph edges;
    for (const auto &[id, node] : nodes) {
      const auto transform =
          std::dynamic_pointer_cast<x3d::nodes::Transform>(node);
      if (!transform)
        return SaiOfflineProvider::error(
            sai::error_code::type_mismatch, std::string(operation),
            "registered native node is not a Transform", "children");
      const auto &children = replacement && id == replacement_id
                                 ? *replacement
                                 : transform->getChildren();
      auto &ids = edges[id];
      ids.reserve(children.size());
      std::unordered_set<std::uint64_t> seen;
      for (const auto &child : children) {
        if (!child) {
          ids.push_back(std::nullopt);
          continue;
        }
        const auto found = identities.find(child.get());
        if (found == identities.end())
          return SaiOfflineProvider::error(
              sai::error_code::invalid_context, std::string(operation),
              "native children contain a node outside this provider", "children");
        if (!seen.insert(found->second).second)
          return SaiOfflineProvider::error(
              sai::error_code::invalid_value, std::string(operation),
              "native children repeat a non-NULL node", "children");
        ids.push_back(found->second);
      }
    }

    // Iterative three-colour DFS: long detached chains cannot exhaust the C++
    // call stack. A completed node may be visited from several parents (USE).
    enum class colour { unseen, active, complete };
    std::unordered_map<std::uint64_t, colour> colours;
    struct frame {
      std::uint64_t id;
      std::size_t next_child = 0;
    };
    std::vector<frame> stack;
    for (const auto &[id, ignored] : edges) {
      if (colours[id] != colour::unseen)
        continue;
      colours[id] = colour::active;
      stack.push_back({id});
      while (!stack.empty()) {
        auto &top = stack.back();
        const auto &children = edges.at(top.id);
        if (top.next_child == children.size()) {
          colours[top.id] = colour::complete;
          stack.pop_back();
          continue;
        }
        const auto child = children[top.next_child++];
        if (!child)
          continue;
        if (colours[*child] == colour::active)
          return SaiOfflineProvider::error(
              sai::error_code::containment_cycle, std::string(operation),
              "native children contain a containment cycle", "children");
        if (colours[*child] == colour::unseen) {
          colours[*child] = colour::active;
          stack.push_back({*child});
        }
      }
    }
    return edges;
  }

  sai::result<std::shared_ptr<x3d::nodes::X3DNode>>
  lookup(std::uint64_t id) const {
    const auto found = nodes.find(id);
    if (found == nodes.end())
      return SaiOfflineProvider::error(sai::error_code::unknown_node, "lookup",
                                       "native node does not exist");
    return found->second;
  }
  sai::result<std::uint64_t>
  identity(const std::shared_ptr<x3d::nodes::X3DNode> &node) const {
    for (const auto &[id, stored] : nodes)
      if (stored == node)
        return id;
    return SaiOfflineProvider::error(
        sai::error_code::invalid_context, "identity",
        "native structure contains a node outside this provider");
  }
};

SaiOfflineProvider::SaiOfflineProvider() : state_(std::make_unique<state>()) {}
SaiOfflineProvider::~SaiOfflineProvider() = default;

sai::result<std::shared_ptr<Scene>> SaiOfflineProvider::native_scene() const {
  if (auto valid = check("native_scene"); !valid)
    return sai::failure(valid.error());
  return state_->scene;
}

sai::result<std::uint64_t>
SaiOfflineProvider::do_create_node(std::string_view type) {
  auto node = x3d::nodes::createX3DNode(std::string(type));
  if (!node)
    return error(sai::error_code::unknown_type, "create_node",
                 "native factory does not know this type");
  const auto id = state_->next_id++;
  state_->nodes.emplace(id, std::move(node));
  return id;
}

sai::result<void> SaiOfflineProvider::do_append_root(std::uint64_t id) {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  state_->scene->addRootNode(*node);
  return {};
}

sai::result<std::vector<std::uint64_t>> SaiOfflineProvider::do_roots() const {
  std::vector<std::uint64_t> ids;
  for (const auto &node : state_->scene->rootNodes) {
    auto id = state_->identity(node);
    if (!id)
      return sai::failure(id.error());
    ids.push_back(*id);
  }
  return ids;
}

sai::result<void> SaiOfflineProvider::do_define_name(std::uint64_t id,
                                                     std::string_view name) {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  if (!(*node)->getDEF().empty() ||
      state_->scene->defs.contains(std::string(name)))
    return error(sai::error_code::duplicate_name, "define_name",
                 "name or native node is already named");
  // Both public native representations are updated; lookup does not consult a
  // shadow SAI name table. Allocation failure is outside the result contract.
  const std::string owned_name{name};
  state_->scene->define(owned_name, *node);
  (*node)->setDEF(owned_name);
  return {};
}

sai::result<std::uint64_t>
SaiOfflineProvider::do_lookup_name(std::string_view name) const {
  auto node = state_->scene->resolve(std::string(name));
  if (!node)
    return error(sai::error_code::invalid_name, "lookup_name",
                 "native scene has no such DEF binding");
  return state_->identity(node);
}

sai::result<std::string>
SaiOfflineProvider::do_node_type(std::uint64_t id) const {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  return (*node)->nodeTypeName();
}

sai::result<std::vector<sai::provider_field>>
SaiOfflineProvider::do_fields(std::uint64_t id) const {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  std::vector<sai::provider_field> result;
  for (const auto &field : (*node)->fields()) {
    if (document_syntax(field.x3dName))
      continue;
    auto field_kind = kind(field.type);
    if (!field_kind)
      return sai::failure(field_kind.error());
    auto field_access = access(field.access);
    const bool children = field.x3dName == "children" &&
                          *field_kind == sai::value_kind::mf_node;
    const bool scalar = *field_kind == sai::value_kind::sf_bool ||
                        *field_kind == sai::value_kind::sf_vec3f ||
                        *field_kind == sai::value_kind::sf_rotation;
    result.push_back({field.x3dName, *field_kind, field_access,
                      children || (scalar &&
                                   field_access != sai::access_type::input_only),
                      children || (scalar &&
                                   field_access == sai::access_type::input_output)});
  }
  return result;
}

sai::result<sai::value>
SaiOfflineProvider::do_read_field(std::uint64_t id,
                                  std::string_view name) const {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  for (const auto &field : (*node)->fields()) {
    if (field.x3dName != name)
      continue;
    if (!field.get)
      return error(sai::error_code::access_denied, "read_field",
                   "native field has no getter", name);
    const auto native = field.get(**node);
    // Reflection's declared X3D kind and the owning std::any payload must
    // agree exactly. Do not convert bool, vector and rotation storage based on
    // a field name or numeric compatibility.
    switch (field.type) {
    case x3d::core::X3DFieldType::SFBool:
      if (const auto *flag = std::any_cast<x3d::core::SFBool>(&native))
        return sai::value{*flag};
      break;
    case x3d::core::X3DFieldType::SFVec3f:
      if (const auto *vec = std::any_cast<x3d::core::SFVec3f>(&native))
        return sai::value{sai::vec3f{vec->x, vec->y, vec->z}};
      break;
    case x3d::core::X3DFieldType::SFRotation:
      if (const auto *rot = std::any_cast<x3d::core::SFRotation>(&native))
        return sai::value{sai::rotation{rot->x, rot->y, rot->z, rot->angle}};
      break;
    default:
      return error(sai::error_code::unsupported_field_type, "read_field",
                   "native field has no supported scalar authoring read", name);
    }
    return error(sai::error_code::type_mismatch, "read_field",
                 "native getter returned the wrong value kind", name);
  }
  return error(sai::error_code::unknown_field, "read_field",
               "native field does not exist", name);
}

sai::result<void> SaiOfflineProvider::do_write_field(std::uint64_t id,
                                                     std::string_view name,
                                                     sai::value payload) {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  auto transform = std::dynamic_pointer_cast<x3d::nodes::Transform>(*node);
  if (!transform)
    return error(sai::error_code::type_mismatch, "write_field",
                 "native node is not a Transform", name);
  // Public generated setters are the native AUTHORING path. This adapter
  // does not use the unchecked reflection setters or a live execution context.
  // The common front end already verifies exact kinds and access. Check again
  // at this conversion boundary so no unsupported payload can reach a setter.
  if (name == "translation" || name == "center" || name == "scale") {
    const auto *vec = std::get_if<sai::vec3f>(&payload);
    if (!vec)
      return error(sai::error_code::type_mismatch, "write_field",
                   "native vector setter requires SFVec3f", name);
    const x3d::core::SFVec3f native{vec->x, vec->y, vec->z};
    if (name == "translation")
      transform->setTranslation(native);
    else if (name == "center")
      transform->setCenter(native);
    else
      transform->setScale(native);
  } else if (name == "rotation" || name == "scaleOrientation") {
    const auto *rot = std::get_if<sai::rotation>(&payload);
    if (!rot)
      return error(sai::error_code::type_mismatch, "write_field",
                   "native rotation setter requires SFRotation", name);
    const x3d::core::SFRotation native{rot->x, rot->y, rot->z, rot->angle};
    if (name == "rotation")
      transform->setRotation(native);
    else
      transform->setScaleOrientation(native);
  } else if (name == "visible" || name == "bboxDisplay") {
    const auto *flag = std::get_if<bool>(&payload);
    if (!flag)
      return error(sai::error_code::type_mismatch, "write_field",
                   "native boolean setter requires SFBool", name);
    if (name == "visible")
      transform->setVisible(*flag);
    else
      transform->setBboxDisplay(*flag);
  } else {
    return error(sai::error_code::unsupported_operation, "write_field",
                 "field has no supported native authoring write", name);
  }
  state_->scene->authoredScalarFields.record(*node, std::string(name));
  return {};
}
sai::result<std::vector<std::optional<std::uint64_t>>>
SaiOfflineProvider::do_read_nodes(std::uint64_t id, std::string_view name) const {
  if (name != "children")
    return error(sai::error_code::unsupported_operation, "read_nodes",
                 "only Transform.children is supported", name);
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  auto graph = state_->children_graph("read_nodes");
  if (!graph)
    return sai::failure(graph.error());
  return std::move(graph->at(id));
}

sai::result<void> SaiOfflineProvider::do_set_nodes(
    std::uint64_t id, std::string_view name,
    const std::vector<std::optional<std::uint64_t>> &ids) {
  if (name != "children")
    return error(sai::error_code::unsupported_operation, "set_nodes",
                 "only Transform.children is supported", name);
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  auto transform = std::dynamic_pointer_cast<x3d::nodes::Transform>(*node);
  if (!transform)
    return error(sai::error_code::type_mismatch, "set_nodes",
                 "native node is not a Transform", name);
  x3d::core::MFNode candidate;
  candidate.reserve(ids.size());
  for (const auto child_id : ids) {
    if (!child_id) {
      candidate.push_back(nullptr);
      continue;
    }
    auto child = state_->lookup(*child_id);
    if (!child)
      return sai::failure(child.error());
    candidate.push_back(*child);
  }
  auto graph = state_->children_graph("set_nodes", id, &candidate);
  if (!graph)
    return sai::failure(graph.error());
  // Publishing the complete vector is the only native mutation. In particular,
  // node-valued fields never enter Scene::authoredScalarFields.
  transform->setChildren(std::move(candidate));
  return {};
}

void SaiOfflineProvider::do_close() noexcept { state_.reset(); }
} // namespace x3d::runtime
