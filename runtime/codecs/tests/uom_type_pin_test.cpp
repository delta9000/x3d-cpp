#include "doctest/doctest.h"
// uom_type_pin_test.cpp
// Codegen-drift pins for fields whose ISO 19775 prose disagrees with the
// X3DUOM, where x3d-cpp is correct *because* it generates from the UOM. These
// findings are only "resolved" so long as the generator keeps emitting the UOM
// type; a binding regression would silently reintroduce the spec's bug. Each
// pin fails the build (static_assert) or the test if that drift happens.
//
// SND-GAIN-TYPE (conformance finding; sound §16.4.13): ISO prose types
// X3DSoundSourceNode/ListenerPointSource `gain` as SFInt32 (`gain 1`) — an
// integer cannot express sub-unity gain. The X3DUOM inherits SFFloat from
// X3DSoundSourceNode and every sibling gain in the component is SFFloat, so the
// UOM (and this engine) are correct; the prose is an upstream erratum. Pin the
// generated accessor to SFFloat so a codegen change reintroducing SFInt32 fails
// to compile here.

#include "ListenerPointSource.hpp"
#include "X3Dtypes.hpp"

#include <type_traits>
#include <utility>

// The generated bindings (ListenerPointSource, SFFloat) live in the global
// namespace — only the hand-written runtime headers open `namespace x3d`, and
// this pin deliberately includes only the generated node + types.

namespace {

// Compile-time: the generated `gain` accessor returns SFFloat (float), not an
// integer. If the UOM/codegen ever drifts to the prose's SFInt32, this fails to
// build — the strongest possible regression guard.
static_assert(
    std::is_same_v<decltype(std::declval<const ListenerPointSource>().getGain()),
                   SFFloat>,
    "ListenerPointSource::gain must be SFFloat (X3DUOM); ISO 19775 §16.4.13 "
    "prose says SFInt32 and is an upstream erratum — do not 'fix' the binding "
    "to match it (SND-GAIN-TYPE).");

static_assert(std::is_same_v<SFFloat, float>,
              "SFFloat is the float field type; this pin assumes it.");

} // namespace

TEST_CASE("uom_type_pin: ListenerPointSource.gain is SFFloat with default 1") {
  // Runtime corollaries of the compile-time pin above: the field is reachable as
  // a float and carries the UOM default (1.0), able to represent sub-unity gain.
  ListenerPointSource lps;
  CHECK(ListenerPointSource::getDefaultGain() == doctest::Approx(1.0f));
  CHECK(lps.getGain() == doctest::Approx(1.0f));

  lps.setGain(0.25f); // sub-unity — impossible under the prose's SFInt32 typing
  CHECK(lps.getGain() == doctest::Approx(0.25f));
}
