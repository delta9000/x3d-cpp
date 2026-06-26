// ProceduralTexture.hpp — generated test textures + a TextureResolver, so
// ImageTexture scenes render WITHOUT any committed binary asset or image decoder.
//
// Two sources, both dependency-free:
//   * "proc:<name>"  — synthesized on the fly (checker / uvgrid / gradient /
//                       brick / bars). No files, no decode.
//   * "*.ppm"        — a binary PPM (P6) file next to the scene (a format we can
//                       decode in ~20 lines). Lets you drop in real images too.
// Everything else (png/jpg/http) -> Failed (the renderer falls back to flat
// color), matching the seam's documented behavior.
//
// Pixel format follows the seam contract: RGBA8, tightly packed, origin
// BOTTOM-LEFT (GL convention — matches MeshData::texcoords, no V-flip).
//
// The "uvgrid" texture is a deliberate orientation probe: red = U, green = V, so
// bottom-left reads dark, bottom-right red, top-left green, top-right yellow —
// a no-V-flip check you can read straight off the rendered image.
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_PROCEDURAL_TEXTURE_HPP
#define X3D_CPURASTER_PROCEDURAL_TEXTURE_HPP

#include "StbTextureResolver.hpp" // x3d_stb decode seam (PNG/JPEG via stb_image)
#include "TextureResolver.hpp" // ex::TexturePixels / TexturePixelResult / TextureResolver

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace x3d::cpuraster {

namespace ex = x3d::runtime::extract;

namespace texgen {

// Bottom-left-origin RGBA writer: row 0 == bottom.
inline void put(std::vector<std::uint8_t> &px, int W, int x, int y,
                std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
  const std::size_t i = (static_cast<std::size_t>(y) * W + x) * 4;
  px[i] = r; px[i + 1] = g; px[i + 2] = b; px[i + 3] = a;
}

inline ex::TexturePixels rgbaSurface(int W, int H, std::vector<std::uint8_t> px) {
  ex::TexturePixels t;
  t.width = static_cast<std::uint32_t>(W);
  t.height = static_cast<std::uint32_t>(H);
  t.rgba = std::move(px);
  return t;
}

} // namespace texgen

// ---- Generators -----------------------------------------------------------

// Checkerboard of `cells` x `cells` squares in two colors.
inline ex::TexturePixels makeChecker(int size = 256, int cells = 8) {
  std::vector<std::uint8_t> px(static_cast<std::size_t>(size) * size * 4);
  const int cell = size / cells;
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x) {
      bool on = ((x / cell) + (y / cell)) & 1;
      if (on) texgen::put(px, size, x, y, 235, 235, 235);
      else    texgen::put(px, size, x, y, 35, 35, 60);
    }
  return texgen::rgbaSurface(size, size, std::move(px));
}

// UV orientation probe: R = u (→ right), G = v (→ up), with white grid lines.
inline ex::TexturePixels makeUvGrid(int size = 256, int cells = 8) {
  std::vector<std::uint8_t> px(static_cast<std::size_t>(size) * size * 4);
  const int cell = size / cells;
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x) {
      std::uint8_t r = static_cast<std::uint8_t>(255.0f * (x + 0.5f) / size);
      std::uint8_t g = static_cast<std::uint8_t>(255.0f * (y + 0.5f) / size);
      bool grid = (x % cell == 0) || (y % cell == 0);
      if (grid) texgen::put(px, size, x, y, 255, 255, 255);
      else      texgen::put(px, size, x, y, r, g, 64);
    }
  return texgen::rgbaSurface(size, size, std::move(px));
}

// Smooth gradient: R = u, G = v, B = 128.
inline ex::TexturePixels makeGradient(int size = 256) {
  std::vector<std::uint8_t> px(static_cast<std::size_t>(size) * size * 4);
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x)
      texgen::put(px, size, x, y,
                  static_cast<std::uint8_t>(255.0f * x / (size - 1)),
                  static_cast<std::uint8_t>(255.0f * y / (size - 1)), 128);
  return texgen::rgbaSurface(size, size, std::move(px));
}

// Running-bond brick pattern (terracotta brick, grey mortar).
inline ex::TexturePixels makeBrick(int size = 256) {
  std::vector<std::uint8_t> px(static_cast<std::size_t>(size) * size * 4);
  const int brickH = size / 8, brickW = size / 4, mortar = size / 64 + 1;
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x) {
      int row = y / brickH;
      int xoff = (row & 1) ? brickW / 2 : 0;
      int bx = (x + xoff) % brickW;
      int by = y % brickH;
      bool isMortar = (by < mortar) || (bx < mortar);
      if (isMortar) texgen::put(px, size, x, y, 200, 200, 195);
      else          texgen::put(px, size, x, y, 170, 70, 50);
    }
  return texgen::rgbaSurface(size, size, std::move(px));
}

// Vertical color bars (white/yellow/cyan/green/magenta/red/blue/black).
inline ex::TexturePixels makeColorBars(int size = 256) {
  static const std::uint8_t bars[8][3] = {
      {235, 235, 235}, {235, 235, 35}, {35, 235, 235}, {35, 235, 35},
      {235, 35, 235}, {235, 35, 35},  {35, 35, 235},   {20, 20, 20}};
  std::vector<std::uint8_t> px(static_cast<std::size_t>(size) * size * 4);
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x) {
      const std::uint8_t *c = bars[(x * 8) / size];
      texgen::put(px, size, x, y, c[0], c[1], c[2]);
    }
  return texgen::rgbaSurface(size, size, std::move(px));
}

// Dispatch a "proc:<name>" url to a generator. Unknown name -> checker.
inline ex::TexturePixels makeNamed(const std::string &name) {
  if (name == "uvgrid")   return makeUvGrid();
  if (name == "gradient") return makeGradient();
  if (name == "brick")    return makeBrick();
  if (name == "bars")     return makeColorBars();
  return makeChecker();
}

// ---- PPM (P6) IO ----------------------------------------------------------

// Decode a binary PPM (P6) into a bottom-left-origin RGBA surface (alpha=255).
// PPM is top-down, so rows are flipped. Returns empty on malformed input.
inline ex::TexturePixels readPpm(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::string magic;
  in >> magic;
  if (magic != "P6") return {};
  auto skipws = [&]() {
    int c;
    while ((c = in.peek()) != EOF) {
      if (c == '#') { std::string l; std::getline(in, l); }
      else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') in.get();
      else break;
    }
  };
  int w = 0, h = 0, maxv = 0;
  skipws(); in >> w; skipws(); in >> h; skipws(); in >> maxv;
  in.get(); // single whitespace after maxval.
  if (w <= 0 || h <= 0 || maxv != 255) return {};
  std::vector<std::uint8_t> rgb(static_cast<std::size_t>(w) * h * 3);
  in.read(reinterpret_cast<char *>(rgb.data()), rgb.size());
  if (!in) return {};
  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * h * 4, 255);
  for (int y = 0; y < h; ++y) {
    const int srcRow = h - 1 - y; // top-down PPM -> bottom-up surface.
    for (int x = 0; x < w; ++x) {
      const std::size_t s = (static_cast<std::size_t>(srcRow) * w + x) * 3;
      const std::size_t d = (static_cast<std::size_t>(y) * w + x) * 4;
      rgba[d] = rgb[s]; rgba[d + 1] = rgb[s + 1]; rgba[d + 2] = rgb[s + 2];
    }
  }
  ex::TexturePixels t;
  t.width = static_cast<std::uint32_t>(w);
  t.height = static_cast<std::uint32_t>(h);
  t.rgba = std::move(rgba);
  return t;
}

// Write a bottom-left-origin RGBA surface to a binary PPM (P6, top-down).
inline bool writePpm(const std::string &path, const ex::TexturePixels &t) {
  if (t.width == 0 || t.height == 0) return false;
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << "P6\n" << t.width << " " << t.height << "\n255\n";
  for (int y = static_cast<int>(t.height) - 1; y >= 0; --y) // flip to top-down.
    for (std::uint32_t x = 0; x < t.width; ++x) {
      const std::size_t i = (static_cast<std::size_t>(y) * t.width + x) * 4;
      out.put(static_cast<char>(t.rgba[i]));
      out.put(static_cast<char>(t.rgba[i + 1]));
      out.put(static_cast<char>(t.rgba[i + 2]));
    }
  return static_cast<bool>(out);
}

// ---- The resolver ---------------------------------------------------------

// A TextureResolver bound to a scene dir: "proc:<name>" -> generated; "*.ppm" ->
// decoded file; anything else -> Failed (flat-color fallback).
inline ex::TextureResolver makeTextureResolver(const std::string &sceneDir) {
  // PNG/JPEG decode is delegated to the SDK's TextureResolver decode-seam
  // backend (x3d_stb, ADR-0024) — same vendored stb_image, bottom-left origin,
  // no parallel decoder. proc:/.ppm stay built-in (synthetic + dependency-free).
  ex::TextureResolver stb = x3d::runtime::io::stb::makeStbTextureResolver();
  return [sceneDir, stb](const std::string &url) -> ex::TexturePixelResult {
    if (url.rfind("proc:", 0) == 0)
      return ex::TexturePixelResult::makeReady(makeNamed(url.substr(5)));
    auto ends = [&](const char *s) {
      const std::string e(s);
      return url.size() >= e.size() && url.compare(url.size() - e.size(), e.size(), e) == 0;
    };
    const std::string path =
        (!url.empty() && url[0] == '/') ? url : (sceneDir + "/" + url);
    if (ends(".ppm")) {
      ex::TexturePixels p = readPpm(path);
      return p.rgba.empty() ? ex::TexturePixelResult::makeFailed()
                            : ex::TexturePixelResult::makeReady(std::move(p));
    }
    if (ends(".png") || ends(".jpg") || ends(".jpeg")) return stb(path);
    return ex::TexturePixelResult::makeFailed();
  };
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_PROCEDURAL_TEXTURE_HPP
