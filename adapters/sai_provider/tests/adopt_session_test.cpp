#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <x3d/nodes/Transform.hpp>
#include <x3d/nodes/X3DNodeFactory.hpp>
#include <x3d/sai_provider.hpp>
#include <x3d/sdk.hpp>

namespace sai = x3d::sai::experimental;
namespace p = sai::provider;
namespace sdk = x3d::sdk;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class T> T take(sai::result<T> value) {
  if (!value)
    throw std::runtime_error(value.error().operation + ": " +
                             value.error().message);
  return std::move(*value);
}
void take(sai::result<void> value) {
  if (!value)
    throw std::runtime_error(value.error().operation + ": " +
                             value.error().message);
}
p::field field(p::service &service, const p::context &context, const char *name,
               const char *slot) {
  return take(service.get_field(take(service.named(context, name)), slot));
}
void adopted_graph_and_extraction() {
  auto document =
      sdk::parseDocument(R"(<X3D profile='Immersive' version='4.0'><Scene>
    <Transform DEF='Moving'><Shape DEF='Shared'>
      <Appearance DEF='Look'><Material DEF='Paint' diffuseColor='0.2 0.4 0.6'/></Appearance>
      <Sphere DEF='Ball' radius='2'/>
    </Shape></Transform>
    <Transform DEF='Second' translation='3 0 0'><Shape USE='Shared'/></Transform>
    <PositionInterpolator DEF='Curve' key='0 1' keyValue='0 0 0 4 8 12'/>
    <ROUTE fromNode='Curve' fromField='value_changed' toNode='Moving' toField='set_translation'/>
  </Scene></X3D>)");
  // Detached root-context names belong to identity discovery too.
  auto detached = x3d::nodes::createX3DNode("Material");
  document.scene.define("Detached", detached);
  detached.reset();
  document.scene.rootNodes.push_back(document.scene.resolve("Curve"));
  auto session = sdk::RuntimeSession::create(std::move(document));
  auto *original = session.get();
  // A host may already have consumed its baseline on the loader thread.
  const auto initial = session->fullSnapshot();
  require(initial.added.size() == 2, "fixture has two shared Shape placements");
  for (auto id : initial.added) {
    const auto &item = session->extractor().item(id);
    require(item.material.phong.diffuse == x3d::core::SFColor{.2f, .4f, .6f},
            "authored native material survives loading");
  }
  const auto initial_mesh =
      session->extractor().item(initial.added.front()).mesh;
  auto adopted = take(sai::native::adopt_session(std::move(session)));
  auto &service = *adopted.service;
  require(service.supported().supports(p::service_id::scene_metadata),
          "scene_metadata stays a service capability");
  {
    const auto refused = service.metadata(adopted.scene);
    require(!refused &&
                refused.error().code == sai::error_code::unsupported_operation,
            "only the adopted scene refuses its undefined META projection");
    const auto other = take(service.create_scene());
    take(service.set_metadata(other, "title", std::string{"Other"}));
    require(take(service.metadata(other, "title")) ==
                std::optional<std::string>{"Other"},
            "other scenes in an adopted service keep scene metadata");
    take(service.retire(other));
  }
  auto context = take(service.root_context(adopted.scene));
  auto *host = take(adopted.host.session(service));
  require(host == original, "adoption keeps the exact prepared session");
  require(host->extractor().item(initial.added.front()).mesh == initial_mesh,
          "adoption does not rebuild the extractor mesh baseline");
  require(host->delta().added.empty(),
          "adoption does not reset the host extraction baseline");
  require(take(service.roots(context)) ==
              std::vector<p::node>{take(service.named(context, "Moving")),
                                   take(service.named(context, "Second")),
                                   take(service.named(context, "Curve")),
                                   take(service.named(context, "Curve"))},
          "authored root order and repeated occurrences retained");
  auto shared = take(service.named(context, "Shared"));
  for (const char *name : {"Moving", "Second"}) {
    auto children = std::get<p::node_values>(
        take(service.read(field(service, context, name, "children"))));
    require(children.size() == 1 && children.front() == p::node_value{shared},
            "DEF/USE traversal returns one Shape identity");
  }
  require(take(service.type_name(take(service.named(context, "Ball")))) ==
              "Sphere",
          "adopted type outside creation whitelist is addressable");
  require(take(service.named_nodes(context)).size() == 8 &&
              take(service.names(shared)) == std::vector<std::string>{"Shared"},
          "root DEF discovery includes detached nodes and reverse names");
  auto geometry = field(service, context, "Shared", "geometry");
  require(std::get<p::node_value>(take(service.read(geometry))) ==
              p::node_value{take(service.named(context, "Ball"))},
          "SFNode traversal uses the same identity registry");
  const auto metadata =
      take(service.read(field(service, context, "Shared", "metadata")));
  require(!std::get<p::node_value>(metadata), "NULL SFNode slots remain NULL");
  auto client = take(service.connect(adopted.active));
  auto translation = field(service, context, "Moving", "translation");
  auto fraction = field(service, context, "Curve", "set_fraction");
  require(!service.read(fraction), "inputOnly remains unreadable");
  require(!service.enqueue(client,
                           {{field(service, context, "Curve", "value_changed"),
                             sai::vec3f{1, 2, 3}}}),
          "outputOnly remains unwritable");
  std::vector<p::notification> observed;
  auto subscription = take(
      service.observe(adopted.active, translation,
                      [&](const auto &notice) { observed.push_back(notice); }));
  auto receipt = take(service.enqueue(client, {{fraction, .5f}}));
  host = nullptr;
  auto report = take(service.pump(adopted.active, {0}));
  host = take(adopted.host.session(service));
  require(report.state == p::activation_state::active && !report.error &&
              take(receipt.status()).state == p::request_state::processed,
          "native adopted turn succeeds");
  require(host->context().tickGeneration() == 1,
          "one pump performs exactly one tick");
  require(observed.empty(), "native execution defers application observation");
  const sai::vec3f expected{2, 4, 6};
  require(std::get<sai::vec3f>(take(service.read(translation))) == expected,
          "retained native ROUTE updates SAI readback");
  auto delta = host->delta();
  require(delta.updatedTransform.size() == 1 && delta.added.empty() &&
              delta.removed.empty(),
          "provider leaves transform extraction for the host");
  const auto world =
      host->extractor().item(delta.updatedTransform.front()).worldTransform.m;
  require(world[12] == 2 && world[13] == 4 && world[14] == 6,
          "same-pump extraction agrees with SAI");
  require(take(service.dispatch_notifications()) == 1 && observed.size() == 1 &&
              std::get<sai::vec3f>(observed.front().value) == expected,
          "deferred routed observation agrees with extraction");
  auto diffuse = field(service, context, "Paint", "diffuseColor");
  std::vector<p::notification> colors;
  auto material_observation =
      take(service.observe(adopted.active, diffuse, [&](const auto &notice) {
        colors.push_back(notice);
      }));
  take(service.enqueue(client, {{diffuse, sai::color3f{.8f, .1f, .3f}}}));
  host = nullptr;
  take(service.pump(adopted.active, {1}));
  host = take(adopted.host.session(service));
  require(colors.empty(), "Material callbacks are deferred");
  delta = host->delta();
  require(delta.updatedMaterial.size() == 2,
          "shared Material invalidates both placements");
  for (auto id : delta.updatedMaterial)
    require(host->extractor().item(id).material.phong.diffuse ==
                x3d::core::SFColor{.8f, .1f, .3f},
            "generic reflected Material write reaches native extraction");
  require(std::get<sai::color3f>(take(service.read(diffuse))) ==
              sai::color3f{.8f, .1f, .3f},
          "generic color readback agrees with material extraction");
  require(take(service.dispatch_notifications()) == 1 && colors.size() == 1 &&
              std::get<sai::color3f>(colors.front().value) ==
                  sai::color3f{.8f, .1f, .3f},
          "Material observation agrees with same-pump readback and extraction");
  require(!service.enqueue(client,
                           {{field(service, context, "Ball", "radius"), 3.f}}),
          "initializeOnly is fixed at adoption");
  require(!service.enqueue(client, {{geometry, p::node_value{shared}}}),
          "adopted node-valued writes fail closed");
  require(!service.enqueue(
              client, {{field(service, context, "Paint", "shininess"), 2.f}}),
          "lenient reflected numeric setter cannot bypass range checks");
  auto alpha = field(service, context, "Look", "alphaMode");
  require(!service.enqueue(client, {{alpha, std::string{"unknown-token"}}}),
          "enum setter cannot silently discard unknown tokens");
  take(service.enqueue(client, {{alpha, std::string{"BLEND"}}}));
  host = nullptr;
  take(service.pump(adopted.active, {2}));
  require(std::get<std::string>(take(service.read(alpha))) == "BLEND",
          "valid enum token reaches native storage");
  bool denied = false;
  std::thread other([&] { denied = !adopted.host.session(service); });
  other.join();
  require(denied, "host access validates owner thread");
  auto foreign = take(sai::native::make_service());
  require(!adopted.host.session(*foreign),
          "host access validates exact service");
  take(service.retire(adopted.active));
  require(!service.read(translation) && !adopted.host.session(service),
          "retirement revokes portable and native access");
}
void same_pump_time_sensor_ingress() {
  auto session = sdk::RuntimeSession::create(sdk::parseDocument(
      R"(<X3D profile='Immersive' version='4.0'><Scene>
      <TimeSensor DEF='Clock' loop='true' cycleInterval='1'/>
      </Scene></X3D>)"));
  auto adopted = take(sai::native::adopt_session(std::move(session)));
  auto &service = *adopted.service;
  auto context = take(service.root_context(adopted.scene));
  auto client = take(service.connect(adopted.active));
  auto output = field(service, context, "Clock", "fraction_changed");
  std::vector<p::notification> notices;
  auto observation = take(service.observe(
      adopted.active, output, [&](const auto &n) { notices.push_back(n); }));
  take(service.enqueue(client,
                       {{field(service, context, "Clock", "cycleInterval"),
                         sai::time_value{2}}}));
  auto report = take(service.pump(adopted.active, {.5}));
  require(report.state == p::activation_state::active && !report.error,
          "standard runtime and retained ingress coexist");
  require(
      std::get<float>(take(service.read(output))) == .25f,
      "TimeSensor sees new cycleInterval before its first output this pump");
  require(notices.empty(), "TimeSensor observation is deferred");
  require(
      take(service.dispatch_notifications()) > 0 &&
          std::all_of(notices.begin(), notices.end(),
                      [](const auto &notice) {
                        return std::get<float>(notice.value) == .25f;
                      }),
      "every native fraction occurrence uses the new interval in this pump");
  require(take(adopted.host.session(service))->context().tickGeneration() == 1,
          "same-pump ingress does not need an extra tick");
}
void adoption_preconditions_and_opaque_content() {
  require(!sai::native::adopt_session(nullptr), "null session refused");
  auto running = sdk::RuntimeSession::create(sdk::parseDocument(
      "<X3D profile='Immersive' version='4.0'><Scene/></X3D>"));
  running->tick(0);
  require(!sai::native::adopt_session(std::move(running)),
          "already-ticked session refused");
  auto session = sdk::RuntimeSession::create(
      sdk::parseDocument(R"(<X3D profile='Immersive' version='4.0'><Scene>
    <ProtoDeclare name='Body'><ProtoBody><Transform><Shape><Sphere/></Shape></Transform></ProtoBody></ProtoDeclare>
    <ProtoInstance DEF='Instance' name='Body'/><Inline DEF='Child' load='false'/>
    <Script DEF='Code'/><IMPORT inlineDEF='Child' importedDEF='Hidden' AS='Alias'/>
    </Scene></X3D>)"));
  auto adopted = take(sai::native::adopt_session(std::move(session)));
  auto &service = *adopted.service;
  auto context = take(service.root_context(adopted.scene));
  for (const auto &[name, type] :
       std::vector<std::pair<const char *, const char *>>{
           {"Instance", "Body"}, {"Child", "Inline"}, {"Code", "Script"}}) {
    require(take(service.type_name(take(service.named(context, name)))) ==
                type,
            "opaque nodes report their real type or PROTO name");
    const auto refused = service.fields(take(service.named(context, name)));
    require(!refused &&
                refused.error().code ==
                    sai::error_code::unsupported_operation &&
                refused.error().message.find("opaque") != std::string::npos,
            "unsupported PROTO/Inline/Script interface is explicitly opaque");
  }
  const auto imported = service.named(context, "Alias");
  require(!imported &&
              imported.error().code == sai::error_code::unsupported_operation &&
              imported.error().message.find("IMPORT") != std::string::npos,
          "IMPORT alias lookup names the unsupported boundary");
  const auto names = take(service.named_nodes(context));
  require(
      std::none_of(names.begin(), names.end(),
                   [](const auto &binding) { return binding.name == "Alias"; }),
      "IMPORT aliases do not enter root DEF enumeration");
  require(take(adopted.host.session(service))->delta().added.size() == 1,
          "provider leaves the first extraction untouched and opaque PROTO "
          "renders");
}
void live_routes_on_adopted_session() {
  auto adopted = take(sai::native::adopt_session(sdk::RuntimeSession::create(
      sdk::parseDocument(R"(<X3D profile='Immersive' version='4.0'><Scene>
        <Transform DEF='Target'><Shape><Appearance DEF='Look'/><Sphere/></Shape></Transform>
        <Anchor DEF='Link' description='text'/>
        <PositionInterpolator DEF='Curve' key='0 1' keyValue='0 0 0 4 8 12'/>
        <Inline DEF='Child' load='false'/>
      </Scene></X3D>)"))));
  auto &service = *adopted.service;
  auto context = take(service.root_context(adopted.scene));
  const auto client = take(service.connect(adopted.active));
  const auto fraction = field(service, context, "Curve", "set_fraction");
  const auto output = field(service, context, "Curve", "value_changed");
  const auto translation = field(service, context, "Target", "translation");
  std::vector<p::notification> notices;
  auto subscription = take(
      service.observe(adopted.active, translation,
                      [&](const auto &notice) { notices.push_back(notice); }));
  auto earlier = take(service.enqueue(client, {{fraction, .25f}}));
  auto route = take(service.enqueue_route(client, output, translation));
  auto later = take(service.enqueue(client, {{fraction, .5f}}));
  require(take(adopted.host.session(service))->scene().routes.empty(),
          "route admission does not mutate retained document");
  take(service.pump(adopted.active, {0}));
  require(take(earlier.status()).state == p::request_state::processed &&
              std::get<sai::vec3f>(take(service.read(translation))) ==
                  sai::vec3f{},
          "an earlier queued event cannot use the future route");
  auto report = take(service.pump(adopted.active, {1}));
  require(!report.error &&
              take(route.status()).state == p::request_state::processed,
          "route installation is an ordinary processed FIFO ticket");
  require(std::get<sai::vec3f>(take(service.read(translation))) == sai::vec3f{},
          "route installation does not replay the previous output");
  {
    const auto *host = take(adopted.host.session(service));
    const auto &routes = host->scene().routes;
    require(
        routes.size() == 1 && routes.front().fromField == "value_changed" &&
            routes.front().toField == "translation" &&
            routes.front().binding ==
                x3d::runtime::Route::Binding::DirectNodes &&
            routes.front().from.lock() == host->scene().resolve("Curve") &&
            routes.front().to.lock() == host->scene().resolve("Target") &&
            host->context().graph().routeCount() == 1,
        "retained document and executable route agree on identity and fields");
  }
  report = take(service.pump(adopted.active, {2}));
  require(!report.error &&
              take(later.status()).state == p::request_state::processed &&
              std::get<sai::vec3f>(take(service.read(translation))) ==
                  sai::vec3f{2, 4, 6},
          "the next queued turn executes the installed native route");
  require(notices.empty(), "live-route observations remain deferred");
  require(take(service.dispatch_notifications()) == 1 &&
              std::get<sai::vec3f>(notices.front().value) ==
                  sai::vec3f{2, 4, 6},
          "deferred observation reports the live routed value");
  require(service.enqueue_route(client, output, fraction).error().code ==
              sai::error_code::invalid_route,
          "live ROUTE refuses wrong exact kind");
  require(service.enqueue_route(client, fraction, translation).error().code ==
              sai::error_code::invalid_route,
          "inputOnly is not a ROUTE source");
  require(service.enqueue_route(client, output, output).error().code ==
              sai::error_code::invalid_route,
          "outputOnly is not a ROUTE destination");
  const auto metadata = field(service, context, "Target", "metadata");
  const auto topology = service.enqueue_route(client, metadata, metadata);
  require(!topology &&
              topology.error().code == sai::error_code::unsupported_operation,
          "live ROUTEs cannot bypass adopted node-valued write refusal, and "
          "the refusal happens at admission");
  const auto alpha = field(service, context, "Look", "alphaMode");
  const auto description = field(service, context, "Link", "description");
  const auto mismatch = service.enqueue_route(client, alpha, description);
  require(!mismatch && mismatch.error().code == sai::error_code::invalid_route,
          "SFEnum -> SFString shares sf_string but fails native admission");
  const auto other = take(service.create_scene());
  const auto other_context = take(service.root_context(other));
  const auto node = take(service.create_node(other_context, "Transform"));
  require(service.enqueue_route(client, output,
                                take(service.get_field(node, "translation")))
                  .error()
                  .code == sai::error_code::invalid_context,
          "live ROUTE refuses another scene's lexical context");
  require(!service.get_field(take(service.named(context, "Child")), "load"),
          "opaque nested contexts cannot supply a live ROUTE endpoint");
  auto duplicate = take(service.enqueue_route(client, output, translation));
  report = take(service.pump(adopted.active, {3}));
  require(!report.error && report.state == p::activation_state::active &&
              take(duplicate.status()).state == p::request_state::processed,
          "a redundant live ROUTE is a successful no-op (ISO 19775-1 4.4.8.2)");
  {
    const auto *host = take(adopted.host.session(service));
    require(host->scene().routes.size() == 1 &&
                host->context().graph().routeCount() == 1,
            "a redundant ROUTE adds no document or executable edge");
  }
  take(service.begin_update(client));
  const auto scale = field(service, context, "Target", "scale");
  auto held_input = take(service.enqueue(client, {{fraction, .75f}}));
  auto held_route = take(service.enqueue_route(client, output, scale));
  auto held_copy = take(service.enqueue_route(client, output, scale));
  auto held_installed = take(service.enqueue_route(client, output, translation));
  take(service.end_update(client));
  report = take(service.pump(adopted.active, {4}));
  require(!report.error &&
              take(held_input.status()).submission ==
                  take(held_route.status()).submission &&
              std::get<sai::vec3f>(take(service.read(scale))) ==
                  sai::vec3f{3, 6, 9},
          "a released gate installs its routes before evaluating its inputs");
  require(take(held_copy.status()).state == p::request_state::processed &&
              take(held_installed.status()).state ==
                  p::request_state::processed &&
              report.state == p::activation_state::active,
          "duplicates in one gate, queued or installed, do not fault it");
  require(take(adopted.host.session(service))->scene().routes.size() == 2,
          "each successfully installed route is retained in the document");
}
void native_node_writes_after_adoption() {
  auto adopted = take(sai::native::adopt_session(sdk::RuntimeSession::create(
      sdk::parseDocument(R"(<X3D profile='Immersive' version='4.0'><Scene>
        <Transform DEF='Watched'/><Transform DEF='Quiet'/>
      </Scene></X3D>)"))));
  auto &service = *adopted.service;
  auto context = take(service.root_context(adopted.scene));
  const auto children = field(service, context, "Watched", "children");
  std::vector<p::notification> notices;
  auto subscription = take(
      service.observe(adopted.active, children,
                      [&](const auto &notice) { notices.push_back(notice); }));
  // Native code (here the host; a Script ROUTE behaves alike) adds nodes the
  // provider never saw at adoption.
  auto added = x3d::nodes::createX3DNode("Transform");
  auto nested = x3d::nodes::createX3DNode("Shape");
  std::dynamic_pointer_cast<x3d::nodes::Transform>(added)->setChildren(
      x3d::core::MFNode{nested});
  {
    auto *host = take(adopted.host.session(service));
    host->context().postEvent(host->scene().resolve("Watched").get(),
                              "addChildren", x3d::core::MFNode{added});
    host->context().postEvent(host->scene().resolve("Quiet").get(),
                              "addChildren",
                              x3d::core::MFNode{x3d::nodes::createX3DNode(
                                  "Group")});
  }
  auto report = take(service.pump(adopted.active, {0}));
  require(!report.error && report.state == p::activation_state::active,
          "an observed native SFNode/MFNode write does not fault the tick");
  require(take(service.dispatch_notifications()) == 1 && notices.size() == 1,
          "the observed children write is published once");
  const auto published = std::get<p::node_values>(notices.front().value);
  require(published.size() == 1 && published.front() &&
              take(service.type_name(*published.front())) == "Transform",
          "the newly reachable node has an SAI identity");
  const auto grandchildren = std::get<p::node_values>(
      take(service.read(take(service.get_field(*published.front(), "children")))));
  require(grandchildren.size() == 1 &&
              take(service.type_name(*grandchildren.front())) == "Shape",
          "its subgraph is reachable through ordinary reads");
  const auto quiet = std::get<p::node_values>(
      take(service.read(field(service, context, "Quiet", "children"))));
  require(quiet.size() == 1 && take(service.type_name(*quiet.front())) == "Group",
          "unobserved native writes register on first read");
}
void inline_diagnostics_drain_through_pump() {
  sdk::SessionOptions options;
  options.inlineResolver = [](const std::vector<std::string> &,
                              const std::string &) {
    return std::shared_ptr<x3d::runtime::Scene>{};
  };
  auto adopted = take(sai::native::adopt_session(sdk::RuntimeSession::create(
      sdk::parseDocument(R"(<X3D profile='Immersive' version='4.0'><Scene>
        <Inline DEF='Child' load='false' url='"missing.x3d"'/>
      </Scene></X3D>)"),
      options)));
  auto &service = *adopted.service;
  {
    auto *host = take(adopted.host.session(service));
    require(host->context().writeField(host->scene().resolve("Child").get(),
                                       "load", true) ==
                x3d::runtime::FieldWriteResult::Ok,
            "host requests a deferred Inline load");
  }
  take(service.pump(adopted.active, {0}));
  const auto *host = take(adopted.host.session(service));
  require(host->scene().pendingInlineWarnings.empty() &&
              std::any_of(host->document().inlineWarnings.begin(),
                          host->document().inlineWarnings.end(),
                          [](const auto &warning) {
                            return warning.detail.find("missing.x3d") !=
                                   std::string::npos;
                          }),
          "pump ticks through RuntimeSession and drains Inline diagnostics");
}
} // namespace
int main() {
  try {
    live_routes_on_adopted_session();
    adopted_graph_and_extraction();
    same_pump_time_sensor_ingress();
    adoption_preconditions_and_opaque_content();
    native_node_writes_after_adoption();
    inline_diagnostics_drain_through_pump();
    std::cout << "native session adoption cases passed\n";
  } catch (const std::exception &error) {
    std::cerr << "native session adoption failed: " << error.what() << '\n';
    return 1;
  }
}
