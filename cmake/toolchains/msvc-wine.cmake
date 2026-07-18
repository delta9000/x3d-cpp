# Real MSVC (cl.exe) under wine, via msvc-wine — the AUTHORITATIVE local Windows
# build, unlike the faster clang-cl loop (clang-cl defines __clang__, so it is
# blind to MSVC-only #ifdef branches; that is how the tinfl.h C1017 slipped past
# it). Slower (each cl invocation spawns wine) but catches what real cl catches —
# ideal as a precommit MSVC check. Set MSVC_WINE_BIN, or default to ~/my-msvc.
#
# One-time setup (needs wine + msitools):
#   git clone https://github.com/mstorsjo/msvc-wine
#   ./msvc-wine/vsdownload.py --accept-license --dest ~/my-msvc
#   ./msvc-wine/install.sh ~/my-msvc
#
# The cl/link/lib/rc/mt wrappers each source msvcenv.sh, so INCLUDE/LIB/WINEPATH
# are set per invocation — no need to source it here.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_CROSSCOMPILING ON)

if(NOT DEFINED MSVC_WINE_BIN)
  if(DEFINED ENV{MSVC_WINE_BIN})
    set(MSVC_WINE_BIN "$ENV{MSVC_WINE_BIN}")
  else()
    set(MSVC_WINE_BIN "$ENV{HOME}/my-msvc/bin/x64")
  endif()
endif()
if(NOT EXISTS "${MSVC_WINE_BIN}/cl")
  message(FATAL_ERROR
    "MSVC_WINE_BIN='${MSVC_WINE_BIN}' has no cl wrapper — run the msvc-wine "
    "vsdownload/install steps in this file's header, or pass -DMSVC_WINE_BIN=<dir>.")
endif()

set(CMAKE_C_COMPILER   "${MSVC_WINE_BIN}/cl")
set(CMAKE_CXX_COMPILER "${MSVC_WINE_BIN}/cl")
set(CMAKE_LINKER       "${MSVC_WINE_BIN}/link")
set(CMAKE_AR           "${MSVC_WINE_BIN}/lib")
set(CMAKE_RC_COMPILER  "${MSVC_WINE_BIN}/rc")
set(CMAKE_MT           "${MSVC_WINE_BIN}/mt")

# The compiler-id probe can't run a Windows exe on the host — probe with a lib.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
