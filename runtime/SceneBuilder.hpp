// SceneBuilder.hpp
// Fluent, checked construction of an X3DDocument for authoring and export
// code. Part of the slim authoring surface (x3d/authoring.hpp): header-only,
// and it needs only the node bindings, reflection and FieldValueIO.
//
//   x3d::authoring::SceneBuilder b;
//   auto spin = b.add<Transform>().def("Spin");
//   spin.add<Shape>().add<Sphere>();              // slot from containerField
//   auto clock = b.add<TimeSensor>().set("loop", "true");
//   auto turn = b.add("OrientationInterpolator")  // by type name
//                   .set("key", "0 1").set("keyValue", "0 1 0 0 0 1 0 3.14");
//   b.route(clock, "fraction_changed", turn, "set_fraction");
//   b.route(turn, "value_changed", spin, "rotation");
//   X3DDocument doc = b.take();
//
// What the builder checks, each by throwing std::invalid_argument at the call
// that got it wrong (so a bad scene fails where it is written, not when a
// browser loads the file):
//   - a child is attached to a field the parent really has, and that field
//     holds nodes;
//   - set() names a real, writable field, and text values are well formed
//     for the field's type (arity and number syntax; enum tokens);
//   - a ROUTE names real fields, runs from an output to an input, and joins
//     fields of one type;
//   - a DEF name is bound to one node only.
// Not checked: whether a child's node type is legal in the slot (the bindings
// keep node slots untyped) and value ranges (run validateRanges on the result).
#ifndef X3D_RUNTIME_SCENE_BUILDER_HPP
#define X3D_RUNTIME_SCENE_BUILDER_HPP

#include "X3DDocument.hpp"
#include "codecs/FieldValueIO.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace x3d::authoring {

class SceneBuilder;

namespace detail {

using x3d::core::AccessType;
using x3d::core::FieldInfo;
using x3d::core::X3DFieldType;
using x3d::nodes::X3DNode;

[[noreturn]] inline void fail(const std::string &what) {
  throw std::invalid_argument("SceneBuilder: " + what);
}

inline std::string describe(const X3DNode &node) {
  const std::string def = node.getDEF();
  return def.empty() ? node.nodeTypeName()
                     : node.nodeTypeName() + " '" + def + "'";
}

inline const FieldInfo *findField(const X3DNode &node,
                                  const std::string &name) {
  for (const FieldInfo &f : node.fields())
    if (f.x3dName == name) return &f;
  return nullptr;
}

// A ROUTE may name an inputOutput field `f` as `set_f` or `f_changed`.
inline const FieldInfo *findRouteField(const X3DNode &node,
                                       const std::string &name) {
  if (const FieldInfo *f = findField(node, name)) return f;
  const FieldInfo *f = nullptr;
  if (name.rfind("set_", 0) == 0)
    f = findField(node, name.substr(4));
  else if (name.size() > 8 &&
           name.compare(name.size() - 8, 8, "_changed") == 0)
    f = findField(node, name.substr(0, name.size() - 8));
  return f && f->access == AccessType::InputOutput ? f : nullptr;
}

} // namespace detail

/**
 * @brief A node being authored: a typed shared_ptr plus the builder it
 *        belongs to. Cheap to copy; every mutator returns the handle so calls
 *        chain. `T` is X3DNode for nodes created by type name.
 */
template <class T> class Node {
public:
  Node(SceneBuilder &builder, std::shared_ptr<T> node)
      : builder_(&builder), node_(std::move(node)) {}

  T *operator->() const { return node_.get(); }
  T &operator*() const { return *node_; }
  const std::shared_ptr<T> &shared() const { return node_; }
  operator std::shared_ptr<x3d::nodes::X3DNode>() const { return node_; }

  /// Bind a DEF name. Throws if the name already names a different node.
  Node &def(const std::string &name);

  /// Create a `C` and attach it. `field` defaults to C's containerField.
  template <class C> Node<C> add(const std::string &field = {});

  /// Create a node by X3D type name and attach it.
  Node<x3d::nodes::X3DNode> add(const std::string &typeName,
                                const std::string &field);

  /// Attach an existing node (a second attach of one node is a USE).
  template <class C>
  Node &add(const Node<C> &child, const std::string &field = {});
  Node &add(std::shared_ptr<x3d::nodes::X3DNode> child,
            const std::string &field = {});

  /// Set a field from X3D text, as it would appear in an XML attribute.
  Node &set(const std::string &field, const std::string &text);
  Node &set(const std::string &field, const char *text) {
    return set(field, std::string(text));
  }

  /// Set a field from a typed value; `V` must be the field's exact C++ type
  /// (SFVec3f for an SFVec3f field, and so on).
  template <class V,
            class = std::enable_if_t<
                !std::is_convertible_v<const V &, std::string>>>
  Node &set(const std::string &field, const V &value);

  /// Run `fn(T&)`: the place for the typed setters (`setRadius(2)`).
  template <class Fn> Node &with(Fn &&fn) {
    std::forward<Fn>(fn)(*node_);
    return *this;
  }

private:
  SceneBuilder *builder_;
  std::shared_ptr<T> node_;
};

/**
 * @brief Builds one X3DDocument. See the file comment for what is checked.
 */
class SceneBuilder {
public:
  using X3DNode = x3d::nodes::X3DNode;

  SceneBuilder() = default;
  /// Continue authoring an existing document (one just parsed, say).
  explicit SceneBuilder(x3d::runtime::X3DDocument doc) : doc_(std::move(doc)) {}

  /// Create a detached node: attach it with Node::add or addRoot.
  template <class T> Node<T> create() {
    return Node<T>(*this, std::make_shared<T>());
  }
  Node<X3DNode> create(const std::string &typeName) {
    auto node = x3d::nodes::X3DNodeFactory::create(typeName);
    if (!node) detail::fail("unknown node type '" + typeName + "'");
    return Node<X3DNode>(*this, std::move(node));
  }

  /// Create a node and make it a root of the scene.
  template <class T> Node<T> add() {
    auto n = create<T>();
    doc_.scene.rootNodes.push_back(n.shared());
    return n;
  }
  Node<X3DNode> add(const std::string &typeName) {
    auto n = create(typeName);
    doc_.scene.rootNodes.push_back(n.shared());
    return n;
  }
  /// Make an existing node a root of the scene.
  SceneBuilder &addRoot(std::shared_ptr<X3DNode> node) {
    if (!node) detail::fail("addRoot: null node");
    adoptDef(node);
    doc_.scene.rootNodes.push_back(std::move(node));
    return *this;
  }

  /// A handle for a node already in the document, by DEF name. Throws if
  /// the name is unknown.
  Node<X3DNode> find(const std::string &defName) {
    auto node = doc_.scene.resolve(defName);
    if (!node) detail::fail("no node with DEF '" + defName + "'");
    return Node<X3DNode>(*this, std::move(node));
  }

  /**
   * @brief Add a checked ROUTE. An endpoint without a DEF gets a generated
   *        one, since an encoded ROUTE can only name its endpoints.
   * @throws std::invalid_argument for an unknown field, a source that is not
   *         an output, a destination that is not an input, or a type mismatch.
   */
  SceneBuilder &route(const std::shared_ptr<X3DNode> &from,
                      const std::string &fromField,
                      const std::shared_ptr<X3DNode> &to,
                      const std::string &toField) {
    using detail::AccessType;
    if (!from || !to) detail::fail("route: null endpoint");
    const auto *src = detail::findRouteField(*from, fromField);
    if (!src)
      detail::fail("route: " + detail::describe(*from) + " has no field '" +
                   fromField + "'");
    const auto *dst = detail::findRouteField(*to, toField);
    if (!dst)
      detail::fail("route: " + detail::describe(*to) + " has no field '" +
                   toField + "'");
    if (src->access != AccessType::OutputOnly &&
        src->access != AccessType::InputOutput)
      detail::fail("route: " + detail::describe(*from) + "." + fromField +
                   " is not an output");
    if (dst->access != AccessType::InputOnly &&
        dst->access != AccessType::InputOutput)
      detail::fail("route: " + detail::describe(*to) + "." + toField +
                   " is not an input");
    if (src->type != dst->type)
      detail::fail("route: " + detail::describe(*from) + "." + fromField +
                   " and " + detail::describe(*to) + "." + toField +
                   " have different field types");
    return routeUnchecked(from, fromField, to, toField);
  }

  /// Add a ROUTE without field checks: for fields reflection cannot see
  /// (Script and shader author fields, ProtoInstance interfaces).
  SceneBuilder &routeUnchecked(const std::shared_ptr<X3DNode> &from,
                               const std::string &fromField,
                               const std::shared_ptr<X3DNode> &to,
                               const std::string &toField) {
    if (!from || !to) detail::fail("route: null endpoint");
    x3d::runtime::Route r(ensureDef(from), fromField, ensureDef(to), toField);
    r.from = from;
    r.to = to;
    doc_.scene.routes.push_back(std::move(r));
    return *this;
  }

  /// Bind `name` to `node` (Node::def calls this).
  void define(const std::shared_ptr<X3DNode> &node, const std::string &name) {
    if (name.empty()) detail::fail("def: empty DEF name");
    auto &defs = doc_.scene.defs;
    auto it = defs.find(name);
    if (it != defs.end() && it->second != node)
      detail::fail("def: '" + name + "' already names a " +
                   it->second->nodeTypeName());
    const std::string old = node->getDEF();
    if (!old.empty() && old != name) {
      auto prev = defs.find(old);
      if (prev != defs.end() && prev->second == node) defs.erase(prev);
      for (auto &r : doc_.scene.routes) {
        if (r.from.lock() == node) r.fromNode = name;
        if (r.to.lock() == node) r.toNode = name;
      }
    }
    node->setDEF(name);
    defs[name] = node;
  }

  /// Register a DEF the node already carries (set through the typed API).
  void adoptDef(const std::shared_ptr<X3DNode> &node) {
    const std::string def = node->getDEF();
    if (!def.empty()) define(node, def);
  }

  /// Attach `child` to `parent.field` (Node::add calls this).
  void attach(X3DNode &parent, std::shared_ptr<X3DNode> child,
              std::string field) {
    if (!child) detail::fail("add: null child");
    if (field.empty()) field = child->defaultContainerField();
    const auto *f = detail::findField(parent, field);
    if (!f)
      detail::fail("add: " + detail::describe(parent) + " has no field '" +
                   field + "' (for a " + child->nodeTypeName() + ")");
    if (!f->isNode() || !f->isWritable() || !f->isReadable())
      detail::fail("add: " + detail::describe(parent) + "." + field +
                   " does not hold nodes");
    adoptDef(child);
    if (f->type == x3d::core::X3DFieldType::SFNode) {
      f->set(parent, std::any(x3d::core::SFNode(std::move(child))));
    } else {
      auto list = std::any_cast<x3d::core::MFNode>(f->get(parent));
      list.push_back(std::move(child));
      f->set(parent, std::any(std::move(list)));
    }
    doc_.scene.recordChildField(&parent, field);
  }

  x3d::runtime::X3DDocument &document() { return doc_; }
  const x3d::runtime::X3DDocument &document() const { return doc_; }

  /// Hand the finished document over; the builder is left empty.
  x3d::runtime::X3DDocument take() {
    x3d::runtime::X3DDocument out = std::move(doc_);
    doc_ = x3d::runtime::X3DDocument{};
    nextDef_ = 1;
    return out;
  }

private:
  std::string ensureDef(const std::shared_ptr<X3DNode> &node) {
    if (node->getDEF().empty()) {
      std::string name;
      do {
        name = "_" + node->nodeTypeName() + std::to_string(nextDef_++);
      } while (doc_.scene.defs.count(name));
      define(node, name);
    } else {
      adoptDef(node);
    }
    return node->getDEF();
  }

  x3d::runtime::X3DDocument doc_;
  unsigned nextDef_ = 1;
};

// ---- Node<T> members that need the complete SceneBuilder ----

template <class T> Node<T> &Node<T>::def(const std::string &name) {
  builder_->define(node_, name);
  return *this;
}

template <class T>
template <class C>
Node<C> Node<T>::add(const std::string &field) {
  auto child = builder_->template create<C>();
  builder_->attach(*node_, child.shared(), field);
  return child;
}

template <class T>
Node<x3d::nodes::X3DNode> Node<T>::add(const std::string &typeName,
                                       const std::string &field) {
  auto child = builder_->create(typeName);
  builder_->attach(*node_, child.shared(), field);
  return child;
}

template <class T>
template <class C>
Node<T> &Node<T>::add(const Node<C> &child, const std::string &field) {
  builder_->attach(*node_, child.shared(), field);
  return *this;
}

template <class T>
Node<T> &Node<T>::add(std::shared_ptr<x3d::nodes::X3DNode> child,
                      const std::string &field) {
  builder_->attach(*node_, std::move(child), field);
  return *this;
}

template <class T>
Node<T> &Node<T>::set(const std::string &field, const std::string &text) {
  const auto *f = detail::findField(*node_, field);
  if (!f)
    detail::fail("set: " + detail::describe(*node_) + " has no field '" +
                 field + "'");
  if (f->isNode())
    detail::fail("set: " + detail::describe(*node_) + "." + field +
                 " holds nodes; use add()");
  if (f->isEnum()) {
    if (!f->setEnumString || !f->getEnumString)
      detail::fail("set: " + detail::describe(*node_) + "." + field +
                   " is not writable");
    f->setEnumString(*node_, text);
    // setEnumString ignores a token it does not know; catch that here.
    if (f->type == x3d::core::X3DFieldType::SFEnum &&
        f->getEnumString(*node_) != x3d::codec::stripEnumQuotes(text))
      detail::fail("set: '" + text + "' is not a value of " +
                   detail::describe(*node_) + "." + field);
    return *this;
  }
  if (!f->isWritable())
    detail::fail("set: " + detail::describe(*node_) + "." + field +
                 " is not writable");
  if (auto problem = x3d::codec::checkValueSyntax(f->type, text))
    detail::fail("set: " + detail::describe(*node_) + "." + field + ": " +
                 *problem);
  std::any value = x3d::codec::parseValue(f->type, text);
  if (!value.has_value())
    detail::fail("set: " + detail::describe(*node_) + "." + field +
                 " cannot be set from text");
  f->set(*node_, value);
  return *this;
}

template <class T>
template <class V, class>
Node<T> &Node<T>::set(const std::string &field, const V &value) {
  const auto *f = detail::findField(*node_, field);
  if (!f)
    detail::fail("set: " + detail::describe(*node_) + " has no field '" +
                 field + "'");
  if (f->isNode())
    detail::fail("set: " + detail::describe(*node_) + "." + field +
                 " holds nodes; use add()");
  if (!f->isWritable())
    detail::fail("set: " + detail::describe(*node_) + "." + field +
                 " is not writable");
  try {
    f->set(*node_, std::any(value));
  } catch (const std::bad_any_cast &) {
    detail::fail("set: wrong C++ type for " + detail::describe(*node_) + "." +
                 field);
  }
  return *this;
}

} // namespace x3d::authoring

#endif // X3D_RUNTIME_SCENE_BUILDER_HPP
