// Feed arbitrary bytes to encoding detection and each reader. Malformed-input
// exceptions are expected; crashes and sanitizer reports are failures.

#include <cstddef>
#include <cstdint>
#include <string>

#include "x3d/sdk.hpp"

namespace {

// One-shot parse with an explicit encoding hint. The parser must never crash,
// regardless of the byte stream — the harness returns 0 unconditionally so
// libFuzzer treats every input as "covered" (the crash is what we detect via
// the signal handlers ASan/UBSan install).
int parse_one(const uint8_t *data, std::size_t size, x3d::sdk::Encoding enc) {
  std::string in(reinterpret_cast<const char *>(data), size);
  try {
    auto doc = x3d::sdk::parseDocument(in, enc);
    // Touch the parsed scene so the compiler doesn't optimize the whole call
    // out (the parser does real work — root nodes is a vector we'd otherwise
    // never read in this harness).
    (void)doc.scene.rootNodes.size();
  } catch (...) {
    // Expected: malformed input raises soft-failure exceptions (we have a
    // documented "never panic" parser contract). Catch + swallow.
  }
  return 0;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size) {
  // Drive all three encodings against the same byte stream. parseDocument's
  // encoding hint is the only differentiator — the byte stream is identical.
  parse_one(data, size, x3d::sdk::Encoding::XML);
  parse_one(data, size, x3d::sdk::Encoding::JSON);
  parse_one(data, size, x3d::sdk::Encoding::ClassicVRML);
  // Also drive the sniff path (Encoding::Unknown) so the auto-detect branch is
  // covered too — the parser reads a magic header / XML decl / leading brace to
  // pick the encoding.
  parse_one(data, size, x3d::sdk::Encoding::Unknown);
  return 0;
}
