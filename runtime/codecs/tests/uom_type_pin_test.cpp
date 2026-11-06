#include "doctest/doctest.h"
// Sound §16.4.13 prose lists integer gain; X3DUOM uses SFFloat.
// Pin the generated type to retain fractional gain values.

#include "x3d/nodes/ListenerPointSource.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <type_traits>
#include <utility>

using namespace x3d::core;

namespace {

static_assert(
    std::is_same_v<std::remove_cvref_t<decltype(
                       std::declval<const x3d::nodes::ListenerPointSource>().getGain())>,
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
  x3d::nodes::ListenerPointSource lps;
  CHECK(x3d::nodes::ListenerPointSource::getDefaultGain() == doctest::Approx(1.0f));
  CHECK(lps.getGain() == doctest::Approx(1.0f));

  lps.setGain(0.25f); // sub-unity — impossible under the prose's SFInt32 typing
  CHECK(lps.getGain() == doctest::Approx(0.25f));
}
