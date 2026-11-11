#include <x3d/sai/experimental/kernel.hpp>
#include <x3d/sai/experimental/session.hpp>
#include <x3d/sai/experimental/image.hpp>
#include <x3d/sai/experimental/matrix.hpp>
#ifdef SAI_CONSUMER_METADATA
#include <x3d/sai/experimental/metadata.hpp>
#endif

#include <array>
#include <cstdlib>
#include <string_view>
#include <utility>

// These consumers use only exported target requirements. Exercise the same
// firewall in embedded builds and relocated SDK installations.
#if __has_include("x3d/core/X3DReflection.hpp") ||                             \
                  __has_include(                                               \
                      "x3d/nodes/X3DSemanticMetadataRegistry.hpp") ||          \
                      __has_include(                                           \
                          "x3d_sai/detail/X3DSemanticMetadataRegistry.hpp") || \
                          __has_include("x3d_sai/detail/metadata_types.hpp")
#error "SAI implementation metadata leaked onto the public include path"
#endif

namespace sai = x3d::sai::experimental;

extern "C" int run_sai_workflow() {
#ifdef SAI_CONSUMER_METADATA
  const std::array names{std::string_view{"Transform"}};
  auto adapted = sai::generated_type_registry(names);
  if (!adapted)
    return EXIT_FAILURE;
  auto registry = std::move(*adapted);
#else
  sai::type_registry registry;
  sai::node_type_descriptor type;
  type.name = "Transform";
  sai::field_descriptor field;
  field.name = "translation";
  field.kind = sai::value_kind::sf_vec3f;
  field.access = sai::access_type::input_output;
  field.default_value = sai::vec3f{};
  type.fields.push_back(std::move(field));
  if (!registry.define(std::move(type)))
    return EXIT_FAILURE;
#endif
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto node = edit.create_node("Transform");
  if (!node)
    return EXIT_FAILURE;
  const auto translation = edit.field(*node, "translation");
  const sai::vec3f authored{1, 2, 3};
  if (!translation || !edit.set(*translation, sai::value{authored}) ||
      !edit.append_root(*node) || !edit.commit())
    return EXIT_FAILURE;
  const auto read = scene.snapshot().read(*translation);
  const auto *actual = read ? std::get_if<sai::vec3f>(&*read) : nullptr;
  if (!actual || *actual != authored)
    return EXIT_FAILURE;
  const auto live = node->get_field("set_translation");
  if (!live || live->name() != "translation")
    return EXIT_FAILURE;
  sai::external_session_host sessions{scene};
  auto client = sessions.create_session();
  if (!client || !client->begin_update())
    return EXIT_FAILURE;
  const sai::vec3f updated{4, 5, 6};
  auto ticket = client->set_value(*live, sai::value{updated});
  const auto before = client->get_value(*live);
  if (!ticket || !before || std::get<sai::vec3f>(*before) != authored ||
      !client->end_update() || !sessions.process_next({1}))
    return EXIT_FAILURE;
  const auto after = client->get_value(*live);
  if (!after || std::get<sai::vec3f>(*after) != updated ||
      ticket->progress().stage != sai::update_stage::processed)
    return EXIT_FAILURE;
  if (!node->set_user_data(sai::user_data_value::make<std::string>("consumer")))
    return EXIT_FAILURE;
  const auto data = node->user_data();
  if (!data)
    return EXIT_FAILURE;
  const auto stored = data->as<std::string>();
  if (!stored || !*stored || **stored != "consumer")
    return EXIT_FAILURE;
  const auto inverse = sai::matrix_inverse(sai::matrix4d{});
  if (!inverse || inverse->elements != sai::matrix4d{}.elements)
    return EXIT_FAILURE;
  const std::array<std::uint32_t, 1> pixel{0xff000080u};
  const auto image = sai::image_from_pixels(1, 1, 4, pixel);
  if (!image || sai::image_pixels(*image).value().front() != pixel.front())
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
