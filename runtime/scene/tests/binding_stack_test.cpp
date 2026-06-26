// binding_stack_test.cpp — pure stack logic; nodes are opaque pointers (never
// dereferenced), so fake pointers are fine.
#include "BindingStack.hpp"
#include "doctest/doctest.h"
#include <string>
#include <vector>
using namespace x3d::runtime;

TEST_CASE("binding_stack_test") {
  int a=0,b=0; // identity stand-ins
  X3DNode* A = reinterpret_cast<X3DNode*>(&a);
  X3DNode* B = reinterpret_cast<X3DNode*>(&b);

  std::vector<std::pair<X3DNode*,bool>> ev;
  BindingStack::Emit emit = [&](X3DNode* n, bool bound){ ev.emplace_back(n,bound); };
  BindingStack s;

  s.pushDefault(A, emit);          // A bound
  CHECK((s.top()==A));
  CHECK((ev.size()==1 && ev[0].first==A && ev[0].second==true));

  ev.clear();
  s.bind(B, emit);                 // A->false, B->true
  CHECK((s.top()==B));
  CHECK((ev.size()==2 && ev[0]==std::make_pair(A,false) && ev[1]==std::make_pair(B,true)));

  ev.clear();
  s.bind(B, emit);                 // already top: no-op
  CHECK((ev.empty() && s.top()==B));

  ev.clear();
  s.bind(A, emit);                 // A already on stack -> move to top: B->false, A->true
  CHECK((s.top()==A));
  CHECK((ev.size()==2 && ev[0]==std::make_pair(B,false) && ev[1]==std::make_pair(A,true)));

  ev.clear();
  s.unbind(A, emit);               // top pops: A->false, new top B->true
  CHECK((s.top()==B));
  CHECK((ev.size()==2 && ev[0]==std::make_pair(A,false) && ev[1]==std::make_pair(B,true)));

  ev.clear();
  s.unbind(A, emit);               // A not on stack (already removed): no-op
  CHECK((ev.empty() && s.top()==B));

  ev.clear();
  s.unbind(B, emit);               // last node pops: B->false, stack empty
  CHECK((s.top()==nullptr));
  CHECK((ev.size()==1 && ev[0]==std::make_pair(B,false)));
  return;
}
