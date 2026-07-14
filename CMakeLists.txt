cmake_minimum_required(VERSION 3.20)

project(x3d_cpp
    VERSION 0.1.0
    DESCRIPTION "Header-only C++ bindings generated from the X3D Unified Object Model"
    LANGUAGES CXX C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/X3DTargetPurpose.cmake")

# ---------------------------------------------------------------------------
# ccache: reuse cached object output across rebuilds. The generated headers
# change rarely, so the ~800-TU compile-test suite is almost entirely cache
# hits after the first run. Auto-detected; a no-op when ccache is absent (e.g.
# a CI runner without it), so it never breaks a fresh checkout.
# ---------------------------------------------------------------------------
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
    message(STATUS "x3d_cpp: ccache enabled (${CCACHE_PROGRAM})")
endif()

# ---------------------------------------------------------------------------
# Fast linker: the ~70 test executables + the all-headers exe link far quicker
# with a parallel linker. Prefer mold, then lld; fall back to the system default
# (no-op) when neither is installed, so a fresh checkout / CI is never broken.
# Applies to every target defined after this point (libs + tests + PoC).
# ---------------------------------------------------------------------------
find_program(X3D_MOLD mold)
find_program(X3D_LLD ld.lld)
if(X3D_MOLD)
    add_link_options("-fuse-ld=mold")
    message(STATUS "x3d_cpp: linker = mold")
elseif(X3D_LLD)
    add_link_options("-fuse-ld=lld")
    message(STATUS "x3d_cpp: linker = lld")
endif()

# ---------------------------------------------------------------------------
# Bound parallel COMPILE jobs under Ninja (a job pool; links and light syntax
# TUs stay unbounded). Ninja-only (the pool property is ignored by the Make
# generator, which we don't use for the dev preset).
#
# History: pre-C1 the inline reflection thunks made each generated TU a memory
# hog that OOM-killed cc1plus above ~-j4, so the cap was pinned at 4. After the
# C1 decl/def split (2026-06-16) the heavy fields()/thunk instantiation moved
# into the x3d_cpp_nodes library and per-compile peak RSS dropped to ~0.86 GB
# (measured), so RAM is no longer the binding constraint — the cap now defaults
# to the logical core count. Lower it (-DX3D_CPP_COMPILE_JOBS=N) only on a
# low-RAM host (budget ~0.9 GB per concurrent compile).
# ---------------------------------------------------------------------------
cmake_host_system_information(RESULT _x3d_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
set(X3D_CPP_COMPILE_JOBS "${_x3d_logical_cores}" CACHE STRING "Max concurrent compile jobs (Ninja job pool)")
if(CMAKE_GENERATOR MATCHES "Ninja")
    set_property(GLOBAL PROPERTY JOB_POOLS x3d_compile=${X3D_CPP_COMPILE_JOBS})
    set(CMAKE_JOB_POOL_COMPILE x3d_compile)
    message(STATUS "x3d_cpp: Ninja compile job pool = ${X3D_CPP_COMPILE_JOBS}")
endif()

# ---------------------------------------------------------------------------
# Header-only INTERFACE library: x3d_cpp
# ---------------------------------------------------------------------------
add_library(x3d_cpp INTERFACE)
add_library(x3d_cpp::x3d_cpp ALIAS x3d_cpp)

# Generated headers live in generated_cpp_bindings/. They #include each other
# by bare name (e.g. #include "X3DNode.hpp"), so that directory must be on the
# include path. The hand-written runtime/ document model (X3DDocument, Scene,
# Route, PROTO, IMPORT/EXPORT) is header-only too and includes the generated
# nodes by bare name, so it joins the same include interface. Use BUILD_INTERFACE
# for in-tree consumers and INSTALL_INTERFACE for installed consumers.
# The hand-written runtime/ subdirs that make up the PUBLIC SDK surface — every
# directory x3d/sdk.hpp transitively includes by bare name. This ONE list drives
# the build-interface include dirs, the install-interface include dirs, AND the
# install() rules further down, so the three can never drift apart.
#
# Internal reference backends are deliberately absent: runtime/io (curl/s3/stb/
# wuffs/freetype/...), runtime/physics (Jolt), and runtime/ext (external geometry).
# They implement seams the embedder supplies as std::function callbacks, are not
# part of the v1 façade, and pull vendored third-party deps — so they are neither
# on the public include path nor installed. The façade (include/x3d/sdk.hpp) is
# the one, well-trod entry point; see docs/sdk/README.md.
set(X3D_CPP_RUNTIME_PUBLIC_SUBDIRS codecs events math scene parse extract script sound)

set(_x3d_cpp_interface_includes
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
foreach(_sub IN LISTS X3D_CPP_RUNTIME_PUBLIC_SUBDIRS)
    list(APPEND _x3d_cpp_interface_includes
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/${_sub}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/${_sub}>")
endforeach()
target_include_directories(x3d_cpp INTERFACE ${_x3d_cpp_interface_includes})

# The generated code targets C++20 (designated initializers, etc.).
target_compile_features(x3d_cpp INTERFACE cxx_std_20)

# ---------------------------------------------------------------------------
# Project-wide warning policy. -Wall -Wextra applied to every target we own;
# vendored TUs (Jolt, quickjs, stb, wuffs, stb_truetype) suppress with their own
# `target_compile_options(... PRIVATE -w)` later in this file so they don't pull
# in this baseline. X3D_CPP_WERROR (default OFF) promotes warnings to errors
# for the merge gate — the `ci` CMake preset flips it ON so local `mise run ci`
# matches the GitHub Actions fast gate.
# ---------------------------------------------------------------------------
add_compile_options(-Wall -Wextra)
option(X3D_CPP_WERROR "Treat compiler warnings as errors (the merge gate enables this via the ci preset)" OFF)
if(X3D_CPP_WERROR)
    add_compile_options(-Werror)
    message(STATUS "x3d_cpp: X3D_CPP_WERROR=ON -> warnings treated as errors")
endif()

# ---------------------------------------------------------------------------
# Sanitizer gates (BLD-2). X3D_CPP_SAN enables AddressSanitizer + UndefinedBehavior
# Sanitizer for every project target — the green doctest run misses the kind of
# memory/UB bugs that SEC-1/2 + MEM-3/4 in the project backlog expose. The `san`
# CMake preset flips it ON; the `cpp-san` GitHub Actions job uses the same.
# X3D_CPP_FUZZ additionally enables the libFuzzer harness for parseDocument
# (Clang-only). The two options compose — `san` is the test gate, `fuzz` is the
# explore-time harness.
# ---------------------------------------------------------------------------
option(X3D_CPP_SAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer for every project target" OFF)
if(X3D_CPP_SAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_SAN=ON -> ASan + UBSan enabled (-fno-sanitize-recover=all)")
endif()
option(X3D_CPP_FUZZ "Build the libFuzzer harness for parseDocument (requires Clang)" OFF)

# ---------------------------------------------------------------------------
# Compiled node library (C1): the generated *.cpp (per-node out-of-line
# reflection/validate definitions + the X3DNodeFactory registry) are compiled
# ONCE here instead of re-instantiated in every consumer TU. Consumers link
# this transitively via x3d_cpp::x3d_cpp.
# ---------------------------------------------------------------------------
file(GLOB X3D_CPP_NODE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/nodes/*.cpp")
add_library(x3d_cpp_nodes STATIC ${X3D_CPP_NODE_SOURCES})
add_library(x3d_cpp::nodes ALIAS x3d_cpp_nodes)

# Unity-build the generated node TUs: batching the per-node .cpp into grouped
# TUs eliminates the repeated framework-header re-parse that dominated this
# library's compile — ~2x faster cold build (measured: 30s -> 14s, 16 cores),
# at ~0.4 GB peak RSS per default 8-node TU (well within CI runner limits; the
# pre-C1-split OOM scare was the inline thunks, since removed). Consumers that
# add_subdirectory() this project inherit the speedup.
#
# The generated value-asserting smoke test (generated_cpp_bindings/test.cpp)
# carries its own main(). It rides inertly in the archive today (no target pulls
# it). It must stay OUT of the unity merge — otherwise its main() lands in a
# real node TU and every consumer linking the archive hits a duplicate-main
# link error.
set_target_properties(x3d_cpp_nodes PROPERTIES
    EXPORT_NAME nodes
    UNITY_BUILD ON)
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/nodes/test.cpp"
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
target_link_libraries(x3d_cpp_nodes PUBLIC x3d_cpp)
target_compile_features(x3d_cpp_nodes PUBLIC cxx_std_20)
# Consumers that link the header interface also link the compiled definitions.
target_link_libraries(x3d_cpp INTERFACE x3d_cpp_nodes)

# ── Authoring slim target: node reflection + writers + range-validate only ──
# INTERFACE facade over the compiled node library plus the authoring-only
# headers (document model, range-validate, codec writers). Deliberately omits
# the parse/execution/extract include dirs so consumers cannot reach those APIs.
add_library(x3d_cpp_authoring INTERFACE)
add_library(x3d_cpp::authoring ALIAS x3d_cpp_authoring)
target_link_libraries(x3d_cpp_authoring INTERFACE x3d_cpp::nodes)
target_include_directories(x3d_cpp_authoring INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
target_compile_features(x3d_cpp_authoring INTERFACE cxx_std_20)
install(TARGETS x3d_cpp_authoring EXPORT x3d_cppTargets)

# ---------------------------------------------------------------------------
# T-SDK: the curated embedder façade. A thin INTERFACE target that adds the
# include/ path (so consumers write #include "x3d/sdk.hpp") on top of the full
# x3d_cpp interface. No new compiled TU — sdk.hpp only re-exports.
# ---------------------------------------------------------------------------
add_library(x3d_cpp_sdk INTERFACE)
add_library(x3d_cpp::sdk ALIAS x3d_cpp_sdk)
set_target_properties(x3d_cpp_sdk PROPERTIES EXPORT_NAME sdk)
target_include_directories(x3d_cpp_sdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp>")
target_link_libraries(x3d_cpp_sdk INTERFACE x3d_cpp::x3d_cpp)

# ---------------------------------------------------------------------------
# Install + export so downstream projects can find_package(x3d_cpp).
# ---------------------------------------------------------------------------
install(TARGETS x3d_cpp x3d_cpp_nodes x3d_cpp_sdk
    EXPORT x3d_cppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

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

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/x3d_cpp")

# Allow find_package against the build tree (no install required).
export(EXPORT x3d_cppTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppTargets.cmake"
    NAMESPACE x3d_cpp::)

# ---------------------------------------------------------------------------
# Tests: compile every generated header.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_TESTS "Build header-compilation tests for x3d_cpp" ${PROJECT_IS_TOP_LEVEL})

# Per-header isolation compile-tests (the ~800 compile_* ctests, one per header).
# They pin a break to a single header, but the aggregate x3d_cpp_all_headers TU
# plus the behavior/integration suite already catch breaks — the per-header sweep
# is the slow part of a cold ctest. ON by default so CI keeps full coverage; the
# dev preset sets it OFF for fast local iteration.
option(X3D_CPP_PER_HEADER_CHECKS "Compile each header in isolation as a ctest" ON)

# ---------------------------------------------------------------------------
# PoC OpenGL renderer (examples/poc_renderer/). OUT-of-SDK consumer, OFF by
# default so the normal build/golden/ctest path is completely unaffected: with
# this OFF we never add_subdirectory it, so no FetchContent of GLFW, no glad
# TU, and it never joins the OOM-prone all-headers compile. It is added at the
# very end of this file once the x3d_cpp::x3d_cpp INTERFACE target exists.
# Build it with:  cmake --preset dev -DX3D_CPP_BUILD_POC=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_POC "Build the PoC OpenGL renderer" OFF)

# Asset-import consumer + authoring footprint smoke (examples/asset_import/).
# OFF by default; Task 11 finalizes the subdirectory wiring. The authoring
# footprint smoke test is registered at the end of this file when this is ON.
option(X3D_CPP_BUILD_ASSET_IMPORT "Build the asset-import consumer" OFF)

# ---------------------------------------------------------------------------
# Headless CPU rasterizer (examples/cpu_raster/). OUT-of-SDK consumer, OFF by
# default. Unlike the PoC it has NO third-party deps (no GLFW/glad/GL/image lib),
# rasterizes on the CPU and writes a PPM, so both the binary and its ctest suite
# run on a bare CI runner. Added at the end of this file once x3d_cpp::x3d_cpp
# exists. Build with:  cmake -S . -B build-cpuraster -DX3D_CPP_BUILD_CPURASTER=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_CPURASTER "Build the headless CPU rasterizer + GLSL emulation" OFF)

# ---------------------------------------------------------------------------
# Extension quarantine (runtime/ext/). Hand-written foreign-format codecs and
# binary-geometry helpers that live in x3d::runtime::ext. One-way dep: ext
# may include runtime/extract/* (PackedMesh, Aabb, …) but core NEVER includes
# runtime/ext/*. Default OFF so the standard build/golden/ctest path is
# completely unaffected. Build with: cmake --preset dev -DX3D_CPP_BUILD_EXT=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_EXT "Build the opt-in x3d::runtime::ext extensions (foreign-format codecs, binary geometry)" OFF)

# ---------------------------------------------------------------------------
# Jolt Physics backend (runtime/physics/jolt/). The flag-gated example backend
# for the engine-agnostic PhysicsBackend seam (runtime/physics/PhysicsBackend.hpp
# + PhysicsSystem.hpp are CORE, header-only, Jolt-free, always present). Default
# OFF so the standard build/golden/ctest path is COMPLETELY unaffected: with this
# OFF Jolt is never fetched and the backend lib never built/linked. Mirrors the
# x3d_duktape isolation rule. Build with:
#   cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_PHYSICS "Build Jolt Physics backend (OFF default; fetches Jolt v5.5.0)" OFF)

if(X3D_CPP_BUILD_PHYSICS)
    include(FetchContent)
    # Jolt's CMake honors these toggles; force them so they cannot clobber our
    # flags (OVERRIDE_CXX_FLAGS), apply -Werror (ENABLE_ALL_WARNINGS), or trip an
    # LTO archive-format mismatch at link (INTERPROCEDURAL_OPTIMIZATION).
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
    set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
    set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
    set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
    set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
    set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
    set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
    set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
    # Jolt's CMakeLists lives in Build/ (SOURCE_SUBDIR), and its target is "Jolt".
    FetchContent_Declare(JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build)
    FetchContent_MakeAvailable(JoltPhysics)

    # The JoltBackend: the single TU where Jolt meets the seam. Static lib so the
    # rest of the build is untouched — only the physics tests / x3d_cli link it.
    add_library(x3d_physics_jolt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/jolt/JoltBackend.cpp")
    # Jolt is PRIVATE: its headers + (critically) its PUBLIC arch flags
    # (-mavx2/-mfma/...) are needed only to compile JoltBackend.cpp, NOT by our
    # consumers. JoltBackend.hpp is Jolt-free (pImpl), so x3d_cli/tests linking
    # x3d_physics_jolt must NOT inherit -mfma — FMA contraction would silently
    # change FP rounding in unrelated (non-physics) code (it shifted a
    # ProximitySensor knife-edge by one tick). The Jolt static archive is still
    # linked transitively because x3d_physics_jolt depends on it.
    target_link_libraries(x3d_physics_jolt PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_physics_jolt PRIVATE Jolt)
    target_include_directories(x3d_physics_jolt PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/jolt")
    target_compile_definitions(x3d_physics_jolt PUBLIC X3D_HAVE_PHYSICS=1)
    # Jolt headers are warning-noisy; silence them on this TU only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_physics_jolt PRIVATE -w)
    endif()

    # Physics tests (only when the test build is also on). enable_testing() runs
    # in the X3D_CPP_BUILD_TESTS block below; CTest tolerates add_test before it.
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # (a) JoltBackend free-fall + rest-on-ground + determinism (engine proof).
        add_executable(x3d_physics_jolt_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/jolt_backend_test.cpp")
        target_link_libraries(x3d_physics_jolt_test PRIVATE x3d_physics_jolt)
        add_test(NAME x3d_physics_jolt COMMAND x3d_physics_jolt_test)

        # (b) PhysicsSystem unit: §37 scene -> attach + tick -> RigidBody.position
        #     updated AND position_changed delivered to a routed Transform.
        add_executable(x3d_physics_system_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/physics_system_test.cpp")
        target_link_libraries(x3d_physics_system_test PRIVATE x3d_physics_jolt)
        add_test(NAME x3d_physics_system COMMAND x3d_physics_system_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# QuickJS ScriptEngine backend (runtime/script/QuickJsBackend.{hpp,cpp}). The
# flag-gated SECOND backend for the language-agnostic ScriptEngine seam
# (runtime/script/ScriptEngine.hpp is CORE, header-only, JS-free, always
# present). This is the Phase-0 genericity pilot: a fully independent backend
# (quickjs-ng) implementing the SAME interface as the Duktape EcmaScriptBackend,
# with zero core changes, to prove the seam is generic.
#
# Mirrors the x3d_physics_jolt isolation rule exactly: option() -> FetchContent
# -> an isolated STATIC lib (x3d_quickjs) whose single TU is QuickJsBackend.cpp;
# quickjs-ng linked PRIVATE so its headers/flags never leak to consumers
# (QuickJsBackend.hpp is QuickJS-free via a pImpl, like JoltBackend.hpp is
# Jolt-free). Default OFF so the standard build/golden/ctest path is COMPLETELY
# unaffected: with this OFF quickjs-ng is never fetched and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_QUICKJS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_QUICKJS "Build QuickJS ScriptEngine backend (OFF default)" OFF)

if(X3D_CPP_BUILD_QUICKJS)
    include(FetchContent)
    # quickjs-ng's CMake honors these toggles; force them so the engine builds as
    # a static lib only (no shared, no examples/libc/install) and cannot clobber
    # our build. Its main library target is `qjs` (see its CMakeLists).
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(QJS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QJS_BUILD_LIBC OFF CACHE BOOL "" FORCE)
    set(QJS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(quickjs_ng
        GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
        GIT_TAG v0.15.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(quickjs_ng)

    # The QuickJsBackend: the single TU where quickjs-ng meets the seam. Static
    # lib so the rest of the build is untouched — only the quickjs test / x3d_cli
    # link it.
    add_library(x3d_quickjs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/QuickJsBackend.cpp")
    # qjs is PRIVATE: its headers (quickjs.h) are needed only to compile
    # QuickJsBackend.cpp, NOT by our consumers. QuickJsBackend.hpp is QuickJS-free
    # (pImpl), so x3d_cli/tests linking x3d_quickjs must NOT inherit QuickJS's
    # include dirs or any flags. The qjs static archive is still linked
    # transitively because x3d_quickjs depends on it.
    target_link_libraries(x3d_quickjs PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_quickjs PRIVATE qjs)
    target_include_directories(x3d_quickjs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script")
    # quickjs-ng headers are warning-noisy; silence them on this TU only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_quickjs PRIVATE -w)
    endif()

    # QuickJS backend test (only when the test build is also on). enable_testing()
    # runs in the X3D_CPP_BUILD_TESTS block below; CTest tolerates add_test before
    # it (same precedent as the physics tests above).
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # U1 lifecycle: load/init/shutdown/prepareEvents/invoke/eventsProcessed,
        # syntax-error -> kInvalidScriptHandle, independent per-script contexts.
        add_executable(x3d_quickjs_backend_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/quickjs_backend_test.cpp")
        target_link_libraries(x3d_quickjs_backend_test PRIVATE x3d_quickjs)
        add_test(NAME x3d_quickjs_backend COMMAND x3d_quickjs_backend_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# Mirrors the x3d_quickjs isolation rule: option() -> find_package -> an
# isolated STATIC lib (x3d_curl) whose single TU is HttpResolver.cpp; libcurl
# linked PRIVATE so its headers/flags never leak to consumers. Default OFF so
# the standard build/golden/ctest path is COMPLETELY unaffected: with this OFF
# libcurl is never linked and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_CURL=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_CURL "Build libcurl HTTP AssetResolver backend (OFF default)" OFF)

if(X3D_CPP_BUILD_CURL)
    find_package(CURL REQUIRED)
    # The single TU where libcurl meets the seam. Static lib so the rest of
    # the build is untouched — only the curl backend test / future swap-test
    # link it.
    add_library(x3d_curl STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/HttpResolver.cpp")
    # CURL::libcurl is PRIVATE: its headers (curl.h) are needed only to
    # compile HttpResolver.cpp, NOT by our consumers. HttpResolver.hpp is
    # libcurl-free (pure std::function AssetResolver return), so tests
    # linking x3d_curl do NOT inherit libcurl's include dirs or any flags.
    target_link_libraries(x3d_curl PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_curl PRIVATE CURL::libcurl)
    target_include_directories(x3d_curl PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl")
    # libcurl headers are warning-noisy on some toolchains; silence on this TU.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_curl PRIVATE -w)
    endif()

    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # U1 per-backend test: URL prefix check + libcurl error path. The
        # success path is tested in U3 (swap-test) against an in-process
        # HTTP server, keeping this test fast + hermetic + offline.
        add_executable(x3d_assetresolver_backend_a_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/curl/tests/asset_resolver_backend_a_test.cpp")
        target_link_libraries(x3d_assetresolver_backend_a_test PRIVATE x3d_curl)
        add_test(NAME x3d_assetresolver_backend_a COMMAND x3d_assetresolver_backend_a_test)
    endif()
endif()

# ---------------------------------------------------------------------------
# Mirrors the x3d_quickjs / x3d_curl isolation rule: option() -> find_package
# -> an isolated STATIC lib (x3d_s3) whose single TU is S3Resolver.cpp; AWS
# SDK linked PRIVATE so its headers/flags never leak to consumers. Default OFF
# so the standard build/golden/ctest path is COMPLETELY unaffected: with this
# OFF the AWS SDK is never linked and the lib never built.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_S3=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_S3 "Build AWS S3 SDK AssetResolver backend (OFF default)" OFF)

if(X3D_CPP_BUILD_S3)
    find_package(AWSSDK REQUIRED COMPONENTS s3)
    # The single TU where the AWS SDK meets the seam. Static lib so the rest
    # of the build is untouched — only the S3 backend test / future swap-test
    # link it.
    add_library(x3d_s3 STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/S3Resolver.cpp")
    # AWS SDK targets are PRIVATE: their headers (aws/s3/S3Client.h etc.) are
    # needed only to compile S3Resolver.cpp, NOT by our consumers.
    # S3Resolver.hpp is AWS-SDK-free (pure std::function AssetResolver return),
    # so tests linking x3d_s3 do NOT inherit the AWS SDK's include dirs or any
    # flags.
    target_link_libraries(x3d_s3 PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_s3 PRIVATE
        aws-cpp-sdk-core
        aws-cpp-sdk-s3)
    target_include_directories(x3d_s3 PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3")
    # AWS SDK headers are warning-noisy on some toolchains; silence on this TU.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_s3 PRIVATE -w)
    endif()

    if(X3D_CPP_BUILD_TESTS)
        enable_testing()
        # U2 per-backend test: URL prefix check + URL parse + SDK init path.
        # The success path is tested in U3 (swap-test) against a docker minio
        # fixture, keeping this test fast + hermetic + offline.
        add_executable(x3d_assetresolver_backend_b_test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/tests/asset_resolver_backend_b_test.cpp")
        target_link_libraries(x3d_assetresolver_backend_b_test PRIVATE x3d_s3)
        add_test(NAME x3d_assetresolver_backend_b COMMAND x3d_assetresolver_backend_b_test)

        # Test-support lib: the swap-test (U3) must seed a fixture bucket so
        # Backend B has something to fetch. The seeding uses the AWS SDK, so it
        # is confined to its own TU here (AWS SDK linked PRIVATE, like x3d_s3)
        # rather than called from the swap-test TU directly — otherwise the AWS
        # SDK would appear in two TUs that can disagree on headers and corrupt
        # ClientConfiguration at runtime. See runtime/io/s3/S3TestSupport.hpp.
        add_library(x3d_s3_testsupport STATIC
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3/S3TestSupport.cpp")
        target_link_libraries(x3d_s3_testsupport PUBLIC x3d_cpp::x3d_cpp)
        target_link_libraries(x3d_s3_testsupport PRIVATE
            aws-cpp-sdk-core aws-cpp-sdk-s3)
        target_include_directories(x3d_s3_testsupport PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/s3")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(x3d_s3_testsupport PRIVATE -w)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# U3 — the AssetResolver/IO genericity proof. The swap-test ctest drives
# identical fixture bytes through Backend A (libcurl HTTP) and Backend B
# (AWS S3) and asserts byte-equal AssetResult.bytes, plus equal Failed
# status for missing keys. Gated by both backends being built.
# Mirrors the x3d_quickjs_swap precedent (line ~1856).
# Build: cmake --preset dev -DX3D_CPP_BUILD_CURL=ON -DX3D_CPP_BUILD_S3=ON
# Run:   ctest -R x3d_assetresolver_swap  (needs $X3D_S3_ENDPOINT pointing
#         at a docker minio with bucket 'x3d-swap-fixtures'; otherwise the
#         test degrades to an HTTP-only parity check with a SKIP message).
# ---------------------------------------------------------------------------
if(TARGET x3d_curl AND TARGET x3d_s3 AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_assetresolver_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/asset_resolver_swap_test.cpp")
    # The swap-test talks only through the seams (makeHttpResolver/makeS3Resolver)
    # plus the AWS-free x3d_s3_testsupport seeding helper. It never includes
    # <aws/...> or <curl/...> itself, so no heavy backend dependency leaks into
    # the test TU (both backends' libs link their dep PRIVATE).
    target_link_libraries(x3d_assetresolver_swap PRIVATE
        x3d_curl x3d_s3 x3d_s3_testsupport)
    target_compile_definitions(x3d_assetresolver_swap PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures")
    add_test(NAME x3d_assetresolver_swap COMMAND x3d_assetresolver_swap)
endif()

# ---------------------------------------------------------------------------
# TextureResolver decode seam — Backend A (stb_image). ADR-0024 / U1.
# Mirrors the x3d_curl isolation rule: option() -> an isolated STATIC lib
# (x3d_stb) whose single TU is StbTextureResolver.cpp; stb_image.h is included
# PRIVATE so it meets the seam in ONE TU and never leaks. With this OFF the lib
# is never built and stb is never compiled into the runtime.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_STB=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_STB "Build stb_image TextureResolver decode backend (OFF default)" OFF)

# ---------------------------------------------------------------------------
# assimp ImportSource backend for the asset-import consumer. Gated OFF by
# default (system-only dependency; not vendored). When ON, the
# AssimpSource.cpp backend is compiled into x3d_asset_import and the
# assimp_source_test runs against a committed tiny .obj fixture.
# Build with: cmake -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_ASSIMP=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_ASSIMP "Build the assimp ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_USD "Build the USD ImportSource backend for asset_import (OFF default)" OFF)
option(X3D_CPP_BUILD_CGLTF "Build the cgltf glTF ImportSource backend for asset_import (header-only, MIT; ON default)" ON)

if(X3D_CPP_BUILD_STB)
    add_library(x3d_stb STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb/StbTextureResolver.cpp")
    target_link_libraries(x3d_stb PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stb PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stb")
    # stb_image.h is PRIVATE: only StbTextureResolver.cpp needs it; the factory
    # returns a std::function exchanging std types, so consumers linking x3d_stb
    # inherit no stb headers (decoder-free StbTextureResolver.hpp).
    target_include_directories(x3d_stb PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/poc_renderer/third_party")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stb PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# ---------------------------------------------------------------------------
# TextureResolver decode seam — Backend B (wuffs). ADR-0024 / U2.
# wuffs is a vendored single-file amalgamation; WUFFS_IMPLEMENTATION is defined
# in the one TU (WuffsTextureResolver.cpp) which #includes it PRIVATE, so the
# 1.8 MB codec body compiles into exactly one object file and never leaks.
# Build with: cmake --preset dev -DX3D_CPP_BUILD_WUFFS=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_WUFFS "Build wuffs TextureResolver decode backend (OFF default)" OFF)

if(X3D_CPP_BUILD_WUFFS)
    add_library(x3d_wuffs STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs/WuffsTextureResolver.cpp")
    target_link_libraries(x3d_wuffs PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_wuffs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/wuffs")
    # The wuffs amalgamation lives under runtime/io/wuffs/vendor and is included
    # by the TU relative to its own dir (PRIVATE — decoder-free public header).
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_wuffs PRIVATE -w)  # vendored amalgamation.
    endif()
endif()

# ---------------------------------------------------------------------------
# FontMetrics seam — Backend A (stb_truetype). ADR-0025 / U1.
# option() -> an isolated STATIC lib (x3d_stbtt) whose single TU is
# StbttFontMetrics.cpp; stb_truetype.h is included PRIVATE so it meets the seam
# in ONE TU and never leaks. Build: cmake --preset dev -DX3D_CPP_BUILD_STBTT=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_STBTT "Build stb_truetype FontMetrics backend (OFF default)" OFF)

# The PoC renderer consumes the stbtt glyph atlas (StbttGlyphAtlas) to render
# Text, so build this backend whenever the PoC is on, not only on explicit opt-in.
if(X3D_CPP_BUILD_STBTT OR X3D_CPP_BUILD_POC)
    add_library(x3d_stbtt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/StbttFontMetrics.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/StbttGlyphAtlas.cpp")
    target_link_libraries(x3d_stbtt PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stbtt PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt")
    # stb_truetype.h is PRIVATE: only StbttFontMetrics.cpp needs it; the factory
    # returns a std::function exchanging std types (decoder-free header).
    target_include_directories(x3d_stbtt PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stbtt PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# ---------------------------------------------------------------------------
# MovieDecoder seam — Backend A (pl_mpeg, MPEG-1). ADR-0041.
# option() -> an isolated STATIC lib (x3d_plmpeg) whose single TU is
# PlMpegMovieDecoder.cpp; pl_mpeg.h is included PRIVATE so the codec meets the
# seam in ONE TU and never leaks. MPEG-1 is royalty-free + pl_mpeg is MIT, so it
# ships default-OFF (encumbered codecs plug into the same seam downstream).
# Build: cmake --preset dev -DX3D_CPP_BUILD_PLMPEG=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_PLMPEG "Build pl_mpeg MovieDecoder backend (OFF default)" OFF)

# The PoC renderer consumes the pl_mpeg decoder to play MovieTextures, so build
# this backend whenever the PoC is on, not only on explicit opt-in.
if(X3D_CPP_BUILD_PLMPEG OR X3D_CPP_BUILD_POC)
    add_library(x3d_plmpeg STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/PlMpegMovieDecoder.cpp")
    target_link_libraries(x3d_plmpeg PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_plmpeg PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg")
    # pl_mpeg.h is PRIVATE: only PlMpegMovieDecoder.cpp needs it; the factory
    # returns a std::function exchanging std types (decoder-free header).
    target_include_directories(x3d_plmpeg PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_plmpeg PRIVATE -w)  # pl_mpeg is warning-noisy.
    endif()
endif()

# ---------------------------------------------------------------------------
# MovieDecoder seam — Backend B (libtheora/libogg). ADR-0041.
# The SECOND independent backend — it makes the seam GENERIC (one frozen interface,
# two format-partitioned codecs) via the shared semantics-contract test. Theora is
# a Xiph BSD codec and royalty-free, so it joins the default-shippable tier
# (flag-gated OFF). libtheora/libogg are NOT single-header; they are found via
# pkg-config and linked PRIVATE to the one TU (the FreeType backend-B pattern), so
# no codec header leaks to consumers.
# Build: cmake --preset dev -DX3D_CPP_BUILD_THEORA=ON  (needs libtheora-dev + libogg-dev)
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_THEORA "Build libtheora MovieDecoder backend (OFF default)" OFF)

if(X3D_CPP_BUILD_THEORA)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(THEORADEC REQUIRED IMPORTED_TARGET theoradec)
    add_library(x3d_theora STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora/TheoraMovieDecoder.cpp")
    target_link_libraries(x3d_theora PUBLIC x3d_cpp::x3d_cpp)
    # theoradec.pc pulls in libogg transitively; both stay PRIVATE to this TU.
    target_link_libraries(x3d_theora PRIVATE PkgConfig::THEORADEC)
    target_include_directories(x3d_theora PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora")
endif()

# ---------------------------------------------------------------------------
# FontMetrics seam — Backend B (FreeType). ADR-0025 / U2.
# FreeType is NOT single-header; it is provided via vcpkg (find_package). Its
# headers are PRIVATE to the one TU (FreetypeFontMetrics.cpp), so no FreeType
# header leaks to consumers. Build: -DX3D_CPP_BUILD_FREETYPE=ON with the vcpkg
# toolchain file (see the CI job / spec §6).
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_FREETYPE "Build FreeType FontMetrics backend (OFF default)" OFF)

if(X3D_CPP_BUILD_FREETYPE)
    find_package(Freetype REQUIRED)
    add_library(x3d_freetype STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype/FreetypeFontMetrics.cpp")
    target_link_libraries(x3d_freetype PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_freetype PRIVATE Freetype::Freetype)  # heavy dep PRIVATE
    target_include_directories(x3d_freetype PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype")
endif()

# ---------------------------------------------------------------------------
# U3 — the FontMetrics genericity proof. One grouped doctest binary drives
# identical font fixtures through stb_truetype and FreeType and asserts
# EXACT-equal advanceEm + Failed-parity. Gated on both backends.
# Run: ctest -R x3d_text   (hermetic + offline — local font files only).
# ---------------------------------------------------------------------------
if(TARGET x3d_stbtt AND TARGET x3d_freetype AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_text_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/font_metrics_tests.cpp")
    target_link_libraries(x3d_text_tests PRIVATE x3d_stbtt x3d_freetype)
    target_include_directories(x3d_text_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_text_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts")
    add_test(NAME x3d_text_tests COMMAND x3d_text_tests)
    set_tests_properties(x3d_text_tests PROPERTIES TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# U3 — the TextureResolver decode genericity proof. One grouped doctest binary
# drives identical image fixtures (PNG/BMP/GIF/TGA) through Backend A (stb) and
# Backend B (wuffs) and asserts BYTE-EQUAL RGBA + Failed-parity, plus the U1/U2
# per-backend cases and the U2.5 sniff-dispatch composer. Gated on both backends.
# Build: cmake --preset dev -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON
# Run:   ctest -R x3d_texture   (hermetic + offline — no services, no fixtures
#        to seed; the decoders take local bytes directly).
# ---------------------------------------------------------------------------
if(TARGET x3d_stb AND TARGET x3d_wuffs AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_texture_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/texture_decode_tests.cpp")
    # The test TU talks only through the seam factories (makeStb/Wuffs/...); it
    # links the backend libs but includes NO decoder header, so no heavy backend
    # header can leak in and disagree with the linked lib (the AssetResolver
    # ABI-skew lesson — trivially clean here since the factories exchange std).
    target_link_libraries(x3d_texture_tests PRIVATE x3d_stb x3d_wuffs)
    target_include_directories(x3d_texture_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_texture_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures/texture")
    add_test(NAME x3d_texture_tests COMMAND x3d_texture_tests)
    set_tests_properties(x3d_texture_tests PROPERTIES TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# MovieDecoder seam — genericity proof (ADR-0041). One doctest binary runs the
# SAME semantics-contract (runContract) against each backend's OWN reference clip:
# pl_mpeg / MPEG-1 (Backend A, a 686-byte raw elementary stream — the NIST corpus
# shape) and, when built, libtheora / Ogg-Theora (Backend B). Two backends passing
# the identical contract (Ready dims + tight opaque RGBA, EOF-holds-last, backward-
# seek rewind, Pending/Failed/garbage parity) is what makes the seam GENERIC.
# Hermetic + offline (committed fixtures). Run: ctest -R x3d_movie
# ---------------------------------------------------------------------------
if(TARGET x3d_plmpeg AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_movie_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/movie_decoder_tests.cpp")
    # Talks ONLY through the seam factories (decoder-free headers); no codec header.
    target_link_libraries(x3d_movie_tests PRIVATE x3d_plmpeg)
    target_include_directories(x3d_movie_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_movie_tests PRIVATE
        PLMPEG_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/plmpeg/tests/fixtures")
    # Backend B joins the SAME binary when built -> the genericity proof runs both.
    if(TARGET x3d_theora)
        target_link_libraries(x3d_movie_tests PRIVATE x3d_theora)
        target_include_directories(x3d_movie_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora")
        target_compile_definitions(x3d_movie_tests PRIVATE
            X3D_MOVIE_HAVE_THEORA=1
            THEORA_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/theora/tests/fixtures")
    endif()
    add_test(NAME x3d_movie_tests COMMAND x3d_movie_tests)
    set_tests_properties(x3d_movie_tests PROPERTIES TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# SDK façade examples (examples/0[123]_*.cpp). Headless, std-only, link
# x3d_cpp::sdk. ON when this is the top-level project so the v1 examples build +
# run as ctests; a downstream consumer that pulls x3d_cpp in gets them OFF.
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_EXAMPLES "Build + run the x3d::sdk façade examples" ${PROJECT_IS_TOP_LEVEL})

if(X3D_CPP_BUILD_TESTS)
    enable_testing()

    set(X3D_CPP_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings")

    # Collect all generated headers. CONFIGURE_DEPENDS re-globs on rebuild if
    # the set of headers changes.
    file(GLOB_RECURSE X3D_CPP_HEADERS CONFIGURE_DEPENDS
        "${X3D_CPP_HEADER_DIR}/x3d/*.hpp")
    list(LENGTH X3D_CPP_HEADERS X3D_CPP_HEADER_COUNT)
    message(STATUS "x3d_cpp: found ${X3D_CPP_HEADER_COUNT} generated headers to compile-test")

    # Hand-written runtime document-model headers (X3DDocument, Scene, Route,
    # PROTO, IMPORT/EXPORT). Compiled alongside the generated headers to prove
    # the document model stays consistent with the generated node bindings.
    set(X3D_CPP_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
    file(GLOB X3D_CPP_RUNTIME_HEADERS CONFIGURE_DEPENDS "${X3D_CPP_RUNTIME_DIR}/*.hpp")
    list(LENGTH X3D_CPP_RUNTIME_HEADERS X3D_CPP_RUNTIME_HEADER_COUNT)
    message(STATUS "x3d_cpp: found ${X3D_CPP_RUNTIME_HEADER_COUNT} runtime headers to compile-test")

    # Hand-written node-agnostic codecs (XML reader/writer, JSON/VRML writers)
    # plus the bundled XmlLite. Compiled in isolation like the runtime headers.
    set(X3D_CPP_CODEC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs")
    file(GLOB X3D_CPP_CODEC_HEADERS CONFIGURE_DEPENDS "${X3D_CPP_CODEC_DIR}/*.hpp")
    list(LENGTH X3D_CPP_CODEC_HEADERS X3D_CPP_CODEC_HEADER_COUNT)
    message(STATUS "x3d_cpp: found ${X3D_CPP_CODEC_HEADER_COUNT} codec headers to compile-test")

    # Hand-written event-system runtime (cascade engine, field address, event
    # graph, execution context, active-node protocol, and reference behaviors).
    set(X3D_CPP_EVENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events")
    file(GLOB X3D_CPP_EVENT_HEADERS CONFIGURE_DEPENDS "${X3D_CPP_EVENT_DIR}/*.hpp")
    list(LENGTH X3D_CPP_EVENT_HEADERS X3D_CPP_EVENT_HEADER_COUNT)
    message(STATUS "x3d_cpp: found ${X3D_CPP_EVENT_HEADER_COUNT} event-system headers to compile-test")

    # Hand-written unified parsing front-end (X3DReader interface, encoding
    # sniffing, shared NodeBuilder + VrmlTokenizer, XmlReaderAdapter, X3DParse
    # front door). Compiled in isolation like the codecs/events headers.
    set(X3D_CPP_PARSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse")
    file(GLOB X3D_CPP_PARSE_HEADERS CONFIGURE_DEPENDS "${X3D_CPP_PARSE_DIR}/*.hpp")
    list(LENGTH X3D_CPP_PARSE_HEADERS X3D_CPP_PARSE_HEADER_COUNT)
    message(STATUS "x3d_cpp: found ${X3D_CPP_PARSE_HEADER_COUNT} parse front-end headers to compile-test")

    # (1) Aggregate target: one .cpp that #includes all generated headers plus
    #     the runtime umbrella and builds it. This proves the whole set is
    #     mutually consistent in one TU.
    set(X3D_CPP_ALL_HEADERS_CPP "${CMAKE_CURRENT_BINARY_DIR}/x3d_cpp_all_headers.cpp")
    set(_x3d_all_content "// Auto-generated by CMake. Includes every generated header.\n")
    foreach(_hdr IN LISTS X3D_CPP_HEADERS)
        file(RELATIVE_PATH _hdr_rel "${X3D_CPP_HEADER_DIR}" "${_hdr}")
        string(APPEND _x3d_all_content "#include \"${_hdr_rel}\"\n")
    endforeach()
    # Pull in the full runtime document model + the node-agnostic codecs on top
    # of the generated nodes.
    string(APPEND _x3d_all_content "#include \"X3DRuntime.hpp\"\n")
    string(APPEND _x3d_all_content "#include \"X3DCodecs.hpp\"\n")
    # Pull in the unified parsing front-end umbrella so it is proven consistent
    # with the generated nodes + codecs in the same TU.
    string(APPEND _x3d_all_content "#include \"X3DParse.hpp\"\n")
    # Pull in the event-system runtime (cascade engine, execution context,
    # reference behaviors) so it is proven consistent in the same TU too.
    foreach(_hdr IN LISTS X3D_CPP_EVENT_HEADERS)
        get_filename_component(_hdr_name "${_hdr}" NAME)
        string(APPEND _x3d_all_content "#include \"${_hdr_name}\"\n")
    endforeach()
    string(APPEND _x3d_all_content "int main() { return 0; }\n")
    file(GENERATE OUTPUT "${X3D_CPP_ALL_HEADERS_CPP}" CONTENT "${_x3d_all_content}")

    add_executable(x3d_cpp_all_headers "${X3D_CPP_ALL_HEADERS_CPP}")
    target_link_libraries(x3d_cpp_all_headers PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_cpp_all_headers COMMAND x3d_cpp_all_headers)

    # (2) Header-isolation contract. Keep one generated translation unit per
    #     public header so diagnostics remain pinned to the exact include, but
    #     put all of those sources in one OBJECT library. This removes hundreds
    #     of CMake/Ninja/CTest targets without weakening header self-containment.
    if(X3D_CPP_PER_HEADER_CHECKS)
        set(X3D_CPP_HEADER_ISOLATION_SOURCES)

        function(_x3d_add_header_isolation_source group include_root header)
            file(RELATIVE_PATH _header_include "${include_root}" "${header}")
            string(REGEX REPLACE
                "[^A-Za-z0-9_]" "_" _source_name "${_header_include}")
            set(_source_dir
                "${CMAKE_CURRENT_BINARY_DIR}/header_isolation/${group}")
            file(MAKE_DIRECTORY "${_source_dir}")
            set(_source "${_source_dir}/${_source_name}.cpp")
            file(GENERATE OUTPUT "${_source}"
                CONTENT "#include \"${_header_include}\"\n")
            set(X3D_CPP_HEADER_ISOLATION_SOURCES
                ${X3D_CPP_HEADER_ISOLATION_SOURCES} "${_source}"
                PARENT_SCOPE)
        endfunction()

        foreach(_hdr IN LISTS X3D_CPP_HEADERS)
            _x3d_add_header_isolation_source(
                generated "${X3D_CPP_HEADER_DIR}" "${_hdr}")
        endforeach()
        foreach(_hdr IN LISTS X3D_CPP_RUNTIME_HEADERS)
            _x3d_add_header_isolation_source(
                runtime "${X3D_CPP_RUNTIME_DIR}" "${_hdr}")
        endforeach()
        foreach(_hdr IN LISTS X3D_CPP_CODEC_HEADERS)
            _x3d_add_header_isolation_source(
                codecs "${X3D_CPP_CODEC_DIR}" "${_hdr}")
        endforeach()
        foreach(_hdr IN LISTS X3D_CPP_EVENT_HEADERS)
            _x3d_add_header_isolation_source(
                events "${X3D_CPP_EVENT_DIR}" "${_hdr}")
        endforeach()
        foreach(_hdr IN LISTS X3D_CPP_PARSE_HEADERS)
            _x3d_add_header_isolation_source(
                parse "${X3D_CPP_PARSE_DIR}" "${_hdr}")
        endforeach()

        add_library(x3d_header_isolation OBJECT EXCLUDE_FROM_ALL
            ${X3D_CPP_HEADER_ISOLATION_SOURCES})
        x3d_set_target_purpose(x3d_header_isolation compile-contract)
        target_link_libraries(x3d_header_isolation PRIVATE x3d_cpp::x3d_cpp)
        add_test(NAME x3d_header_isolation
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
                    --target x3d_header_isolation --config "$<CONFIG>")
    endif()

    # (5) Stage C codec integration test: build the sample scene, round-trip it
    #     through X3D-XML (model -> XML -> model -> XML, stable modulo
    #     whitespace), assert DEF/USE identity + ROUTE survival, and emit JSON +
    #     ClassicVRML. Built and RUN (not just compiled).

    # AUD-RTC: comprehensive codec round-trip audit via fixture files (Unit 17).
    add_executable(x3d_codec_roundtrip_audit
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/codec_roundtrip_audit_test.cpp")
    target_link_libraries(x3d_codec_roundtrip_audit PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_codec_roundtrip_audit
        COMMAND x3d_codec_roundtrip_audit
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/proto"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/json"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/x3dv"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/wrl")

    # Regression for the corpus-audit Family-A finding: VRML writer must bracket MF.

    # Regression for corpus-audit Family-B: un-expanded ProtoInstances must round-trip.

    # Regression for corpus-audit Family-C: ProtoBody DEFs must not leak to scene scope.

    # Regression for corpus-audit Family-D: quoted MFEnum/SFEnum tokens must not drop.


    # Proto round-trip (Task 12): parseDocument expands a <ProtoInstance> to its
    # primary node + records expandedSources; the XML writer must re-emit the
    # original <ProtoInstance> and suppress the expanded subtree.


    # PRF-2: an IS/connect on a body node nested deeper than the direct body
    # child must survive read -> write -> re-read (recursive attach/collect).

    # PRF-1: IS connections in a ProtoDeclare body must be re-emitted by the JSON
    # and ClassicVRML writers and survive a read -> write -> re-read cycle.


    # PRF-4: @appinfo / @documentation on ProtoDeclare and ExternProtoDeclare must
    # survive a JSON read -> write -> re-read cycle (mirroring the XML codec).

    # PRF-5: an ExternProtoDeclare @url with multiple elements (JSON array form)
    # must survive a JSON read -> write -> re-read cycle (mirroring the XML codec).

    # PRF-3: a Case-A nested ProtoInstance (nested under a parent body node) must
    # be re-emitted by the JSON/VRML writers INSIDE that parent node's child slot
    # (mirroring XmlWriter), preserving its parent linkage across a round trip.

    # Scene-level nested ProtoInstance round-trip: an unexpanded ProtoInstance
    # sitting inside a non-root node field (e.g. Shape geometry) must survive
    # serialisation in all three writers (was silently dropped before the fix).


    # ------------------------------------------------------------------------
    # S1 dynamic-field foundation (file-authored Script un-tabling, Phase 0).
    # ------------------------------------------------------------------------
    # Phase-0 unit test: AuthorFieldDecl/DynamicFieldStore round-trip,
    # effectiveFields() concatenation, and an author-field ROUTE wiring +
    # delivering end-to-end through X3DSceneBridge.

    # AUD-MEM: DynamicFieldStore std::any type safety & cleanup audit (Unit 15).

    # Phase-1 test targets, PRE-REGISTERED here in Phase 0 so the parallel wave
    # never touches CMakeLists.txt (the single shared-write hazard). Each points
    # at a file the corresponding Phase-1 agent will fill; the files currently
    # hold a trivial `int main(){return 0;}` placeholder so the build/ctest suite
    # stays green until Phase 1 replaces the body. The reader tests are passed
    # their source-tree fixture dir as argv[1] (Phase-1 agents drop fixtures
    # there); the runtime test links the real EcmaScript backend (registered
    # after that library is defined, below).
    #   A. XML capture
    add_executable(x3d_xml_script_field
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_script_field_test.cpp")
    target_link_libraries(x3d_xml_script_field PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_xml_script_field
        COMMAND x3d_xml_script_field
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/data")
    #   A'. XML ComposedShader capture (author <field>s + ShaderPart source)
    add_executable(x3d_xml_composed_shader
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_composed_shader_test.cpp")
    target_link_libraries(x3d_xml_composed_shader PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_xml_composed_shader COMMAND x3d_xml_composed_shader)
    #   B. ClassicVRML / VRML97 capture
    add_executable(x3d_vrml_script_field
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/vrml_script_field_test.cpp")
    target_link_libraries(x3d_vrml_script_field PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_vrml_script_field
        COMMAND x3d_vrml_script_field
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data")
    #   C. JSON capture
    add_executable(x3d_json_script_field
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/json_script_field_test.cpp")
    target_link_libraries(x3d_json_script_field PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_json_script_field
        COMMAND x3d_json_script_field
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data")
    #   E. RTC-5 + RTC-6 cascade conformance — registered below alongside the
    #      other event-cascade integration tests (see x3d_cascade_conformance).

    # (3) Event-system integration tests, built and RUN (not just compiled):
    #     - cascade_test: route propagation, fan-out, loop-breaking, inputOnly
    #       handler delivery, outputOnly emit cascading.
    #     - animation_test: a TimeSensor -> PositionInterpolator -> Transform
    #       chain driven by the clock, end-to-end through every layer.










    #   - cascade_conformance: RTC-5 per-field event cap (fan-in delivers once,
    #     cyclic re-drive broken) + RTC-6 §4.4.8.3 step-4 tick re-eval loop
    #     (sensor->route->sensor chain resolves in one tick). See BACKLOG RTC-5/6.

    # AUD-CAS: inputOutput field alias resolution per §4.4.2.2.


    # M2C-4: HAnimHumanoid, HAnimJoint, CADPart treated as transform-bearing.







    # M2.5 T0: extract/RenderItem.hpp descriptor layer (pure-POD compile gate).

    # M2.5 T5: extract/MaterialSystem.hpp — Appearance -> MaterialDesc + textures.

    # MAT-007: TextureRef::texCoordMapping (X3D v4 xxxTextureMapping).

    # MAT-008: ORM single-slot + channel-packing documentation pin.

    # M2.5 T2: extract/MeshBuilder.hpp — strips/fans/quads + ElevationGrid with
    # per-triangle strip winding flip + ok()-guards on every index/count array.

    # M2.5 T3: extract/MeshBuilder.hpp — Normal/Color/ColorRGBA/TextureCoordinate
    # resolution honoring *PerVertex + *Index, SFColor->SFColorRGBA promotion, and
    # FLAT per-face normal generation when no Normal node is authored.

    # M2.5 T4: extract/MeshBuilder.hpp — parametric tessellation of the analytic
    # primitives Box/Sphere/Cone/Cylinder (density-driven, CCW outward normals).

    # Browser-level B5: GeoElevationGrid branch + GeoProjection embedder seam.
    # Flat-fallback emits geometry with NO projection wired (the ~72% corpus
    # unlock); both grid emitters retain a lattice-index map (for B6); a wired
    # GeoProjection std::function on MeshBuildOptions is honored per lattice vertex.

    # Browser-level B3: Extrusion tessellation. Sweeps a 2D crossSection along a
    # 3D spine via the Spine-aligned Cross-section frame; emits side walls + caps
    # in lattice-index-retaining form (for B6); winding honors ccw.

    # Browser-level B4: MeshData Topology enum + IndexedLineSet/LineSet/PointSet
    # flow through the seam as Lines/Points, always-unlit (hasNormals=false,
    # solid=false), vertex Color honored. recognizedGeometryType lists them.

    # Browser-level B6: creaseAngle smooth-normal generation. A creaseAngle-aware
    # post-pass replaces the flat per-face fill when no Normal is authored,
    # re-deriving adjacency in SOURCE-index space (coordIndex for IFS, lattice id
    # for ElevationGrid/Extrusion/GeoElevationGrid). creaseAngle=0 is byte-identical
    # to the flat fill (non-regressive); PI fully smooths shared verts.

    # TC1: default (implicit) texture-coordinate generation. When a composed/
    # triangle geometry node carries NO TextureCoordinate, MeshBuilder generates
    # the ISO/IEC 19775-1 normative bounding-box-projection UVs (largest dim = S,
    # second-largest = T, both divided by the largest dim to preserve aspect) so a
    # consumer's texture path works. An authored TextureCoordinate ALWAYS wins
    # (byte-unchanged).

    # TC2: DEFAULT (implicit) texcoords for the height grids (ElevationGrid /
    # GeoElevationGrid) by the NORMATIVE grid parameterization (s=i/(xDim-1),
    # t=j/(zDim-1)) when texCoord is NULL. An authored TextureCoordinate ALWAYS
    # wins (resolved per lattice vertex via the retained latticeIndex map).

    # TC3: DEFAULT (implicit) texcoords for Extrusion (which has NO authored
    # TextureCoordinate field — implicit-only per ISO/IEC 19775-1). SIDE walls:
    # S = accumulated cross-section CHORD length / perimeter, T = accumulated
    # SPINE length / total spine length. begin/end CAPS: cross-section's own 2D
    # coords normalized to the cross-section bbox. Guards zero-length spine/cs.

    # M2.5 TC4: DEFAULT (implicit) texcoords for the analytic primitives
    # (Box/Sphere/Cone/Cylinder) — these carry NO texCoord field, so the
    # per-primitive NORMATIVE mapping is generated during tessellation alongside
    # positions+normals: Box per-face unit square; Sphere s=lon/2pi, t=lat
    # (0 at -Y pole .. 1 at +Y pole); Cylinder/Cone side s=angle/2pi, t along
    # axis, caps radial in [0,1]^2.

    # TXC-1: spec-exact seam placement for Sphere / Cone / Cylinder default UVs.
    # ISO/IEC 19775-1 §13.3.7/§13.3.2/§13.3.3: the seam is at the BACK of each
    # primitive (X=0 plane, Z<0), S=0 there, wrapping CCW from above. Cap
    # orientations per spec ("right side up when tilted toward ±Z"). Box is
    # already spec-exact (§13.3.1) and is not modified.

    # T-TEX (v1-closure): textures end-to-end — TextureExtract.hpp. Authored
    # TextureCoordinate extraction (MeshBuilder, §13.3.6) + TextureTransform bake
    # into mesh texcoords (§18.4.10) + ExtendedSampler/TextureProperties boundary+
    # filter modes (§18.4.9) + TextureCoordinateGenerator descriptor (§18.4.8) +
    # the TextureResolver round-trip threading resolvedPixels (Url through the
    # resolver, Inline/Movie skipped).

    # T-TEXT: the pure §15 Text layout engine (justify/spacing/maxExtent/length/
    # bounds/origin) with a monospaced font-metrics stub — IO-free layout math.

    # T-TEXT: Text extraction — buildTextMesh glyph quads via the font-metrics
    # seam + setTextOutputs (textBounds/lineBounds/origin) + the SceneExtractor
    # dispatch hook + an atlas-UV seam round-trip.

    # Browser-level B8: asset-resolver seam (the BYTES path). ONE AssetResolver
    # std::function type, TWO documented invocation contracts (render-time texture
    # MAY return Pending; parse-time Inline/EXTERNPROTO resolves SYNC). The SDK
    # never does I/O — TextureRef carries URLs verbatim / pixels inline; the
    # consumer (PoC) owns the file resolver + decode.

    # M2.5 T7a: minimal SceneExtractor — the M2C-1 per-path acceptance proof.

    # M2.5 T7: SceneExtractor FULL form — visibility-aware DFS (Switch/LOD) +
    # MeshBuilder[T4]/MaterialSystem[T5]/LightSystem[T6] wired + reverse indices +
    # lights()/background()/OrthoViewpoint read-outs.

    # M2.5 T8: extract/SceneExtractor.hpp delta() incremental engine — transform-
    # only tick yields ONLY updatedTransform (per-path re-accumulation, no re-walk);
    # parent scale does NOT pollute updatedGeometry; material-color => updatedMaterial;
    # DirtyChildren => added/removed via the cached entry-matrix subtree re-walk.

    # Browser-level B2: unsupported-geometry signal — the silent empty-mesh drop in
    # SceneExtractor::walk() becomes an observable pull channel. An UNRECOGNIZED
    # geometry type (Extrusion) increments skippedGeometryCounts(); a recognized-
    # but-legitimately-empty one (empty-coordIndex IFS) does NOT.

    #     - scene_extractor_col2_test: COL-2 — Collision.proxy geometry is
    #       collision-only and must never be emitted as a render item (§23.4.2).

    # M2.5 T6: extract/LightSystem.hpp — world-resolved lights + authored-flag
    # fidelity (DirectionalLight global=false carried, not promoted).

    # M25-5: SceneExtractor tags each RenderItem with in-scope lights.
    # Non-global (scoped) DirectionalLight illuminates only geometry inside its
    # parent grouping node; global lights tag all items. Closure of M25-5.

    # AUD-EXTRACT (Unit 13): regression audit of the scene extraction walk.

    # AUD-RENDER-FEED (Unit 14): regression audit of render-feed consistency.

    # Phase 1 binary geometry abstractions: PackedMesh + TextureDesc + Geometry union + seam.




    # M2.5 T10 (FIRST LIGHT): the shipped first-light asset parses through the
    # real front door and the minimal SceneExtractor yields a 3-vertex mesh.
    # This is the headless, NO-GL half of T10's acceptance; the on-screen
    # triangle is the user's visual verify. Runs in the default ctest path (no
    # display needed) — the GL renderer itself stays behind X3D_CPP_BUILD_POC.








    # M2C-3: dirty-aware writeField helper (ctx.writeField bypasses the cascade
    # but still classifies + marks the node dirty, identical to the cascade path).


    # AUD-EXEC-TICK: tick-loop correctness and edge-case regression tests.

    # M2.5 T2: PointerState holder + X3DExecutionContext input setters

    # M2.5 T3: PointingSensorSystem driving a fully spec-complete TouchSensor
    # on the input seam (isOver/isActive/touchTime/hitPoint/hitNormal/hitTexCoord).



    # Task 3: NavigationSystem honors the pointer-consumed arbitration flag
    # (sensor grab suppresses nav drag) and accepts a forced-mode override.

    # Task 4: attachInteractive helper + headless interaction acceptance gate.
    # Proves PointingSensorSystem + NavigationSystem wire up in arbitration order
    # so a click fires a TouchSensor and WASD drives FLY navigation.

    # M2D: pure drag-sensor math (PlaneSensor/SphereSensor/CylinderSensor),
    # ISO/IEC 19775-1 §20.4.x + §20.2.2. Codegen-free, no node/System coupling.

    # M2D U3 (PDS-4): KeyState holder + X3DExecutionContext keyboard seam.

    # M2D NavigationSystem: interactive EXAMINE/FLY/LOOKAT/NONE (collision-free)
    # mutating the bound Viewpoint from the input seam (ISO/IEC 19775-1 §23.x).

    # §37 contact reporting: ContactReporter translates resolved collisions into
    # CollisionSensor.contacts/intersections/isActive events (CONF-RBP). Ungated —
    # the reporter is engine-free; only the Jolt drain path needs the physics flag.
    add_executable(x3d_contact_reporter
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/contact_reporter_test.cpp")
    target_link_libraries(x3d_contact_reporter PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_contact_reporter PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_contact_reporter COMMAND x3d_contact_reporter)

    # PhysicsSystem contact wiring: drain -> resolve handles -> ContactReporter.
    # Ungated (a fake backend scripts a contact); proves the engine-free wiring.
    add_executable(x3d_physics_contact_wiring
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/physics_contact_wiring_test.cpp")
    target_link_libraries(x3d_physics_contact_wiring PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_physics_contact_wiring PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_physics_contact_wiring COMMAND x3d_physics_contact_wiring)

    # §37 inertia/centerOfMass: MassProperties seam + PhysicsSystem detection.
    # Ungated — the detection is engine-free; only the Jolt mapping needs the flag.
    add_executable(x3d_inertia_massprops
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/inertia_massprops_test.cpp")
    target_link_libraries(x3d_inertia_massprops PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_inertia_massprops PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_inertia_massprops COMMAND x3d_inertia_massprops)

    # §37 contact-response params: PhysicsSystem reads CollisionCollection
    # frictionCoefficients/bounce and forwards them to setContactResponse with
    # non-default detection. Ungated — engine-free; uses a recording fake backend.
    add_executable(x3d_contact_response
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/contact_response_test.cpp")
    target_link_libraries(x3d_contact_response PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_contact_response PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_contact_response COMMAND x3d_contact_response)

    #     - viewpoint_offset_test: CONF-VIEWNAV Phase 1 — the user-offset model.
    #       Navigation accumulates a process-local offset (+ head-pose seam) instead
    #       of mutating the authored Viewpoint pose (§23.2.3, BIND-01/03).

    #     - viewpoint_bind_test: CONF-VIEWNAV Phase 2 — bind-time offset rules
    #       (jump / retainUserOffsets / stored relative transform; BIND-04/07/08).


    #     - interpolator_test: per-interpolator cascade tests for the
    #       interpolator-System family (Scalar, Position, Position2D, Color/HSV,
    #       Orientation/SLERP, Coordinate, Coordinate2D, Normal). Each drives a
    #       fraction at 0/0.5/1 + one off-key value and asserts the emitted
    #       value_changed; a final multi-interpolator scene shares one source.

    #     - x3d_event_followers: FollowerArith<T> trait conformance (§39 Followers).
    #       Covers all 7 value types: float, SFVec2f, SFVec3f, SFColor, SFRotation,
    #       MFVec3f, MFVec2f — lerp boundary conditions + dist non-negativity.

    #     - interpolator_conformance_test: campaign wave-2 interpolator gaps.
    #       INTERP-02 empty-key-emits-no-event guard; INTERP-01 Spline Hermite
    #       (Scalar/Position/Position2D), SquadOrientationInterpolator, and
    #       EaseInEaseOut Systems; PIV-1 attachInterpolators production wiring.

    #     - event_utility_test: campaign wave-3 fix cycle. §30 Event Utilities
    #       Systems — BooleanTrigger/IntegerTrigger/TimeTrigger, Boolean/Integer
    #       Sequencer (stepwise select + next/previous), BooleanFilter/BooleanToggle,
    #       and attachEventUtilities production wiring.

    #     - key_device_sensor_test: campaign wave-4 fix cycle. §21 KeySensor +
    #       StringSensor Systems over the extended KeyState seam (keyPress/Release,
    #       actionKey, modifiers, isActive; enteredText/finalText accumulator) +
    #       attachKeyDeviceSensors production wiring.

    #     - scene_bridge_test: the Scene -> EventGraph bridge. Validates ROUTE
    #       resolution (DEF names -> FieldAddress) and rejection of unknown-field
    #       / wrong-direction / type-mismatch routes (dangling DEFs skipped), then
    #       parses a .x3dv document with ROUTEs, bridges it via buildFrom, attaches
    #       the reference behaviors to the DEF-resolved nodes, and ticks the clock
    #       end-to-end. The .x3dv fixtures dir is passed as argv[1] so the test
    #       runs from any working directory.
    add_executable(x3d_event_scene_bridge
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/scene_bridge_test.cpp")
    target_link_libraries(x3d_event_scene_bridge PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_event_scene_bridge
        COMMAND x3d_event_scene_bridge
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/x3dv")

    # AUD-NODE-LIFECYCLE: node creation / destruction / ownership audit.

    #     - timesensor_test: the full TimeSensor / X3DTimeDependentNode lifecycle
    #       (TimeSensorSystem over X3DTimeDependentSystem) driven through the
    #       clock. Exercises the activation/deactivation edges and continuous
    #       outputs: enabled gating, startTime activation, single-shot completion
    #       + deactivation, looping fraction wrap with one cycleTime pulse per
    #       cycle boundary, stopTime early stop, pause/resume elapsed-time
    #       freezing, and the cycleInterval<=0 guard. Built and RUN.

    # Conformance regression tests from the 2026-06-16 RAG audit fixes:
    # RTC-1..4 (TimeSensor), CDC-1..4 (codec), MSH-1..3 (mesh geometry),
    # RTC-7 (dynamic-route mutation), RTC-8 (slerpNormal antipodal guard).


    # AUD-FVAL-EXT: extended FieldValueIO coverage (matrices, images, RGBA, MF
    # strings, enums, locale, rotations).

    # AUD-SCR-CDATA: Script node CDATA wrapper audit.


    #     - mesh_builder_ext002_test: EXT-002 — fan/strip sets apply one color/
    #       normal per fan/strip (not per triangle) when colorPerVertex=FALSE.



    #     - sai_context_test: M4 Track-A unit. The language-agnostic Script
    #       runtime foundation (runtime/script/): the ScriptEngine seam +
    #       in-process SaiContext + a recording MockScriptEngine backend — NO
    #       JavaScript. Asserts field get/set round-trips, the directOutput
    #       write-gate (cross-node write rejected when FALSE, allowed via the
    #       cascade when TRUE), addRoute/deleteRoute gating + effect in the
    #       graph, browser-info getters, and seam-call lifecycle ordering.
    add_executable(x3d_sai_context
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/sai_context_test.cpp")
    target_link_libraries(x3d_sai_context PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_sai_context COMMAND x3d_sai_context)

    #     - script_system_test: M4 Track-A unit. ScriptSystem — the System that
    #       wires Script lifecycle (load/initialize/shutdown) + event delivery
    #       (inputOnly -> engine.invoke, prepareEvents/eventsProcessed phasing,
    #       outputOnly -> cascade) into the tick/cascade, mock-tested (NO JS).
    #       Asserts the §29.2 ordering: initialize before first event;
    #       prepareEvents before routes; outputOnly at the triggering timestamp;
    #       eventsProcessed once per batch; set_url reload; directOutput gate;
    #       mustEvaluate eager-vs-lazy delivery.
    add_executable(x3d_script_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/script_system_test.cpp")
    target_link_libraries(x3d_script_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_script_system COMMAND x3d_script_system)

    # ---------------------------------------------------------------------------
    # M4 Track-B: Duktape ECMAScript backend (U3 skeleton).
    #
    # Duktape 2.7.0 is vendored at runtime/script/vendor/duktape/ (MIT license).
    # It is a plain C source file; compile it as C (not C++) so it picks up its
    # own duk_config.h without any C++ name-mangling. Isolated into a static lib
    # (x3d_duktape) so the rest of the build is completely unaffected — nothing
    # except the ECMAScript backend target links it.
    # ---------------------------------------------------------------------------
    add_library(x3d_duktape STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c")
    # Compile as C (it must not be parsed as C++).
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape/duktape.c"
        PROPERTIES LANGUAGE C)
    target_include_directories(x3d_duktape PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")
    # Keep Duktape warnings from polluting the main build.
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_duktape PRIVATE -w)
    endif()

    # EcmaScriptBackend: the Duktape-backed ScriptEngine implementation.
    add_library(x3d_ecmascript_backend STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/EcmaScriptBackend.cpp")
    target_link_libraries(x3d_ecmascript_backend PUBLIC
        x3d_cpp::x3d_cpp
        x3d_duktape)
    target_include_directories(x3d_ecmascript_backend PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/vendor/duktape")

    #     - ecmascript_backend_test: M4 Track-B unit. Proves the full backend:
    #       field marshalling std::any<->JS (all core SF/MF types), the Browser
    #       global object (getName/getVersion/currentTime/addRoute/print backed
    #       by SaiContext), handler dispatch with (value, timestamp), an
    #       end-to-end ROUTE wired by the script via Browser.addRoute, and the
    #       directOutput gate throwing into JS.
    add_executable(x3d_ecmascript_backend_test
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/ecmascript_backend_test.cpp")
    target_link_libraries(x3d_ecmascript_backend_test PRIVATE
        x3d_ecmascript_backend)
    add_test(NAME x3d_ecmascript_backend COMMAND x3d_ecmascript_backend_test)

    #     - ecmascript_corpus_smoke_test: M4 Track-B corpus smoke. Drives
    #       parse -> expand -> attach ScriptSystem(EcmaScriptBackend) -> tick on
    #       an authored known scene (deterministic Browser.print) and a few real
    #       Script-bearing .x3d from testdata (no-crash robustness).
    add_executable(x3d_ecmascript_corpus_smoke_test
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/ecmascript_corpus_smoke_test.cpp")
    target_link_libraries(x3d_ecmascript_corpus_smoke_test PRIVATE
        x3d_ecmascript_backend)
    add_test(NAME x3d_ecmascript_corpus_smoke
        COMMAND x3d_ecmascript_corpus_smoke_test)

    #   D. Runtime wiring (Phase-1 placeholder pre-registered in Phase 0).
    #      Source-from-sourceCode, author-event dispatch, author-field
    #      marshalling + an end-to-end "author field drives a ROUTE" test. Links
    #      the real EcmaScript backend (defined just above) so the Phase-1 agent
    #      that fills this file needs no CMake edit. Currently a trivial main.
    add_executable(x3d_script_author_runtime
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/script_author_runtime_test.cpp")
    target_link_libraries(x3d_script_author_runtime PRIVATE
        x3d_ecmascript_backend)
    add_test(NAME x3d_script_author_runtime COMMAND x3d_script_author_runtime)

    #   Phase-2 CORPUS END-TO-END proof: a FILE-AUTHORED Script that v1 shipped
    #   INERT now actually executes through the real reader -> expand -> attach ->
    #   tick path (XML: the Savage CanopyExample.x3d; ClassicVRML: an authored
    #   .x3dv fixture). Closes SCR-SAI-DYN (S1) behaviorally. The data dir arg
    #   locates the .x3dv fixture; the .x3d is the absolute corpus path.
    add_executable(x3d_script_corpus_e2e
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/script_corpus_e2e_test.cpp")
    target_link_libraries(x3d_script_corpus_e2e PRIVATE
        x3d_ecmascript_backend)
    add_test(NAME x3d_script_corpus_e2e
        COMMAND x3d_script_corpus_e2e
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/data")

    # (4) Parse front-end integration test (WS-A), built and RUN (not just
    #     compiled): encoding sniffing (extension + content + override + gzip),
    #     the shared VrmlTokenizer, NodeBuilder::collectFieldValue value
    #     round-trips, and the XmlReaderAdapter / parseDocument front door
    #     reproducing the raw XmlReader.
    #     Fix 1 (parser-gap plan) adds testGzipInflate(): the data dir is passed
    #     as argv[1] so parseFile() can round-trip the committed gzip fixtures
    #     (runtime/parse/tests/data/gzip/) against their uncompressed siblings.
    add_executable(x3d_parse_reader
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/reader_test.cpp")
    target_link_libraries(x3d_parse_reader PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_parse_reader
        COMMAND x3d_parse_reader
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data")

    # (10) ClassicVRML (.x3dv) reader test (WS-B), built and RUN. Parses the four
    #      corpus .x3dv fixtures (copied into runtime/parse/tests/data/x3dv/) plus
    #      hand-authored grammar snippets into a Scene and asserts node types,
    #      field values, DEF/USE shared identity, ROUTEs, header statements, MF
    #      re-join, and PROTO/EXTERNPROTO/instance/IS capture. The fixtures dir is
    #      passed as argv[1] so the test runs from any working directory.
    add_executable(x3d_classic_vrml_reader
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/classic_vrml_reader_test.cpp")
    target_link_libraries(x3d_classic_vrml_reader PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_classic_vrml_reader
        COMMAND x3d_classic_vrml_reader
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/x3dv")



    # Front-door PROTO expansion (Task 10): parseDocument/parseFile drive
    # expandScene with the default local-file resolver. Asserts local-PROTO
    # in-memory expansion, sibling-file EXTERNPROTO resolution (baseUrl derived
    # from the file path), and termination of a mutually-referential EXTERN
    # cycle. The fixtures dir is passed as argv[1] so it runs from any cwd.
    add_executable(x3d_proto_front_door
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_front_door_test.cpp")
    target_link_libraries(x3d_proto_front_door PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_proto_front_door
        COMMAND x3d_proto_front_door
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/proto")




    # AUD-PROTO-EXP regression: PROTO expansion + IS routing + depth guard +
    # redirect-validation. The fixtures dir is passed as argv[1] so the test
    # runs from any cwd (default: runtime/parse/tests/data/proto).
    add_executable(x3d_proto_expand_audit
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_expand_audit_test.cpp")
    target_link_libraries(x3d_proto_expand_audit PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_proto_expand_audit
        COMMAND x3d_proto_expand_audit
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/proto")

    # Task 1 (Inline expansion): smoke test that data carriers compile + are
    # default-empty (InlineWarning, Scene::expandedInlines, resolvedInlineRoutes,
    # X3DDocument::inlineWarnings).

    # Task 2 (Inline expansion): hermetic unit test for expandInlines — Group splice,
    # DEF isolation, synthetic Group->expandedInlines mapping, injected resolver.

    # Task 3 (Inline round-trip): writer redirect — re-emit <Inline url=.../>
    # via expandedInlines and do NOT serialize the synthetic Group's child content.
    add_executable(x3d_inline_roundtrip
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_roundtrip_test.cpp")
    target_link_libraries(x3d_inline_roundtrip PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_roundtrip
        COMMAND x3d_inline_roundtrip
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")

    # Task 4 (Inline route hoisting): resolvedInlineRoutes populated by
    # expandInlines and registered on the execution context by the scene-bridge,
    # so self-animating inlined assets tick.
    add_executable(x3d_inline_routes
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_routes_test.cpp")
    target_link_libraries(x3d_inline_routes PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_routes
        COMMAND x3d_inline_routes
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")

    add_executable(x3d_inline_cycle
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_cycle_test.cpp")
    target_link_libraries(x3d_inline_cycle PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_cycle
        COMMAND x3d_inline_cycle
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")

    # SEC-5: inflateGzip caps decompressed output (gzip decompression-bomb DoS).
    add_executable(x3d_inflate_bomb
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inflate_bomb_test.cpp")
    target_link_libraries(x3d_inflate_bomb PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inflate_bomb COMMAND x3d_inflate_bomb)

    # SEC-3/SEC-4: confineLocalIncludePath rejects path-traversal/absolute reads
    # and de-aliases the cycle-guard key.
    add_executable(x3d_path_confine
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/path_confine_test.cpp")
    target_link_libraries(x3d_path_confine PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_path_confine COMMAND x3d_path_confine)

    # Regression: expandInlines walk must not stack-overflow on containment cycles
    # (USE-shared or self-referential nodes). Guard mirrors sibling system walkers.
    add_executable(x3d_inline_containment_cycle
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_containment_cycle_test.cpp")
    target_link_libraries(x3d_inline_containment_cycle PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_containment_cycle
        COMMAND x3d_inline_containment_cycle
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")

    # (10b) VRML97 (.wrl) reader test (WS-C), built and RUN. Parses the five text
    #       corpus .wrl fixtures (copied into runtime/parse/tests/data/wrl/) plus
    #       hand-authored snippets into a Scene and asserts the VRML97 dialect:
    #       node/field reflection resolution, the dialect rename-table tripwire
    #       (every mapped X3D target must resolve, catching generated-binding
    #       drift), TRUE/FALSE + access-keyword aliases, LOD.level/Switch.choice
    #       -> children, PROTO/Script carry-through, DEF/USE + ROUTE recovery,
    #       VRML 1.0 rejection, the unknown-node/field warning path, and the
    #       parseDocument front-door VRML97 dispatch. The fixtures dir is passed
    #       as argv[1] so the test runs from any working directory.
    add_executable(x3d_vrml97_reader
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/vrml97_reader_test.cpp")
    target_link_libraries(x3d_vrml97_reader PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_vrml97_reader
        COMMAND x3d_vrml97_reader
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/wrl")

    # (11) X3D-JSON (.json) reader test (WS-D), built and RUN. Parses the corpus-
    #      derived .json fixtures (runtime/parse/tests/data/json/, produced from
    #      corpus X3D via the XML/ClassicVRML readers + JsonWriter) plus hand-
    #      authored snippets into a Scene and asserts profile/version, head
    #      metadata, node types + field values, DEF/USE shared identity, ROUTE/
    #      IMPORT/EXPORT recovery, the JsonWriter->JsonReader->JsonWriter byte-
    #      stable round-trip (read/write symmetry), and parseDocument front-door
    #      JSON dispatch. The fixtures dir is passed as argv[1] so the test runs
    #      from any working directory.
    add_executable(x3d_json_reader
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/json_reader_test.cpp")
    target_link_libraries(x3d_json_reader PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_json_reader
        COMMAND x3d_json_reader
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/json")

    # Lenient read: out-of-range values are kept (not rejected), so a document
    # never fails to load over one bad field; typed setX stays strict.

    # AUD-PARSE-LEX: parse front-door + lexers regression coverage
    # (Encoding::sniff precedence, Inflate::inflateGzip round-trip + malformed-
    # input rejection, VrmlTokenizer punctuation/numbers/escapes, JsonLite
    # numberLexeme + BOM + trailing whitespace + \uXXXX + surrogate-pair).

    # Range-warning collection: validateRanges()/collectRangeWarnings() surface
    # out-of-range values the lenient read path keeps (structured diagnostics
    # on X3DDocument.rangeWarnings, populated at the X3DParse front door).

    # AUD-RNG-VAL: corpus audit of range validation + lenient reflection path.
    # Verifies reflection thunk uniformity, validateRanges() per-element
    # counting, ColorRGBA [0,1] for 4 components, the cycle guard's
    # path-based contract, generated validateRanges() completeness, and
    # the parseDocument front-door rangeWarnings channel.


    # AUD-PARSE-READERS: cross-reader regression matrix on the per-node
    # traversal audit axes — lenient capture (unknown node/field), JSON
    # @containerField default + non-default, access-type aliases incl.
    # literal-case default branch, PROTO body DEF scope in VRML97 (extends
    # AUD-C to the VRML97 subclass), header-line version floor across all
    # four encodings, EXTERNPROTO URL capture across encodings, ROUTE
    # capture across encodings.

    # T-SDK: the façade surface test. Drives load -> context -> extract -> tick
    # using ONLY x3d::sdk::* names through the single "x3d/sdk.hpp" include, so a
    # dropped/renamed re-export fails to compile here.
    add_executable(x3d_sdk_facade
        "${CMAKE_CURRENT_SOURCE_DIR}/include/x3d/tests/sdk_facade_test.cpp")
    target_link_libraries(x3d_sdk_facade PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_sdk_facade COMMAND x3d_sdk_facade)
    # Installed-package embed smoke: install THIS build, then configure/build/run
    # examples/embed_minimal against it via find_package(x3d_cpp), so a broken
    # export or a missing installed header fails CI on the actual embedder path.
    # The parent generator is forwarded so the downstream configure matches it.
    #
    # Skipped under X3D_CPP_SAN: the sanitizer build instruments the installed
    # archive globally (add_compile/link_options -fsanitize), but the downstream
    # example builds plain — linking it against the instrumented archive fails
    # with undefined __asan_*/__ubsan_* symbols. Install wiring is orthogonal to
    # sanitizers, so the dev/CI fast gate already proves it.
    if(NOT X3D_CPP_SAN)
        add_test(NAME x3d_install_embed_smoke
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_install_embed.sh"
                    "${CMAKE_CURRENT_BINARY_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}/install-embed-smoke"
                    "${CMAKE_GENERATOR}")
    endif()

    # T-SOUND: the §16 Sound seam (runtime/sound/). The AudioBackend seam +
    # SoundSystem are CORE, header-only, DSP-free (mirrors PhysicsBackend +
    # PhysicsSystem). Unlike physics, the default DSP backend has NO external
    # dependency (pure math), so it is always built — no flag gate. The test
    # drives the §16 children-graph in two tiers (recording graph + real DSP
    # samples) plus param-animation + determinism.
    add_executable(x3d_sound_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/dsp/BuiltinDspBackend.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/tests/sound_system_test.cpp")
    target_link_libraries(x3d_sound_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_sound_system COMMAND x3d_sound_system)

    # Author-shader binding plan (vocabulary + introspection).
endif()

# ---------------------------------------------------------------------------
# T-SDK: the headless façade examples. Built + run as ctests so they cannot
# rot. Each links only x3d_cpp::sdk and uses only the x3d::sdk surface.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_EXAMPLES)
    foreach(_ex
        01_load_validate_convert
        02_extract_render_feed
        03_attach_behavior_tick)
        add_executable("x3d_example_${_ex}"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/${_ex}.cpp")
        target_link_libraries("x3d_example_${_ex}" PRIVATE x3d_cpp::sdk)
        add_test(NAME "x3d_example_${_ex}" COMMAND "x3d_example_${_ex}")
    endforeach()

    # -----------------------------------------------------------------------
    # Corpus smoke — drive the full pipeline over a whole archive of real
    # scenes (parse->context->extract->tick) and report categorized stats.
    # The binary takes a dir + flags; on-demand full sweep via `mise run corpus`.
    # The ctest runs a BOUNDED, deterministic subset and SKIPS (exit 0) if the
    # corpus dir is absent, so it stays portable.
    # -----------------------------------------------------------------------
    add_executable(x3d_corpus_sweep
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/corpus_sweep.cpp")
    target_link_libraries(x3d_corpus_sweep PRIVATE x3d_cpp::sdk)

    # Root of the X3D conformance archive for the corpus smoke. The archive is
    # not bundled (size + per-file Web3D attribution); point this at a local
    # checkout via -DX3D_CPP_CORPUS_DIR=... or the X3D_CORPUS_DIR env var. When
    # unset/absent the corpus_sweep exits 0 + "SKIPPED", so the test stays green.
    set(X3D_CPP_CORPUS_DIR "$ENV{X3D_CORPUS_DIR}"
        CACHE PATH "Root of the X3D conformance archive for the corpus smoke")
    add_test(NAME x3d_corpus_smoke
        COMMAND x3d_corpus_sweep "${X3D_CPP_CORPUS_DIR}"
                --limit 250 --min-success-ratio 0.85 --quiet)

    # -----------------------------------------------------------------------
    # Phase 0: the DIFFERENTIAL AUDIT harness. One clean binary that runs the
    # oracle-free invariant + round-trip-fidelity checks over the archive and
    # emits a machine-clusterable JSONL of findings. Check logic lives in the
    # shared header tools/corpus_audit.hpp (also driven by the selftest below).
    # The ctest runs a BOUNDED subset and SKIPS (exit 0) if the corpus is absent.
    # -----------------------------------------------------------------------
    add_executable(x3d_corpus_audit
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/corpus_audit.cpp")
    target_include_directories(x3d_corpus_audit
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    target_link_libraries(x3d_corpus_audit PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_corpus_audit_smoke
        COMMAND x3d_corpus_audit "${X3D_CPP_CORPUS_DIR}"
                --limit 250 --roundtrip-limit 250 --quiet
                --out "${CMAKE_CURRENT_BINARY_DIR}/corpus_audit_findings.jsonl")

    # TDD selftest for the audit engine: a clean scene yields ZERO findings
    # across all 3 encodings; a deliberately mismatched / unparseable case is
    # detected. Links the full node set so it can build a real scene graph.
    add_executable(x3d_corpus_audit_selftest
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/corpus_audit_selftest.cpp")
    target_include_directories(x3d_corpus_audit_selftest
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    target_link_libraries(x3d_corpus_audit_selftest PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_corpus_audit_selftest COMMAND x3d_corpus_audit_selftest)

    # CLI-level regression test for the corpus tools (arg-parsing edge cases,
    # empty dirs, exit-code consistency, usage completeness).
    add_test(NAME x3d_corpus_tools_cli_test
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/corpus_tools_cli_test.sh"
                "$<TARGET_FILE:x3d_corpus_sweep>"
                "$<TARGET_FILE:x3d_corpus_audit>")
endif()

# ---------------------------------------------------------------------------
# x3d CLI — thin `x3d` binary over the x3d::sdk façade.
# Subcommands: convert (Task A); validate slots in next (Task B).
# Always built alongside the other tools/ binaries in the default preset.
# ---------------------------------------------------------------------------
add_executable(x3d_cli
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d_cli.cpp")
set_target_properties(x3d_cli PROPERTIES OUTPUT_NAME "x3d")
target_include_directories(x3d_cli PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_cli PRIVATE x3d_cpp::sdk)
# `x3d sim` drives the behavior runtime; link the ECMAScript backend so Script
# nodes actually run when it is available (it lives behind X3D_CPP_BUILD_TESTS).
# When absent (tests OFF), sim degrades cleanly: scripts are inert (reported,
# not a failure). The compile define gates the optional ScriptSystem include.
if(TARGET x3d_ecmascript_backend)
    target_link_libraries(x3d_cli PRIVATE x3d_ecmascript_backend)
    target_compile_definitions(x3d_cli PRIVATE X3D_SIM_HAVE_SCRIPT=1)
endif()
# When the Jolt backend is built (X3D_CPP_BUILD_PHYSICS=ON) link it so `x3d sim`
# simulates §37 rigid bodies; X3D_HAVE_PHYSICS comes transitively from the lib.
# OFF (default) => attachPhysics is the documented no-op; x3d_cli is unchanged.
if(TARGET x3d_physics_jolt)
    target_link_libraries(x3d_cli PRIVATE x3d_physics_jolt)
endif()

# CLI test: arg parsing, convert round-trips, help, error paths, + `sim`
# (golden traces, --watch, --json, determinism, malformed-input robustness).
# Passes the binary path, in-repo fixture dir, and the goldens dir to the harness.
# The 4th arg signals whether x3d_cli was built with the Jolt physics backend so
# the harness runs (or skips) its §37 sim-physics assertions accordingly.
if(TARGET x3d_physics_jolt)
    set(_x3d_cli_physics 1)
else()
    set(_x3d_cli_physics 0)
endif()
add_test(NAME x3d_cli_test
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/tests/x3d_cli_test.sh"
            "$<TARGET_FILE:x3d_cli>"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/goldens"
            "${_x3d_cli_physics}")

# sim behavioral self-oracle (C++): the runtime actually interpolating
# (translation.x ~= 5 at t~=0.5) + ProximitySensor enter/exit on a viewer path +
# determinism (two runs byte-identical). Links the full SDK; the fixtures dir is
# passed as argv[1].
add_executable(x3d_sim_behavior_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/sim_behavior_test.cpp")
target_include_directories(x3d_sim_behavior_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_sim_behavior_test PRIVATE x3d_cpp::sdk)
if(TARGET x3d_ecmascript_backend)
    target_link_libraries(x3d_sim_behavior_test PRIVATE x3d_ecmascript_backend)
    target_compile_definitions(x3d_sim_behavior_test PRIVATE X3D_SIM_HAVE_SCRIPT=1)
endif()
add_test(NAME x3d_sim_behavior_test
    COMMAND x3d_sim_behavior_test
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures")

# ---------------------------------------------------------------------------
# U3 ScriptEngine GENERICITY PROOF — the backend swap-test (x3d_quickjs_swap).
# Drives the SAME authored Script fixtures through the full runtime under BOTH
# the Duktape EcmaScriptBackend and the QuickJsBackend over an identical tick
# schedule, and asserts an IDENTICAL observable trace (author-field values +
# ROUTE-target values). Built ONLY when X3D_CPP_BUILD_QUICKJS=ON (when ON,
# x3d_ecmascript_backend — the always-built Duktape backend — is also present,
# so both engines link). Placed here, after both targets are defined, so the
# single TU links both backends.
# ---------------------------------------------------------------------------
if(TARGET x3d_quickjs AND TARGET x3d_ecmascript_backend AND X3D_CPP_BUILD_TESTS)
    add_executable(x3d_quickjs_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/quickjs_swap_test.cpp")
    target_link_libraries(x3d_quickjs_swap PRIVATE
        x3d_quickjs
        x3d_ecmascript_backend)
    add_test(NAME x3d_quickjs_swap
        COMMAND x3d_quickjs_swap
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/script/tests/data")
endif()

# ---------------------------------------------------------------------------
# x3d-cli differential gate:
#   x3d_scene_equiv_test  — unit tests for sceneEquivalent() (always built)
#   x3d_cli_gate          — Java-free differential gate (validate-diff +
#                           convert-roundtrip vs X3DJSAIL golden verdicts)
#
# Both targets include tools/x3d-cli/ so they can see scene_equiv.hpp and
# cli_gate.cpp without polluting the SDK include path.
# The self-contained tests remain in the default build. The external-corpus
# gates exist as explicit targets, but are excluded from `all` and from CTest;
# their mise tasks build them before use.
# ---------------------------------------------------------------------------
add_executable(x3d_scene_equiv_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/scene_equiv_test.cpp")
target_include_directories(x3d_scene_equiv_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_scene_equiv_test PRIVATE x3d_cpp::sdk)
add_test(NAME x3d_scene_equiv_test COMMAND x3d_scene_equiv_test)

# Regression for the gate's Inline/EXTERNPROTO baseUrl asymmetry (BUG-1): the
# reparse must use the same baseUrl as the source parse so relative externals
# expand symmetrically. Self-contained (writes its own temp fixtures), so it is
# a real ctest (unlike x3d_cli_gate which needs the external corpus).
add_executable(x3d_gate_baseurl_roundtrip_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/gate_baseurl_roundtrip_test.cpp")
target_include_directories(x3d_gate_baseurl_roundtrip_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_gate_baseurl_roundtrip_test PRIVATE x3d_cpp::sdk)
add_test(NAME x3d_gate_baseurl_roundtrip_test
         COMMAND x3d_gate_baseurl_roundtrip_test)

add_executable(x3d_cli_gate EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/cli_gate.cpp")
target_include_directories(x3d_cli_gate PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_cli_gate PRIVATE x3d_cpp::sdk)
# Note: x3d_cli_gate is NOT registered as a ctest — it requires an external
# corpus and committed golden files. Run via: mise run cli-gate

# ---------------------------------------------------------------------------
# x3d canonicalize — X3DC14N canonical form gate:
#   x3d_canonicalize_unit_test  — unit tests for CanonicalXmlWriter (ctest)
#   x3d_canon_gate              — tiered differential gate vs X3DJSAIL golden
#                                 fixtures; NOT a ctest (needs external corpus).
#                                 Run via: mise run canon-gate
# ---------------------------------------------------------------------------
add_executable(x3d_canonicalize_unit_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/canonicalize_unit_test.cpp")
target_include_directories(x3d_canonicalize_unit_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs")
target_link_libraries(x3d_canonicalize_unit_test PRIVATE x3d_cpp::sdk)
add_test(NAME x3d_canonicalize_unit_test COMMAND x3d_canonicalize_unit_test)

add_executable(x3d_canon_gate EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/canon_gate.cpp")
target_include_directories(x3d_canon_gate PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
target_link_libraries(x3d_canon_gate PRIVATE x3d_cpp::sdk)
# Note: x3d_canon_gate is NOT registered as a ctest — requires external corpus.
# Run via: mise run canon-gate

# ---------------------------------------------------------------------------
# PoC OpenGL renderer — conditional, OFF by default. Added LAST so the
# x3d_cpp::x3d_cpp INTERFACE target it links already exists. When OFF this is a
# no-op and the default build/golden/ctest path is byte-for-byte unchanged.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_POC)
    add_subdirectory(examples/poc_renderer)
endif()

# ---------------------------------------------------------------------------
# Headless CPU rasterizer — conditional, OFF by default. Dependency-free; when
# OFF this is a no-op and the default build/golden/ctest path is unchanged.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_CPURASTER)
    enable_testing() # so the rasterizer's ctests register even when BUILD_TESTS is OFF.
    add_subdirectory(examples/cpu_raster)
endif()

# ---------------------------------------------------------------------------
# Asset-import consumer — conditional, OFF by default. See
# examples/asset_import/CMakeLists.txt for the authoring footprint smoke and
# the ImportScene IR tests. Task 11 finalizes the full subdirectory.
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_ASSET_IMPORT)
    enable_testing() # register ctests even when BUILD_TESTS is OFF.
    add_subdirectory(examples/asset_import)
endif()

# ---------------------------------------------------------------------------
# Extension quarantine — x3d_cpp_ext (INTERFACE, header-only).
# Lives in runtime/ext/; namespace x3d::runtime::ext.
# One-way dep: ext→core (includes PackedMesh, Aabb, …). Core NEVER includes ext.
# Default OFF — no TU in runtime/ext/ is ever compiled or linked in the
# standard build path. Tests registered ONLY inside this block, so the default
# ctest suite is unaffected. Build with: -DX3D_CPP_BUILD_EXT=ON
# ---------------------------------------------------------------------------
if(X3D_CPP_BUILD_EXT)
    add_library(x3d_cpp_ext INTERFACE)
    add_library(x3d_cpp::ext ALIAS x3d_cpp_ext)
    target_include_directories(x3d_cpp_ext INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/codecs>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_compile_features(x3d_cpp_ext INTERFACE cxx_std_20)
    # Ext code may #include PackedMesh.hpp, Aabb.hpp, etc. from core.
    target_link_libraries(x3d_cpp_ext INTERFACE x3d_cpp::x3d_cpp)

    # Ext tests — registered inside the quarantine block only.
    if(X3D_CPP_BUILD_TESTS)
        enable_testing()

        add_executable(x3d_ext_stl_reader
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/stl_reader_test.cpp")
        target_link_libraries(x3d_ext_stl_reader PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_stl_reader COMMAND x3d_ext_stl_reader)

        # STL writer round-trip test: writeStlBinary → parseStlBinary (bit-exact).
        # Uses StlReader (ext-gated) to validate the writer output.
        add_executable(x3d_stl_write_test
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/stl_write_test.cpp")
        target_include_directories(x3d_stl_write_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
        target_link_libraries(x3d_stl_write_test PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_stl_write_test COMMAND x3d_stl_write_test)

        # Extract self-oracle: X3D → writeStlBinary → parseStlBinary → compare.
        # Proves extract path agrees with StlReader end-to-end.
        add_executable(x3d_extract_oracle_test
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/extract_oracle_test.cpp")
        target_include_directories(x3d_extract_oracle_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli")
        target_link_libraries(x3d_extract_oracle_test PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_extract_oracle_test
            COMMAND x3d_extract_oracle_test
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures/simple.x3d"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/x3d-cli/fixtures/validate-clean.x3d")

        add_executable(x3d_ext_external_geometry_node
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_node_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_node PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_node COMMAND x3d_ext_external_geometry_node)

        add_executable(x3d_ext_external_geometry_roundtrip
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_roundtrip_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_roundtrip PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_roundtrip COMMAND x3d_ext_external_geometry_roundtrip)

        # Round 2 — lazy ExternalGeometry materialization end-to-end proof.
        add_executable(x3d_ext_external_geometry_e2e
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/ext/tests/external_geometry_e2e_test.cpp")
        target_link_libraries(x3d_ext_external_geometry_e2e PRIVATE x3d_cpp::ext)
        add_test(NAME x3d_ext_external_geometry_e2e COMMAND x3d_ext_external_geometry_e2e)
    endif()
endif()

# --- PROTO: grouped doctest binary (math + scene tests, was 22 separate exes) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_geometry_scene_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/aabb_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_inverse_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/vec_math_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/intersect_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/geometry_bounds_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/getfield_typecheck_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/dirty_tracker_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/view_dependent_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/transform_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/transform_system_hanim_cadpart_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_shared_subgraph_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_cycle_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/cycle_breaker_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/walker_cycle_guard_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/walker_budget_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/pick_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/interface_registry_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/scene_extractor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/poc_triangle_asset_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_stack_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bind_time_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_stack_audit_test.cpp")
    target_link_libraries(x3d_geometry_scene_tests PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_geometry_scene_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    # poc_triangle_asset_test needs its fixture path (was a per-target define).
    target_compile_definitions(x3d_geometry_scene_tests PRIVATE
        "X3D_POC_TRIANGLE_ASSET=\"${CMAKE_CURRENT_SOURCE_DIR}/examples/poc_renderer/assets/triangle.x3d\"")
    add_test(NAME x3d_geometry_scene COMMAND x3d_geometry_scene_tests)
    set_tests_properties(x3d_geometry_scene PROPERTIES TIMEOUT 120)
endif()

# --- Namespace taxonomy lock (ADR-0039): pin x3d::core / x3d::nodes placement ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_namespace_taxonomy
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/tests/namespace_taxonomy_test.cpp")
    target_link_libraries(x3d_namespace_taxonomy PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_namespace_taxonomy PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_namespace_taxonomy COMMAND x3d_namespace_taxonomy)
endif()

# --- doctest grouped binary: codecs tests (19 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_codecs_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/vrml_mf_bracket_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_instance_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_body_defscope_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/enum_quote_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_proto_capture_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_decl_body_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_deep_is_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_is_json_vrml_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_writer_parity_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_appinfo_json_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_extern_url_json_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_nested_instance_placement_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/nested_protoinstance_roundtrip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/initializeonly_read_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/codec_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/fval_extended_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/sfimage_overflow_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/xml_depth_guard_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/script_cdata_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/uom_type_pin_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/hanim_container_order_test.cpp")
    target_link_libraries(x3d_codecs_tests PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_codecs_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_codecs_tests COMMAND x3d_codecs_tests)
    set_tests_properties(x3d_codecs_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: parse tests (13 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_parse_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_clone_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_expand_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_nested_body_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/json_proto_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/vrml97_proto_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_carriers_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_expand_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/lenient_read_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/encoding_lex_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/range_warnings_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/range_validate_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/version_floor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/reader_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/parser_depth_guard_test.cpp")
    target_link_libraries(x3d_parse_tests PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_parse_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_parse_tests COMMAND x3d_parse_tests)
    set_tests_properties(x3d_parse_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: extract tests (35 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_extract_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_item_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_delta_perf_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/material_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_mapping_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_orm_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_t4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b5_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_elevationgrid_winding_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b6_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc3_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_tc4_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_txc1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/text_layout_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/text_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/asset_resolver_b8_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_t7_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_t8_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_b2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_col2_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_cad1_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/light_system_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_m25_5_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/scene_extractor_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_feed_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/packed_mesh_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_desc_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/render_item_geometry_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/external_geom_seam_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_b7_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_ext002_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/shader_binding_plan_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/castshadow_extract_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/nurbs_eval_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_nurbs_test.cpp")
    target_link_libraries(x3d_extract_tests PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_extract_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_extract_tests COMMAND x3d_extract_tests)
    set_tests_properties(x3d_extract_tests PROPERTIES TIMEOUT 180)
endif()

# --- doctest grouped binary: events tests (34 cases, one link) ---
if(X3D_CPP_BUILD_TESTS AND TARGET x3d_cpp_nodes)
    add_executable(x3d_events_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/dynamic_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/mem_safety_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_observer_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_alias_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2a_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2b_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2c_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/write_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2d_tick_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/tick_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointer_state_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointing_sensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointing_sensor_skip_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointer_arbitration_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/standard_runtime_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/nav_arbitration_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interactive_wiring_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/drag_math_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/key_state_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/navigation_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/nav_pointer_screen_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/viewpoint_offset_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/viewpoint_bind_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/animation_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interpolator_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/follower_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interpolator_conformance_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/event_utility_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/key_device_sensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/node_lifecycle_audit_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/timesensor_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/timesensor_rtc_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_dynamic_route_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_author_field_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/slerp_normal_test.cpp")
    target_link_libraries(x3d_events_tests PRIVATE x3d_cpp::sdk)
    target_include_directories(x3d_events_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
    add_test(NAME x3d_events_tests COMMAND x3d_events_tests)
    set_tests_properties(x3d_events_tests PROPERTIES TIMEOUT 180)
endif()

# ---------------------------------------------------------------------------
# AudioBackend seam — Backend B (miniaudio). ADR-0025 / U1–U4.
# Mirrors the x3d_physics_jolt isolation rule exactly: option() → FetchContent
# → an isolated STATIC lib (x3d_miniaudio) whose single TU is
# MiniaudioBackend.cpp; miniaudio.h is included PRIVATE
# (MINIAUDIO_IMPLEMENTATION defined there) so the ~100 k-line body compiles
# into exactly one object file and never leaks to consumers.
# MiniaudioBackend.hpp is miniaudio-free (pImpl).
# Build with:
#   cmake -S . -B build-audio -G Ninja \
#         -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON \
#         -DX3D_CPP_PER_HEADER_CHECKS=OFF
#   cmake --build build-audio --target x3d_miniaudio
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_MINIAUDIO "Build miniaudio AudioBackend backend (OFF default; fetches miniaudio v0.11.25)" OFF)

if(X3D_CPP_BUILD_MINIAUDIO)
    include(FetchContent)
    # miniaudio is a single-header library; fetch only the source tree and use
    # its repo root as a PRIVATE include directory. Do NOT add_subdirectory its
    # own CMake project — x3d_miniaudio only needs miniaudio.h.
    FetchContent_Declare(miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE)
    FetchContent_GetProperties(miniaudio)
    if(NOT miniaudio_POPULATED)
        # Single-arg FetchContent_Populate is deprecated under CMP0169 (CMake
        # 3.30+); set the policy to OLD to keep using it. The policy is unknown
        # to CMake < 3.30 (e.g. 3.28 on CI), where setting it is a hard error —
        # so guard the SET on POLICY existence. PUSH/POP scope it either way.
        cmake_policy(PUSH)
        if(POLICY CMP0169)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Populate(miniaudio)
        cmake_policy(POP)
    endif()

    # MiniaudioBackend: the single TU where miniaudio meets the AudioBackend seam.
    # MINIAUDIO_IMPLEMENTATION + MA_NO_DEVICE_IO are defined in MiniaudioBackend.cpp
    # so the entire miniaudio body compiles into exactly one object file and never
    # leaks to consumers. MiniaudioBackend.hpp is miniaudio-free (pImpl).
    # Threads is required before any target that uses it: miniaudio references
    # pthread_* symbols internally (for its node-graph spinlocks) even with
    # MA_NO_DEVICE_IO.
    find_package(Threads REQUIRED)

    add_library(x3d_miniaudio STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio/MiniaudioBackend.cpp")
    target_link_libraries(x3d_miniaudio PUBLIC x3d_cpp::x3d_cpp)
    # Link Threads publicly so any consumer of this lib inherits the link.
    target_link_libraries(x3d_miniaudio PUBLIC Threads::Threads)
    target_include_directories(x3d_miniaudio PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio")
    # miniaudio.h is PRIVATE: only MiniaudioBackend.cpp needs it; consumers
    # that link x3d_miniaudio inherit no miniaudio headers.
    target_include_directories(x3d_miniaudio PRIVATE
        "${miniaudio_SOURCE_DIR}")
    # miniaudio is warning-noisy on the TU that defines MINIAUDIO_IMPLEMENTATION.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_miniaudio PRIVATE -w)  # fetched single header.
    endif()

    # x3d_sound_swaptest: headless SWAP-TEST — the Audio seam genericity proof.
    # Renders the same §16 graphs through both BuiltinDsp and miniaudio headless,
    # asserts numeric agreement for synthesis and structural invariants for spatial.
    # Replaces the Task-1 throwaway spike (spike_probe.cpp + spike_spatial.cpp).
    if(X3D_CPP_BUILD_TESTS)
        add_executable(x3d_sound_swaptest
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/dsp/BuiltinDspBackend.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/tests/sound_swap_test.cpp")
        target_link_libraries(x3d_sound_swaptest PRIVATE x3d_cpp::x3d_cpp x3d_miniaudio)
        add_test(NAME x3d_sound_swaptest COMMAND x3d_sound_swaptest)
        set_tests_properties(x3d_sound_swaptest PROPERTIES TIMEOUT 120)
    endif()
endif()

# ---------------------------------------------------------------------------
# BLD-2: libFuzzer harness for parseDocument (Clang-only). Builds only when
# X3D_CPP_FUZZ=ON and the compiler is Clang (libFuzzer requires the
# `-fsanitize=fuzzer` driver, which is GCC 14+ but most reliable on Clang).
# The fuzz preset sets both; the `cpp-fuzz` CI job uses the same. The
# harness itself catches all exceptions and just asserts the parser doesn't
# crash — the ASan/UBSan signal handlers are what actually catch the bug.
# ---------------------------------------------------------------------------
if(X3D_CPP_FUZZ)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "X3D_CPP_FUZZ requires Clang (libFuzzer driver). Detected: ${CMAKE_CXX_COMPILER_ID}")
    endif()
    add_executable(x3d_parse_fuzz
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/parse_fuzz.cpp")
    target_link_libraries(x3d_parse_fuzz PRIVATE x3d_cpp::sdk)
    # COMPILE-time instrumentation is what makes the harness useful: `fuzzer`
    # inserts SanitizerCoverage (the edge feedback libFuzzer mutates against —
    # without it the fuzzer runs blind and finds nothing), and `address,undefined`
    # insert the ASan/UBSan checks that actually catch the memory/UB bugs. The
    # SDK is header-only, so parseDocument inlines into this one TU — instrumenting
    # it instruments the whole parse path. (The fuzz preset deliberately does NOT
    # set X3D_CPP_SAN globally, so these flags are scoped to the harness target.)
    target_compile_options(x3d_parse_fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    # Link the libFuzzer driver + ASan/UBSan runtimes so the fuzzer can run + signal.
    target_link_options(x3d_parse_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    message(STATUS "x3d_cpp: X3D_CPP_FUZZ=ON -> x3d_parse_fuzz built with libFuzzer + ASan + UBSan")
endif()

# ---------------------------------------------------------------------------
# Target purpose inventory. Primary purpose answers why each repository-owned
# target exists; test-suite membership is a separate property added in the
# aggregate-target phase. Optional names are kept in the inventory even when
# their feature flag is off so enabling a backend cannot create an unclassified
# target later.
# ---------------------------------------------------------------------------
function(_x3d_classify_present_targets purpose)
    foreach(_target IN LISTS ARGN)
        if(TARGET "${_target}")
            x3d_set_target_purpose("${_target}" "${purpose}")
        endif()
    endforeach()
endfunction()

_x3d_classify_present_targets(production
    x3d_cpp
    x3d_cpp_nodes
    x3d_cpp_authoring
    x3d_cpp_sdk
    x3d_ecmascript_backend
    x3d_cli)

_x3d_classify_present_targets(compile-contract
    x3d_cpp_all_headers)

_x3d_classify_present_targets(behavior
    x3d_codec_roundtrip_audit
    x3d_xml_script_field
    x3d_xml_composed_shader
    x3d_vrml_script_field
    x3d_json_script_field
    x3d_contact_reporter
    x3d_physics_contact_wiring
    x3d_inertia_massprops
    x3d_contact_response
    x3d_event_scene_bridge
    x3d_sai_context
    x3d_script_system
    x3d_ecmascript_backend_test
    x3d_ecmascript_corpus_smoke_test
    x3d_script_author_runtime
    x3d_script_corpus_e2e
    x3d_parse_reader
    x3d_classic_vrml_reader
    x3d_proto_front_door
    x3d_proto_expand_audit
    x3d_inline_roundtrip
    x3d_inline_routes
    x3d_inline_cycle
    x3d_inflate_bomb
    x3d_path_confine
    x3d_inline_containment_cycle
    x3d_vrml97_reader
    x3d_json_reader
    x3d_sdk_facade
    x3d_sound_system
    x3d_corpus_sweep
    x3d_corpus_audit
    x3d_corpus_audit_selftest
    x3d_sim_behavior_test
    x3d_scene_equiv_test
    x3d_gate_baseurl_roundtrip_test
    x3d_canonicalize_unit_test
    x3d_geometry_scene_tests
    x3d_namespace_taxonomy
    x3d_codecs_tests
    x3d_parse_tests
    x3d_extract_tests
    x3d_events_tests)

_x3d_classify_present_targets(opt-in
    x3d_physics_jolt
    x3d_physics_jolt_test
    x3d_physics_system_test
    x3d_quickjs
    x3d_quickjs_backend_test
    x3d_curl
    x3d_assetresolver_backend_a_test
    x3d_s3
    x3d_assetresolver_backend_b_test
    x3d_assetresolver_swap
    x3d_stb
    x3d_wuffs
    x3d_stbtt
    x3d_plmpeg
    x3d_theora
    x3d_freetype
    x3d_text_tests
    x3d_texture_tests
    x3d_movie_tests
    x3d_example_01_load_validate_convert
    x3d_example_02_extract_render_feed
    x3d_example_03_attach_behavior_tick
    x3d_quickjs_swap
    x3d_cli_gate
    x3d_canon_gate
    x3d_cpp_ext
    x3d_ext_stl_reader
    x3d_stl_write_test
    x3d_extract_oracle_test
    x3d_ext_external_geometry_node
    x3d_ext_external_geometry_roundtrip
    x3d_ext_external_geometry_e2e
    x3d_miniaudio
    x3d_sound_swaptest
    x3d_parse_fuzz)

_x3d_classify_present_targets(internal
    x3d_duktape
    x3d_s3_testsupport)

# Test-suite membership is deliberately independent of primary target purpose:
# x3d_cli is production software and still belongs in the behavior/sanitizer
# build graph, while behavior test executables naturally belong in both.
if(X3D_CPP_BUILD_TESTS)
    x3d_get_targets_by_purpose(_x3d_behavior_targets behavior)
    foreach(_target IN LISTS _x3d_behavior_targets)
        x3d_register_test_target("${_target}" SUITES behavior sanitizer)
    endforeach()
    if(TARGET x3d_cli)
        x3d_register_test_target(x3d_cli SUITES behavior sanitizer)
    endif()

    x3d_get_targets_by_purpose(_x3d_contract_targets compile-contract)
    foreach(_target IN LISTS _x3d_contract_targets)
        x3d_register_test_target("${_target}" SUITES compile-contract)
    endforeach()

    x3d_get_test_targets(_x3d_behavior_suite_targets behavior)
    add_custom_target(x3d_behavior_tests)
    add_dependencies(x3d_behavior_tests ${_x3d_behavior_suite_targets})
    x3d_set_target_purpose(x3d_behavior_tests internal)

    x3d_get_test_targets(_x3d_sanitizer_suite_targets sanitizer)
    add_custom_target(x3d_sanitizer_tests)
    add_dependencies(x3d_sanitizer_tests ${_x3d_sanitizer_suite_targets})
    x3d_set_target_purpose(x3d_sanitizer_tests internal)

    x3d_get_test_targets(_x3d_contract_suite_targets compile-contract)
    add_custom_target(x3d_compile_contracts)
    add_dependencies(x3d_compile_contracts ${_x3d_contract_suite_targets})
    x3d_set_target_purpose(x3d_compile_contracts internal)

    # Keep execution selection as explicit as build selection. Some test names
    # differ from their owning targets, and shell-driven tests have no owning
    # executable, so CTest labels are assigned from the public test names.
    set(_x3d_behavior_tests
        x3d_codec_roundtrip_audit
        x3d_xml_script_field
        x3d_xml_composed_shader
        x3d_vrml_script_field
        x3d_json_script_field
        x3d_contact_reporter
        x3d_physics_contact_wiring
        x3d_inertia_massprops
        x3d_contact_response
        x3d_event_scene_bridge
        x3d_sai_context
        x3d_script_system
        x3d_ecmascript_backend
        x3d_ecmascript_corpus_smoke
        x3d_script_author_runtime
        x3d_script_corpus_e2e
        x3d_parse_reader
        x3d_classic_vrml_reader
        x3d_proto_front_door
        x3d_proto_expand_audit
        x3d_inline_roundtrip
        x3d_inline_routes
        x3d_inline_cycle
        x3d_inflate_bomb
        x3d_path_confine
        x3d_inline_containment_cycle
        x3d_vrml97_reader
        x3d_json_reader
        x3d_sdk_facade
        x3d_sound_system
        x3d_corpus_smoke
        x3d_corpus_audit_smoke
        x3d_corpus_audit_selftest
        x3d_corpus_tools_cli_test
        x3d_cli_test
        x3d_sim_behavior_test
        x3d_scene_equiv_test
        x3d_gate_baseurl_roundtrip_test
        x3d_canonicalize_unit_test
        x3d_geometry_scene
        x3d_namespace_taxonomy
        x3d_codecs_tests
        x3d_parse_tests
        x3d_extract_tests
        x3d_events_tests)
    set_tests_properties(${_x3d_behavior_tests} PROPERTIES LABELS behavior)

    set(_x3d_compile_contract_tests x3d_cpp_all_headers)
    if(NOT X3D_CPP_SAN)
        list(APPEND _x3d_compile_contract_tests x3d_install_embed_smoke)
    endif()
    if(X3D_CPP_PER_HEADER_CHECKS)
        list(APPEND _x3d_compile_contract_tests x3d_header_isolation)
    endif()
    set_tests_properties(
        ${_x3d_compile_contract_tests} PROPERTIES LABELS compile-contract)
endif()

x3d_validate_target_purposes()
