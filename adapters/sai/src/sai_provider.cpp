#include "x3d/sai_provider.hpp"
#include "X3DDocument.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <unordered_map>

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
    result.push_back({field.x3dName, *field_kind, field_access,
                      *field_kind == sai::value_kind::sf_vec3f &&
                          field_access != sai::access_type::input_only,
                      *field_kind == sai::value_kind::sf_vec3f &&
                          field_access == sai::access_type::input_output});
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
    const auto *vec = std::any_cast<x3d::core::SFVec3f>(&native);
    if (!vec)
      return error(sai::error_code::type_mismatch, "read_field",
                   "native getter returned the wrong value kind", name);
    return sai::value{sai::vec3f{vec->x, vec->y, vec->z}};
  }
  return error(sai::error_code::unknown_field, "read_field",
               "native field does not exist", name);
}

sai::result<void> SaiOfflineProvider::do_write_field(std::uint64_t id,
                                                     std::string_view name,
                                                     sai::vec3f payload) {
  auto node = state_->lookup(id);
  if (!node)
    return sai::failure(node.error());
  auto transform = std::dynamic_pointer_cast<x3d::nodes::Transform>(*node);
  if (!transform)
    return error(sai::error_code::type_mismatch, "write_field",
                 "native node is not a Transform", name);
  const x3d::core::SFVec3f native{payload.x, payload.y, payload.z};
  // Checked generated setters are the native AUTHORING path. This adapter does
  // not use the lenient/unchecked reflection setters or a live execution
  // context.
  if (name == "translation")
    transform->setTranslation(native);
  else if (name == "center")
    transform->setCenter(native);
  else if (name == "scale")
    transform->setScale(native);
  else
    return error(sai::error_code::unsupported_operation, "write_field",
                 "field has no supported native authoring write", name);
  state_->scene->authoredScalarFields.record(*node, std::string(name));
  return {};
}
void SaiOfflineProvider::do_close() noexcept { state_.reset(); }
} // namespace x3d::runtime
