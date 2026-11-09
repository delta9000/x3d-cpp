# Real MSVC under Wine. Unlike clang-cl, cl.exe exercises MSVC-only preprocessor
# branches. Set MSVC_WINE_BIN or use ~/my-msvc, provisioned with msvc-wine.
# Wrapper scripts set INCLUDE/LIB/WINEPATH for each invocation.
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
