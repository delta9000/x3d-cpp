#!/usr/bin/env python3
"""End-to-end pixel checks for PoC FillProperties, using its GL screenshot path."""
from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    # This renderer writes exactly `P6\n<width> <height>\n255\n`; parse only
    # those header lines so leading whitespace-valued pixel bytes stay intact.
    lines = data.split(b"\n", 3)
    if len(lines) != 4 or lines[0] != b"P6" or lines[2] != b"255":
        raise AssertionError(f"unexpected PPM header in {path}")
    width, height = map(int, lines[1].split())
    pixels = lines[3]
    if len(pixels) != width * height * 3:
        raise AssertionError(f"truncated PPM pixel data in {path}")
    return width, height, pixels


def scene(material: str, fill: str, rear: bool = False, reverse: bool = False) -> str:
    indices = "3 2 1 0" if reverse else "0 1 2 3"
    rear_shape = '''<Shape><Appearance><UnlitMaterial emissiveColor="0 0 1"/></Appearance>
    <IndexedFaceSet coordIndex="0 1 2 3 -1" solid="false">
      <Coordinate point="-4 -4 -1, 4 -4 -1, 4 4 -1, -4 4 -1"/>
    </IndexedFaceSet></Shape>''' if rear else ""
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Full" version="4.0"><Scene>
  <Viewpoint position="0 0 8" fieldOfView="0.785398"/>
  <Background skyColor="0 0 0"/><NavigationInfo headlight="false"/>
  <DirectionalLight direction="0 0 -1" intensity="1"/>
  {rear_shape}
  <Shape><Appearance>{material}{fill}</Appearance>
    <IndexedFaceSet coordIndex="{indices} -1" solid="false" normalPerVertex="false" normalIndex="0">
      <Coordinate point="-4 -4 0, 4 -4 0, 4 4 0, -4 4 0"/>
      <Normal vector="0 0 1"/>
    </IndexedFaceSet>
  </Shape>
</Scene></X3D>'''


MATERIALS = {
    "phong": '<Material diffuseColor="0 1 0" ambientIntensity="1"/>',
    "physical": '<PhysicalMaterial baseColor="0 1 0" metallic="0" roughness="1"/>',
    "unlit": '<UnlitMaterial emissiveColor="0 1 0"/>',
}


def render(binary: Path, root: Path, name: str, material: str, fill: str,
           rear: bool = False, reverse: bool = False):
    source, shot = root / f"{name}.x3d", root / f"{name}.ppm"
    source.write_text(scene(material, fill, rear, reverse), encoding="utf-8")
    result = subprocess.run([str(binary), "--screenshot", str(shot), str(source)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if result.returncode:
        raise RuntimeError(f"renderer failed for {name}:\n{result.stderr}")
    return read_ppm(shot)


def near(pixel: tuple[int, int, int], target: tuple[int, int, int], tol: int = 35) -> bool:
    return max(abs(a - b) for a, b in zip(pixel, target)) <= tol


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("renderer", type=Path)
    args = parser.parse_args()
    binary = args.renderer.resolve()
    with tempfile.TemporaryDirectory(prefix="poc-fill-") as tmp:
        root = Path(tmp)
        # Absent FillProperties must preserve the material's fill. Present node
        # defaults enable both fill and white style-1 hatching.
        base = render(binary, root, "absent", MATERIALS["unlit"], "")
        default = render(binary, root, "default", MATERIALS["unlit"],
                          '<FillProperties hatchColor="1 1 1"/>')
        w, h, pixels = base
        _, _, default_pixels = default
        green = [near(tuple(pixels[i:i+3]), (0, 255, 0)) for i in range(0, len(pixels), 3)]
        if sum(green) < w * h // 8:
            raise AssertionError("baseline plane did not cover enough pixels")
        white = [near(tuple(default_pixels[i:i+3]), (255, 255, 255))
                  for i in range(0, len(default_pixels), 3)]
        if sum(white) < sum(green) // 12:
            raise AssertionError("default FillProperties did not add white hatch pixels")

        # Four filled/hatched states. For the hatch-only state, the hatch must
        # remain visible while most of the baseline green surface is discarded.
        cases = [
            ("fill_only", 'filled="true" hatched="false"', True, False),
            ("hatch_only", 'filled="false" hatched="true" hatchColor="1 0 0"', False, True),
            ("neither", 'filled="false" hatched="false"', False, False),
            ("both", 'filled="true" hatched="true" hatchColor="1 0 0"', True, True),
        ]
        for name, attrs, has_fill, has_hatch in cases:
            _, _, data = render(binary, root, name, MATERIALS["unlit"],
                                f"<FillProperties {attrs}/>")
            colors = [tuple(data[i:i+3]) for i in range(0, len(data), 3)]
            g = sum(near(p, (0, 255, 0)) for p in colors)
            r = sum(near(p, (255, 0, 0)) for p in colors)
            if has_fill and g < sum(green) * 0.8:
                raise AssertionError(f"{name}: filled surface missing ({g} green pixels)")
            if not has_fill and g > sum(green) // 20:
                raise AssertionError(f"{name}: filled surface remained ({g} green pixels)")
            if has_hatch and r < sum(green) // 15:
                raise AssertionError(f"{name}: hatch overlay missing ({r} red pixels)")
            if not has_hatch and r:
                raise AssertionError(f"{name}: unexpected hatch pixels ({r} red pixels)")

        # Ensure all required styles make hatch pixels and produce distinct
        # screen-space masks; unsupported style must fall back to style 1.
        masks: dict[int, bytes] = {}
        for style in range(1, 7):
            _, _, data = render(binary, root, f"style{style}", MATERIALS["unlit"],
                                f'<FillProperties filled="false" hatchStyle="{style}" hatchColor="1 0 0"/>')
            masks[style] = bytes(1 if near(tuple(data[i:i+3]), (255, 0, 0)) else 0
                                 for i in range(0, len(data), 3))
            if sum(masks[style]) < sum(green) // 15:
                raise AssertionError(f"hatch style {style} produced too few visible pixels")
            def expected(x: int, y: int) -> bool:
                # PPM rows are top-down; gl_FragCoord uses bottom-up window y.
                gx, gy = x, h - 1 - y
                on_h = gy % 8 == 0
                on_v = gx % 8 == 0
                on_pos = (gx - gy) % 8 == 0
                on_neg = (gx + gy) % 8 == 0
                return (on_h, on_v, on_pos, on_neg,
                        on_h or on_v, on_pos or on_neg)[style - 1]
            matches = checked = 0
            for idx, inside in enumerate(green):
                if not inside:
                    continue
                y, x = divmod(idx, w)
                matches += masks[style][idx] == expected(x, y)
                checked += 1
            if checked == 0 or matches / checked < 0.98:
                raise AssertionError(f"hatch style {style}: screen-space 8px pattern matched {matches}/{checked} interior pixels")
        for a, b in ((1, 2), (1, 3), (2, 4), (3, 4), (5, 1), (6, 3)):
            if masks[a] == masks[b]:
                raise AssertionError(f"hatch styles {a} and {b} produced identical masks")
        _, _, unsupported = render(binary, root, "unsupported", MATERIALS["unlit"],
                                   '<FillProperties filled="false" hatchStyle="999" hatchColor="1 0 0"/>')
        unsupported_mask = bytes(1 if near(tuple(unsupported[i:i+3]), (255, 0, 0)) else 0
                                 for i in range(0, len(unsupported), 3))
        if unsupported_mask != masks[1]:
            raise AssertionError("unsupported hatchStyle did not fall back to style 1")

        # Discarded hatch gaps must expose the blue rear polygon. This verifies
        # holes in the foreground also leave depth untouched.
        _, _, depth_data = render(binary, root, "depth_holes", MATERIALS["unlit"],
                                  '<FillProperties filled="false" hatchStyle="1" hatchColor="1 0 0"/>',
                                  rear=True)
        red_count = blue_count = 0
        for i in range(0, len(depth_data), 3):
            pixel = tuple(depth_data[i:i+3])
            red_count += near(pixel, (255, 0, 0))
            blue_count += near(pixel, (0, 0, 255))
        if red_count < sum(green) // 15 or blue_count < sum(green) // 3:
            raise AssertionError(f"hatch holes/depth: expected red hatch and blue rear surface, got {red_count}/{blue_count}")

        # Reverse the polygon winding while keeping solid=false. Appearance fill
        # properties apply to either face; the unlit color keeps this check
        # independent of the renderer's two-sided lighting result.
        _, _, backface = render(binary, root, "backface", MATERIALS["unlit"],
                                '<FillProperties hatchStyle="1" hatchColor="1 0 0"/>',
                                reverse=True)
        back_red = sum(near(tuple(backface[i:i+3]), (255, 0, 0))
                       for i in range(0, len(backface), 3))
        if back_red < sum(green) // 15:
            raise AssertionError(f"back-facing polygon did not hatch ({back_red} red pixels)")

        # Hatch RGB should be applied on top of each built-in material shader.
        for name, mat in MATERIALS.items():
            _, _, data = render(binary, root, f"material_{name}", mat,
                                '<FillProperties hatchStyle="1" hatchColor="1 0 0"/>')
            count = sum(near(tuple(data[i:i+3]), (255, 0, 0))
                        for i in range(0, len(data), 3))
            if count < sum(green) // 15:
                raise AssertionError(f"{name}: hatch overlay missing ({count} red pixels)")
    print("OK: FillProperties GL pixels (defaults, four fill states, hatch styles 1-6/fallback, Phong/PBR/Unlit, depth holes, backfaces)")


if __name__ == "__main__":
    main()
