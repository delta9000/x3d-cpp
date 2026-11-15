# Native SAI provider

This optional package implements the SAI SDK's portable `provider::service`
([`sai/`](../../sai/README.md)) over native x3d-cpp state: an `X3DDocument`
during setup and a `RuntimeSession` after activation.

```cmake
find_package(x3d_cpp_sai CONFIG REQUIRED)
target_link_libraries(my_host PRIVATE x3d_cpp::sai_provider)
```

```cpp
#include <x3d/sai_provider.hpp>
auto service = x3d::sai::experimental::native::make_service();
```

The contract is experimental with no stable ABI. Build the SDK, runtime and
adapter from the same x3d-cpp revision.

## Documentation

| Topic | Page |
|---|---|
| Build the three packages and link | [Build and link the native backend](../../docs/wiki/sai/how-to/use-the-native-backend.md) |
| Drive an already loaded scene | [Adopt a loaded session](../../docs/wiki/sai/how-to/adopt-a-session.md) |
| Render from immutable frames | [Present frames from the native feed](../../docs/wiki/sai/how-to/present-frames.md) |
| Supported nodes, field kinds, restrictions | [Native provider reference](../../docs/wiki/sai/reference/native-provider.md) |
| Frame and delta semantics | [Presentation feed reference](../../docs/wiki/sai/reference/presentation-feed.md) |
| A complete host | [`examples/sai_host`](../../examples/sai_host/README.md) |

## Verify

```sh
mise run sai-provider
```

See [Test suites and gates](../../docs/wiki/sai/reference/test-suites.md#provider-harness).
