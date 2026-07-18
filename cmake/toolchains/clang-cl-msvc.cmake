# clang-cl + xwin cross-toolchain: build the x86_64-pc-windows-msvc target from
# Linux/macOS, no Windows machine. Fast local pre-flight for the GitHub MSVC job
# (which stays the source of truth — clang-cl emulates MSVC but is not identical).
#
# One-time SDK fetch (needs `xwin` — https://github.com/Jake-Shadle/xwin — plus
# clang-cl + lld-link from LLVM):
#
#   xwin --accept-license --arch x86_64 splat \
#        --use-winsysroot-style --preserve-ms-arch-notation --output ~/.xwin
#
#   # --use-winsysroot-style   -> clang-cl's /winsysroot can consume it
#   # --preserve-ms-arch-notation -> lib dirs are x64 (what /winsysroot expects),
#   #                                not LLVM's x86_64
#   # (keep the default case-fix symlinks: Linux is case-sensitive, the SDK ships
#   #  Windows.h / Kernel32.lib but code includes windows.h / links kernel32.lib)
#
# Then, from `mise run build-msvc`, or by hand:
#
#   cmake -S . -B build-msvc -G Ninja \
#     --toolchain cmake/toolchains/clang-cl-msvc.cmake \
#     -DCMAKE_BUILD_TYPE=Release \        # xwin ships the release CRT, not debug
#     -DX3D_CPP_BUILD_TESTS=ON -DX3D_CPP_ENABLE_DEV_TOOLING=OFF
#   cmake --build build-msvc --target x3d_events_tests
#   wine build-msvc/x3d_events_tests.exe  # run the tests (needs wine)
#
# Point elsewhere than ~/.xwin with -DXWIN_ROOT=/path or the XWIN_ROOT env var.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_CROSSCOMPILING ON)

if(NOT DEFINED XWIN_ROOT)
  if(DEFINED ENV{XWIN_ROOT})
    set(XWIN_ROOT "$ENV{XWIN_ROOT}")
  else()
    set(XWIN_ROOT "$ENV{HOME}/.xwin")
  endif()
endif()
if(NOT EXISTS "${XWIN_ROOT}/VC")
  message(FATAL_ERROR
    "XWIN_ROOT='${XWIN_ROOT}' has no VC/ — run the `xwin ... splat` step in this "
    "file's header first, or pass -DXWIN_ROOT=<dir>.")
endif()

set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER       lld-link)
set(CMAKE_RC_COMPILER  llvm-rc)
set(CMAKE_MT           llvm-mt)
set(CMAKE_AR           llvm-lib)

# clang-cl reads the MSVC CRT/SDK headers+libs from /winsysroot; -Wno-unused-…
# silences the harmless "argument unused during compilation" cross noise.
set(_xwin_common "--target=x86_64-pc-windows-msvc /winsysroot ${XWIN_ROOT} -Wno-unused-command-line-argument")
set(CMAKE_C_FLAGS_INIT   "${_xwin_common}")
set(CMAKE_CXX_FLAGS_INIT "${_xwin_common}")

set(_xwin_link "/winsysroot:${XWIN_ROOT} /machine:x64")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_xwin_link}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_xwin_link}")

# The compiler-id probe can't run a Windows exe on the host — probe with a lib.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
