#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/bindings/RigidBody.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace sai = x3d::sai::experimental;
namespace bindings = x3d::sai::experimental::bindings;

// A deliberately small free-motion host, not a RigidBodyPhysics implementation.
// The private allocation stands in for a move-only backend resource.
class motion_state {
public:
  motion_state(sai::vec3f position, sai::vec3f velocity, sai::event_time time)
      : data_(std::make_unique<data>(data{position, velocity, time})) {}
  motion_state(motion_state &&) noexcept = default;
  motion_state &operator=(motion_state &&) noexcept = default;
  sai::vec3f position() const { return data_->position; }
  sai::vec3f velocity() const { return data_->velocity; }
  sai::event_time time() const { return data_->time; }

private:
  struct data {
    sai::vec3f position;
    sai::vec3f velocity;
    sai::event_time time;
  };
  std::unique_ptr<data> data_;
};

struct motion_frame {
  sai::field<sai::vec3f> position_port;
  sai::field<sai::vec3f> velocity_port;
  sai::vec3f position;
  sai::vec3f velocity;
  sai::event_time time;
  bool fail_finish;

  sai::result<void> process(sai::runtime_step &step) {
    if (!step.input())
      return step.emit(position_port, position);
    if (step.input()->target == position_port.node() &&
        step.input()->field == position_port.name())
      return step.ignore_input();
    if (step.input()->target == velocity_port.node() &&
        step.input()->field == velocity_port.name())
      velocity = std::get<sai::vec3f>(step.input()->payload);
    return {};
  }
  sai::result<motion_state> finish() {
    if (fail_finish)
      throw std::runtime_error(
          "backend failed after its routed outputs settled");
    return motion_state{position, velocity, time};
  }
};
static_assert(sai::runtime_frame_adapter<motion_frame>);
static_assert(!std::copy_constructible<motion_state>);

int main() {
  auto registry = sai::generated_type_registry_for<bindings::RigidBody,
                                                   bindings::Transform>();
  if (!registry)
    return EXIT_FAILURE;
  sai::browser host{std::move(*registry)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  auto body = edit.create<bindings::RigidBody>();
  auto transform = edit.create<bindings::Transform>();
  if (!body || !transform)
    return EXIT_FAILURE;
  const auto position = body->field(bindings::RigidBody::position);
  const auto velocity = body->field(bindings::RigidBody::linearVelocity);
  const auto translation = transform->field(bindings::Transform::translation);
  if (!edit.set(velocity, sai::vec3f{3, 0, 0}) ||
      !edit.add_route(position, translation) || !edit.append_root(*body) ||
      !edit.append_root(*transform) || !edit.commit())
    return EXIT_FAILURE;

  bool fail_finish = false;
  auto installed = host.set_runtime_factory(
      scene,
      [=, &fail_finish](const sai::scene_snapshot &base, sai::event_time time)
          -> sai::result<sai::runtime_frame> {
        auto previous = base.runtime_state<motion_state>();
        auto p = base.read(position);
        auto v = base.read(velocity);
        if (!previous)
          return sai::failure(previous.error());
        if (!p)
          return sai::failure(p.error());
        if (!v)
          return sai::failure(v.error());
        if (*previous) {
          const float dt =
              static_cast<float>(time.seconds - (*previous)->time().seconds);
          p->x += v->x * dt;
          p->y += v->y * dt;
          p->z += v->z * dt;
        }
        return sai::runtime_frame{
            motion_frame{position, velocity, *p, *v, time, fail_finish}};
      });
  if (!installed)
    return EXIT_FAILURE;
  bool observations_consistent = true;
  int observations = 0;
  auto observer = scene.observe([&](const sai::change_set &change) {
    const auto snapshot = scene.snapshot();
    const auto state = snapshot.runtime_state<motion_state>();
    const auto p = snapshot.read(position);
    const auto t = snapshot.read(translation);
    const auto v = snapshot.read(velocity);
    observations_consistent &= state && *state && p && t && v &&
                               snapshot.revision() == change.after_revision &&
                               (*state)->position() == *p && *p == *t &&
                               (*state)->velocity() == *v;
    ++observations;
  });
  const auto check_frame = [&](double time, float expected) {
    auto completed = scene.events({time}).commit();
    if (!completed || !completed->state_changes)
      return false;
    const auto snapshot = scene.snapshot();
    auto state = snapshot.runtime_state<motion_state>();
    auto p = snapshot.read(position);
    auto t = snapshot.read(translation);
    if (!state || !*state || !p || !t ||
        (*state)->position() != sai::vec3f{expected, 0, 0} ||
        (*state)->time().seconds != time || *p != (*state)->position() ||
        *t != *p)
      return false;
    return scene.drain().delivered == 1;
  };
  if (!check_frame(1, 0) || !check_frame(3, 6))
    return EXIT_FAILURE;
  const auto historical = scene.snapshot();
  const auto pinned = historical.runtime_state<motion_state>().value();
  fail_finish = true;
  auto failed_frame = scene.events({4});
  if (!failed_frame.send(velocity, sai::vec3f{1, 0, 0}))
    return EXIT_FAILURE;
  auto failed = failed_frame.commit();
  if (failed || failed.error().code != sai::error_code::callback_failed ||
      scene.snapshot().revision() != historical.revision() ||
      scene.snapshot().runtime_state<motion_state>().value().get() !=
          pinned.get() ||
      scene.snapshot().read(velocity).value() != sai::vec3f{3, 0, 0} ||
      scene.drain().delivered != 0)
    return EXIT_FAILURE;
  fail_finish = false;
  auto retried = scene.events({4});
  if (!retried.send(velocity, sai::vec3f{1, 0, 0}) || !retried.commit() ||
      scene.snapshot().read(translation).value() != sai::vec3f{9, 0, 0} ||
      scene.drain().delivered != 1 || !check_frame(5, 10) ||
      pinned->position() != sai::vec3f{6, 0, 0} ||
      pinned->time().seconds != 3 ||
      historical.read(translation).value() != pinned->position() ||
      !observations_consistent || observations != 4)
    return EXIT_FAILURE;
  std::cout
      << "Generated stateful host: move-only checkpoints, routed motion, "
         "late failure, timestamp retry, and historical snapshots verified.\n";
  return EXIT_SUCCESS;
}
