#include "RuntimeSession.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/sai_hosted.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <type_traits>
#include <unordered_map>

namespace x3d::runtime {
namespace sai = x3d::sai::experimental;
namespace hosted = sai::hosted;
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
bool supported_kind(x3d::core::X3DFieldType t) {
  using T = x3d::core::X3DFieldType;
  return t == T::SFFloat || t == T::MFFloat || t == T::SFVec3f ||
         t == T::MFVec3f;
}
sai::vec3f portable(const x3d::core::SFVec3f &v) { return {v.x, v.y, v.z}; }
std::any native(const hosted::payload &v) {
  return std::visit(
      [](const auto &p) -> std::any {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, sai::vec3f>) {
          return x3d::core::SFVec3f{p.x, p.y, p.z};
        } else if constexpr (std::is_same_v<T, std::vector<sai::vec3f>>) {
          x3d::core::MFVec3f out;
          out.reserve(p.size());
          for (const auto &v : p)
            out.push_back({v.x, v.y, v.z});
          return out;
        } else if constexpr (std::is_same_v<T, float> ||
                             std::is_same_v<T, std::vector<float>>) {
          return p;
        } else {
          throw std::invalid_argument(
              "payload kind is outside the hosted native four-kind slice");
        }
      },
      v);
}
hosted::payload portable(const x3d::core::FieldInfo &f, const X3DNode &n) {
  const auto v = f.get(n);
  using T = x3d::core::X3DFieldType;
  switch (f.type) {
  case T::SFFloat:
    return std::any_cast<float>(v);
  case T::MFFloat:
    return std::any_cast<x3d::core::MFFloat>(v);
  case T::SFVec3f:
    return portable(std::any_cast<x3d::core::SFVec3f>(v));
  case T::MFVec3f: {
    std::vector<sai::vec3f> out;
    for (const auto &p : std::any_cast<const x3d::core::MFVec3f &>(v))
      out.push_back(portable(p));
    return out;
  }
  default:
    throw std::logic_error("unsupported native field conversion");
  }
}
const x3d::core::FieldInfo *field_info(const X3DNode &n,
                                       std::string_view name) {
  const auto &fs = n.fields();
  const auto found = std::find_if(
      fs.begin(), fs.end(), [&](const auto &f) { return f.x3dName == name; });
  return found == fs.end() ? nullptr : &*found;
}
} // namespace

struct SaiHostedService::storage {
  struct fixture {
    std::unique_ptr<X3DDocument> setup = std::make_unique<X3DDocument>();
    std::unique_ptr<RuntimeSession> session;
    // Identity registry only, scoped to these three immutable built-in roots.
    // Values and routes are authoritative in native nodes/document/session.
    std::map<std::uint64_t, std::shared_ptr<X3DNode>> nodes;
    std::unordered_map<const X3DNode *, std::uint64_t> identities;
    native_evidence evidence;
    Scene &scene() { return session ? session->scene() : setup->scene; }
    const Scene &scene() const {
      return session ? session->scene() : setup->scene;
    }
  };
  std::map<std::uint64_t, std::unique_ptr<fixture>> scenes;
  std::uint64_t next_scene = 1;
};
SaiHostedService::SaiHostedService() : state_(std::make_unique<storage>()) {}
SaiHostedService::~SaiHostedService() { close(); }

sai::result<std::uint64_t> SaiHostedService::do_create_fixture() {
  auto f = std::make_unique<storage::fixture>();
  auto transform = std::make_shared<x3d::nodes::Transform>();
  transform->setDEF("Transform");
  auto shape = std::make_shared<x3d::nodes::Shape>();
  shape->setGeometry(std::make_shared<x3d::nodes::Box>());
  transform->setChildren({shape});
  auto interpolator = std::make_shared<x3d::nodes::PositionInterpolator>();
  interpolator->setDEF("Interpolator");
  interpolator->setKey({0, 1});
  interpolator->setKeyValue({{0, 0, 0}, {10, 20, 30}});
  f->setup->scene.addRootNode(transform);
  f->setup->scene.addRootNode(interpolator);
  f->setup->scene.authoredScalarFields.record(interpolator, "key");
  f->setup->scene.authoredScalarFields.record(interpolator, "keyValue");
  auto mirror = std::make_shared<x3d::nodes::Transform>();
  mirror->setDEF("Mirror");
  f->setup->scene.addRootNode(mirror);
  f->setup->scene.routes.emplace_back("Interpolator", "value_changed",
                                      "Transform", "translation");
  f->nodes.emplace(1, transform);
  f->nodes.emplace(2, interpolator);
  f->nodes.emplace(3, mirror);
  for (const auto &[id, n] : f->nodes)
    f->identities.emplace(n.get(), id);
  const auto id = state_->next_scene++;
  state_->scenes.emplace(id, std::move(f));
  return id;
}
sai::result<std::vector<std::uint64_t>>
SaiHostedService::do_roots(std::uint64_t scene) const {
  const auto &f = *state_->scenes.at(scene);
  std::vector<std::uint64_t> out;
  for (const auto &n : f.scene().rootNodes)
    out.push_back(f.identities.at(n.get()));
  return out;
}
sai::result<std::uint64_t>
SaiHostedService::do_named(std::uint64_t scene, std::string_view name) const {
  const auto &f = *state_->scenes.at(scene);
  const auto n = f.scene().resolve(std::string(name));
  if (!n)
    return error(sai::error_code::unknown_node, "native.named",
                 "unknown fixture DEF name");
  return f.identities.at(n.get());
}
sai::result<std::string>
SaiHostedService::do_type_name(const address &a) const {
  return state_->scenes.at(a.scene)->nodes.at(a.node)->nodeTypeName();
}
sai::result<std::vector<hosted::field_info>>
SaiHostedService::do_fields(const address &a) const {
  std::vector<hosted::field_info> out;
  for (const auto &f : state_->scenes.at(a.scene)->nodes.at(a.node)->fields())
    if (!document_syntax(f.x3dName))
      out.push_back(
          {f.x3dName, kind(f.type), access(f.access), supported_kind(f.type)});
  return out;
}
sai::result<hosted::payload> SaiHostedService::do_read(const address &a) const {
  const auto &n = *state_->scenes.at(a.scene)->nodes.at(a.node);
  const auto *f = field_info(n, a.field);
  if (!f)
    return error(sai::error_code::unknown_field, "native.read",
                 "unknown native field");
  if (!supported_kind(f->type))
    return error(sai::error_code::unsupported_field_type, "native.read",
                 "field is outside the four-kind hosted slice");
  if (!f->get)
    return error(sai::error_code::access_denied, "native.read",
                 "native inputOnly field is not readable");
  return portable(*f, n);
}
sai::result<void> SaiHostedService::do_author(const address &a,
                                              const hosted::payload &value) {
  auto &fixture = *state_->scenes.at(a.scene);
  if (fixture.session)
    return error(sai::error_code::access_denied, "native.author",
                 "setup-only authoring after activation");
  auto &n = *fixture.nodes.at(a.node);
  const auto *f = field_info(n, a.field);
  if (!f || !f->set)
    return error(sai::error_code::unknown_field, "native.author",
                 "unknown or unsettable native field");
  f->set(n,
         native(value)); // Setup authoring only. Never used for live ingress.
  fixture.setup->scene.authoredScalarFields.record(fixture.nodes.at(a.node),
                                                   a.field);
  return {};
}
sai::result<void> SaiHostedService::do_add_route(const address &from,
                                                 const address &to) {
  auto &f = *state_->scenes.at(from.scene);
  if (f.session)
    return error(sai::error_code::access_denied, "native.add_route",
                 "setup-only ROUTE authoring after activation");
  const auto from_name = f.nodes.at(from.node)->getDEF();
  const auto to_name = f.nodes.at(to.node)->getDEF();
  const auto duplicate =
      std::any_of(f.setup->scene.routes.begin(), f.setup->scene.routes.end(),
                  [&](const auto &r) {
                    return r.fromNode == from_name &&
                           r.fromField == from.field && r.toNode == to_name &&
                           r.toField == to.field;
                  });
  if (duplicate)
    return error(sai::error_code::invalid_route, "native.add_route",
                 "ROUTE already exists");
  f.setup->scene.routes.emplace_back(from_name, from.field, to_name, to.field);
  return {};
}
sai::result<void> SaiHostedService::do_validate_configuration(
    std::uint64_t scene, const std::vector<admitted_write> &writes) const {
  const auto &f = *state_->scenes.at(scene);
  const auto &interpolator =
      dynamic_cast<const x3d::nodes::PositionInterpolator &>(*f.nodes.at(2));
  auto keys = interpolator.getKey();
  std::vector<sai::vec3f> positions;
  for (const auto &v : interpolator.getKeyValue())
    positions.push_back(portable(v));
  for (const auto &w : writes) {
    if (w.target.node != 2)
      continue;
    if (w.target.field == "key")
      keys = std::get<std::vector<float>>(w.value);
    else if (w.target.field == "keyValue")
      positions = std::get<std::vector<sai::vec3f>>(w.value);
  }
  if (keys.empty() || keys.size() != positions.size() ||
      !std::is_sorted(keys.begin(), keys.end()) ||
      !std::all_of(keys.begin(), keys.end(),
                   [](float v) { return std::isfinite(v); }) ||
      !std::all_of(positions.begin(), positions.end(), [](const auto &v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
      }))
    return error(sai::error_code::invalid_value, "native.configuration",
                 "interpolation requires nonempty ordered finite keys and "
                 "matching finite keyValue entries");
  // Bounded hosted proof restriction: avoid backend-dependent duplicate-key
  // selection and native float segment/difference overflow. This does not
  // change the general native interpolator's semantics or claim full ISO range.
  for (std::size_t i = 1; i < keys.size(); ++i) {
    const float key_delta = keys[i] - keys[i - 1];
    const float x_delta = positions[i].x - positions[i - 1].x;
    const float y_delta = positions[i].y - positions[i - 1].y;
    const float z_delta = positions[i].z - positions[i - 1].z;
    if (!(keys[i] > keys[i - 1]) || !std::isfinite(key_delta) ||
        !std::isfinite(x_delta) || !std::isfinite(y_delta) ||
        !std::isfinite(z_delta))
      return error(sai::error_code::invalid_value, "native.configuration",
                   "hosted curves require strictly increasing keys and finite "
                   "float adjacent differences");
  }
  return {};
}
sai::result<void> SaiHostedService::do_activate(std::uint64_t scene) {
  auto &f = *state_->scenes.at(scene);
  f.session = RuntimeSession::create(std::move(*f.setup));
  f.setup.reset();
  if (!f.session->routes().ok())
    return error(sai::error_code::invalid_route, "native.activate",
                 "native ROUTE bridge rejected a validated fixture route");
  f.evidence.routes = f.session->routes().routesAdded;
  f.evidence.snapshot_items = f.session->fullSnapshot().added.size();
  f.session->context().addFieldWriteListener(
      [this, scene](const FieldAddress &a) {
        const auto &fixture = *state_->scenes.at(scene);
        const auto id = fixture.identities.find(a.node);
        if (id == fixture.identities.end())
          return;
        const auto *info = field_info(*a.node, a.field);
        if (info && supported_kind(info->type) && info->get)
          record_event({scene, 1, id->second, info->x3dName},
                       portable(*info, *a.node));
      });
  return {};
}
SaiHostedService::backend_result
SaiHostedService::do_turn(std::uint64_t scene, sai::event_time time,
                          const std::vector<admitted_write> &writes) {
  auto &f = *state_->scenes.at(scene);
  // Convert the entire admitted batch before entering the event queue. Once
  // postEvent starts, any failure is conservatively partial_or_unknown.
  std::vector<std::any> values;
  values.reserve(writes.size());
  for (const auto &w : writes)
    values.push_back(native(w.value));
  // The bounded common policy admits one complete interpolation curve: seed
  // key/keyValue configuration first, then inputs, stable within each group.
  // This is explicit staging, not a general atomic transaction/order promise.
  for (bool configuration : {true, false})
    for (std::size_t i = 0; i != writes.size(); ++i) {
      const auto &target = writes[i].target;
      const bool is_configuration =
          target.node == 2 &&
          (target.field == "key" || target.field == "keyValue");
      if (configuration == is_configuration)
        f.session->context().postEvent(f.nodes.at(target.node).get(),
                                       target.field, std::move(values[i]));
    }
  f.session->tick(time.seconds);
  const auto delta = f.session->delta();
  auto &e = f.evidence;
  e.tick = f.session->context().tickGeneration();
  e.host_time = f.session->context().now();
  e.translation_authored = f.session->scene().authoredScalarFields.contains(
      f.nodes.at(1), "translation");
  e.key_authored =
      f.session->scene().authoredScalarFields.contains(f.nodes.at(2), "key");
  e.key_value_authored = f.session->scene().authoredScalarFields.contains(
      f.nodes.at(2), "keyValue");
  const auto &transform =
      dynamic_cast<const x3d::nodes::Transform &>(*f.nodes.at(1));
  const auto &interpolator =
      dynamic_cast<const x3d::nodes::PositionInterpolator &>(*f.nodes.at(2));
  e.translation = portable(transform.getTranslation());
  e.interpolated = portable(interpolator.getValue_changed());
  const auto world = f.session->context().worldTransform(&transform);
  e.world_translation = {world.m[12], world.m[13], world.m[14]};
  e.added = delta.added.size();
  e.removed = delta.removed.size();
  e.transformed = delta.updatedTransform.size();
  e.geometry = delta.updatedGeometry.size();
  e.material = delta.updatedMaterial.size();
  e.rendered_translations.clear();
  for (const auto id : delta.updatedTransform) {
    const auto &m = f.session->extractor().item(id).worldTransform.m;
    e.rendered_translations.push_back({m[12], m[13], m[14]});
  }
  return {};
}
SaiHostedService::native_evidence
SaiHostedService::inspect_builtin(std::uint64_t scene) const {
  return state_->scenes.at(scene)->evidence;
}
void SaiHostedService::do_retire(std::uint64_t scene) noexcept {
  // Context/session die before the strong native-node identity registry. Native
  // callback lifetime guards make even retained node handlers inert on
  // teardown.
  const auto found = state_->scenes.find(scene);
  if (found == state_->scenes.end())
    return;
  found->second->session.reset();
  state_->scenes.erase(found);
}
} // namespace x3d::runtime
