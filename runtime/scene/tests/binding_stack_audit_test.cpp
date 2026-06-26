// binding_stack_audit_test.cpp — AUD-BIND-STACK regression tests.
// Covers empty-stack unbind, rapid transitions, bindTime precision,
// event ordering, and default-vs-explicit bind interaction.
#include "BindingStack.hpp"
#include "BindingSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <vector>
using namespace x3d::runtime;

static X3DBindableNode* bnd(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

TEST_CASE("binding_stack_audit_test") {
  // ── 1. Empty binding stack: unbind when nothing bound → no-op ─────────────
  {
    int a=0;
    X3DNode* A = reinterpret_cast<X3DNode*>(&a);
    std::vector<std::pair<X3DNode*,bool>> ev;
    BindingStack::Emit emit = [&](X3DNode* n, bool bound){ ev.emplace_back(n,bound); };
    BindingStack s;
    CHECK((s.top() == nullptr));
    s.unbind(A, emit);
    CHECK((ev.empty()));
    CHECK((s.top() == nullptr));
  }

  // ── 2. Multiple rapid bind/unbind transitions ─────────────────────────────
  {
    int a=0,b=0,c=0;
    X3DNode* A = reinterpret_cast<X3DNode*>(&a);
    X3DNode* B = reinterpret_cast<X3DNode*>(&b);
    X3DNode* C = reinterpret_cast<X3DNode*>(&c);
    std::vector<std::pair<X3DNode*,bool>> ev;
    BindingStack::Emit emit = [&](X3DNode* n, bool bound){ ev.emplace_back(n,bound); };
    BindingStack s;

    s.bind(A, emit); ev.clear();
    s.bind(B, emit); ev.clear();
    s.unbind(B, emit); // B->false, A->true
    CHECK((s.top() == A));
    CHECK((ev.size() == 2));
    CHECK((ev[0] == std::make_pair(B, false)));
    CHECK((ev[1] == std::make_pair(A, true)));

    ev.clear();
    s.bind(A, emit); // already top → no-op
    CHECK((ev.empty()));

    ev.clear();
    s.bind(C, emit); // A->false, C->true
    CHECK((s.top() == C));
    CHECK((ev.size() == 2));

    ev.clear();
    s.unbind(A, emit); // A not on stack → no-op
    CHECK((ev.empty() && s.top() == C));

    ev.clear();
    s.unbind(C, emit); // C->false, stack empty
    CHECK((s.top() == nullptr));
    CHECK((ev.size() == 1 && ev[0] == std::make_pair(C, false)));
  }

  // ── 3. bindTime precision: fractional seconds preserved ───────────────────
  {
    auto vp = createX3DNode("Viewpoint");
    Scene scene; scene.addRootNode(vp);
    X3DExecutionContext ctx;
    constexpr double kFrac = 12345.678901234;
    ctx.tick(kFrac);
    ctx.buildSceneGraph(scene);
    CHECK((bnd(vp)->getBindTime() == kFrac && "fractional bindTime must be exact"));
  }

  // ── 4. isBound event ordering: isBound before bindTime ────────────────────
  {
    auto vp1 = createX3DNode("Viewpoint");
    auto vp2 = createX3DNode("Viewpoint");
    Scene scene; scene.addRootNode(vp1); scene.addRootNode(vp2);

    std::vector<std::string> order;
    BindingSystem bs;
    bs.enrollScene(scene,
        [&](X3DNode* n, const std::string& f, std::any v){
          (void)n; (void)v;
          order.push_back(f);
        },
        [&]{ return 42.0; });

    // Live bind of vp2 → poster fires isBound then bindTime
    bnd(vp2)->onSet_bind(true);
    bool foundIsBound = false;
    for (const auto& ev : order) {
      if (ev == "isBound") foundIsBound = true;
      if (ev == "bindTime") {
        CHECK((foundIsBound && "bindTime must not arrive before isBound"));
        foundIsBound = false; // reset for next pair
      }
    }
  }

  // ── 5. Default bind vs explicit bind: re-binding top is no-op ─────────────
  {
    auto vp1 = createX3DNode("Viewpoint");
    auto vp2 = createX3DNode("Viewpoint");
    Scene scene; scene.addRootNode(vp1); scene.addRootNode(vp2);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    // vp1 is default-bound
    CHECK((bnd(vp1)->getIsBound() == true));
    double t0 = bnd(vp1)->getBindTime();

    // Explicit bind of already-top node → no transition, bindTime unchanged
    bnd(vp1)->onSet_bind(true);
    ctx.process();
    CHECK((ctx.boundViewpoint() == vp1.get()));
    CHECK((bnd(vp1)->getIsBound() == true));
    CHECK((bnd(vp1)->getBindTime() == t0 && "re-binding top must not change bindTime"));
  }

  return;
}
