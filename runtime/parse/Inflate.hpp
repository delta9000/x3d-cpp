// RFC 1952 gzip decoding via bundled public-domain miniz tinfl (see tinfl.h).
// Throws std::runtime_error on truncation, corruption, checksum mismatch, or output limits.

#ifndef X3D_INFLATE_HPP
#define X3D_INFLATE_HPP

#include "tinfl.h"

#include "../RecursionLimits.hpp" // x3d::kMaxDecompressedBytes (decompressed-size cap)

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::codec {

namespace detail {

/// Standard CRC32 (RFC 1952 / zlib polynomial 0xEDB88320), table-on-first-use.
inline std::uint32_t crc32(const unsigned char *data, std::size_t len) {
  static const auto table = [] {
    std::vector<std::uint32_t> t(256);
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k)
        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      t[i] = c;
    }
    return t;
  }();
  std::uint32_t c = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < len; ++i)
    c = table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

} // namespace detail

/// Inflate one gzip member starting at `compressed[0]` and append its bytes to
/// `out`, which may grow to at most `maxOut` bytes in total. Returns the number
/// of input bytes the member occupied (header + DEFLATE body + footer).
inline std::size_t inflateGzipMember(std::string_view compressed,
                                     std::string &out, std::size_t maxOut) {
  const auto *p = reinterpret_cast<const unsigned char *>(compressed.data());
  const std::size_t n = compressed.size();

  // Fixed 10-byte header: ID1 ID2 CM FLG MTIME(4) XFL OS.
  if (n < 18) // 10 header + at least 0 body + 8 footer
    throw std::runtime_error("inflateGzip: input too short for a gzip stream");
  if (p[0] != 0x1f || p[1] != 0x8b)
    throw std::runtime_error("inflateGzip: not a gzip stream (bad magic)");
  if (p[2] != 0x08)
    throw std::runtime_error(
        "inflateGzip: unsupported compression method (expected DEFLATE/0x08)");

  const unsigned char flg = p[3];
  const bool fhcrc = flg & 0x02;
  const bool fextra = flg & 0x04;
  const bool fname = flg & 0x08;
  const bool fcomment = flg & 0x10;
  if (flg & 0xE0)
    throw std::runtime_error("inflateGzip: reserved gzip flag bits set");

  std::size_t pos = 10;
  auto need = [&](std::size_t bytes) {
    if (pos + bytes > n)
      throw std::runtime_error("inflateGzip: truncated gzip header");
  };

  if (fextra) {
    need(2);
    std::size_t xlen = std::size_t(p[pos]) | (std::size_t(p[pos + 1]) << 8);
    pos += 2;
    need(xlen);
    pos += xlen;
  }
  if (fname) {
    while (true) {
      need(1);
      if (p[pos++] == 0)
        break;
    }
  }
  if (fcomment) {
    while (true) {
      need(1);
      if (p[pos++] == 0)
        break;
    }
  }
  if (fhcrc) {
    need(2);
    pos += 2; // 16-bit header CRC; not verified.
  }

  // The DEFLATE body runs from here to its final block; tinfl reports how many
  // input bytes it used, and the 8-byte footer (CRC32 + ISIZE) follows.
  if (pos + 8 > n)
    throw std::runtime_error("inflateGzip: truncated gzip body/footer");
  const std::size_t deflate_begin = pos;
  const std::size_t avail = n - deflate_begin;

  // Single-shot, non-wrapping output: grow the buffer and re-run on overflow.
  // Every member shares the one maxOut ceiling, so the room left for this
  // member is what earlier members did not use.
  const std::size_t start = out.size();
  const std::size_t room = maxOut - start;
  tinfl_decompressor decomp;
  // Initial guess from the input size (the footer's ISIZE is only reachable
  // once the body has been decoded), bounded so a large input does not
  // reserve a large zero-filled buffer up front.
  std::size_t cap = avail * 4 + 64;
  if (cap > (std::size_t{16} << 20))
    cap = std::size_t{16} << 20;
  if (cap > room)
    cap = room;

  std::size_t used = 0;
  for (;;) {
    tinfl_init(&decomp);
    out.resize(start + cap);
    std::size_t in_size = avail;
    std::size_t out_size = cap;
    auto *dst = reinterpret_cast<unsigned char *>(out.data()) + start;
    tinfl_status st = tinfl_decompress(
        &decomp, p + deflate_begin, &in_size, dst, dst, &out_size,
        TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (st == TINFL_STATUS_DONE) {
      out.resize(start + out_size);
      used = in_size;
      break;
    }
    if (st == TINFL_STATUS_HAS_MORE_OUTPUT) {
      // the stream wants more than `cap` bytes. If we are already at the
      // ceiling the decompressed output exceeds the policy limit — reject it as
      // a decompression bomb rather than doubling the buffer toward OOM.
      if (cap >= room)
        throw ParseResourceLimitError(
            "inflateGzip: decompressed size exceeds the kMaxDecompressedBytes "
            "cap (possible gzip decompression bomb)");
      cap = cap > room / 2 ? room : cap * 2; // grow, clamped to the ceiling
      continue;
    }
    throw std::runtime_error(
        "inflateGzip: DEFLATE stream is corrupt or truncated");
  }

  const std::size_t footer_at = deflate_begin + used;
  if (footer_at + 8 > n)
    throw std::runtime_error("inflateGzip: truncated gzip footer");
  const auto *footer = p + footer_at;
  const std::uint32_t expected_crc =
      std::uint32_t(footer[0]) | (std::uint32_t(footer[1]) << 8) |
      (std::uint32_t(footer[2]) << 16) | (std::uint32_t(footer[3]) << 24);
  const std::uint32_t expected_isize =
      std::uint32_t(footer[4]) | (std::uint32_t(footer[5]) << 8) |
      (std::uint32_t(footer[6]) << 16) | (std::uint32_t(footer[7]) << 24);

  const std::size_t produced = out.size() - start;
  const std::uint32_t actual_crc = detail::crc32(
      reinterpret_cast<const unsigned char *>(out.data()) + start, produced);
  if (actual_crc != expected_crc)
    throw std::runtime_error("inflateGzip: CRC32 mismatch (corrupt stream)");
  if (static_cast<std::uint32_t>(produced) != expected_isize)
    throw std::runtime_error("inflateGzip: ISIZE mismatch (corrupt stream)");
  return footer_at + 8;
}

/// Inflate a gzip (RFC 1952) stream to its decompressed bytes. `compressed`
/// must begin with the gzip magic. A stream of several members (RFC 1952
/// §2.2, e.g. `cat a.gz b.gz`) decodes to their concatenation; trailing zero
/// padding after the last member is ignored, any other trailing bytes are an
/// error. Throws std::runtime_error on a malformed header, truncated body,
/// DEFLATE failure, CRC32/ISIZE mismatch, or trailing garbage, and
/// ParseResourceLimitError once the total output would pass `maxOut`.
inline std::string inflateGzip(std::string_view compressed,
                               std::size_t maxOut = kMaxDecompressedBytes) {
  std::string out;
  std::size_t pos = inflateGzipMember(compressed, out, maxOut);
  while (pos < compressed.size()) {
    std::string_view rest = compressed.substr(pos);
    if (rest.find_first_not_of('\0') == std::string_view::npos)
      break; // zero padding (e.g. a tape block), as gzip(1) accepts
    if (rest.size() < 2 || static_cast<unsigned char>(rest[0]) != 0x1f ||
        static_cast<unsigned char>(rest[1]) != 0x8b)
      throw std::runtime_error(
          "inflateGzip: trailing bytes after the gzip stream");
    pos += inflateGzipMember(rest, out, maxOut);
  }
  return out;
}

} // namespace x3d::codec

#endif // X3D_INFLATE_HPP
