#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/bindings/PositionInterpolator.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace sai = x3d::sai::experimental;
namespace bindings = x3d::sai::experimental::bindings;

int main() {
  auto registry =
      sai::generated_type_registry_for<bindings::PositionInterpolator,
                                       bindings::Transform>();
  if (!registry)
    return EXIT_FAILURE;
  sai::browser host{std::move(*registry)};
  auto scene = host.current_scene();
  auto author = scene.edit();
  auto interpolator = author.create<bindings::PositionInterpolator>();
  auto transform = author.create<bindings::Transform>();
  if (!interpolator || !transform)
    return EXIT_FAILURE;
  const auto input =
      interpolator->field(bindings::PositionInterpolator::set_fraction);
  const auto output =
      interpolator->field(bindings::PositionInterpolator::value_changed);
  const auto keys = interpolator->field(bindings::PositionInterpolator::key);
  const auto points =
      interpolator->field(bindings::PositionInterpolator::keyValue);
  const auto translation = transform->field(bindings::Transform::translation);
  if (!author.set(keys, sai::float_list{0, 1}) ||
      !author.set(points, sai::vec3f_list{{0, 0, 0}, {10, 0, 0}}) ||
      !author.add_route(output, translation) ||
      !author.append_root(*interpolator) || !author.append_root(*transform) ||
      !author.commit())
    return EXIT_FAILURE;

  // This evaluator uses only the public SAI. The kernel supplies time, causal
  // links, output validation, ROUTE propagation, and atomic publication.
  auto installed = host.set_runtime_processor(
      scene, [=](sai::runtime_step &step) -> sai::result<void> {
        const auto *event = step.input();
        if (!event || event->target != interpolator->id() ||
            event->field != input.name())
          return {};
        auto key = step.read(keys);
        auto point = step.read(points);
        if (!key)
          return sai::failure(key.error());
        if (!point)
          return sai::failure(point.error());
        if (key->empty())
          return {};
        if (key->size() != point->size() ||
            !std::is_sorted(key->begin(), key->end()))
          return sai::failure(
              {.code = sai::error_code::invalid_value,
               .operation = "interpolate",
               .message = "key and keyValue must have matching ordered entries",
               .node = std::nullopt,
               .declaration = std::nullopt,
               .field = {}});
        const float fraction = std::get<float>(event->payload);
        if (std::isnan(fraction) ||
            !std::all_of(key->begin(), key->end(),
                         [](float k) { return std::isfinite(k); }))
          return sai::failure({.code = sai::error_code::invalid_value,
                               .operation = "interpolate",
                               .message = "this evaluator requires finite keys "
                                          "and a non-NaN fraction",
                               .node = std::nullopt,
                               .declaration = std::nullopt,
                               .field = {}});
        if (fraction <= key->front())
          return step.emit(output, point->front());
        if (fraction >= key->back())
          return step.emit(output, point->back());
        const auto upper = std::upper_bound(key->begin(), key->end(), fraction);
        const auto right = static_cast<std::size_t>(upper - key->begin());
        const auto left = right - 1;
        const float t =
            (fraction - (*key)[left]) / ((*key)[right] - (*key)[left]);
        const auto a = (*point)[left];
        const auto b = (*point)[right];
        return step.emit(output, sai::vec3f{a.x + t * (b.x - a.x),
                                            a.y + t * (b.y - a.y),
                                            a.z + t * (b.z - a.z)});
      });
  if (!installed)
    return EXIT_FAILURE;
  int observations = 0;
  auto observer = scene.observe(output, [&](const sai::event_delivery &event) {
    auto position = scene.snapshot().read(translation);
    if (position && *position == std::get<sai::vec3f>(event.payload))
      ++observations;
  });
  if (!observer)
    return EXIT_FAILURE;
  const std::array fractions{-0.5f, 0.25f, 1.5f};
  const std::array expected{0.f, 2.5f, 10.f};
  for (std::size_t i = 0; i < fractions.size(); ++i) {
    auto events = scene.events({static_cast<double>(i + 1)});
    if (!events.send(input, fractions[i]))
      return EXIT_FAILURE;
    auto completed = events.commit();
    if (!completed || completed->deliveries.size() != 3 ||
        completed->deliveries[1].origin != sai::event_origin::runtime ||
        completed->deliveries[1].cause != 0 ||
        completed->deliveries[2].cause != 1)
      return EXIT_FAILURE;
    auto position = scene.snapshot().read(translation);
    auto last_output = scene.snapshot().read(output);
    const sai::vec3f expected_position{expected[i], 0, 0};
    if (!position || !last_output || *position != expected_position ||
        *last_output != expected_position || scene.drain().delivered != 1)
      return EXIT_FAILURE;
  }
  if (observations != 3)
    return EXIT_FAILURE;
  std::cout
      << "Generated host: interpolation, endpoint clamping, causal ROUTEs, "
         "and settled observation verified for 3 frames.\n";
  return EXIT_SUCCESS;
}
