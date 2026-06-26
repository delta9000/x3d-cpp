// Inflate.hpp
// Header-only gzip (RFC 1952) inflate for the parsing front-end.
//
// Wraps the bundled public-domain DEFLATE decompressor (tinfl.h, from miniz —
// Unlicense; see that file's header for full provenance/license) with a small
// C++ helper that:
//   1. Validates the gzip magic (0x1f 0x8b) and the DEFLATE method byte (0x08).
//   2. Parses the fixed 10-byte RFC-1952 header and any optional FEXTRA / FNAME
//      / FCOMMENT / FHCRC fields indicated by the flag byte.
//   3. Hands the raw DEFLATE stream to tinfl in single-shot, non-wrapping mode,
//      growing the output buffer until TINFL_STATUS_DONE.
//   4. Verifies the trailing CRC32 and ISIZE (mod 2^32) from the 8-byte footer.
//
// The point of bundling tinfl is to inflate without an -lz client dependency,
// matching the header-only / zero-dependency promise of XmlLite.hpp /
// JsonLite.hpp. Throws std::runtime_error("inflateGzip: ...") on any
// truncation / corruption / checksum mismatch.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_INFLATE_HPP
#define X3D_INFLATE_HPP

#include "tinfl.h"

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

/// Inflate a gzip (RFC 1952) stream to its decompressed bytes. `compressed`
/// must begin with the gzip magic. Throws std::runtime_error on a malformed
/// header, truncated body, DEFLATE failure, or CRC32/ISIZE mismatch.
inline std::string inflateGzip(std::string_view compressed) {
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

  // The 8-byte footer (CRC32 + ISIZE) trails the DEFLATE body.
  if (pos + 8 > n)
    throw std::runtime_error("inflateGzip: truncated gzip body/footer");
  const std::size_t deflate_begin = pos;
  const std::size_t deflate_len = n - 8 - deflate_begin;

  const auto *footer = p + (n - 8);
  const std::uint32_t expected_crc =
      std::uint32_t(footer[0]) | (std::uint32_t(footer[1]) << 8) |
      (std::uint32_t(footer[2]) << 16) | (std::uint32_t(footer[3]) << 24);
  const std::uint32_t expected_isize =
      std::uint32_t(footer[4]) | (std::uint32_t(footer[5]) << 8) |
      (std::uint32_t(footer[6]) << 16) | (std::uint32_t(footer[7]) << 24);

  // Single-shot, non-wrapping output: grow the buffer and re-run on overflow.
  // ISIZE is a hint for the initial reservation (mod 2^32; trust but verify).
  tinfl_decompressor decomp;
  std::string out;
  std::size_t cap = expected_isize ? expected_isize : (deflate_len * 4 + 64);
  if (cap < 64)
    cap = 64;

  for (;;) {
    tinfl_init(&decomp);
    out.assign(cap, '\0');
    std::size_t in_size = deflate_len;
    std::size_t out_size = cap;
    tinfl_status st = tinfl_decompress(
        &decomp, p + deflate_begin, &in_size,
        reinterpret_cast<unsigned char *>(&out[0]),
        reinterpret_cast<unsigned char *>(&out[0]), &out_size,
        TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (st == TINFL_STATUS_DONE) {
      out.resize(out_size);
      break;
    }
    if (st == TINFL_STATUS_HAS_MORE_OUTPUT) {
      cap *= 2; // output buffer too small; grow and retry from the start.
      continue;
    }
    throw std::runtime_error(
        "inflateGzip: DEFLATE stream is corrupt or truncated");
  }

  const std::uint32_t actual_crc = detail::crc32(
      reinterpret_cast<const unsigned char *>(out.data()), out.size());
  if (actual_crc != expected_crc)
    throw std::runtime_error("inflateGzip: CRC32 mismatch (corrupt stream)");
  if (static_cast<std::uint32_t>(out.size()) != expected_isize)
    throw std::runtime_error("inflateGzip: ISIZE mismatch (corrupt stream)");

  return out;
}

} // namespace x3d::codec

#endif // X3D_INFLATE_HPP
