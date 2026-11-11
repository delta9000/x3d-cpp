#include <x3d/sai/experimental/bindings/PositionInterpolator.hpp>
#include <x3d/sai/experimental/bindings/Shape.hpp>
#include <x3d/sai/experimental/bindings/Transform.hpp>
#include <x3d/sai/experimental/reference_provider.hpp>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sai = x3d::sai::experimental;
namespace provider = sai::provider;
namespace bindings = sai::bindings;

namespace {
template <class T> T checked(sai::result<T> result) {
  if (!result)
    throw std::runtime_error(result.error().operation + ": " +
                             result.error().message);
  return std::move(*result);
}
void checked(sai::result<void> result) {
  if (!result)
    throw std::runtime_error(result.error().operation + ": " +
                             result.error().message);
}

// All application operations depend on the common service, not its backend.
void animate_scene(provider::service &service) {
  const auto scene = checked(service.create_scene());
  const auto context = checked(service.root_context(scene));
  const auto mover = checked(service.create_node(context, "Transform"));
  const auto shape = checked(service.create_node(context, "Shape"));
  const auto box = checked(service.create_node(context, "Box"));
  const auto path = checked(service.create_node(context, "PositionInterpolator"));
  checked(service.author(shape, bindings::Shape::geometry,
                         provider::node_value{box}));
  checked(service.author(mover, bindings::Transform::children,
                         provider::node_values{shape}));
  checked(service.author(path, bindings::PositionInterpolator::key,
                         sai::float_list{0, 1}));
  checked(service.author(path, bindings::PositionInterpolator::keyValue,
                         sai::vec3f_list{{0, 0, 0}, {10, 0, 0}}));
  checked(service.append_root(context, mover));
  checked(service.append_root(context, path));
  const auto translation =
      checked(service.get_field(mover, bindings::Transform::translation));
  checked(service.add_route(
      context,
      checked(service.get_field(path, bindings::PositionInterpolator::value_changed)),
      translation));

  const auto active = checked(service.activate(scene));
  const auto client = checked(service.connect(active));
  std::optional<sai::vec3f> observed;
  auto subscription = checked(service.observe(
      active, translation, [&](const provider::notification &notice) {
        observed = std::get<sai::vec3f>(notice.value);
      }));

  // Acceptance queues owning input. Only the explicit host pump evaluates it.
  const auto receipt = checked(service.enqueue(
      client, path, bindings::PositionInterpolator::set_fraction, 0.5f));
  const auto turn = checked(service.pump(active, {1.0}));
  if (turn.error)
    throw std::runtime_error(turn.error->operation + ": " + turn.error->message);
  if (turn.state != provider::activation_state::active ||
      checked(receipt.status()).state != provider::request_state::processed)
    throw std::runtime_error("host turn did not process the submitted input");
  if (observed)
    throw std::runtime_error("notification ran before the explicit drain");
  checked(service.dispatch_notifications());
  const auto position = checked(service.read(mover, bindings::Transform::translation));
  if (position != sai::vec3f{5, 0, 0} || observed != position)
    throw std::runtime_error("routed position and notification disagree");
  std::cout << service.supported().backend_name << ": translation = "
            << position.x << ' ' << position.y << ' ' << position.z << '\n';
  subscription.cancel();
  checked(service.retire(scene));
}
} // namespace

int main() {
  try {
    // Select the backend here; animate_scene uses the same provider contract.
    auto service = checked(sai::make_reference_provider());
    animate_scene(*service);
    checked(service->close());
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
