#include "x3d/sai_provider.hpp"
#include "x3d/sai_presentation.hpp"
#include "RuntimeSession.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DChildNode.hpp"
#include "x3d/nodes/X3DGeometryNode.hpp"
#include "x3d/nodes/X3DMetadataObject.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <cmath>
#include <map>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace x3d::sai::experimental::native {
namespace sai = x3d::sai::experimental;
namespace p = sai::provider;
using x3d::nodes::X3DNode;
using x3d::runtime::RuntimeSession;
using x3d::runtime::Scene;
using x3d::runtime::X3DDocument;
namespace {
bool document_syntax(std::string_view n) {
  return n == "DEF" || n == "USE" || n == "IS" || n == "class" || n == "id" ||
         n == "style";
}
sai::access_type access(x3d::core::AccessType a) {
  using A = x3d::core::AccessType;
  switch (a) {
  case A::InitializeOnly:
    return sai::access_type::initialize_only;
  case A::InputOnly:
    return sai::access_type::input_only;
  case A::OutputOnly:
    return sai::access_type::output_only;
  case A::InputOutput:
    return sai::access_type::input_output;
  }
  throw std::logic_error("unknown native access category");
}
sai::value_kind kind(x3d::core::X3DFieldType t) {
  using T = x3d::core::X3DFieldType;
  using V = sai::value_kind;
  switch (t) {
#define KIND(native, portable)                                                 \
  case T::native:                                                              \
    return V::portable
    KIND(SFBool, sf_bool);
    KIND(SFColor, sf_color);
    KIND(SFColorRGBA, sf_color_rgba);
    KIND(SFDouble, sf_double);
    KIND(SFFloat, sf_float);
    KIND(SFImage, sf_image);
    KIND(SFInt32, sf_int32);
    KIND(SFMatrix3d, sf_matrix3d);
    KIND(SFMatrix3f, sf_matrix3f);
    KIND(SFMatrix4d, sf_matrix4d);
    KIND(SFMatrix4f, sf_matrix4f);
    KIND(SFNode, sf_node);
    KIND(SFRotation, sf_rotation);
    KIND(SFString, sf_string);
    KIND(SFTime, sf_time);
    KIND(SFVec2d, sf_vec2d);
    KIND(SFVec2f, sf_vec2f);
    KIND(SFVec3d, sf_vec3d);
    KIND(SFVec3f, sf_vec3f);
    KIND(SFVec4d, sf_vec4d);
    KIND(SFVec4f, sf_vec4f);
    KIND(MFBool, mf_bool);
    KIND(MFColor, mf_color);
    KIND(MFColorRGBA, mf_color_rgba);
    KIND(MFDouble, mf_double);
    KIND(MFFloat, mf_float);
    KIND(MFImage, mf_image);
    KIND(MFInt32, mf_int32);
    KIND(MFMatrix3d, mf_matrix3d);
    KIND(MFMatrix3f, mf_matrix3f);
    KIND(MFMatrix4d, mf_matrix4d);
    KIND(MFMatrix4f, mf_matrix4f);
    KIND(MFNode, mf_node);
    KIND(MFRotation, mf_rotation);
    KIND(MFString, mf_string);
    KIND(MFTime, mf_time);
    KIND(MFVec2d, mf_vec2d);
    KIND(MFVec2f, mf_vec2f);
    KIND(MFVec3d, mf_vec3d);
    KIND(MFVec3f, mf_vec3f);
    KIND(MFVec4d, mf_vec4d);
    KIND(MFVec4f, mf_vec4f);
    KIND(SFEnum, sf_string);
    KIND(MFEnum, mf_string);
#undef KIND
  }
  throw std::logic_error("unknown native field kind");
}

const x3d::core::FieldInfo *field_info(const X3DNode &n,
                                       std::string_view name) {
  const auto &fs = n.fields();
  const auto found = std::find_if(
      fs.begin(), fs.end(), [&](const auto &f) { return f.x3dName == name; });
  return found == fs.end() ? nullptr : &*found;
}
bool supported_field(const X3DNode &n, const x3d::core::FieldInfo &f) {
  using T = x3d::core::X3DFieldType;
  if (f.x3dName == "metadata")
    return true;
  if (n.nodeTypeName() == "Transform")
    return f.x3dName == "children" || f.type == T::SFBool ||
           f.type == T::SFVec3f || f.type == T::SFRotation;
  if (n.nodeTypeName() == "PositionInterpolator")
    return f.type == T::SFFloat || f.type == T::MFFloat ||
           f.type == T::SFVec3f || f.type == T::MFVec3f;
  if (n.nodeTypeName() == "Shape")
    return f.x3dName == "geometry";
  if (n.nodeTypeName() == "Box")
    return f.x3dName == "size" || f.x3dName == "solid";
  return false;
}
bool accepts_node(const X3DNode &owner, std::string_view field,
                  const std::shared_ptr<X3DNode> &child) {
  if (!child)
    return true;
  if (field == "metadata")
    return dynamic_cast<const x3d::nodes::X3DMetadataObject *>(child.get()) !=
           nullptr;
  if (owner.nodeTypeName() == "Transform" && field == "children")
    return dynamic_cast<const x3d::nodes::X3DChildNode *>(child.get()) !=
           nullptr;
  if (owner.nodeTypeName() == "Shape" && field == "geometry")
    return dynamic_cast<const x3d::nodes::X3DGeometryNode *>(child.get()) !=
           nullptr;
  return false;
}
sai::vec3f portable(const x3d::core::SFVec3f &v) { return {v.x, v.y, v.z}; }
std::any scalar_native(const sai::value &v) {
  return std::visit(
      [](const auto &p) -> std::any {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, sai::vec3f>)
          return x3d::core::SFVec3f{p.x, p.y, p.z};
        else if constexpr (std::is_same_v<T, sai::rotation>)
          return x3d::core::SFRotation{p.x, p.y, p.z, p.angle};
        else if constexpr (std::is_same_v<T, std::vector<sai::vec3f>>) {
          x3d::core::MFVec3f out;
          out.reserve(p.size());
          for (const auto &v : p)
            out.push_back({v.x, v.y, v.z});
          return out;
        } else if constexpr (std::is_same_v<T, bool> ||
                             std::is_same_v<T, float> ||
                             std::is_same_v<T, std::vector<float>>)
          return p;
        else
          throw std::invalid_argument("payload kind is outside native provider "
                                      "conversion capabilities");
      },
      v);
}
sai::value scalar_portable(const x3d::core::FieldInfo &f, const X3DNode &n) {
  const auto v = f.get(n);
  using T = x3d::core::X3DFieldType;
  switch (f.type) {
  case T::SFBool:
    return std::any_cast<bool>(v);
  case T::SFFloat:
    return std::any_cast<float>(v);
  case T::MFFloat:
    return std::any_cast<x3d::core::MFFloat>(v);
  case T::SFVec3f:
    return portable(std::any_cast<x3d::core::SFVec3f>(v));
  case T::SFRotation: {
    const auto r = std::any_cast<x3d::core::SFRotation>(v);
    return sai::rotation{r.x, r.y, r.z, r.angle};
  }
  case T::MFVec3f: {
    std::vector<sai::vec3f> out;
    for (const auto &entry : std::any_cast<const x3d::core::MFVec3f &>(v))
      out.push_back(portable(entry));
    return out;
  }
  default:
    throw std::logic_error("unsupported native scalar conversion");
  }
}
} // namespace

namespace detail {
struct presentation_state {
  const p::service *owner = nullptr; // Compared only; never dereferenced.
  p::scene source;
  std::shared_ptr<const presentation_frame> current;
  std::set<x3d::runtime::extract::RenderItemId> live;
  std::map<std::vector<std::uint64_t>, std::uint64_t> placements;
  using mesh_ref = x3d::runtime::extract::MeshRef;
  std::map<mesh_ref, std::uint64_t, std::owner_less<mesh_ref>> meshes;
  std::uint64_t next_placement = 1, next_mesh = 1;

  static std::uint64_t mint(std::uint64_t &next) {
    if (next == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("native presentation identity exhausted");
    return next++;
  }
  result<void> capture(RuntimeSession &session,
               const std::unordered_map<const X3DNode *, std::uint64_t> &ids,
               const x3d::runtime::extract::RenderDelta &delta,
               std::optional<event_time> time) {
    if (session.extractor().budgetExceeded())
      return failure(sai_error{error_code::resource_limit, "native.presentation.capture",
          "native extraction budget prevented a complete presentation frame"});
    auto next_live = live;
    for (auto id : delta.removed) next_live.erase(id);
    for (auto id : delta.added) next_live.insert(id);
    decltype(placements) next_placements;
    decltype(meshes) next_meshes;
    auto placement_counter = next_placement, mesh_counter = next_mesh;
    auto frame = std::make_shared<presentation_frame>();
    frame->source = source;
    frame->native_tick = session.context().tickGeneration();
    frame->host_time = time;
    frame->items.reserve(next_live.size());
    for (auto id : next_live) {
      const auto &item = session.extractor().item(id);
      std::vector<std::uint64_t> path;
      path.reserve(item.path.size());
      for (auto node : item.path) {
        const auto found = ids.find(node);
        if (found == ids.end())
          throw std::logic_error("presentation path contains a foreign node");
        path.push_back(found->second);
      }
      const auto previous = placements.find(path);
      const auto placement = previous == placements.end()
          ? mint(placement_counter) : previous->second;
      if (!next_placements.emplace(std::move(path), placement).second)
        throw std::logic_error("native extractor repeated a live path");
      auto mesh = next_meshes.find(item.mesh);
      if (mesh == next_meshes.end()) {
        const auto previous_mesh = meshes.find(item.mesh);
        const auto key = previous_mesh == meshes.end()
            ? mint(mesh_counter) : previous_mesh->second;
        mesh = next_meshes.emplace(item.mesh, key).first;
      }
      frame->items.push_back({placement, mesh->second, item.worldTransform,
                              item.mesh});
    }
    // Commit only a complete projection. These maps contain current objects,
    // never all historical paths or mesh owners. Older frames belong to hosts.
    live = std::move(next_live);
    placements = std::move(next_placements);
    meshes = std::move(next_meshes);
    next_placement = placement_counter;
    next_mesh = mesh_counter;
    current = std::move(frame);
    return {};
  }
};
} // namespace detail

struct backend::storage {
  struct fixture {
    std::shared_ptr<X3DDocument> setup = std::make_shared<X3DDocument>();
    std::unique_ptr<RuntimeSession> session;
    std::map<std::uint64_t, std::shared_ptr<X3DNode>> nodes;
    std::unordered_map<const X3DNode *, std::uint64_t> identities;
    std::uint64_t next_node = 1;
    native_evidence evidence;
    std::shared_ptr<detail::presentation_state> presentation;
    std::vector<address> retained_seeds;
    std::size_t next_retained_seed = 0;
    Scene &scene() { return session ? session->scene() : setup->scene; }
    const Scene &scene() const {
      return session ? session->scene() : setup->scene;
    }
    const X3DDocument &document() const {
      return session ? session->document() : *setup;
    }
    result<std::uint64_t> identity(const std::shared_ptr<X3DNode> &n) const {
      const auto i = identities.find(n.get());
      if (i == identities.end())
        return backend::error(error_code::invalid_context, "native.identity",
                              "native graph contains a foreign node");
      return i->second;
    }
    using edges = std::unordered_map<std::uint64_t,
                                     std::vector<std::optional<std::uint64_t>>>;
    result<edges>
    graph(const address *replace = nullptr, const std::any *candidate = nullptr,
          const std::vector<admitted_write> *writes = nullptr) const {
      edges out;
      for (const auto &[id, node] : nodes) {
        auto &children = out[id];
        for (const auto &f : node->fields()) {
          if (!supported_field(*node, f) || !f.get ||
              (f.type != x3d::core::X3DFieldType::SFNode &&
               f.type != x3d::core::X3DFieldType::MFNode))
            continue;
          auto v =
              (replace && replace->node == id && replace->field == f.x3dName)
                  ? *candidate
                  : f.get(*node);
          if (writes)
            for (const auto &w : *writes)
              if (w.target.node == id && w.target.field == f.x3dName)
                v = native_value(w.value);
          x3d::core::MFNode list;
          if (f.type == x3d::core::X3DFieldType::SFNode)
            list.push_back(std::any_cast<x3d::core::SFNode>(v));
          else
            list = std::any_cast<x3d::core::MFNode>(v);
          std::unordered_set<std::uint64_t> seen;
          for (const auto &child : list) {
            if (!child) {
              children.push_back(std::nullopt);
              continue;
            }
            auto id = identity(child);
            if (!id)
              return failure(id.error());
            if (!accepts_node(*node, f.x3dName, child))
              return backend::error(
                  error_code::type_mismatch, "native.graph",
                  "node type is not accepted by the native field");
            if (!seen.insert(*id).second)
              return backend::error(
                  error_code::invalid_value, "native.graph",
                  "native node field repeats a non-NULL node");
            children.push_back(*id);
          }
        }
      }
      enum class colour { unseen, active, complete };
      std::unordered_map<std::uint64_t, colour> colours;
      struct frame {
        std::uint64_t id;
        std::size_t next = 0;
      };
      std::vector<frame> stack;
      for (const auto &[id, unused] : out) {
        if (colours[id] != colour::unseen)
          continue;
        colours[id] = colour::active;
        stack.push_back({id});
        while (!stack.empty()) {
          auto &top = stack.back();
          const auto &children = out.at(top.id);
          if (top.next == children.size()) {
            colours[top.id] = colour::complete;
            stack.pop_back();
            continue;
          }
          const auto child = children[top.next++];
          if (!child)
            continue;
          if (colours[*child] == colour::active)
            return backend::error(
                error_code::containment_cycle, "native.graph",
                "native node fields contain a containment cycle");
          if (colours[*child] == colour::unseen) {
            colours[*child] = colour::active;
            stack.push_back({*child});
          }
        }
      }
      return out;
    }
    std::any native_value(const sai::value &value) const {
      if (const auto *id = std::get_if<sai::node_id>(&value))
        return id->value ? nodes.at(id->value) : x3d::core::SFNode{};
      if (const auto *ids = std::get_if<sai::node_list>(&value)) {
        x3d::core::MFNode out;
        for (const auto id : *ids)
          out.push_back(id.value ? nodes.at(id.value) : nullptr);
        return out;
      }
      return scalar_native(value);
    }
  };
  std::map<std::uint64_t, std::unique_ptr<fixture>> scenes;
  std::shared_ptr<detail::presentation_state> *creation_capture = nullptr;
  std::uint64_t next_scene = 1;
  std::thread::id thread = std::this_thread::get_id();
};
backend::backend() : state_(std::make_unique<storage>()) {}
backend::~backend() = default;
p::capabilities backend::supported() const {
  using S = p::service_id;
  using V = value_kind;
  return {"x3d-cpp native",
          1,
          {{S::scene_authoring},
           {S::host_turns},
           {S::notifications},
           {S::node_values},
           {S::indexed_writes},
           {S::user_data},
           {S::scene_units},
           {S::scene_metadata}},
          {{"Transform", "Grouping", 1},
           {"PositionInterpolator", "Interpolation", 1},
           {"Shape", "Shape", 1},
           {"Box", "Geometry3D", 1}},
          {V::sf_bool, V::sf_float, V::sf_vec3f, V::sf_rotation, V::sf_node,
           V::mf_node, V::mf_float, V::mf_vec3f}};
}
result<std::uint64_t> backend::do_create_scene() {
  auto id = state_->next_scene++;
  auto fixture = std::make_unique<storage::fixture>();
  if (state_->creation_capture) {
    fixture->presentation = std::make_shared<detail::presentation_state>();
    *state_->creation_capture = fixture->presentation;
  }
  state_->scenes.emplace(id, std::move(fixture));
  return id;
}
result<std::vector<unit_declaration>>
backend::do_units(std::uint64_t scene) const {
  const auto &units = state_->scenes.at(scene)->document().head.units;
  std::vector<unit_declaration> out;
  out.reserve(units.size());
  for (const auto &unit : units)
    out.push_back({unit.category, unit.name, unit.conversionFactor});
  return out;
}
result<void> backend::do_declare_unit(std::uint64_t scene,
                                     const unit_declaration &unit) {
  auto &f = *state_->scenes.at(scene);
  if (f.session)
    return error(error_code::access_denied, "native.declare_unit",
                 "setup-only unit declaration after activation");
  // Preserve document provenance without exposing a half-updated document if
  // preparing either owning vector fails. The frontend validates declarations.
  auto units = f.setup->head.units;
  units.push_back({unit.category, unit.name, unit.conversion_factor});
  auto source_units = units;
  f.setup->head.units.swap(units);
  f.setup->scene.sourceUnits.swap(source_units);
  return {};
}
result<std::vector<metadata_entry>>
backend::do_metadata(std::uint64_t scene) const {
  const auto &metadata = state_->scenes.at(scene)->document().head.meta;
  std::vector<metadata_entry> out;
  out.reserve(metadata.size());
  for (const auto &entry : metadata)
    out.push_back({entry.name, entry.content});
  return out;
}
result<void> backend::do_set_metadata(
    std::uint64_t scene, std::string_view key,
    const std::optional<std::string> &value) {
  auto &f = *state_->scenes.at(scene);
  if (f.session)
    return error(error_code::access_denied, "native.set_metadata",
                 "setup-only metadata authoring after activation");
  auto metadata = f.setup->head.meta;
  const auto found = std::find_if(metadata.begin(), metadata.end(),
      [&](const auto &entry) { return entry.name == key; });
  if (value) {
    if (found == metadata.end()) {
      x3d::runtime::Meta entry;
      entry.name = key;
      entry.content = *value;
      metadata.push_back(std::move(entry));
    } else
      found->content = *value;
  } else if (found != metadata.end())
    metadata.erase(found);
  f.setup->head.meta.swap(metadata);
  return {};
}
result<std::uint64_t> backend::do_create_node(std::uint64_t scene,
                                              std::string_view type) {
  auto node = x3d::nodes::createX3DNode(std::string(type));
  if (!node)
    return error(error_code::unknown_type, "native.create_node",
                 "native factory does not know the type");
  const auto caps = supported();
  if (std::none_of(caps.nodes.begin(), caps.nodes.end(),
                   [&](const auto &n) { return n.name == type; }))
    return error(error_code::unsupported_operation, "native.create_node",
                 "known native node is outside provider capabilities");
  auto &f = *state_->scenes.at(scene);
  auto id = f.next_node++;
  f.identities.emplace(node.get(), id);
  f.nodes.emplace(id, std::move(node));
  return id;
}
result<void> backend::do_append_root(const address &a) {
  auto &f = *state_->scenes.at(a.scene);
  f.scene().addRootNode(f.nodes.at(a.node));
  return {};
}
result<void> backend::do_define_name(const address &a, std::string_view name) {
  auto &f = *state_->scenes.at(a.scene);
  auto n = f.nodes.at(a.node);
  if (!n->getDEF().empty() || f.scene().defs.contains(std::string(name)))
    return error(error_code::duplicate_name, "native.define_name",
                 "node or name already named");
  f.scene().define(std::string(name), n);
  n->setDEF(std::string(name));
  return {};
}
result<std::vector<std::uint64_t>>
backend::do_roots(std::uint64_t scene) const {
  const auto &f = *state_->scenes.at(scene);
  std::vector<std::uint64_t> out;
  for (const auto &n : f.scene().rootNodes) {
    auto id = f.identity(n);
    if (!id)
      return failure(id.error());
    out.push_back(*id);
  }
  return out;
}
result<std::uint64_t> backend::do_named(std::uint64_t scene,
                                        std::string_view name) const {
  const auto &f = *state_->scenes.at(scene);
  auto n = f.scene().resolve(std::string(name));
  if (!n)
    return error(error_code::unknown_node, "native.named", "unknown DEF name");
  return f.identity(n);
}
result<std::string> backend::do_type_name(const address &a) const {
  return state_->scenes.at(a.scene)->nodes.at(a.node)->nodeTypeName();
}
result<std::vector<p::field_info>> backend::do_fields(const address &a) const {
  const auto &n = *state_->scenes.at(a.scene)->nodes.at(a.node);
  std::vector<p::field_info> out;
  for (const auto &f : n.fields())
    if (!document_syntax(f.x3dName))
      out.push_back(
          {f.x3dName, kind(f.type), access(f.access), supported_field(n, f)});
  return out;
}
result<backend::backend_value> backend::do_read(const address &a) const {
  const auto &fixture = *state_->scenes.at(a.scene);
  const auto &n = *fixture.nodes.at(a.node);
  const auto *f = field_info(n, a.field);
  if (!f)
    return error(error_code::unknown_field, "native.read",
                 "unknown native field");
  if (!supported_field(n, *f))
    return error(error_code::unsupported_field_type, "native.read",
                 "field outside provider capabilities");
  if (!f->get)
    return error(error_code::access_denied, "native.read",
                 "native field has no getter");
  if (f->type == x3d::core::X3DFieldType::SFNode ||
      f->type == x3d::core::X3DFieldType::MFNode) {
    auto graph = fixture.graph();
    if (!graph)
      return failure(graph.error());
    auto v = f->get(n);
    if (f->type == x3d::core::X3DFieldType::SFNode) {
      auto node = std::any_cast<x3d::core::SFNode>(v);
      if (!node)
        return sai::value{sai::node_id{}};
      auto id = fixture.identity(node);
      if (!id)
        return failure(id.error());
      return sai::value{sai::node_id{*id}};
    }
    sai::node_list out;
    for (const auto &node : std::any_cast<x3d::core::MFNode>(v)) {
      if (!node)
        out.push_back({});
      else {
        auto id = fixture.identity(node);
        if (!id)
          return failure(id.error());
        out.push_back({*id});
      }
    }
    return sai::value{std::move(out)};
  }
  return scalar_portable(*f, n);
}
result<void> backend::do_author(const address &a, const backend_value &value) {
  auto &fixture = *state_->scenes.at(a.scene);
  if (fixture.session)
    return error(error_code::access_denied, "native.author",
                 "setup-only authoring after activation");
  auto n = fixture.nodes.at(a.node);
  const auto *f = field_info(*n, a.field);
  if (!f || !f->set)
    return error(error_code::unknown_field, "native.author",
                 "unknown native setter");
  if (!supported_field(*n, *f))
    return error(error_code::unsupported_operation, "native.author",
                 "field outside provider capabilities");
  auto v = fixture.native_value(value);
  bool nodes = f->type == x3d::core::X3DFieldType::SFNode ||
               f->type == x3d::core::X3DFieldType::MFNode;
  if (nodes) {
    auto graph = fixture.graph(&a, &v);
    if (!graph)
      return failure(graph.error());
  }
  // Generated native authoring setters; never use this path for live ingress.
  if (auto t = std::dynamic_pointer_cast<x3d::nodes::Transform>(n)) {
    if (a.field == "translation")
      t->setTranslation(std::any_cast<x3d::core::SFVec3f>(v));
    else if (a.field == "center")
      t->setCenter(std::any_cast<x3d::core::SFVec3f>(v));
    else if (a.field == "scale")
      t->setScale(std::any_cast<x3d::core::SFVec3f>(v));
    else if (a.field == "rotation")
      t->setRotation(std::any_cast<x3d::core::SFRotation>(v));
    else if (a.field == "scaleOrientation")
      t->setScaleOrientation(std::any_cast<x3d::core::SFRotation>(v));
    else if (a.field == "visible")
      t->setVisible(std::any_cast<bool>(v));
    else if (a.field == "bboxDisplay")
      t->setBboxDisplay(std::any_cast<bool>(v));
    else if (a.field == "children")
      t->setChildren(std::any_cast<x3d::core::MFNode>(v));
    else
      f->set(*n, v);
  } else
    f->set(*n, v);
  if (a.field == "keyValue") {
    if (auto interpolator =
            std::dynamic_pointer_cast<x3d::nodes::PositionInterpolator>(n)) {
      const auto &initial = interpolator->getKeyValue();
      // §19.3.1 pre-input output readback is actual native storage, initialized
      // without an event, timestamp, authored mark or application callback.
      interpolator->emitValue_changed(initial.empty() ? x3d::core::SFVec3f{}
                                                      : initial.front());
    }
  }
  if (!nodes) {
    fixture.scene().authoredScalarFields.record(n, a.field);
    // Provider payloads are already canonical SI, unlike parsed source-unit
    // literals. Carry the mark with every owned node, including detached ones,
    // so runtime activation never scales provider-authored values again.
    fixture.scene().normalizedUnitFields.record(n, a.field);
  }
  return {};
}
result<void> backend::do_add_route(const address &from, const address &to) {
  if (from.scene != to.scene || from.context != to.context || from.context != 1)
    return error(error_code::invalid_context, "native.add_route",
                 "ROUTE endpoints must belong to the same native context");
  auto &f = *state_->scenes.at(from.scene);
  const auto source = f.nodes.find(from.node), sink = f.nodes.find(to.node);
  if (source == f.nodes.end() || sink == f.nodes.end())
    return error(error_code::unknown_node, "native.add_route",
                 "unknown native ROUTE endpoint");
  if (std::any_of(f.scene().routes.begin(), f.scene().routes.end(),
                  [&](const auto &r) {
                    return r.from.lock() == source->second &&
                           r.fromField == from.field &&
                           r.to.lock() == sink->second && r.toField == to.field;
                  }))
    return error(error_code::invalid_route, "native.add_route",
                 "duplicate ROUTE");
  x3d::runtime::Route route(source->second->getDEF(), from.field,
                            sink->second->getDEF(), to.field);
  route.binding = x3d::runtime::Route::Binding::DirectNodes;
  route.from = source->second;
  route.to = sink->second;
  f.scene().routes.push_back(std::move(route));
  return {};
}

result<void> backend::do_validate_configuration(
    std::uint64_t scene, const std::vector<admitted_write> &writes) const {
  const auto &f = *state_->scenes.at(scene);
  auto graph = f.graph(nullptr, nullptr, &writes);
  if (!graph)
    return failure(graph.error());
  if (f.presentation) {
    // The extractor silently truncates deep paths. Its complete-frame adapter
    // therefore admits only a conservative bounded registered graph. Compute
    // longest paths in topological order: a shared node may have a longer path
    // than the first DFS path that reaches it. Detached nodes count as well.
    std::unordered_map<std::uint64_t, std::size_t> incoming, depth;
    for (const auto &[id, children] : *graph) {
      incoming.try_emplace(id, 0);
      depth.emplace(id, 1);
      for (const auto child : children)
        if (child) ++incoming[*child];
    }
    std::vector<std::uint64_t> ready;
    for (const auto &[id, count] : incoming)
      if (count == 0) ready.push_back(id);
    std::size_t consumed = 0;
    while (!ready.empty()) {
      const auto id = ready.back();
      ready.pop_back();
      ++consumed;
      for (const auto child : graph->at(id)) {
        if (!child) continue;
        if (depth.at(id) >= x3d::kMaxNestingDepth)
          return error(error_code::resource_limit, "native.presentation.preflight",
                       "registered graph exceeds complete-presentation depth");
        depth.at(*child) = std::max(depth.at(*child), depth.at(id) + 1);
        if (--incoming.at(*child) == 0) ready.push_back(*child);
      }
    }
    if (consumed != graph->size())
      return error(error_code::containment_cycle, "native.presentation.preflight",
                   "presentation graph could not be ordered");
  }
  for (const auto &[id, node] : f.nodes) {
    const auto interpolator =
        std::dynamic_pointer_cast<x3d::nodes::PositionInterpolator>(node);
    if (!interpolator)
      continue;
    auto keys = interpolator->getKey();
    std::vector<vec3f> positions;
    for (const auto &v : interpolator->getKeyValue())
      positions.push_back(portable(v));
    for (const auto &w : writes) {
      if (w.target.node != id)
        continue;
      if (w.target.field == "key")
        keys = std::get<std::vector<float>>(w.value);
      else if (w.target.field == "keyValue")
        positions = std::get<std::vector<vec3f>>(w.value);
    }
    if (keys.empty() || keys.size() != positions.size() ||
        !std::is_sorted(keys.begin(), keys.end()) ||
        !std::all_of(keys.begin(), keys.end(),
                     [](float v) { return std::isfinite(v); }) ||
        !std::all_of(positions.begin(), positions.end(), [](const auto &v) {
          return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }))
      return error(error_code::invalid_value, "native.configuration",
                   "interpolation requires nonempty ordered finite keys and "
                   "matching finite keyValue entries");
    for (std::size_t i = 1; i < keys.size(); ++i)
      if (!(keys[i] > keys[i - 1]) || !std::isfinite(keys[i] - keys[i - 1]) ||
          !std::isfinite(positions[i].x - positions[i - 1].x) ||
          !std::isfinite(positions[i].y - positions[i - 1].y) ||
          !std::isfinite(positions[i].z - positions[i - 1].z))
        return error(error_code::invalid_value, "native.configuration",
                     "bounded curves require strictly increasing keys and "
                     "finite float adjacent differences");
  }
  return {};
}
result<void> backend::do_activate(std::uint64_t scene) {
  auto &f = *state_->scenes.at(scene);
  x3d::runtime::SessionOptions options;
  options.standardRuntime = false;
  f.session = RuntimeSession::create(std::move(*f.setup), options);
  f.setup.reset();
  // The bounded backend supports interpolation behavior only. Register the
  // real native systems over every owned node, including detached nodes; root
  // occurrence lists remain unmodified. The native context owns these systems.
  for (auto &system : x3d::runtime::makeInterpolatorSystems()) {
    for (const auto &[id, node] : f.nodes)
      system->attach(node.get(), f.session->context());
    f.session->context().addSystem(std::move(system));
  }
  if (!f.session->routes().ok())
    return error(error_code::invalid_route, "native.activate",
                 "native ROUTE bridge rejected an admitted route");
  f.evidence.routes = f.session->routes().routesAdded;
  const auto initial = f.session->fullSnapshot();
  f.evidence.snapshot_items = initial.added.size();
  if (f.presentation) {
    auto captured = f.presentation->capture(*f.session, f.identities, initial, std::nullopt);
    if (!captured) return failure(captured.error());
  }
  f.session->context().addFieldWriteListener(
      [this, scene](const x3d::runtime::FieldAddress &a) {
        auto &f = *state_->scenes.at(scene);
        const auto id = f.identities.find(a.node);
        if (id == f.identities.end())
          return;
        const auto *info = field_info(*a.node, a.field);
        if (info && info->access == x3d::core::AccessType::InputOnly)
          ++f.evidence.input_occurrences;
        const auto publish = [&](const address &source) {
          auto value = do_read(source);
          if (!value)
            throw std::runtime_error(value.error().message);
          record_event(source, std::move(*value));
        };
        if (f.next_retained_seed < f.retained_seeds.size()) {
          const auto &expected = f.retained_seeds[f.next_retained_seed];
          if (expected.node != id->second || expected.field != a.field)
            throw std::runtime_error("native retained seed prefix was interrupted");
          if (++f.next_retained_seed != f.retained_seeds.size())
            return;
          // Only the admitted inputOutput seed prefix is buffered. Its final
          // graph was preflighted as a whole, so an intermediate graph must
          // not be validated or published before all seeds are installed.
          for (const auto &source : f.retained_seeds)
            publish(source);
          f.retained_seeds.clear();
          f.next_retained_seed = 0;
          return;
        }
        if (info && supported_field(*a.node, *info) && info->get)
          publish({scene, 1, id->second, info->x3dName});
      });
  return {};
}
backend::backend_result
backend::do_turn(std::uint64_t scene, event_time time,
                 const std::vector<admitted_write> &writes) {
  auto &f = *state_->scenes.at(scene);
  std::vector<std::any> values;
  values.reserve(writes.size());
  for (const auto &w : writes)
    values.push_back(f.native_value(w.value));
  f.evidence.input_occurrences = 0;
  f.retained_seeds.clear();
  f.next_retained_seed = 0;
  // Retained inputOutput state is staged before inputOnly behavior. Preserve
  // occurrence order within each group, in one native tick and loop-guard
  // scope. No time perturbation or per-input reset is introduced here.
  // EventCascade delivers these direct seeds FIFO before appended ROUTEs and
  // outputs. This bounded backend installs only inputOnly interpolation
  // handlers; retained writes classify dirtiness without evaluating the graph.
  // The listener checks that exact prefix, then resumes per-occurrence reads.
  for (bool retained : {true, false})
    for (std::size_t i = 0; i < writes.size(); ++i) {
      const auto &a = writes[i].target;
      const auto *info = field_info(*f.nodes.at(a.node), a.field);
      const bool stateful =
          info && info->access == x3d::core::AccessType::InputOutput;
      if (stateful == retained) {
        if (stateful)
          f.retained_seeds.push_back(a);
        f.session->context().postEvent(f.nodes.at(a.node).get(), a.field,
                                       std::move(values[i]));
      }
    }
  f.session->tick(time.seconds);
  if (!f.retained_seeds.empty())
    throw std::runtime_error("native retained seed prefix was not delivered");
  const auto delta = f.session->delta();
  auto &e = f.evidence;
  e.tick = f.session->context().tickGeneration();
  e.host_time = f.session->context().now();
  if (auto t = std::dynamic_pointer_cast<x3d::nodes::Transform>(
          f.scene().resolve("Transform"))) {
    e.translation = portable(t->getTranslation());
    e.translation_authored =
        f.scene().authoredScalarFields.contains(t, "translation");
    const auto world = f.session->context().worldTransform(t.get());
    e.world_translation = {world.m[12], world.m[13], world.m[14]};
  }
  if (auto i = std::dynamic_pointer_cast<x3d::nodes::PositionInterpolator>(
          f.scene().resolve("Interpolator"))) {
    e.interpolated = portable(i->getValue_changed());
    e.key_authored = f.scene().authoredScalarFields.contains(i, "key");
    e.key_value_authored =
        f.scene().authoredScalarFields.contains(i, "keyValue");
  }
  e.added = delta.added.size();
  e.removed = delta.removed.size();
  e.transformed = delta.updatedTransform.size();
  e.geometry = delta.updatedGeometry.size();
  e.material = delta.updatedMaterial.size();
  e.rendered_translations.clear();
  for (auto id : delta.updatedTransform) {
    const auto &m = f.session->extractor().item(id).worldTransform.m;
    e.rendered_translations.push_back({m[12], m[13], m[14]});
  }
  if (f.presentation) {
    auto captured = f.presentation->capture(*f.session, f.identities, delta, time);
    if (!captured)
      return tl::unexpected<backend_failure>{
          backend_failure{captured.error(), p::failure_effect::partial_or_unknown}};
  }
  return {};
}
backend::native_evidence backend::inspect(std::uint64_t scene) const {
  if (std::this_thread::get_id() != state_->thread)
    throw std::logic_error("native inspection is owner-thread-affine");
  return state_->scenes.at(scene)->evidence;
}
result<std::shared_ptr<Scene>>
backend::native_scene(std::uint64_t scene) const {
  if (std::this_thread::get_id() != state_->thread)
    return error(error_code::wrong_thread, "native.scene",
                 "native extension is owner-thread-affine");
  const auto found = state_->scenes.find(scene);
  if (found == state_->scenes.end())
    return error(error_code::stale_handle, "native.scene", "scene retired");
  const auto &f = *found->second;
  if (!f.setup)
    return error(error_code::unsupported_operation, "native.scene",
                 "mutable storage inspection is setup-only");
  return std::shared_ptr<Scene>(f.setup, &f.setup->scene);
}
void backend::do_retire(std::uint64_t scene) noexcept {
  const auto f = state_->scenes.find(scene);
  if (f != state_->scenes.end()) {
    f->second->session.reset();
    state_->scenes.erase(f);
  }
}
std::unique_ptr<p::backend> make_backend() {
  return std::make_unique<backend>();
}
result<std::unique_ptr<p::service>> make_service(p::resource_limits limits) {
  return std::make_unique<p::service>(make_backend(), limits);
}

namespace detail {
struct presentation_access {
  static result<presented_scene> create(p::resource_limits limits) {
    auto engine = std::make_unique<backend>();
    auto &native = *engine;
    auto service = std::make_unique<p::service>(std::move(engine), limits);
    std::shared_ptr<presentation_state> captured;
    struct capture_scope {
      backend::storage &storage;
      explicit capture_scope(backend::storage &s,
                             std::shared_ptr<presentation_state> &out)
          : storage(s) { storage.creation_capture = &out; }
      ~capture_scope() { storage.creation_capture = nullptr; }
    };
    result<p::scene> created;
    {
      capture_scope capture(*native.state_, captured);
      created = service->create_scene();
    }
    if (!created) return failure(created.error());
    if (!captured)
      throw std::logic_error("native scene creation did not bind a render feed");
    captured->owner = service.get();
    captured->source = *created;
    render_feed feed;
    feed.state_ = captured;
    return presented_scene{std::move(service), *created, std::move(feed)};
  }
};
} // namespace detail

result<presented_scene> make_presented_scene(p::resource_limits limits) {
  return detail::presentation_access::create(limits);
}
result<std::shared_ptr<const presentation_frame>>
render_feed::snapshot(const p::service &service) const {
  const auto state = state_.lock();
  const auto fail = [](error_code code, std::string message) {
    return failure(sai_error{code, "native.presentation.snapshot", std::move(message)});
  };
  if (!state) return fail(error_code::stale_handle, "native scene is no longer available");
  if (state->owner != &service)
    return fail(error_code::invalid_context, "render feed belongs to another service");
  // The frontend can fault after native execution succeeds (for example when
  // capturing notifications overflows). Never expose that candidate as active.
  auto status = service.state(state->source);
  if (!status) return failure(status.error());
  if (*status != p::activation_state::active)
    return fail(error_code::access_denied, "render feed requires an active scene");
  if (!state->current)
    return fail(error_code::operation_in_progress, "native frame is not available");
  return state->current;
}
} // namespace x3d::sai::experimental::native
