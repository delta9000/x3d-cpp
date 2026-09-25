# cmake/x3d/install.cmake — install rules and the find_package(x3d_cpp) export.
# Included in order from the top-level CMakeLists.txt (include(), so it shares
# the top-level scope and CMAKE_CURRENT_SOURCE_DIR is the repository root).

# ---------------------------------------------------------------------------
# Install + export so downstream projects can find_package(x3d_cpp).
# ---------------------------------------------------------------------------
install(TARGETS
        x3d_cpp_headers
        x3d_cpp
        x3d_cpp_nodes
        x3d_cpp_authoring_runtime
        x3d_cpp_runtime
        x3d_cpp_sdk
    EXPORT x3d_cppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Non-public artifacts that must never reach the installed include tree. Kept
# byte-for-byte in sync with the leak-gate in scripts/verify_install_embed.sh so
# the install rules and the gate can't disagree about what counts as private.
set(_x3d_install_excludes
    PATTERN "tests" EXCLUDE
    PATTERN "fixtures" EXCLUDE
    PATTERN "test_support" EXCLUDE
    PATTERN "vendor" EXCLUDE
    PATTERN "*TestSupport.*" EXCLUDE
    PATTERN "*test_support*" EXCLUDE)

# The SDK is IO-free: it ships seam INTERFACES + core systems, never a concrete
# backend. Top-level reference backends (io/physics/ext) are kept out simply by
# their absence from X3D_CPP_RUNTIME_PUBLIC_SUBDIRS; these reference backends live
# NESTED inside the otherwise-public sound/ subdir, so they're excluded by name.
# sound/ therefore ships only AudioBackend.hpp (the seam) + SoundSystem.hpp (the
# core System) — the miniaudio (device-IO) and builtin-DSP reference backends, and
# the RecordingBackend test double, all stay out. Kept in sync with the reference-
# backend leak check in scripts/verify_install_embed.sh.
set(_x3d_runtime_backend_excludes
    PATTERN "miniaudio" EXCLUDE          # reference AudioBackend over a real device
    PATTERN "dsp" EXCLUDE                # reference built-in software DSP backend
    PATTERN "RecordingBackend.hpp" EXCLUDE)  # op-recording test double (tests only)

# The curated façade header (include/x3d/sdk.hpp) installs preserving its x3d/
# subdir so installed consumers `#include "x3d/sdk.hpp"`.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp"
    FILES_MATCHING
        PATTERN "*.hpp"
        ${_x3d_install_excludes})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/x3d"
    FILES_MATCHING
        PATTERN "*.hpp"
        ${_x3d_install_excludes})

# The hand-written runtime document model. Root-level headers (X3DDocument,
# X3DExecutionContext, X3DRuntime, ...) install flat; each PUBLIC subdir installs
# to its matching include/x3d_cpp/<sub>/, mirroring the install-interface include
# dirs above. Both are driven by the SAME X3D_CPP_RUNTIME_PUBLIC_SUBDIRS list, so
# the include path and the shipped headers stay in lockstep — and the internal
# backends (io/physics/ext, plus nested reference backends) are never installed.
file(GLOB _x3d_runtime_root_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/*.hpp")
install(FILES ${_x3d_runtime_root_headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp")
foreach(_sub IN LISTS X3D_CPP_RUNTIME_PUBLIC_SUBDIRS)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/runtime/${_sub}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/${_sub}"
        FILES_MATCHING
            PATTERN "*.hpp"
            # parse/tinfl.h (bundled public-domain DEFLATE decompressor, included
            # by parse/Inflate.hpp) is the only public non-.hpp header; allow *.h
            # so it ships with parse/. Vendored *.h (e.g. script/vendor/duktape)
            # is held out by the vendor EXCLUDE below.
            PATTERN "*.h"
            ${_x3d_install_excludes}
            ${_x3d_runtime_backend_excludes})
endforeach()

install(EXPORT x3d_cppTargets
    FILE x3d_cppTargets.cmake
    NAMESPACE x3d_cpp::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/x3d_cppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp")

# The package installs compiled shared libraries, so it is NOT arch-independent:
# ARCH_INDEPENDENT would delete the CMAKE_SIZEOF_VOID_P guard and let a consumer
# with a different pointer size link a mismatched ABI. SameMinorVersion (not
# SameMajorVersion) because PROJECT_VERSION is 0.x: at major 0, SameMajorVersion
# treats every future 0.y as compatible with 0.1. Contract + the exact reasoning:
# tests/cmake/package_version/check_version_file.cmake.
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp")

# Redistribution requires the license texts: LICENSE (MIT) plus NOTICE, which
# carries the attribution/license inventory for every vendored and fetched
# third-party component. An installed prefix without them is not redistributable.
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/x3d_cpp")

# Allow find_package against the build tree (no install required).
export(EXPORT x3d_cppTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppTargets.cmake"
    NAMESPACE x3d_cpp::)
