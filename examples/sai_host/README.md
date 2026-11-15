# Headless native SAI host

A standalone consumer of the [native SAI provider](../../adapters/sai_provider/README.md).
It builds a small scene through the ordinary provider service, replays a local
recording through an explicit host loop, and rasterizes frames from the
presentation feed on the CPU. It has no network access and draws nothing on
screen.

## Build and run

From source, at the x3d-cpp root:

```sh
cmake -S examples/sai_host -B build-sai-host -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DX3D_CPP_SOURCE_DIR="$PWD" -DX3D_SAI_SOURCE_DIR="$PWD/sai"
cmake --build build-sai-host --target sai_cpu_host
./build-sai-host/sai_cpu_host examples/sai_host/recorded-workload.txt
```

Against an installed prefix (this directory may be copied anywhere):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/relocated/prefix
cmake --build build --target sai_cpu_host --parallel 1
./build/sai_cpu_host ./recorded-workload.txt
ctest --test-dir build --output-on-failure
```

## Documentation

- Walkthrough: [Run a scene on the native runtime](../../docs/wiki/sai/tutorials/native-host.md)
- Recording format, limits and what the run checks:
  [Headless host example](../../docs/wiki/sai/reference/host-example.md)
