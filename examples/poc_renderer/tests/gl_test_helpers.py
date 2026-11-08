"""Launch the software GL renderer and read its P6 screenshots."""
import subprocess


def render(renderer, scene, output):
    result = subprocess.run([
        'xvfb-run', '-a', 'env', 'LIBGL_ALWAYS_SOFTWARE=1',
        'GALLIUM_DRIVER=llvmpipe', '__GLX_VENDOR_LIBRARY_NAME=mesa',
        str(renderer), '--screenshot', str(output), str(scene),
    ], timeout=120, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    # Some xvfb-run builds redirect the command's stderr to stdout, so read both.
    if result.returncode:
        raise RuntimeError(result.stdout)
    return result.stdout


def pixels(path):
    header, data = path.read_bytes().split(b'\n255\n', 1)
    assert header.startswith(b'P6\n'), header[:32]
    width, height = map(int, header.split()[1:3])
    return width, height, data
