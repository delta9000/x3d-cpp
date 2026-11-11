#ifndef X3D_SAI_EXPERIMENTAL_REFERENCE_PROVIDER_HPP
#define X3D_SAI_EXPERIMENTAL_REFERENCE_PROVIDER_HPP
#include "provider.hpp"

namespace x3d::sai::experimental {
// Reference-kernel backend of the one provider-neutral contract. Capabilities
// describe this bounded adapter, not general ISO/profile conformance. Kernel
// snapshots, atomic scene edits and external sessions remain separate extensions.
result<std::unique_ptr<provider::service>>
make_reference_provider(provider::resource_limits = {});
} // namespace x3d::sai::experimental
#endif
