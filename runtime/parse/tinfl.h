/* tinfl.h -- bundled single-file public-domain DEFLATE (RFC 1951) decompressor.
 *
 * PROVENANCE
 *   Extracted from miniz (the "tinfl" low-level inflate API) by Rich Geldreich
 *   <richgel99@gmail.com>, based on Sean Barrett's puff/stb work. miniz is
 *   public domain (Unlicense). Upstream: https://github.com/richgel999/miniz
 *   This file is the inflate-only subset of miniz.c 2.x (tinfl_decompress and
 *   its supporting tables/macros), carved out so x3d-cpp-gen can inflate
 *   gzip/zlib/raw-DEFLATE streams with zero external dependencies and no
 *   compression code. Behavior of tinfl_decompress() is unchanged from upstream.
 *
 * LICENSE (Unlicense / public domain)
 *   This is free and unencumbered software released into the public domain.
 *   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 *   this software, either in source code form or as a compiled binary, for any
 *   purpose, commercial or non-commercial, and by any means.
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. See the full
 *   Unlicense text at https://unlicense.org/.
 *
 * Implements RFC 1951 (DEFLATE). The gzip (RFC 1952) / zlib (RFC 1950) framing
 * is handled by the C++ wrapper in Inflate.hpp; here we expose the raw inflate
 * coroutine plus the TINFL_FLAG_PARSE_ZLIB_HEADER flag for zlib-wrapped input.
 */
#ifndef X3D_TINFL_H
#define X3D_TINFL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char tinfl_uint8;
typedef int16_t tinfl_int16;
typedef uint16_t tinfl_uint16;
typedef uint32_t tinfl_uint32;
typedef uint64_t tinfl_uint64;

/* Decompression flags used by tinfl_decompress(). */
enum {
  TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
  TINFL_FLAG_HAS_MORE_INPUT = 2,
  TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
  TINFL_FLAG_COMPUTE_ADLER32 = 8
};

struct tinfl_decompressor_tag;
typedef struct tinfl_decompressor_tag tinfl_decompressor;

/* Max size of LZ dictionary. */
#define TINFL_LZ_DICT_SIZE 32768

/* Return status. */
typedef enum {
  TINFL_STATUS_BAD_PARAM = -3,
  TINFL_STATUS_ADLER32_MISMATCH = -2,
  TINFL_STATUS_FAILED = -1,
  TINFL_STATUS_DONE = 0,
  TINFL_STATUS_NEEDS_MORE_INPUT = 1,
  TINFL_STATUS_HAS_MORE_OUTPUT = 2
} tinfl_status;

/* Initialize the decompressor to its initial state. */
#define tinfl_init(r)                                                          \
  do {                                                                         \
    (r)->m_state = 0;                                                          \
  } while (0)
#define tinfl_get_adler32(r) (r)->m_check_adler32

/* Main low-level decompressor coroutine function. */
static tinfl_status tinfl_decompress(tinfl_decompressor *r,
                                     const tinfl_uint8 *pIn_buf_next,
                                     size_t *pIn_buf_size,
                                     tinfl_uint8 *pOut_buf_start,
                                     tinfl_uint8 *pOut_buf_next,
                                     size_t *pOut_buf_size,
                                     const tinfl_uint32 decomp_flags);

/* Internal/private bits follow. */
#define TINFL_MAX_HUFF_TABLES 3
#define TINFL_MAX_HUFF_SYMBOLS_0 288
#define TINFL_MAX_HUFF_SYMBOLS_1 32
#define TINFL_MAX_HUFF_SYMBOLS_2 19
#define TINFL_FAST_LOOKUP_BITS 10
#define TINFL_FAST_LOOKUP_SIZE (1 << TINFL_FAST_LOOKUP_BITS)

// Must be a preprocessor-evaluable constant: this feeds `#if
// TINFL_HAS_64BIT_REGISTERS` below, where `sizeof` is illegal (real MSVC cl.exe
// rejects it with C1017). UINTPTR_MAX (from <stdint.h>, included above) is
// preprocessor-safe on GCC/Clang AND MSVC. clang-cl defines __clang__ so it
// already took this branch — only genuine cl.exe hit the sizeof fallback.
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
#define TINFL_HAS_64BIT_REGISTERS (UINTPTR_MAX > 0xFFFFFFFFu)
#else
#define TINFL_HAS_64BIT_REGISTERS (sizeof(size_t) > 4)
#endif

typedef struct {
  tinfl_uint8 m_code_size[TINFL_MAX_HUFF_SYMBOLS_0];
  tinfl_int16 m_look_up[TINFL_FAST_LOOKUP_SIZE];
  tinfl_int16 m_tree[TINFL_MAX_HUFF_SYMBOLS_0 * 2];
} tinfl_huff_table;

#if TINFL_HAS_64BIT_REGISTERS
typedef tinfl_uint64 tinfl_bit_buf_t;
#define TINFL_BITBUF_SIZE (64)
#else
typedef tinfl_uint32 tinfl_bit_buf_t;
#define TINFL_BITBUF_SIZE (32)
#endif

struct tinfl_decompressor_tag {
  tinfl_uint32 m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final,
      m_type, m_check_adler32, m_dist, m_counter, m_num_extra,
      m_table_sizes[TINFL_MAX_HUFF_TABLES];
  tinfl_bit_buf_t m_bit_buf;
  size_t m_dist_from_out_buf_start;
  tinfl_huff_table m_tables[TINFL_MAX_HUFF_TABLES];
  tinfl_uint8 m_raw_header[4],
      m_len_codes[TINFL_MAX_HUFF_SYMBOLS_0 + TINFL_MAX_HUFF_SYMBOLS_1 + 137];
};

/* --------------------------------------------------------------------------
 * Implementation (tinfl_decompress).
 * ------------------------------------------------------------------------ */

#define TINFL_CR_BEGIN                                                          \
  switch (r->m_state) {                                                         \
  case 0:
#define TINFL_CR_RETURN(state_index, result)                                    \
  do {                                                                          \
    status = result;                                                           \
    r->m_state = state_index;                                                  \
    goto common_exit;                                                          \
  case state_index:;                                                           \
  } while (0)
#define TINFL_CR_RETURN_FOREVER(state_index, result)                           \
  do {                                                                         \
    for (;;) {                                                                 \
      TINFL_CR_RETURN(state_index, result);                                    \
    }                                                                          \
  } while (0)
#define TINFL_CR_FINISH }

#define TINFL_GET_BYTE(state_index, c)                                          \
  do {                                                                         \
    while (pIn_buf_cur >= pIn_buf_end) {                                       \
      TINFL_CR_RETURN(state_index,                                             \
                      (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT)               \
                          ? TINFL_STATUS_NEEDS_MORE_INPUT                      \
                          : TINFL_STATUS_FAILED);                              \
    }                                                                          \
    c = *pIn_buf_cur++;                                                        \
  } while (0)

#define TINFL_NEED_BITS(state_index, n)                                        \
  do {                                                                         \
    tinfl_uint32 c;                                                            \
    TINFL_GET_BYTE(state_index, c);                                            \
    bit_buf |= (((tinfl_bit_buf_t)c) << num_bits);                            \
    num_bits += 8;                                                             \
  } while (num_bits < (tinfl_uint32)(n))
#define TINFL_SKIP_BITS(state_index, n)                                        \
  do {                                                                         \
    if (num_bits < (tinfl_uint32)(n)) {                                       \
      TINFL_NEED_BITS(state_index, n);                                         \
    }                                                                          \
    bit_buf >>= (n);                                                          \
    num_bits -= (n);                                                          \
  } while (0)
#define TINFL_GET_BITS(state_index, b, n)                                      \
  do {                                                                         \
    if (num_bits < (tinfl_uint32)(n)) {                                       \
      TINFL_NEED_BITS(state_index, n);                                         \
    }                                                                          \
    b = bit_buf & ((1 << (n)) - 1);                                           \
    bit_buf >>= (n);                                                          \
    num_bits -= (n);                                                          \
  } while (0)

/* TINFL_HUFF_BITBUF_FILL is used internally by TINFL_HUFF_DECODE. It tries to
   keep the bit buffer full by reading bytes from the input. */
#define TINFL_HUFF_BITBUF_FILL(state_index, pHuff)                             \
  do {                                                                         \
    temp = (pHuff)->m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)];        \
    if (temp >= 0) {                                                          \
      code_len = temp >> 9;                                                   \
      if ((code_len) && (num_bits >= code_len))                              \
        break;                                                                \
    } else if (num_bits > TINFL_FAST_LOOKUP_BITS) {                          \
      code_len = TINFL_FAST_LOOKUP_BITS;                                      \
      do {                                                                    \
        temp = (pHuff)                                                        \
                   ->m_tree[~temp + ((bit_buf >> code_len++) & 1)];          \
      } while ((temp < 0) && (num_bits >= (code_len + 1)));                   \
      if (temp >= 0)                                                          \
        break;                                                                \
    }                                                                         \
    TINFL_GET_BYTE(state_index, c);                                           \
    bit_buf |= (((tinfl_bit_buf_t)c) << num_bits);                           \
    num_bits += 8;                                                            \
  } while (num_bits < 15)

#define TINFL_HUFF_DECODE(state_index, sym, pHuff)                             \
  do {                                                                         \
    int temp;                                                                  \
    tinfl_uint32 code_len, c;                                                  \
    if (num_bits < 15) {                                                      \
      if ((pIn_buf_end - pIn_buf_cur) < 2) {                                  \
        TINFL_HUFF_BITBUF_FILL(state_index, pHuff);                           \
      } else {                                                                \
        bit_buf |= (((tinfl_bit_buf_t)pIn_buf_cur[0]) << num_bits) |          \
                   (((tinfl_bit_buf_t)pIn_buf_cur[1]) << (num_bits + 8));     \
        pIn_buf_cur += 2;                                                     \
        num_bits += 16;                                                       \
      }                                                                       \
    }                                                                         \
    if ((temp = (pHuff)->m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= \
        0)                                                                    \
      code_len = temp >> 9, temp &= 511;                                     \
    else {                                                                    \
      code_len = TINFL_FAST_LOOKUP_BITS;                                      \
      do {                                                                    \
        temp = (pHuff)->m_tree[~temp + ((bit_buf >> code_len++) & 1)];        \
      } while (temp < 0);                                                     \
    }                                                                         \
    sym = temp;                                                               \
    bit_buf >>= code_len;                                                     \
    num_bits -= code_len;                                                     \
  } while (0)

static tinfl_status tinfl_decompress(tinfl_decompressor *r,
                                     const tinfl_uint8 *pIn_buf_next,
                                     size_t *pIn_buf_size,
                                     tinfl_uint8 *pOut_buf_start,
                                     tinfl_uint8 *pOut_buf_next,
                                     size_t *pOut_buf_size,
                                     const tinfl_uint32 decomp_flags) {
  static const int s_length_base[31] = {
      3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
      35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};
  static const int s_length_extra[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
                                         1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
                                         4, 4, 5, 5, 5, 5, 0, 0, 0};
  static const int s_dist_base[32] = {
      1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
      49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
      2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};
  static const int s_dist_extra[32] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                       4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                       9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
  static const tinfl_uint8 s_length_dezigzag[19] = {
      16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  static const int s_min_table_sizes[3] = {257, 1, 4};
#ifndef MZ_MAX
#define MZ_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

  tinfl_status status = TINFL_STATUS_FAILED;
  tinfl_uint32 num_bits, dist, counter, num_extra;
  tinfl_bit_buf_t bit_buf;
  const tinfl_uint8 *pIn_buf_cur = pIn_buf_next,
                    *const pIn_buf_end = pIn_buf_next + *pIn_buf_size;
  tinfl_uint8 *pOut_buf_cur = pOut_buf_next,
              *const pOut_buf_end = pOut_buf_next + *pOut_buf_size;
  size_t out_buf_size_mask =
             (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)
                 ? (size_t)-1
                 : ((pOut_buf_next - pOut_buf_start) + *pOut_buf_size) - 1,
         dist_from_out_buf_start;

  /* Ensure the output buffer's size is a power of 2, unless the output buffer
     is large enough to hold the entire output file (in which case it doesn't
     matter). */
  if (((out_buf_size_mask + 1) & out_buf_size_mask) ||
      (pOut_buf_next < pOut_buf_start)) {
    *pIn_buf_size = *pOut_buf_size = 0;
    return TINFL_STATUS_BAD_PARAM;
  }

  num_bits = r->m_num_bits;
  bit_buf = r->m_bit_buf;
  dist = r->m_dist;
  counter = r->m_counter;
  num_extra = r->m_num_extra;
  dist_from_out_buf_start = r->m_dist_from_out_buf_start;
  TINFL_CR_BEGIN

  bit_buf = num_bits = dist = counter = num_extra = r->m_zhdr0 = r->m_zhdr1 = 0;
  r->m_z_adler32 = r->m_check_adler32 = 1;
  if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER) {
    TINFL_GET_BYTE(1, r->m_zhdr0);
    TINFL_GET_BYTE(2, r->m_zhdr1);
    counter = (((r->m_zhdr0 * 256 + r->m_zhdr1) % 31 != 0) ||
               (r->m_zhdr1 & 32) || ((r->m_zhdr0 & 15) != 8));
    if (!(decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF))
      counter |=
          (((1U << (8U + (r->m_zhdr0 >> 4))) > 32768U) ||
           ((out_buf_size_mask + 1) < (size_t)(1U << (8U + (r->m_zhdr0 >> 4)))));
    if (counter) {
      TINFL_CR_RETURN_FOREVER(36, TINFL_STATUS_FAILED);
    }
  }

  do {
    TINFL_GET_BITS(3, r->m_final, 3);
    r->m_type = r->m_final >> 1;
    if (r->m_type == 0) {
      TINFL_SKIP_BITS(5, num_bits & 7);
      for (counter = 0; counter < 4; ++counter) {
        if (num_bits)
          TINFL_GET_BITS(6, r->m_raw_header[counter], 8);
        else
          TINFL_GET_BYTE(7, r->m_raw_header[counter]);
      }
      if ((counter = (r->m_raw_header[0] | (r->m_raw_header[1] << 8))) !=
          (tinfl_uint32)(0xFFFF ^
                         (r->m_raw_header[2] | (r->m_raw_header[3] << 8)))) {
        TINFL_CR_RETURN_FOREVER(39, TINFL_STATUS_FAILED);
      }
      while ((counter) && (num_bits)) {
        TINFL_GET_BITS(51, dist, 8);
        while (pOut_buf_cur >= pOut_buf_end) {
          TINFL_CR_RETURN(52, TINFL_STATUS_HAS_MORE_OUTPUT);
        }
        *pOut_buf_cur++ = (tinfl_uint8)dist;
        counter--;
      }
      while (counter) {
        size_t n;
        while (pOut_buf_cur >= pOut_buf_end) {
          TINFL_CR_RETURN(9, TINFL_STATUS_HAS_MORE_OUTPUT);
        }
        while (pIn_buf_cur >= pIn_buf_end) {
          TINFL_CR_RETURN(38, (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT)
                                  ? TINFL_STATUS_NEEDS_MORE_INPUT
                                  : TINFL_STATUS_FAILED);
        }
        n = ((size_t)(pOut_buf_end - pOut_buf_cur) <
             (size_t)(pIn_buf_end - pIn_buf_cur))
                ? (size_t)(pOut_buf_end - pOut_buf_cur)
                : (size_t)(pIn_buf_end - pIn_buf_cur);
        if (n > counter)
          n = counter;
        memcpy(pOut_buf_cur, pIn_buf_cur, n);
        pIn_buf_cur += n;
        pOut_buf_cur += n;
        counter -= (tinfl_uint32)n;
      }
    } else if (r->m_type == 3) {
      TINFL_CR_RETURN_FOREVER(10, TINFL_STATUS_FAILED);
    } else {
      if (r->m_type == 1) {
        tinfl_uint8 *p = r->m_tables[0].m_code_size;
        tinfl_uint32 i;
        r->m_table_sizes[0] = 288;
        r->m_table_sizes[1] = 32;
        memset(r->m_tables[1].m_code_size, 5, 32);
        for (i = 0; i <= 143; ++i)
          *p++ = 8;
        for (; i <= 255; ++i)
          *p++ = 9;
        for (; i <= 279; ++i)
          *p++ = 7;
        for (; i <= 287; ++i)
          *p++ = 8;
      } else {
        for (counter = 0; counter < 3; counter++) {
          TINFL_GET_BITS(11, r->m_table_sizes[counter], "\05\05\04"[counter]);
          r->m_table_sizes[counter] += s_min_table_sizes[counter];
        }
        memset(r->m_tables[2].m_code_size, 0, sizeof(r->m_tables[2].m_code_size));
        for (counter = 0; counter < r->m_table_sizes[2]; counter++) {
          tinfl_uint32 s;
          TINFL_GET_BITS(14, s, 3);
          r->m_tables[2].m_code_size[s_length_dezigzag[counter]] =
              (tinfl_uint8)s;
        }
        r->m_table_sizes[2] = 19;
      }
      for (; (int)r->m_type >= 0; r->m_type--) {
        int tree_next, tree_cur;
        tinfl_huff_table *pTable;
        tinfl_uint32 i, j, used_syms, total, sym_index, next_code[17],
            total_syms[16];
        pTable = &r->m_tables[r->m_type];
        memset(total_syms, 0, sizeof(total_syms));
        memset(pTable->m_look_up, 0, sizeof(pTable->m_look_up));
        memset(pTable->m_tree, 0, sizeof(pTable->m_tree));
        for (i = 0; i < r->m_table_sizes[r->m_type]; ++i)
          total_syms[pTable->m_code_size[i]]++;
        used_syms = 0, total = 0;
        next_code[0] = next_code[1] = 0;
        for (i = 1; i <= 15; ++i) {
          used_syms += total_syms[i];
          next_code[i + 1] = (total = ((total + total_syms[i]) << 1));
        }
        if ((65536 != total) && (used_syms > 1)) {
          TINFL_CR_RETURN_FOREVER(35, TINFL_STATUS_FAILED);
        }
        for (tree_next = -1, sym_index = 0;
             sym_index < r->m_table_sizes[r->m_type]; ++sym_index) {
          tinfl_uint32 rev_code = 0, l, cur_code,
                       code_size = pTable->m_code_size[sym_index];
          if (!code_size)
            continue;
          cur_code = next_code[code_size]++;
          for (l = code_size; l > 0; l--, cur_code >>= 1)
            rev_code = (rev_code << 1) | (cur_code & 1);
          if (code_size <= TINFL_FAST_LOOKUP_BITS) {
            tinfl_int16 k =
                (tinfl_int16)((code_size << 9) | sym_index);
            while (rev_code < TINFL_FAST_LOOKUP_SIZE) {
              pTable->m_look_up[rev_code] = k;
              rev_code += (1 << code_size);
            }
            continue;
          }
          if (0 ==
              (tree_cur = pTable->m_look_up[rev_code &
                                            (TINFL_FAST_LOOKUP_SIZE - 1)])) {
            pTable->m_look_up[rev_code & (TINFL_FAST_LOOKUP_SIZE - 1)] =
                (tinfl_int16)tree_next;
            tree_cur = tree_next;
            tree_next -= 2;
          }
          rev_code >>= (TINFL_FAST_LOOKUP_BITS - 1);
          for (j = code_size; j > (TINFL_FAST_LOOKUP_BITS + 1); j--) {
            tree_cur -= ((rev_code >>= 1) & 1);
            if (!pTable->m_tree[-tree_cur - 1]) {
              pTable->m_tree[-tree_cur - 1] = (tinfl_int16)tree_next;
              tree_cur = tree_next;
              tree_next -= 2;
            } else
              tree_cur = pTable->m_tree[-tree_cur - 1];
          }
          tree_cur -= ((rev_code >>= 1) & 1);
          pTable->m_tree[-tree_cur - 1] = (tinfl_int16)sym_index;
        }
        if (r->m_type == 2) {
          for (counter = 0;
               counter < (r->m_table_sizes[0] + r->m_table_sizes[1]);) {
            tinfl_uint32 s;
            TINFL_HUFF_DECODE(16, dist, &r->m_tables[2]);
            if (dist < 16) {
              r->m_len_codes[counter++] = (tinfl_uint8)dist;
              continue;
            }
            if ((dist == 16) && (!counter)) {
              TINFL_CR_RETURN_FOREVER(17, TINFL_STATUS_FAILED);
            }
            num_extra = "\02\03\07"[dist - 16];
            TINFL_GET_BITS(18, s, num_extra);
            s += "\03\03\013"[dist - 16];
            memset(&r->m_len_codes[counter],
                   (dist == 16) ? r->m_len_codes[counter - 1] : 0, s);
            counter += s;
          }
          if ((r->m_table_sizes[0] + r->m_table_sizes[1]) != counter) {
            TINFL_CR_RETURN_FOREVER(21, TINFL_STATUS_FAILED);
          }
          memcpy(r->m_tables[0].m_code_size, r->m_len_codes,
                 r->m_table_sizes[0]);
          memcpy(r->m_tables[1].m_code_size,
                 r->m_len_codes + r->m_table_sizes[0], r->m_table_sizes[1]);
        }
      }
      for (;;) {
        tinfl_uint8 *pSrc;
        for (;;) {
          if (((pIn_buf_end - pIn_buf_cur) < 4) ||
              ((pOut_buf_end - pOut_buf_cur) < 2)) {
            TINFL_HUFF_DECODE(23, counter, &r->m_tables[0]);
            if (counter >= 256)
              break;
            while (pOut_buf_cur >= pOut_buf_end) {
              TINFL_CR_RETURN(24, TINFL_STATUS_HAS_MORE_OUTPUT);
            }
            *pOut_buf_cur++ = (tinfl_uint8)counter;
          } else {
            int sym2;
            tinfl_uint32 code_len;
#if TINFL_HAS_64BIT_REGISTERS
            if (num_bits < 30) {
              bit_buf |=
                  (((tinfl_bit_buf_t)((tinfl_uint32)pIn_buf_cur[0] |
                                      ((tinfl_uint32)pIn_buf_cur[1] << 8) |
                                      ((tinfl_uint32)pIn_buf_cur[2] << 16) |
                                      ((tinfl_uint32)pIn_buf_cur[3] << 24)))
                   << num_bits);
              pIn_buf_cur += 4;
              num_bits += 32;
            }
#else
            if (num_bits < 15) {
              bit_buf |=
                  (((tinfl_bit_buf_t)((tinfl_uint32)pIn_buf_cur[0] |
                                      ((tinfl_uint32)pIn_buf_cur[1] << 8)))
                   << num_bits);
              pIn_buf_cur += 2;
              num_bits += 16;
            }
#endif
            if ((sym2 =
                     r->m_tables[0]
                         .m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >=
                0)
              code_len = sym2 >> 9;
            else {
              code_len = TINFL_FAST_LOOKUP_BITS;
              do {
                sym2 = r->m_tables[0]
                           .m_tree[~sym2 + ((bit_buf >> code_len++) & 1)];
              } while (sym2 < 0);
            }
            counter = sym2;
            bit_buf >>= code_len;
            num_bits -= code_len;
            if (counter & 256)
              break;

#if !TINFL_HAS_64BIT_REGISTERS
            if (num_bits < 15) {
              bit_buf |=
                  (((tinfl_bit_buf_t)((tinfl_uint32)pIn_buf_cur[0] |
                                      ((tinfl_uint32)pIn_buf_cur[1] << 8)))
                   << num_bits);
              pIn_buf_cur += 2;
              num_bits += 16;
            }
#endif
            if ((sym2 =
                     r->m_tables[0]
                         .m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >=
                0)
              code_len = sym2 >> 9;
            else {
              code_len = TINFL_FAST_LOOKUP_BITS;
              do {
                sym2 = r->m_tables[0]
                           .m_tree[~sym2 + ((bit_buf >> code_len++) & 1)];
              } while (sym2 < 0);
            }
            bit_buf >>= code_len;
            num_bits -= code_len;

            pOut_buf_cur[0] = (tinfl_uint8)counter;
            if (sym2 & 256) {
              pOut_buf_cur++;
              counter = sym2;
              break;
            }
            pOut_buf_cur[1] = (tinfl_uint8)sym2;
            pOut_buf_cur += 2;
          }
        }
        if ((counter &= 511) == 256)
          break;

        num_extra = s_length_extra[counter - 257];
        counter = s_length_base[counter - 257];
        if (num_extra) {
          tinfl_uint32 extra_bits;
          TINFL_GET_BITS(25, extra_bits, num_extra);
          counter += extra_bits;
        }

        TINFL_HUFF_DECODE(26, dist, &r->m_tables[1]);
        num_extra = s_dist_extra[dist];
        dist = s_dist_base[dist];
        if (num_extra) {
          tinfl_uint32 extra_bits;
          TINFL_GET_BITS(27, extra_bits, num_extra);
          dist += extra_bits;
        }

        dist_from_out_buf_start = pOut_buf_cur - pOut_buf_start;
        if ((dist > dist_from_out_buf_start) &&
            (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)) {
          TINFL_CR_RETURN_FOREVER(37, TINFL_STATUS_FAILED);
        }

        pSrc = pOut_buf_start +
               ((dist_from_out_buf_start - dist) & out_buf_size_mask);

        if ((MZ_MAX(pOut_buf_cur, pSrc) + counter) > pOut_buf_end) {
          while (counter--) {
            while (pOut_buf_cur >= pOut_buf_end) {
              TINFL_CR_RETURN(53, TINFL_STATUS_HAS_MORE_OUTPUT);
            }
            *pOut_buf_cur++ =
                pOut_buf_start[(dist_from_out_buf_start++ - dist) &
                               out_buf_size_mask];
          }
          continue;
        }
#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES
        else if ((counter >= 9) && (counter <= dist)) {
          const tinfl_uint8 *pSrc_end = pSrc + (counter & ~7);
          do {
            memcpy(pOut_buf_cur, pSrc, 8);
            pOut_buf_cur += 8;
            pSrc += 8;
          } while ((pSrc += 0, pSrc) < pSrc_end);
          if ((counter &= 7) < 3) {
            if (counter) {
              pOut_buf_cur[0] = pSrc[0];
              if (counter > 1)
                pOut_buf_cur[1] = pSrc[1];
              pOut_buf_cur += counter;
            }
            continue;
          }
        }
#endif
        while (counter > 2) {
          pOut_buf_cur[0] = pSrc[0];
          pOut_buf_cur[1] = pSrc[1];
          pOut_buf_cur[2] = pSrc[2];
          pOut_buf_cur += 3;
          pSrc += 3;
          counter -= 3;
        }
        if (counter > 0) {
          pOut_buf_cur[0] = pSrc[0];
          if (counter > 1)
            pOut_buf_cur[1] = pSrc[1];
          pOut_buf_cur += counter;
        }
      }
    }
  } while (!(r->m_final & 1));

  /* Ensure byte alignment and put back any bytes from the bit buffer. */
  if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER) {
    TINFL_SKIP_BITS(32, num_bits & 7);
    for (counter = 0; counter < 4; ++counter) {
      tinfl_uint32 s;
      if (num_bits)
        TINFL_GET_BITS(41, s, 8);
      else
        TINFL_GET_BYTE(42, s);
      r->m_z_adler32 = (r->m_z_adler32 << 8) | s;
    }
  }
  TINFL_CR_RETURN_FOREVER(34, TINFL_STATUS_DONE);

  TINFL_CR_FINISH

common_exit:
  /* As long as we aren't telling the caller that we NEED more input to make
     forward progress: put back any bytes we overread from the input buffer
     beyond the current end. */
  if ((status != TINFL_STATUS_NEEDS_MORE_INPUT) &&
      (status != TINFL_STATUS_FAILED)) {
    while (num_bits >= 8) {
      --pIn_buf_cur;
      num_bits -= 8;
    }
  }
  r->m_num_bits = num_bits;
  r->m_bit_buf = bit_buf & (tinfl_bit_buf_t)((((tinfl_uint64)1)
                                              << num_bits) -
                                             (tinfl_uint64)1);
  r->m_dist = dist;
  r->m_counter = counter;
  r->m_num_extra = num_extra;
  r->m_dist_from_out_buf_start = dist_from_out_buf_start;
  *pIn_buf_size = pIn_buf_cur - pIn_buf_next;
  *pOut_buf_size = pOut_buf_cur - pOut_buf_next;
  if ((decomp_flags &
       (TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_COMPUTE_ADLER32)) &&
      (status >= 0)) {
    const tinfl_uint8 *ptr = pOut_buf_next;
    size_t buf_len = *pOut_buf_size;
    tinfl_uint32 i, s1 = r->m_check_adler32 & 0xffff,
                    s2 = r->m_check_adler32 >> 16;
    size_t block_len = buf_len % 5552;
    while (buf_len) {
      for (i = 0; i + 7 < block_len; i += 8, ptr += 8) {
        s1 += ptr[0], s2 += s1;
        s1 += ptr[1], s2 += s1;
        s1 += ptr[2], s2 += s1;
        s1 += ptr[3], s2 += s1;
        s1 += ptr[4], s2 += s1;
        s1 += ptr[5], s2 += s1;
        s1 += ptr[6], s2 += s1;
        s1 += ptr[7], s2 += s1;
      }
      for (; i < block_len; ++i)
        s1 += *ptr++, s2 += s1;
      s1 %= 65521U, s2 %= 65521U;
      buf_len -= block_len;
      block_len = 5552;
    }
    r->m_check_adler32 = (s2 << 16) + s1;
    if ((status == TINFL_STATUS_DONE) &&
        (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER) &&
        (r->m_check_adler32 != r->m_z_adler32))
      status = TINFL_STATUS_ADLER32_MISMATCH;
  }
  return status;
}

#ifdef __cplusplus
}
#endif

#endif /* X3D_TINFL_H */
