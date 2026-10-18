#include "doctest/doctest.h"
#include "x3d/sai/experimental/provider.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include <map>
#include <bit>
#include <thread>
namespace s = x3d::sai::experimental;
namespace p = s::provider;
namespace {
struct probe : p::backend {
  struct scene_data { std::vector<std::uint64_t> roots; std::map<std::string, std::uint64_t> names; };
  std::map<std::uint64_t, scene_data> scenes;
  std::map<std::pair<std::uint64_t,std::uint64_t>, std::map<std::string,backend_value>> values;
  std::vector<admitted_write> delivered;
  std::size_t turns = 0;
  std::shared_ptr<int> retired;
  bool partial = false;
  explicit probe(std::shared_ptr<int> counter = {}) : retired(std::move(counter)) {}
  p::capabilities supported() const override {
    return {"contract-probe", 1, {{p::service_id::scene_authoring,1}, {p::service_id::host_turns,1}, {p::service_id::notifications,1}, {p::service_id::node_values,1}},
      {{"Transform", "Grouping", 1}}, {s::value_kind::sf_float,s::value_kind::sf_vec3f,s::value_kind::sf_node,s::value_kind::mf_float,s::value_kind::mf_node}};
  }
  s::result<std::uint64_t> do_create_scene() override {
    std::uint64_t id = 1; while (scenes.contains(id)) ++id; scenes.emplace(id,scene_data{}); return id;
  }
  s::result<std::uint64_t> do_create_node(std::uint64_t scene, std::string_view type) override {
    if (type != "Transform") return error(s::error_code::unknown_type,"probe.create","unknown type");
    std::uint64_t id=1; while(values.contains({scene,id})) ++id;
    values[{scene,id}]={{"x",0.f},{"limit",3.f},{"translation",s::vec3f{}},{"metadata",s::node_id{}},{"children",s::node_list{}},{"values",s::float_list{0,1}},{"inputs",s::float_list{}}};
    return id;
  }
  s::result<void> do_append_root(const address &a) override { scenes.at(a.scene).roots.push_back(a.node); return {}; }
  s::result<void> do_define_name(const address &a,std::string_view n) override { scenes.at(a.scene).names.emplace(n,a.node); return {}; }
  s::result<std::vector<std::uint64_t>> do_roots(std::uint64_t scene) const override { return scenes.at(scene).roots; }
  s::result<std::uint64_t> do_named(std::uint64_t scene,std::string_view name) const override {
    auto i=scenes.at(scene).names.find(std::string(name));
    if(i==scenes.at(scene).names.end()) return error(s::error_code::unknown_node,"probe.named","unknown node"); return i->second;
  }
  s::result<std::string> do_type_name(const address &a) const override {
    if(!values.contains({a.scene,a.node})) return error(s::error_code::unknown_node,"probe.type","unknown node"); return std::string("Transform");
  }
  s::result<std::vector<p::field_info>> do_fields(const address &) const override {
    using enum s::value_kind; using enum s::access_type;
    return std::vector<p::field_info>{{"x",sf_float,input_output},{"limit",sf_float,input_output},{"input",sf_float,input_only},
      {"translation",sf_vec3f,input_output},{"metadata",sf_node,input_output},{"children",mf_node,input_output},{"values",mf_float,input_output},{"inputs",mf_float,input_only}};
  }
  s::result<backend_value> do_read(const address &a) const override { return values.at({a.scene,a.node}).at(a.field); }
  s::result<void> do_author(const address &a,const backend_value &v) override { values.at({a.scene,a.node})[a.field]=v; return {}; }
  s::result<void> do_add_route(const address &,const address &) override { return {}; }
  s::result<void> do_validate_configuration(std::uint64_t scene,const std::vector<admitted_write> &writes) const override {
    for(const auto &[key,fields]:values) if(key.first==scene) {
      float x=std::get<float>(fields.at("x")),limit=std::get<float>(fields.at("limit"));
      for(const auto &w:writes) if(w.target.node==key.second) {
        if(w.target.field=="x") x=std::get<float>(w.value); if(w.target.field=="limit") limit=std::get<float>(w.value);
      }
      if(x>limit) return error(s::error_code::invalid_value,"probe.validate","x exceeds limit");
    }
    return {};
  }
  s::result<void> do_activate(std::uint64_t) override { return {}; }
  backend_result do_turn(std::uint64_t,s::event_time,const std::vector<admitted_write> &writes) override {
    ++turns; delivered=writes;
    for(const auto &w:writes) { values.at({w.target.scene,w.target.node})[w.target.field]=w.value; record_event(w.target,w.value); }
    if(partial) return tl::unexpected{backend_failure{error(s::error_code::callback_failed,"probe.turn","partial").value(),p::failure_effect::partial_or_unknown}};
    return {};
  }
  void do_retire(std::uint64_t scene) noexcept override {
    scenes.erase(scene); std::erase_if(values,[&](const auto &entry){return entry.first.first==scene;}); if(retired) ++*retired;
  }
};
struct world {
  probe *implementation;
  std::unique_ptr<p::service> service;
  p::scene scene; p::context context; p::node node;
  explicit world(p::resource_limits limits = {}) {
    auto backend=std::make_unique<probe>(); implementation=backend.get(); service=std::make_unique<p::service>(std::move(backend),limits);
    scene=service->create_scene().value(); context=service->root_context(scene).value(); node=service->create_node(context,"Transform").value();
    REQUIRE(service->append_root(context,node));
  }
  p::field field(const char *name) { return service->get_field(node,name).value(); }
};
}
TEST_CASE("one provider identity survives setup and live node-valued projections") {
  world w,foreign;
  const auto other=w.service->create_node(w.context,"Transform").value();
  REQUIRE(w.service->define_name(w.node,"Owner"));
  REQUIRE(w.service->author(w.node,s::bindings::Transform::metadata,p::node_value{other}));
  REQUIRE(w.service->author(w.node,s::bindings::Transform::children,p::node_values{std::nullopt,other}));
  CHECK(w.service->read(w.node,s::bindings::Transform::metadata).value()==p::node_value{other});
  CHECK(w.service->read(w.node,s::bindings::Transform::children).value()==p::node_values{std::nullopt,other});
  CHECK_FALSE(w.service->author(w.node,s::bindings::Transform::metadata,p::node_value{foreign.node}));
  CHECK_FALSE(w.service->author(w.node,s::bindings::Transform::metadata,p::node_value{p::node{}}));
  auto disposed=other; disposed.dispose(); CHECK_FALSE(w.service->author(w.node,s::bindings::Transform::metadata,p::node_value{disposed}));
  auto active=w.service->activate(w.scene).value(); auto client=w.service->connect(active).value();
  CHECK(w.service->named(w.context,"Owner").value()==w.node);
  auto ticket=w.service->enqueue(client,w.node,s::bindings::Transform::children,p::node_values{other,std::nullopt}).value();
  REQUIRE(w.service->pump(active,{0}));
  CHECK(ticket.status()->state==p::request_state::processed);
  CHECK(w.service->read(w.node,s::bindings::Transform::children).value()==p::node_values{other,std::nullopt});
  CHECK_FALSE(w.service->author(w.node,s::bindings::Transform::children,p::node_values{}));
}
TEST_CASE("provider gate releases one candidate and all per-call receipts share outcome") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value(); auto second=w.service->connect(a).value();
  REQUIRE(w.service->end_update(c)); REQUIRE(w.service->begin_update(c)); REQUIRE(w.service->begin_update(c));
  auto x=w.service->enqueue(c,{{w.field("x"),4.f}}).value();
  auto limit=w.service->enqueue(c,{{w.field("limit"),5.f}}).value();
  auto input1=w.service->enqueue(c,{{w.field("input"),1.f}}).value();
  auto input2=w.service->enqueue(c,{{w.field("input"),1.f}}).value();
  auto free=w.service->enqueue(second,{{w.field("x"),2.f}}).value();
  REQUIRE(w.service->end_update(c));
  CHECK(x.status()->submission==limit.status()->submission);
  CHECK(input1.status()->submission==input2.status()->submission);
  auto t1=w.service->pump(a,{1}).value(); CHECK(t1.request==free.id());
  auto t2=w.service->pump(a,{1}).value(); CHECK(t2.request==x.id());
  CHECK(x.status()->turn==limit.status()->turn); CHECK(input1.status()->turn==x.status()->turn); CHECK(input2.status()->turn==x.status()->turn);
  REQUIRE(w.implementation->delivered.size()==4);
  CHECK(w.implementation->delivered[2].target.field=="input"); CHECK(w.implementation->delivered[3].target.field=="input");
  CHECK(std::get<float>(w.service->read(w.field("x")).value())==4.f);
}
TEST_CASE("provider MF aliases aggregate indexed edits while preserving whole input occurrences") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  REQUIRE(w.service->begin_update(c));
  REQUIRE(w.service->enqueue(c,{{w.field("values"),s::float_list{1,2,3}}}));
  REQUIRE(w.service->enqueue(c,{{w.field("set_values"),9.f,p::write_mode::set_element,1}}));
  REQUIRE(w.service->enqueue(c,{{w.field("values_changed"),8.f,p::write_mode::set_element,1}}));
  CHECK_FALSE(w.service->enqueue(c,{{w.field("inputs"),1.f,p::write_mode::set_element,0}}));
  REQUIRE(w.service->enqueue(c,{{w.field("inputs"),s::float_list{4,5}}}));
  REQUIRE(w.service->enqueue(c,{{w.field("inputs"),6.f,p::write_mode::set_element,1}}));
  REQUIRE(w.service->enqueue(c,{{w.field("inputs"),s::float_list{7,8}}}));
  REQUIRE(w.service->enqueue(c,{{w.field("inputs"),9.f,p::write_mode::set_element,0}}));
  REQUIRE(w.service->end_update(c)); REQUIRE(w.service->pump(a,{0}));
  REQUIRE(w.implementation->delivered.size()==3);
  CHECK(std::get<s::float_list>(w.implementation->delivered[0].value)==s::float_list{1,8,3});
  CHECK(std::get<s::float_list>(w.implementation->delivered[1].value)==s::float_list{4,6});
  CHECK(std::get<s::float_list>(w.implementation->delivered[2].value)==s::float_list{9,8});
}
TEST_CASE("provider harmless stale submission leaves later work and completed notices intact") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  std::size_t calls=0; auto observation=w.service->observe(a,w.field("x"),[&](const auto &){++calls;}).value();
  auto completed=w.service->enqueue(c,{{w.field("x"),1.f}}).value(); REQUIRE(w.service->pump(a,{1}));
  auto lower=w.service->enqueue(c,{{w.field("limit"),1.f}}).value();
  auto stale=w.service->enqueue(c,{{w.field("x"),2.f}}).value();
  auto later=w.service->enqueue(c,{{w.field("x"),0.f}}).value();
  REQUIRE(w.service->pump(a,{1})); auto failed=w.service->pump(a,{1}).value();
  REQUIRE(failed.error); CHECK(failed.state==p::activation_state::active); CHECK(failed.effects==p::failure_effect::none);
  CHECK(stale.status()->state==p::request_state::failed); CHECK(later.status()->state==p::request_state::submitted);
  CHECK(w.implementation->turns==2); CHECK(calls==0); CHECK(w.service->dispatch_notifications().value()==1); CHECK(calls==1);
  REQUIRE(w.service->pump(a,{1})); CHECK(later.status()->state==p::request_state::processed);
}
TEST_CASE("provider retired identities cannot revive when backend numeric IDs are reused") {
  world w; auto old=w.node; auto scene=w.scene;
  REQUIRE(w.service->retire(scene)); CHECK(w.service->state(scene).value()==p::activation_state::retired);
  auto replacement=w.service->create_scene().value(); auto context=w.service->root_context(replacement).value(); auto node=w.service->create_node(context,"Transform").value();
  CHECK_FALSE(old==node); CHECK_FALSE(scene==replacement); CHECK_FALSE(w.service->type_name(old)); CHECK(w.service->state(scene).value()==p::activation_state::retired);
}
TEST_CASE("provider composition retires backend before owning receipts outlive it") {
  auto retired=std::make_shared<int>(0); p::ticket held; p::scene scene;
  {
    auto host=std::make_unique<p::service>(std::make_unique<probe>(retired));
    scene=host->create_scene().value(); auto context=host->root_context(scene).value(); auto node=host->create_node(context,"Transform").value(); REQUIRE(host->append_root(context,node)); auto a=host->activate(scene).value(); auto c=host->connect(a).value();
    REQUIRE(host->begin_update(c)); held=host->enqueue(c,{{host->get_field(node,"x").value(),1.f}}).value();
  }
  CHECK(*retired==1); CHECK(scene.expired()); CHECK(held.status()->state==p::request_state::cancelled);
}
TEST_CASE("provider release rejection fails one complete group without poisoning activation") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  REQUIRE(w.service->begin_update(c)); auto first=w.service->enqueue(c,{{w.field("x"),8.f}}).value(); auto last=w.service->enqueue(c,{{w.field("limit"),7.f}}).value();
  CHECK_FALSE(w.service->end_update(c)); CHECK(first.status()->state==p::request_state::failed); CHECK(last.status()->state==p::request_state::failed);
  CHECK(first.status()->submission==last.status()->submission); CHECK(first.status()->turn==0); CHECK(w.service->state(w.scene).value()==p::activation_state::active);
  REQUIRE(w.service->enqueue(c,{{w.field("x"),1.f}})); REQUIRE(w.service->pump(a,{0}));
}
TEST_CASE("provider checked close rejects a foreign thread without cancelling work") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value(); auto ticket=w.service->enqueue(c,{{w.field("x"),1.f}}).value();
  s::result<void> denied;
  std::thread wrong([&]{denied=w.service->close();}); wrong.join();
  REQUIRE_FALSE(denied); CHECK(denied.error().code==s::error_code::wrong_thread);
  CHECK(ticket.status()->state==p::request_state::submitted); CHECK_FALSE(w.scene.expired());
  REQUIRE(w.service->pump(a,{0})); REQUIRE(w.service->close()); REQUIRE(w.service->close());
  CHECK(ticket.status()->state==p::request_state::processed); CHECK(w.scene.expired());
}
TEST_CASE("provider released cancellation is a submission-wide decision") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  REQUIRE(w.service->begin_update(c)); auto first=w.service->enqueue(c,{{w.field("x"),1.f}}).value(); auto second=w.service->enqueue(c,{{w.field("limit"),5.f}}).value();
  REQUIRE(w.service->end_update(c)); REQUIRE(w.service->cancel(c,second));
  CHECK(first.status()->state==p::request_state::cancelled); CHECK(second.status()->state==p::request_state::cancelled);
  CHECK_FALSE(w.service->pump(a,{0})->request);
}
TEST_CASE("provider partial failure faults activation but preserves completed notices") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  std::vector<float> observed; auto observer=w.service->observe(a,w.field("x"),[&](const auto &n){observed.push_back(std::get<float>(n.value));}).value();
  REQUIRE(w.service->enqueue(c,{{w.field("x"),1.f}})); REQUIRE(w.service->pump(a,{0}));
  auto failing=w.service->enqueue(c,{{w.field("x"),2.f}}).value(); auto later=w.service->enqueue(c,{{w.field("x"),3.f}}).value();
  w.implementation->partial=true; auto turn=w.service->pump(a,{0}).value(); REQUIRE(turn.error);
  CHECK(turn.state==p::activation_state::faulted); CHECK(failing.status()->effects==p::failure_effect::partial_or_unknown); CHECK(later.status()->state==p::request_state::cancelled);
  CHECK(w.service->dispatch_notifications().value()==1); CHECK(observed==std::vector<float>{1.f}); CHECK_FALSE(w.service->pump(a,{0}));
}
TEST_CASE("provider metadata cleanup can reenter and destroy its owner") {
  world w;
  struct release { std::function<void()> action; explicit release(std::function<void()> value):action(std::move(value)){} ~release(){action();} };
  bool saw_new=false;
  REQUIRE(w.service->set_user_data(w.node,s::user_data_value::make<release>([&]{
    auto value=w.service->user_data(w.node).value().as<int>().value(); saw_new=value&&*value==42;
  })));
  REQUIRE((w.service->set_user_data(w.node,s::user_data_value::make<int>(42)).has_value() && saw_new));
  REQUIRE(w.service->set_user_data(w.field("x"),s::user_data_value::make<release>([&]{w.service.reset();})));
  auto *service=w.service.get(); REQUIRE(service->set_user_data(w.field("x"),{})); CHECK_FALSE(w.service);
}
TEST_CASE("provider slow notification consumer receives preflight backpressure without lost work") {
  p::resource_limits limits; limits.queued_notifications=1;
  world w(limits); auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  std::vector<float> seen; auto observer=w.service->observe(a,w.field("x"),[&](const auto &n){seen.push_back(std::get<float>(n.value));}).value();
  REQUIRE(w.service->enqueue(c,{{w.field("x"),1.f}})); REQUIRE(w.service->pump(a,{0}));
  auto pending=w.service->enqueue(c,{{w.field("x"),2.f}}).value();
  auto full=w.service->pump(a,{0}); REQUIRE_FALSE(full); CHECK(full.error().code==s::error_code::resource_limit);
  CHECK(pending.status()->state==p::request_state::submitted); CHECK(w.implementation->turns==1);
  CHECK(w.service->dispatch_notifications().value()==1); CHECK(seen==std::vector<float>{1.f});
  auto next=w.service->pump(a,{0}).value(); CHECK(next.turn==2); CHECK(pending.status()->state==p::request_state::processed);
  CHECK(w.service->dispatch_notifications().value()==1); CHECK(seen==std::vector<float>{1.f,2.f});
}
TEST_CASE("provider notification overflow during a turn preserves earlier successful history") {
  p::resource_limits limits; limits.queued_notifications=2;
  world w(limits); auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  std::vector<float> seen; auto one=w.service->observe(a,w.field("x"),[&](const auto &n){seen.push_back(std::get<float>(n.value));}).value();
  REQUIRE(w.service->enqueue(c,{{w.field("x"),1.f}})); REQUIRE(w.service->pump(a,{0}));
  auto two=w.service->observe(a,w.field("x"),[&](const auto &){FAIL("failed turn notice escaped");}).value();
  auto failing=w.service->enqueue(c,{{w.field("x"),2.f}}).value(); auto later=w.service->enqueue(c,{{w.field("x"),3.f}}).value();
  auto turn=w.service->pump(a,{0}).value(); REQUIRE(turn.error); CHECK(turn.error->code==s::error_code::resource_limit);
  CHECK(turn.effects==p::failure_effect::partial_or_unknown); CHECK(turn.state==p::activation_state::faulted);
  CHECK(failing.status()->state==p::request_state::failed); CHECK(later.status()->state==p::request_state::cancelled);
  CHECK(w.service->dispatch_notifications().value()==1); CHECK(seen==std::vector<float>{1.f});
}
TEST_CASE("provider notification payload bytes use checked logical accounting") {
  p::resource_limits limits; limits.notification_bytes=sizeof(p::notification)+std::string_view("values").size()+2*sizeof(float);
  world w(limits); auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  auto observer=w.service->observe(a,w.field("values"),[](const auto &){}).value();
  REQUIRE(w.service->enqueue(c,{{w.field("values"),s::float_list{1,2}}})); REQUIRE(w.service->pump(a,{0}));
  CHECK(w.service->dispatch_notifications().value()==1);
  auto ticket=w.service->enqueue(c,{{w.field("values"),s::float_list{1,2,3}}}).value();
  auto failed=w.service->pump(a,{0}).value(); REQUIRE(failed.error); CHECK(failed.error->code==s::error_code::resource_limit);
  CHECK(ticket.status()->effects==p::failure_effect::partial_or_unknown); CHECK(w.service->dispatch_notifications().value()==0);
}
TEST_CASE("provider cancelled registration and notice budgets are reclaimed without growing history") {
  p::resource_limits limits; limits.live_registrations=1; limits.queued_notifications=1;
  world w(limits); auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  auto first=w.service->observe(a,w.field("x"),[](const auto &){}).value();
  auto denied=w.service->observe(a,w.field("x"),[](const auto &){}); REQUIRE_FALSE(denied); CHECK(denied.error().code==s::error_code::resource_limit);
  REQUIRE(w.service->enqueue(c,{{w.field("x"),1.f}})); REQUIRE(w.service->pump(a,{0})); first.cancel();
  for(int i=0;i<2000;++i) {
    auto replacement=w.service->observe(a,w.field("x"),[](const auto &){}); REQUIRE(replacement); replacement->cancel();
  }
  auto final=w.service->observe(a,w.field("x"),[](const auto &){}).value();
  REQUIRE(w.service->enqueue(c,{{w.field("x"),2.f}})); REQUIRE(w.service->pump(a,{0})); CHECK(w.service->dispatch_notifications().value()==1);
}
TEST_CASE("provider cancelled capture cleanup can close service during replacement registration") {
  p::resource_limits limits; limits.live_registrations=1;
  world w(limits); auto a=w.service->activate(w.scene).value();
  struct capture { std::function<void()> action; explicit capture(std::function<void()> value):action(std::move(value)){} ~capture(){action();} };
  auto owned=std::make_shared<capture>([&]{REQUIRE(w.service->close());});
  auto first=w.service->observe(a,w.field("x"),[owned](const auto &){}).value(); owned.reset(); first.cancel();
  const auto field=w.field("x");
  auto replacement=w.service->observe(a,field,[](const auto &){}); REQUIRE(replacement);
  CHECK(w.scene.expired()); CHECK_FALSE(w.service->read(field));
}
TEST_CASE("provider grouped owning outcomes publish safely to concurrent receipt readers") {
  world w; auto a=w.service->activate(w.scene).value(); auto c=w.service->connect(a).value();
  REQUIRE(w.service->begin_update(c));
  auto first=w.service->enqueue(c,{{w.field("x"),1.f}}).value();
  auto second=w.service->enqueue(c,{{w.field("x"),2.f}}).value();
  std::atomic_bool done=false, valid=true;
  std::thread reader([&]{
    while(!done.load()) for(const auto &receipt:{first,second}) {
      auto status=receipt.status();
      if(!status || (status->state!=p::request_state::accepted && status->state!=p::request_state::submitted && status->state!=p::request_state::processed)) valid=false;
      if(status && status->state==p::request_state::processed && (status->turn!=1 || status->submission!=first.id())) valid=false;
    }
  });
  const auto released=w.service->end_update(c); const auto processed=w.service->pump(a,{0}); const auto closed=w.service->close();
  done=true; reader.join();
  REQUIRE(released); REQUIRE(processed); REQUIRE(closed); CHECK(valid);
  CHECK(first.status()->state==p::request_state::processed); CHECK(second.status()->state==p::request_state::processed);
  CHECK(first.status()->submission==second.status()->submission);
}

namespace {
// This model deliberately omits backend dimension validation, proving that the
// one canonical frontend rule protects every provider, including malformed reads.
struct dimension_probe final : probe {
  std::string type;
  explicit dimension_probe(std::string name) : type(std::move(name)) {}
  p::capabilities supported() const override {
    auto result = probe::supported(); result.nodes = {{type,"Test",1}}; return result;
  }
  s::result<std::uint64_t> do_create_node(std::uint64_t scene,std::string_view name) override {
    if (name != type) return error(s::error_code::unknown_type,"dimensions.create","unknown type");
    auto created = probe::do_create_node(scene,"Transform");
    if (!created) return created;
    auto &stored = values.at({scene,*created});
    stored["size"] = s::vec3f{2,2,2}; stored["bboxSize"] = s::vec3f{-1,-1,-1};
    stored["bboxCenter"] = s::vec3f{}; stored["center"] = s::vec3f{}; stored["scale"] = s::vec3f{1,1,1};
    return created;
  }
  s::result<std::string> do_type_name(const address &where) const override {
    if (auto found=probe::do_type_name(where); !found) return found;
    return type;
  }
  s::result<std::vector<p::field_info>> do_fields(const address &where) const override {
    auto fields=probe::do_fields(where).value();
    for (const auto name:{"size","bboxSize","bboxCenter"})
      fields.push_back({name,s::value_kind::sf_vec3f,s::access_type::initialize_only,true});
    fields.push_back({"center",s::value_kind::sf_vec3f,s::access_type::input_output,true});
    fields.push_back({"scale",s::value_kind::sf_vec3f,s::access_type::input_output,true});
    return fields;
  }
  backend_value &raw(std::string_view field) { return values.begin()->second.at(std::string(field)); }
};
struct dimension_world {
  dimension_probe *backend;
  p::service service;
  p::scene scene;
  p::node node;
  explicit dimension_world(const char *type)
    : backend(new dimension_probe(type)), service(std::unique_ptr<dimension_probe>{backend}) {
    scene=service.create_scene().value();
    node=service.create_node(service.root_context(scene).value(),type).value();
  }
  p::field field(const char *name) { return service.get_field(node,name).value(); }
};
}
TEST_CASE("canonical Box size rejects zero negative and nonfinite dimensions without mutation") {
  dimension_world w("Box"); const auto size=w.field("size");
  const s::vec3f before{2,2,2};
  const float nan=std::numeric_limits<float>::quiet_NaN();
  const float infinity=std::numeric_limits<float>::infinity();
  for (const auto invalid:{0.f,-0.f,-1.f,nan,infinity,-infinity})
    for (int axis=0;axis<3;++axis) {
      auto candidate=before;
      if(axis==0) candidate.x=invalid; else if(axis==1) candidate.y=invalid; else candidate.z=invalid;
      auto rejected=w.service.author(size,candidate); REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code==s::error_code::invalid_value);
      CHECK(std::get<s::vec3f>(w.backend->raw("size"))==before);
      CHECK(std::get<s::vec3f>(w.service.read(size).value())==before);
    }
  const s::vec3f valid{std::numeric_limits<float>::min(),2,3};
  REQUIRE(w.service.author(size,valid)); CHECK(std::get<s::vec3f>(w.service.read(size).value())==valid);
  CHECK_FALSE(w.service.author(size,s::vec3f{-1,2,3}));
  CHECK(std::get<s::vec3f>(w.backend->raw("size"))==valid);
}
TEST_CASE("canonical Transform and Shape bounds permit exact sentinel or componentwise nonnegative dimensions") {
  const float nan=std::numeric_limits<float>::quiet_NaN();
  const float infinity=std::numeric_limits<float>::infinity();
  for (const auto type:{"Transform","Shape"}) {
    dimension_world w(type); const auto bounds=w.field("bboxSize");
    for(const auto invalid:{s::vec3f{-1,0,0},s::vec3f{0,-1,0},s::vec3f{0,0,-1},
        s::vec3f{-1,-1,0},s::vec3f{-2,-2,-2},s::vec3f{nan,0,0},
        s::vec3f{0,infinity,0},s::vec3f{0,0,-infinity}}) {
      const auto before=w.service.read(bounds).value();
      auto rejected=w.service.author(bounds,invalid); REQUIRE_FALSE(rejected);
      CHECK(rejected.error().code==s::error_code::invalid_value);
      CHECK(w.service.read(bounds).value()==before);
      CHECK(std::get<s::vec3f>(w.backend->raw("bboxSize"))==std::get<s::vec3f>(before));
    }
    for(const auto valid:{s::vec3f{0,0,0},s::vec3f{-0.f,2,0},s::vec3f{1,2,3},s::vec3f{-1,-1,-1}}) {
      REQUIRE(w.service.author(bounds,valid)); CHECK(std::get<s::vec3f>(w.service.read(bounds).value())==valid);
    }
    REQUIRE(w.service.author(w.field("bboxCenter"),s::vec3f{-20,0,20}));
    REQUIRE(w.service.author(w.field("scale"),s::vec3f{-1,0,2}));
  }
}
TEST_CASE("canonical dimension reads reject malformed backend storage while valid setup writes repair it") {
  for(const auto type:{"Box","Transform","Shape"}) {
    dimension_world w(type);
    const auto name=std::string_view(type)=="Box"?"size":"bboxSize";
    const auto field=w.field(name); const s::vec3f repair{1,2,3};
    for(const auto invalid:{s::vec3f{-2,1,1},s::vec3f{0,std::numeric_limits<float>::quiet_NaN(),0},
        s::vec3f{0,0,std::numeric_limits<float>::infinity()}}) {
      w.backend->raw(name)=invalid;
      auto read=w.service.read(field); REQUIRE_FALSE(read); CHECK(read.error().code==s::error_code::invalid_value);
      REQUIRE(w.service.author(field,repair));
      CHECK(std::get<s::vec3f>(w.service.read(field).value())==repair);
    }
    auto active=w.service.activate(w.scene).value(); auto client=w.service.connect(active).value();
    auto invalid=w.service.enqueue(client,{{field,s::vec3f{-2,1,1}}}); REQUIRE_FALSE(invalid);
    CHECK(invalid.error().code==s::error_code::invalid_value);
    auto initialized=w.service.enqueue(client,{{field,repair}}); REQUIRE_FALSE(initialized);
    CHECK(initialized.error().code==s::error_code::access_denied); // Real fields stay initializeOnly.
    CHECK_FALSE(w.service.pump(active,{0})->request);
    CHECK(std::get<s::vec3f>(w.backend->raw(name))==repair);
  }
  dimension_world custom("Unrelated");
  REQUIRE(custom.service.author(custom.field("size"),s::vec3f{-2,0,3}));
  REQUIRE(custom.service.author(custom.field("bboxSize"),s::vec3f{-2,0,3}));
}

TEST_CASE("canonical generic Transform vectors preserve IEEE bits outside scoped dimension and curve policies") {
  dimension_world w("Transform");
  const s::vec3f authored{std::bit_cast<float>(std::uint32_t{0x7fc01234}),
      std::numeric_limits<float>::infinity(),-0.f};
  const auto same_bits=[](const s::vec3f &a,const s::vec3f &b) {
    return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
      std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
      std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
  };
  for(const auto name:{"translation","center","scale","bboxCenter"}) {
    const auto field=w.field(name);
    REQUIRE(w.service.author(field,authored));
    CHECK(same_bits(std::get<s::vec3f>(w.service.read(field).value()),authored));
    CHECK(same_bits(std::get<s::vec3f>(w.backend->raw(name)),authored));
  }
  auto active=w.service.activate(w.scene).value(); auto client=w.service.connect(active).value();
  const s::vec3f live{-std::numeric_limits<float>::infinity(),
      std::bit_cast<float>(std::uint32_t{0x7fc05678}),0.f};
  std::vector<s::vec3f> observed;
  auto subscription=w.service.observe(active,w.field("translation"),[&](const auto &notice) {
    observed.push_back(std::get<s::vec3f>(notice.value));
  }).value();
  auto receipt=w.service.enqueue(client,{{w.field("translation"),live}}).value();
  REQUIRE(w.service.pump(active,{0})); CHECK(receipt.status()->state==p::request_state::processed);
  CHECK(same_bits(std::get<s::vec3f>(w.service.read(w.field("translation")).value()),live));
  CHECK(w.service.dispatch_notifications().value()==1); REQUIRE(observed.size()==1);
  CHECK(same_bits(observed.front(),live));
}

namespace {
struct scene_info_probe : probe {
  bool implemented = true;
  std::vector<s::unit_declaration> declarations;
  p::capabilities supported() const override {
    auto out = probe::supported();
    out.services.push_back({p::service_id::scene_units});
    out.services.push_back({p::service_id::scene_metadata});
    return out;
  }
  s::result<std::vector<s::unit_declaration>> do_units(std::uint64_t id) const override {
    if (!implemented) return p::backend::do_units(id);
    return declarations;
  }
  s::result<std::vector<s::metadata_entry>> do_metadata(std::uint64_t id) const override {
    if (!implemented) return p::backend::do_metadata(id);
    return std::vector<s::metadata_entry>{{"title", "retained"}};
  }
};
}
TEST_CASE("scene information capabilities fail closed without backend implementation") {
  auto backend = std::make_unique<scene_info_probe>();
  auto *raw = backend.get();
  raw->implemented = false;
  p::service advertised{std::move(backend)};
  p::service absent{std::make_unique<probe>()};
  for (auto *service : {&advertised, &absent}) {
    const auto scene = service->create_scene().value();
    const auto context = service->root_context(scene).value();
    const auto units = service->units(context);
    REQUIRE_FALSE(units);
    CHECK(units.error().code == s::error_code::unsupported_operation);
    const auto declaration = service->declare_unit(scene, {"length", "centimetre", .01});
    REQUIRE_FALSE(declaration);
    CHECK(declaration.error().code == s::error_code::unsupported_operation);
    const auto metadata = service->metadata(scene);
    REQUIRE_FALSE(metadata);
    CHECK(metadata.error().code == s::error_code::unsupported_operation);
    const auto lookup = service->metadata(scene, "title");
    REQUIRE_FALSE(lookup);
    CHECK(lookup.error().code == s::error_code::unsupported_operation);
    const auto write = service->set_metadata(scene, "title", "new");
    REQUIRE_FALSE(write);
    CHECK(write.error().code == s::error_code::unsupported_operation);
  }
  // A query implementation must not accidentally imply declaration support.
  raw->implemented = true;
  const auto scene = advertised.create_scene().value();
  const auto declaration = advertised.declare_unit(scene, {"length", "centimetre", .01});
  REQUIRE_FALSE(declaration);
  CHECK(declaration.error().code == s::error_code::unsupported_operation);
  CHECK(raw->declarations.empty());
}
TEST_CASE("scene units reject malformed backend declarations before returning effective values") {
  auto backend = std::make_unique<scene_info_probe>();
  auto *raw = backend.get();
  p::service service{std::move(backend)};
  const auto context = service.root_context(service.create_scene().value()).value();
  const std::vector<s::unit_declaration> invalid{
      {"time", "second", 1}, {"length", "", 1}, {"mass", "two words", 1},
      {"angle", "degree", 0}, {"force", "newton", -1},
      {"length", "metre", std::numeric_limits<double>::infinity()},
      {"mass", "gram", std::numeric_limits<double>::quiet_NaN()}};
  for (const auto &unit : invalid) {
    raw->declarations = {unit};
    const auto result = service.units(context);
    REQUIRE_FALSE(result);
    CHECK(result.error().code == s::error_code::invalid_descriptor);
  }
  raw->declarations = {{"length", "metre", 1}, {"length", "centimetre", .01}};
  const auto duplicate = service.units(context);
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == s::error_code::invalid_descriptor);
  raw->declarations.clear();
  REQUIRE(service.units(context));
  CHECK(service.units(context)->size() == 4);
}
TEST_CASE("scene information remains inspectable after a fault without permitting mutation") {
  auto backend = std::make_unique<scene_info_probe>();
  auto *raw = backend.get();
  p::service service{std::move(backend)};
  const auto scene = service.create_scene().value();
  const auto context = service.root_context(scene).value();
  const auto active = service.activate(scene).value();
  raw->partial = true;
  const auto turn = service.pump(active, {0});
  REQUIRE(turn);
  REQUIRE(turn->error);
  CHECK(service.state(scene).value() == p::activation_state::faulted);
  CHECK(service.units(context)->size() == 4);
  CHECK(service.metadata(scene, "title").value() == "retained");
  const auto declaration = service.declare_unit(scene, {"length", "centimetre", .01});
  REQUIRE_FALSE(declaration);
  CHECK(declaration.error().code == s::error_code::access_denied);
  const auto write = service.set_metadata(scene, "title", "new");
  REQUIRE_FALSE(write);
  CHECK(write.error().code == s::error_code::access_denied);
  CHECK(service.metadata(scene, "title").value() == "retained");
  REQUIRE(service.retire(scene));
  const auto stale = service.metadata(scene);
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == s::error_code::stale_handle);
}

TEST_CASE("named-node discovery defaults fail closed and rejects malformed bindings") {
  world legacy;
  CHECK(legacy.service->named_nodes(legacy.context).error().code == s::error_code::unsupported_operation);
  CHECK(legacy.service->names(legacy.node).error().code == s::error_code::unsupported_operation);
  struct discovery_probe : probe {
    std::vector<name_binding> bindings;
    s::result<std::vector<name_binding>> do_named_nodes(std::uint64_t) const override {
      return bindings;
    }
  };
  auto implementation = std::make_unique<discovery_probe>();
  auto &backend = *implementation;
  p::service service(std::move(implementation));
  const auto scene = service.create_scene().value();
  const auto context = service.root_context(scene).value();
  const auto node = service.create_node(context, "Transform").value();
  backend.bindings = {{"Zulu", 1}, {"Alpha", 1}};
  CHECK(service.names(node).value() == std::vector<std::string>{"Alpha", "Zulu"});
  backend.bindings = {{"Duplicate", 1}, {"Duplicate", 2}};
  CHECK(service.named_nodes(context).error().code == s::error_code::invalid_descriptor);
  backend.bindings = {{"Null", 0}};
  CHECK(service.named_nodes(context).error().code == s::error_code::invalid_descriptor);
  backend.bindings = {{"bad name", 1}};
  CHECK(service.named_nodes(context).error().code == s::error_code::invalid_descriptor);
  backend.bindings = {{"Named", 1}};
  REQUIRE(service.define_name(node, "Named"));
  const auto active = service.activate(scene).value();
  const auto client = service.connect(active).value();
  backend.partial = true;
  REQUIRE(service.enqueue(client, {{service.get_field(node, "x").value(), 1.f}}));
  REQUIRE(service.pump(active, {0}));
  REQUIRE(service.state(scene).value() == p::activation_state::faulted);
  CHECK(service.named_nodes(context).value() == std::vector<p::named_node>{{"Named", node}});
  CHECK(service.names(node).value() == std::vector<std::string>{"Named"});
}

namespace {
struct route_probe : probe {
  std::vector<admitted_route> installed;
  std::vector<std::size_t> routes_at_turn;
  bool implement = true;
  p::capabilities supported() const override {
    auto caps = probe::supported();
    caps.services.push_back({p::service_id::live_routes});
    return caps;
  }
  backend_result do_add_live_route(const address &from,
                                   const address &to) override {
    if (!implement)
      return p::backend::do_add_live_route(from, to);
    if (!installed.empty() && installed.back().from.field == from.field &&
        installed.back().to.field == to.field)
      return tl::unexpected{backend_failure{
          error(s::error_code::invalid_route, "probe.live_route", "duplicate")
              .value(),
          p::failure_effect::none}};
    installed.push_back({from, to});
    return {};
  }
  backend_result do_turn(std::uint64_t scene, s::event_time time,
                         const std::vector<admitted_write> &writes) override {
    routes_at_turn.push_back(installed.size());
    return probe::do_turn(scene, time, writes);
  }
};
} // namespace
TEST_CASE("optional queued ROUTEs share FIFO tickets gates and cancellation") {
  auto engine = std::make_unique<route_probe>();
  auto &native = *engine;
  p::service service(std::move(engine));
  const auto scene = service.create_scene().value();
  const auto context = service.root_context(scene).value();
  const auto node = service.create_node(context, "Transform").value();
  const auto x = service.get_field(node, "x").value();
  const auto limit = service.get_field(node, "limit").value();
  const auto active = service.activate(scene).value();
  const auto client = service.connect(active).value();
  auto before = service.enqueue(client, {{x, 1.f}}).value();
  auto route = service.enqueue_route(client, x, limit).value();
  auto after = service.enqueue(client, {{x, 2.f}}).value();
  REQUIRE(native.installed.empty());
  REQUIRE(service.pump(active, {0}));
  CHECK(before.status()->state == p::request_state::processed);
  CHECK(native.routes_at_turn == std::vector<std::size_t>{0});
  REQUIRE(service.pump(active, {1}));
  CHECK(route.status()->state == p::request_state::processed);
  CHECK(native.routes_at_turn == std::vector<std::size_t>{0, 1});
  REQUIRE(service.pump(active, {2}));
  CHECK(after.status()->state == p::request_state::processed);
  CHECK(native.routes_at_turn == std::vector<std::size_t>{0, 1, 1});
  REQUIRE(service.begin_update(client));
  auto held_write = service.enqueue(client, {{x, 2.f}}).value();
  auto held_route = service.enqueue_route(client, limit, x).value();
  CHECK(held_route.status()->state == p::request_state::accepted);
  REQUIRE(service.end_update(client));
  CHECK(held_route.status()->submission == held_write.status()->submission);
  REQUIRE(service.pump(active, {3}));
  CHECK(native.routes_at_turn.back() == 2);
  CHECK(held_route.status()->state == p::request_state::processed);
  auto cancelled = service.enqueue_route(client, x, limit).value();
  REQUIRE(service.cancel(client, cancelled));
  CHECK(cancelled.status()->state == p::request_state::cancelled);
  REQUIRE(service.pump(active, {4}));
  CHECK(native.installed.size() == 2);
  REQUIRE(service.begin_update(client));
  auto cancelled_route = service.enqueue_route(client, x, limit).value();
  auto cancelled_write = service.enqueue(client, {{x, 1.f}}).value();
  REQUIRE(service.end_update(client));
  REQUIRE(service.cancel(client, cancelled_route));
  CHECK(cancelled_write.status()->state == p::request_state::cancelled);
  REQUIRE(service.pump(active, {5}));
  CHECK(native.installed.size() == 2);
  CHECK(std::get<float>(service.read(x).value()) == 2.f);
  const auto input = service.get_field(node, "input").value();
  const auto vector = service.get_field(node, "translation").value();
  CHECK(service.enqueue_route(client, input, x).error().code ==
        s::error_code::invalid_route);
  CHECK(service.enqueue_route(client, x, vector).error().code ==
        s::error_code::invalid_route);
  const auto other_scene = service.create_scene().value();
  const auto other_context = service.root_context(other_scene).value();
  const auto other_node =
      service.create_node(other_context, "Transform").value();
  CHECK(
      service
          .enqueue_route(client, x, service.get_field(other_node, "x").value())
          .error()
          .code == s::error_code::invalid_context);
  world foreign;
  CHECK(service.enqueue_route(client, x, foreign.field("x")).error().code ==
        s::error_code::invalid_context);
  auto disposed = x;
  disposed.dispose();
  CHECK(service.enqueue_route(client, disposed, limit).error().code ==
        s::error_code::disposed);
}
TEST_CASE("live ROUTE defaults fail closed and route requests obey budgets") {
  world legacy;
  const auto active = legacy.service->activate(legacy.scene).value();
  const auto client = legacy.service->connect(active).value();
  CHECK(legacy.service
            ->enqueue_route(client, legacy.field("x"), legacy.field("limit"))
            .error()
            .code == s::error_code::unsupported_operation);
  auto engine = std::make_unique<route_probe>();
  engine->implement = false;
  p::service service(std::move(engine), p::resource_limits{1, 4096});
  const auto scene = service.create_scene().value();
  const auto node =
      service.create_node(service.root_context(scene).value(), "Transform")
          .value();
  const auto x = service.get_field(node, "x").value();
  const auto limit = service.get_field(node, "limit").value();
  const auto a = service.activate(scene).value();
  const auto c = service.connect(a).value();
  auto route = service.enqueue_route(c, x, limit).value();
  CHECK(service.enqueue(c, {{x, 1.f}}).error().code ==
        s::error_code::resource_limit);
  const auto report = service.pump(a, {0}).value();
  REQUIRE(report.error);
  CHECK(report.error->code == s::error_code::unsupported_operation);
  CHECK(report.effects == p::failure_effect::none);
  CHECK(report.state == p::activation_state::active);
  CHECK(route.status()->state == p::request_state::failed);
  REQUIRE(service.enqueue(c, {{x, 1.f}}));
  REQUIRE(service.pump(a, {1}));
  p::service no_bytes(std::make_unique<route_probe>(),
                      p::resource_limits{1, 0});
  const auto empty = no_bytes.create_scene().value();
  const auto n =
      no_bytes.create_node(no_bytes.root_context(empty).value(), "Transform")
          .value();
  const auto ca = no_bytes.connect(no_bytes.activate(empty).value()).value();
  CHECK(no_bytes
            .enqueue_route(ca, no_bytes.get_field(n, "x").value(),
                           no_bytes.get_field(n, "limit").value())
            .error()
            .code == s::error_code::resource_limit);
}
TEST_CASE(
    "a later ROUTE failure cannot claim an earlier installation rolled back") {
  auto engine = std::make_unique<route_probe>();
  auto &native = *engine;
  p::service service(std::move(engine));
  const auto scene = service.create_scene().value();
  const auto node =
      service.create_node(service.root_context(scene).value(), "Transform")
          .value();
  const auto x = service.get_field(node, "x").value();
  const auto limit = service.get_field(node, "limit").value();
  const auto active = service.activate(scene).value();
  const auto client = service.connect(active).value();
  REQUIRE(service.begin_update(client));
  auto first = service.enqueue_route(client, x, limit).value();
  auto duplicate = service.enqueue_route(client, x, limit).value();
  REQUIRE(service.end_update(client));
  auto later = service.enqueue(client, {{x, 1.f}}).value();
  const auto report = service.pump(active, {0}).value();
  CHECK(report.state == p::activation_state::faulted);
  CHECK(report.effects == p::failure_effect::partial_or_unknown);
  CHECK(native.installed.size() == 1);
  CHECK(native.turns == 0);
  CHECK(first.status()->state == p::request_state::failed);
  CHECK(duplicate.status()->effects == p::failure_effect::partial_or_unknown);
  CHECK(later.status()->state == p::request_state::cancelled);
}
