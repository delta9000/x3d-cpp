#!/usr/bin/env python3
"""Generate deterministic, controlled 8-bit image fixtures for the TextureResolver
decode swap-test (ADR-0024 / U3).

The swap-test asserts the stb_image and wuffs backends decode each file to the
*byte-identical* RGBA8 surface. That only holds for LOSSLESS formats, so every
fixture here is PNG/BMP/GIF/TGA — no JPEG (lossy IDCT diverges by construction).
Fixtures are tiny (16x16), 8-bit, and carry no gAMA/iCCP color-management chunks
so decode is library-independent.

These bytes are committed; regenerate only if the matrix changes:
    python3 runtime/io/tests/fixtures/texture/generate_fixtures.py
Requires Pillow (already in the project dev env).
"""
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
W = H = 16


def rgba_pattern():
    """A 16x16 RGBA image with a deterministic gradient + varying straight alpha.
    Few enough colors to also survive GIF palette quantization losslessly."""
    img = Image.new("RGBA", (W, H))
    px = img.load()
    for y in range(H):
        for x in range(W):
            r = (x * 16) & 0xFF
            g = (y * 16) & 0xFF
            b = ((x + y) * 8) & 0xFF
            a = 0xFF if (x + y) % 4 else 0x80
            px[x, y] = (r, g, b, a)
    return img


def rgb_pattern():
    return rgba_pattern().convert("RGB")


def main():
    rgba = rgba_pattern()
    rgb = rgb_pattern()

    # PNG — RGBA, straight alpha, 8-bit, no color-management chunks.
    rgba.save(os.path.join(HERE, "rgba_gradient.png"), format="PNG")

    # BMP — 24-bit BGR, opaque (BMP alpha support is patchy across decoders).
    rgb.save(os.path.join(HERE, "rgb_checker.bmp"), format="BMP")

    # GIF — palette (<=256 colors), opaque, single frame. No transparent index:
    # transparent-pixel RGB is decoder-defined and would break byte-equality.
    rgb.convert("P", palette=Image.ADAPTIVE, colors=256).save(
        os.path.join(HERE, "palette.gif"), format="GIF"
    )

    # TGA — 32-bit uncompressed with alpha. No magic number (routed structurally).
    rgba.save(os.path.join(HERE, "rgba.tga"), format="TGA", compression=None)

    # Corrupt input for Failed-parity: a valid PNG signature followed by garbage,
    # so both decoders accept the magic then fail to decode the body.
    with open(os.path.join(HERE, "rgba_gradient.png"), "rb") as f:
        good = f.read()
    with open(os.path.join(HERE, "truncated.png"), "wb") as f:
        f.write(good[:24] + b"\x00" * 16)  # PNG sig + IHDR start, then junk

    # Unsniffable blob for the composer's negative path.
    with open(os.path.join(HERE, "garbage.bin"), "wb") as f:
        f.write(b"not an image, no magic here\n")

    print("wrote fixtures to", HERE)


if __name__ == "__main__":
    main()
