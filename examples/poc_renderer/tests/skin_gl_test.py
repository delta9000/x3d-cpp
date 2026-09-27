"""Compare a posed GPU skin with equivalent ordinary geometry under llvmpipe."""
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
fixtures = pathlib.Path(sys.argv[2])


def pixels(path):
    data = path.read_bytes()
    header, image = data.split(b'\n255\n', 1)
    assert header.startswith(b'P6\n'), header[:32]
    return image


def render(scene, output):
    result = subprocess.run([
        'xvfb-run', '-a', 'env', 'LIBGL_ALWAYS_SOFTWARE=1',
        'GALLIUM_DRIVER=llvmpipe', '__GLX_VENDOR_LIBRARY_NAME=mesa',
        str(renderer), '--screenshot', str(output), str(scene),
    ], timeout=120, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    return result.stderr


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    posed = tmp / 'posed.ppm'
    expected = tmp / 'expected.ppm'
    rest = tmp / 'rest.ppm'
    rest_scene = tmp / 'rest.x3d'
    skin_scene = fixtures / 'skinned_triangle.x3d'
    rest_scene.write_text(skin_scene.read_text().replace(
        'rotation="0 0 1 1.57079632679"', 'rotation="0 0 1 0"'))
    log = render(skin_scene, posed)
    assert 'GPU palette' in log, log
    render(fixtures / 'deformed_triangle.x3d', expected)
    render(rest_scene, rest)
    a, b, c = pixels(posed), pixels(expected), pixels(rest)
    assert len(a) == len(b) == len(c)
    differing = sum(x != y for x, y in zip(a, b))
    moved = sum(x != y for x, y in zip(a, c))
    assert differing < len(a) * 0.005, f'GPU/reference pixel difference: {differing}'
    assert moved > len(a) * 0.005, f'skin did not visibly move: {moved}'
    print(f'GPU skin/reference differing channels: {differing}; rest/posed: {moved}')

    five = tmp / 'five.ppm'
    five_expected = tmp / 'five_expected.ppm'
    log = render(fixtures / 'five_influences.x3d', five)
    assert 'GPU palette, 15 influences' in log, log
    render(fixtures / 'five_influences_expected.x3d', five_expected)
    diff = sum(x != y for x, y in zip(pixels(five), pixels(five_expected)))
    assert diff < len(a) * 0.005, f'fifth influence lost: {diff}'
