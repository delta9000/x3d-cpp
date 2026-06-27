// inflate_bomb_test.cpp
// SEC-5 regression: inflateGzip must cap decompressed output so a gzip
// "decompression bomb" — a tiny compressed stream that expands to gigabytes, or
// a forged ISIZE footer — cannot drive unbounded heap growth into an OOM DoS.
// The cap is `kMaxDecompressedBytes` (RecursionLimits.hpp), overridable per-call
// via inflateGzip's `maxOut` parameter (mirrors MeshBuildOptions.maxWalkVisits).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "Inflate.hpp"
#include "RecursionLimits.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace x3d::codec;

namespace {

int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

// Wrap `payload` in a valid gzip stream using a single STORED (uncompressed)
// DEFLATE block. tinfl inflates this like any gzip, and a stored block lets us
// pick the exact decompressed size by hand. `payload.size()` must be < 65536.
std::string gzipStored(const std::string &payload) {
  std::string g;
  const unsigned char hdr[10] = {0x1f, 0x8b, 0x08, 0x00, 0, 0, 0, 0, 0, 0xff};
  g.append(reinterpret_cast<const char *>(hdr), 10);
  // One final stored block: byte0 = BFINAL(1)|BTYPE(00) = 0x01, then LEN/~LEN LE.
  const std::uint16_t len = static_cast<std::uint16_t>(payload.size());
  const std::uint16_t nlen = static_cast<std::uint16_t>(~len);
  g.push_back(0x01);
  g.push_back(static_cast<char>(len & 0xff));
  g.push_back(static_cast<char>(len >> 8));
  g.push_back(static_cast<char>(nlen & 0xff));
  g.push_back(static_cast<char>(nlen >> 8));
  g += payload;
  // Footer: CRC32(payload) then ISIZE (= len), both little-endian.
  const std::uint32_t crc = detail::crc32(
      reinterpret_cast<const unsigned char *>(payload.data()), payload.size());
  const std::uint32_t isize = static_cast<std::uint32_t>(payload.size());
  for (int i = 0; i < 4; ++i)
    g.push_back(static_cast<char>((crc >> (8 * i)) & 0xff));
  for (int i = 0; i < 4; ++i)
    g.push_back(static_cast<char>((isize >> (8 * i)) & 0xff));
  return g;
}

bool throwsInflate(const std::string &gz, std::size_t maxOut) {
  try {
    inflateGzip(gz, maxOut);
    return false;
  } catch (const std::exception &) {
    return true;
  }
}

} // namespace

int main() {
  const std::string payload(4096, 'A');
  const std::string gz = gzipStored(payload);

  // Sanity: a legitimate stream inflates to its payload under the default cap.
  check(inflateGzip(gz) == payload,
        "stored-block gzip inflates to the original payload");

  // SEC-5: output larger than the cap throws rather than growing unbounded.
  check(throwsInflate(gz, 1024),
        "inflateGzip throws when the output would exceed maxOut");

  // A cap at or above the true output size still inflates successfully.
  check(inflateGzip(gz, 8192) == payload,
        "inflateGzip honors a maxOut above the output size");

  // The default cap is the shared, generous kMaxDecompressedBytes constant.
  check(x3d::kMaxDecompressedBytes >= (1u << 20),
        "kMaxDecompressedBytes default is generous (>= 1 MiB)");

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all inflate_bomb checks passed\n";
  return 0;
}
