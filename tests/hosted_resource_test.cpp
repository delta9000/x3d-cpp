#include "doctest/doctest.h"
#include "x3d/sai/experimental/provider.hpp"
#include "../src/provider_budget.hpp"

#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>
#include <thread>

// This dedicated executable measures outstanding ordinary allocations, rather
// than relying on a wall-clock threshold or a production diagnostic history.
namespace { std::atomic<std::size_t> allocations{0}; }
void *operator new(std::size_t size) {
  if (auto *p = std::malloc(size ? size : 1)) { ++allocations; return p; }
  throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { if (p) { --allocations; std::free(p); } }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete(void *p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }

namespace s = x3d::sai::experimental;
namespace h = s::provider;
namespace {
struct counters {
  std::size_t turns = 0, retired = 0;
  float current = 0;
  bool fail = false;
};
class minimal_backend final : public h::backend {
public:
  explicit minimal_backend(std::shared_ptr<counters> state) : counters_(std::move(state)) {}
  h::capabilities supported() const override {
    return {"resource-probe",1,{{h::service_id::scene_authoring,1},{h::service_id::host_turns,1}},
      {{"Probe","Test",1}},{s::value_kind::sf_float,s::value_kind::mf_float}};
  }
protected:
  s::result<std::uint64_t> do_create_scene() override { return next_scene_++; }
  s::result<std::uint64_t> do_create_node(std::uint64_t,std::string_view) override { return 1; }
  s::result<void> do_append_root(const address &) override { return {}; }
  s::result<void> do_define_name(const address &,std::string_view) override { return {}; }
  s::result<std::vector<std::uint64_t>> do_roots(std::uint64_t) const override { return std::vector<std::uint64_t>{1}; }
  s::result<std::uint64_t> do_named(std::uint64_t, std::string_view) const override { return 1; }
  s::result<std::string> do_type_name(const address &) const override { return std::string("Probe"); }
  s::result<std::vector<h::field_info>> do_fields(const address &) const override {
    return std::vector<h::field_info>{{"value", s::value_kind::sf_float, s::access_type::input_output, true},
                                    {"list", s::value_kind::mf_float, s::access_type::input_output, true}};
  }
  s::result<backend_value> do_read(const address &where) const override {
    return where.field == "list" ? backend_value{s::float_list{}} : backend_value{counters_->current};
  }
  s::result<void> do_author(const address &, const backend_value &) override { return {}; }
  s::result<void> do_add_route(const address &, const address &) override { return {}; }
  s::result<void> do_validate_configuration(std::uint64_t, const std::vector<admitted_write> &) const override { return {}; }
  s::result<void> do_activate(std::uint64_t) override { return {}; }
  backend_result do_turn(std::uint64_t, s::event_time, const std::vector<admitted_write> &writes) override {
    ++counters_->turns;
    for (const auto &w : writes) if (const auto *v = std::get_if<float>(&w.value)) counters_->current = *v;
    if (counters_->fail) return tl::make_unexpected(backend_failure{
        error(s::error_code::callback_failed, "probe.turn", "partial native effect").value(),
        h::failure_effect::partial_or_unknown});
    return {};
  }
  void do_retire(std::uint64_t) noexcept override { ++counters_->retired; }
private:
  std::shared_ptr<counters> counters_;
  std::uint64_t next_scene_ = 1;
};
// Test-owned diagnostics survive backend destruction without retaining it.
struct minimal {
  std::shared_ptr<counters> counters_ = std::make_shared<counters>();
  h::service api;
  std::size_t &turns = counters_->turns, &retired = counters_->retired;
  float &current = counters_->current;
  bool &fail = counters_->fail;
  explicit minimal(h::resource_limits limits = {})
    : api(std::make_unique<minimal_backend>(counters_), limits) {}
  static std::size_t write_cost(std::string_view field = "value") {
    return sizeof(h::write) + field.size();
  }
};
struct world {
  h::scene scene;
  h::field input, list;
  h::activation active;
  h::client client;
  explicit world(minimal &host) {
    scene = host.api.create_scene().value();
    auto context = host.api.root_context(scene).value();
    auto node = host.api.create_node(context,"Probe").value();
    REQUIRE(host.api.append_root(context,node));
    input = host.api.get_field(node, "value").value();
    list = host.api.get_field(node, "list").value();
    active = host.api.activate(scene).value();
    client = host.api.connect(active).value();
  }
};
void expect_full(const s::result<h::ticket> &result) {
  REQUIRE_FALSE(result);
  CHECK(result.error().code == s::error_code::resource_limit);
}
}

TEST_CASE("provider byte accounting checks products and sums without overflow") {
  const auto maximum = std::numeric_limits<std::size_t>::max();
  h::detail::byte_budget bytes(maximum);
  CHECK_FALSE(bytes.add(maximum, 2));
  CHECK(bytes.remaining() == maximum);
  CHECK(bytes.add(maximum - 3));
  CHECK_FALSE(bytes.add(4));
  CHECK(bytes.remaining() == 3);
  CHECK(bytes.add(1, 3));
  CHECK(bytes.add(maximum, 0));
  CHECK_FALSE(bytes.add(1));
  CHECK(bytes.remaining() == 0);
  h::detail::byte_budget zero(0);
  CHECK(zero.add(0, maximum));
  CHECK_FALSE(zero.add(1));
  h::detail::byte_budget nested(2 * sizeof(std::string) + 5);
  CHECK(h::detail::account_dynamic(nested, s::string_list{"ab", "cde"}));
  CHECK(nested.remaining() == 0);
  h::detail::byte_budget images(2 * sizeof(s::image) + 3);
  CHECK(h::detail::account_dynamic(images, s::image_list{{1,1,1,{1}}, {2,1,1,{2,3}}}));
  CHECK(images.remaining() == 0);
  h::detail::byte_budget booleans(2);
  CHECK(h::detail::account_dynamic(booleans, s::bool_list{false,true}));
  CHECK(booleans.remaining() == 0);
}

TEST_CASE("provider payload budget includes empty writes and recovers exactly") {
  SUBCASE("zero request capacity") {
    minimal host({0, std::numeric_limits<std::size_t>::max()}); world w(host);
    expect_full(host.api.enqueue(w.client, {{w.input, 1.f}}));
    CHECK_FALSE(host.api.pump(w.active, {0})->request);
  }
  SUBCASE("zero byte capacity rejects even an empty list") {
    minimal host({100, 0}); world w(host);
    expect_full(host.api.enqueue(w.client, {{w.list, s::float_list{}}}));
  }
  SUBCASE("write object and canonical field bytes count at exact boundary") {
    minimal host({100, minimal::write_cost("list")}); world w(host);
    auto first = host.api.enqueue(w.client, {{w.list, s::float_list{}}}).value();
    expect_full(host.api.enqueue(w.client, {{w.list, s::float_list{}}}));
    REQUIRE(host.api.cancel(w.client, first));
    REQUIRE(host.api.enqueue(w.client, {{w.list, s::float_list{}}}));
    REQUIRE(host.api.pump(w.active, {0}));
    REQUIRE(host.api.enqueue(w.client, {{w.list, s::float_list{}}}));
  }
  SUBCASE("owning vector payload exact boundary and one-byte-under reject") {
    const auto cost = minimal::write_cost("list") + 3 * sizeof(float);
    minimal tight({5, cost - 1}); world a(tight);
    expect_full(tight.api.enqueue(a.client, {{a.list, s::float_list{0,1,2}}}));
    minimal exact({5, cost}); world b(exact);
    auto ticket = exact.api.enqueue(b.client, {{b.list, s::float_list{0,1,2}}}).value();
    expect_full(exact.api.enqueue(b.client, {{b.input, 1.f}}));
    REQUIRE(exact.api.pump(b.active, {0}));
    CHECK(ticket.status()->state == h::request_state::processed);
    REQUIRE(exact.api.enqueue(b.client, {{b.list, s::float_list{0,1,2}}}));
  }
}

TEST_CASE("provider limits cover all scenes and gates without discarding accepted work") {
  minimal host({3, std::numeric_limits<std::size_t>::max()}); world a(host), b(host);
  auto other = host.api.connect(a.active).value();
  REQUIRE(host.api.begin_update(a.client));
  REQUIRE(host.api.begin_update(a.client)); // Repeated Begin is a no-op.
  auto held1 = host.api.enqueue(a.client, {{a.input, 1.f}}).value();
  auto held2 = host.api.enqueue(a.client, {{a.input, 2.f}}).value();
  auto free = host.api.enqueue(other, {{a.input, 3.f}}).value();
  expect_full(host.api.enqueue(b.client, {{b.input, 4.f}}));
  CHECK(held1.status()->state == h::request_state::accepted);
  CHECK(host.api.pump(a.active, {0})->request == free.id());
  REQUIRE(host.api.enqueue(b.client, {{b.input, 4.f}}));
  expect_full(host.api.enqueue(other, {{a.input, 5.f}}));
  REQUIRE(host.api.end_update(a.client));
  REQUIRE(host.api.end_update(a.client)); // Unmatched End is a no-op.
  CHECK(host.api.pump(a.active, {0})->request == held1.id());
  CHECK(host.current == 2.f); // One final retained-field candidate per gate.
  CHECK(held2.status()->state == h::request_state::processed);
  CHECK(held1.status()->submission == held2.status()->submission);
  CHECK(held1.status()->turn == held2.status()->turn);
  CHECK_FALSE(host.api.pump(a.active, {0})->request);
  CHECK(host.api.pump(b.active, {0})->request.has_value());
  CHECK(host.current == 4.f);
}

TEST_CASE("provider arbitrary cancellations leave a live FIFO with no tombstones") {
  minimal host({3, std::numeric_limits<std::size_t>::max()}); world w(host);
  auto first = host.api.enqueue(w.client, {{w.input, 1.f}}).value();
  auto middle = host.api.enqueue(w.client, {{w.input, 2.f}}).value();
  auto last = host.api.enqueue(w.client, {{w.input, 3.f}}).value();
  REQUIRE(host.api.cancel(w.client, middle));
  auto replacement = host.api.enqueue(w.client, {{w.input, 4.f}}).value();
  REQUIRE(host.api.cancel(w.client, last));
  CHECK(host.api.pump(w.active, {0})->request == first.id());
  CHECK(host.api.pump(w.active, {0})->request == replacement.id());
  CHECK_FALSE(host.api.pump(w.active, {0})->request);
  CHECK(middle.status()->state == h::request_state::cancelled);
  CHECK_FALSE(host.api.cancel(w.client, middle));
}

TEST_CASE("provider receipts survive disconnect retirement close and destruction") {
  h::ticket processed, cancelled, held;
  h::scene authority;
  {
    auto host = std::make_unique<minimal>(h::resource_limits{3, 4096}); world w(*host);
    authority = w.scene;
    processed = host->api.enqueue(w.client, {{w.input, 1.f}}).value();
    REQUIRE(host->api.pump(w.active, {0}));
    cancelled = host->api.enqueue(w.client, {{w.input, 2.f}}).value();
    REQUIRE(host->api.disconnect(w.client));
    CHECK(cancelled.status()->state == h::request_state::cancelled);
    auto new_client = host->api.connect(w.active).value();
    REQUIRE(host->api.begin_update(new_client));
    held = host->api.enqueue(new_client, {{w.input, 3.f}}).value();
    REQUIRE(host->api.retire(w.active));
    CHECK(host->retired == 1);
    CHECK(held.status()->state == h::request_state::cancelled);
    world another(*host);
    held = host->api.enqueue(another.client, {{another.input, 4.f}}).value();
    REQUIRE(host->api.close());
    CHECK(host->retired == 2);
    CHECK(authority.expired());
    CHECK(held.status()->state == h::request_state::cancelled);
    CHECK(processed.status()->state == h::request_state::processed);
  }
  CHECK(authority.expired());
  CHECK(processed.status()->turn == 1);
  CHECK(cancelled.status()->state == h::request_state::cancelled);
  auto copy = processed;
  processed = {};
  CHECK(copy.status()->state == h::request_state::processed);
  CHECK_FALSE(processed.status());
  CHECK(processed.id() == 0);
}

TEST_CASE("provider receipt inspection is concurrent while mutation retains authority") {
  minimal host; world w(host);
  auto first = host.api.enqueue(w.client, {{w.input, 1.f}}).value();
  minimal foreign; world other(foreign);
  auto wrong_client = host.api.connect(w.active).value();
  CHECK_FALSE(host.api.cancel(wrong_client, first));
  CHECK_FALSE(foreign.api.cancel(other.client, first));
  CHECK_FALSE(host.api.cancel(w.client, h::ticket{}));
  std::atomic_bool done = false, wrong_thread = false, valid = true;
  std::thread reader([&] {
    auto denied = host.api.cancel(w.client, first);
    wrong_thread = !denied && denied.error().code == s::error_code::wrong_thread;
    do {
      auto status = first.status();
      if (!status || (status->state != h::request_state::submitted &&
                      status->state != h::request_state::processed)) valid = false;
    } while (!done.load());
  });
  REQUIRE(host.api.pump(w.active, {0}));
  REQUIRE(host.api.close());
  done = true;
  reader.join();
  CHECK(valid);
  CHECK(wrong_thread);
  CHECK(first.status()->state == h::request_state::processed);
}

TEST_CASE("provider fault releases submitted and held payload budgets across scenes") {
  minimal host({3, 3 * minimal::write_cost()}); world w(host), survivor(host);
  auto held_client = host.api.connect(w.active).value();
  REQUIRE(host.api.begin_update(held_client));
  auto held = host.api.enqueue(held_client, {{w.input, 1.f}}).value();
  auto failing = host.api.enqueue(w.client, {{w.input, 2.f}}).value();
  auto behind = host.api.enqueue(w.client, {{w.input, 3.f}}).value();
  host.fail = true;
  auto report = host.api.pump(w.active, {0}).value();
  CHECK(report.state == h::activation_state::faulted);
  CHECK(failing.status()->effects == h::failure_effect::partial_or_unknown);
  CHECK(failing.status()->state == h::request_state::failed);
  CHECK(held.status()->state == h::request_state::cancelled);
  CHECK(behind.status()->state == h::request_state::cancelled);
  host.fail = false;
  for (int i = 0; i < 3; ++i) REQUIRE(host.api.enqueue(survivor.client, {{survivor.input, float(i)}}));
  expect_full(host.api.enqueue(survivor.client, {{survivor.input, 4.f}}));
}

TEST_CASE("provider completed history and discarded receipts retain no service allocations") {
  minimal host({1, 4096}); world w(host);
  const auto before = allocations.load();
  for (int i = 0; i < 32000; ++i) {
    auto ticket = host.api.enqueue(w.client, {{w.input, float(i)}});
    if (!ticket || !host.api.pump(w.active, {double(i)})) FAIL("bounded sequential admission failed");
  }
  const auto after = allocations.load();
  CHECK(after == before);
  for (int i = 0; i < 100; ++i) REQUIRE_FALSE(host.api.pump(w.active, {32000})->request);
  // Dropping the receipt preserves the accepted request until processing.
  { auto dropped = host.api.enqueue(w.client, {{w.input, 42.f}}).value(); }
  CHECK(host.api.pump(w.active, {32000})->request.has_value());
  CHECK(host.current == 42.f);
  std::vector<h::ticket> receipts;
  receipts.reserve(1000);
  const auto without_receipts = allocations.load();
  for (int i = 0; i < 1000; ++i) {
    receipts.push_back(host.api.enqueue(w.client, {{w.input, float(i)}}).value());
    REQUIRE(host.api.pump(w.active, {32000}));
  }
  CHECK(allocations.load() > without_receipts);
  receipts.clear();
  const auto released = allocations.load();
  CHECK(released == without_receipts);
}

TEST_CASE("provider retirement immediately frees gated and submitted owning payloads") {
  minimal host({4, 1000000}); world w(host), survivor(host);
  auto gated = host.api.connect(w.active).value();
  REQUIRE(host.api.begin_update(gated));
  auto free = host.api.enqueue(w.client, {{w.list, s::float_list(10000, 1.f)}}).value();
  auto held = host.api.enqueue(gated, {{w.list, s::float_list(10000, 2.f)}}).value();
  const auto pending = allocations.load();
  REQUIRE(host.api.retire(w.active));
  CHECK(free.status()->state == h::request_state::cancelled);
  CHECK(held.status()->state == h::request_state::cancelled);
  CHECK(allocations.load() < pending);
  REQUIRE(host.api.enqueue(survivor.client, {{survivor.list, s::float_list(20000, 3.f)}}));
}

TEST_CASE("provider byte capacity is service-wide and rejects a batch before mutation") {
  minimal host({100, 2 * minimal::write_cost()}); world a(host), b(host);
  REQUIRE(host.api.begin_update(a.client));
  auto held = host.api.enqueue(a.client, {{a.input, 1.f}}).value();
  auto released = host.api.enqueue(b.client, {{b.input, 2.f}}).value();
  expect_full(host.api.enqueue(b.client, {{b.input, 3.f}}));
  CHECK(held.status()->state == h::request_state::accepted);
  CHECK(released.status()->state == h::request_state::submitted);
  CHECK(host.turns == 0);
  CHECK(host.current == 0.f);
  REQUIRE(host.api.cancel(a.client, held));
  // This two-write batch exceeds the recovered byte budget. The first write
  // must not be appended independently before discovering that fact.
  expect_full(host.api.enqueue(a.client, {{a.input, 4.f}, {a.list, s::float_list{}}}));
  CHECK(host.api.pump(b.active, {0})->request == released.id());
  CHECK(host.current == 2.f);
  REQUIRE(host.api.end_update(a.client));
  CHECK_FALSE(host.api.pump(a.active, {0})->request);
  REQUIRE(host.api.enqueue(a.client, {{a.input, 5.f}}));
  REQUIRE(host.api.enqueue(b.client, {{b.input, 6.f}}));
}

TEST_CASE("provider disconnected client churn retains no historical client records") {
  minimal host; world w(host);
  const auto before = allocations.load();
  for (int i = 0; i < 1000; ++i) {
    auto client = host.api.connect(w.active).value();
    REQUIRE(host.api.begin_update(client));
    auto receipt = host.api.enqueue(client, {{w.list, s::float_list{1,2,3}}}).value();
    REQUIRE(host.api.disconnect(client));
    CHECK(receipt.status()->state == h::request_state::cancelled);
  }
  const auto after = allocations.load();
  CHECK(after == before);
}

TEST_CASE("provider gate release preserves constant-time cancellation links") {
  minimal host; world w(host);
  auto other = host.api.connect(w.active).value();
  REQUIRE(host.api.begin_update(w.client));
  auto first = host.api.enqueue(w.client, {{w.input, 1.f}}).value();
  auto last = host.api.enqueue(w.client, {{w.input, 2.f}}).value();
  auto before = host.api.enqueue(other, {{w.input, 3.f}}).value();
  REQUIRE(host.api.end_update(w.client));
  REQUIRE(host.api.cancel(w.client, first));
  CHECK(host.api.pump(w.active, {0})->request == before.id());
  CHECK_FALSE(host.api.pump(w.active, {0})->request);
  CHECK(first.status()->state == h::request_state::cancelled);
  CHECK(last.status()->state == h::request_state::cancelled);
  CHECK(first.status()->submission == last.status()->submission);
  CHECK_FALSE(host.api.pump(w.active, {0})->request);
}
