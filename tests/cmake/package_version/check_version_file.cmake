# Contract test for the generated x3d_cppConfigVersion.cmake.
# Invoked as:
#   cmake -DVERSION_FILE=<path> -DBUILD_SIZEOF_VOID_P=<n> -P check_version_file.cmake
#
# The version file is a plain CMake script that consumes
# PACKAGE_FIND_VERSION{,_MAJOR,_MINOR} and CMAKE_SIZEOF_VOID_P, and answers via
# PACKAGE_VERSION_COMPATIBLE / PACKAGE_VERSION_UNSUITABLE. Driving it directly
# keeps this hermetic: no cross-compiler and no second install tree needed.
#
# What it pins down:
#   - the package must NOT be arch-independent (it ships compiled .so files), so
#     a consumer with a different pointer size must be UNSUITABLE;
#   - at 0.x, SameMajorVersion would treat every future 0.y as compatible, so
#     compatibility must be scoped to the MINOR.

if(NOT DEFINED VERSION_FILE)
  message(FATAL_ERROR "check_version_file.cmake: -DVERSION_FILE=<path> is required")
endif()
if(NOT EXISTS "${VERSION_FILE}")
  message(FATAL_ERROR "check_version_file.cmake: no such file: ${VERSION_FILE}")
endif()
if(NOT DEFINED BUILD_SIZEOF_VOID_P)
  message(FATAL_ERROR "check_version_file.cmake: -DBUILD_SIZEOF_VOID_P=<n> is required")
endif()

# The "matching" arch is whatever this build actually is -- do not hardcode 8.
# A 32-bit build must still see its own pointer size accepted, and the mismatch
# case must be the OTHER size, whichever that is.
set(_match_void_p "${BUILD_SIZEOF_VOID_P}")
if(_match_void_p EQUAL 8)
  set(_mismatch_void_p 4)
else()
  set(_mismatch_void_p 8)
endif()

set(_failures "")

function(_probe out_compatible out_unsuitable find_version major minor sizeof_void_p)
  set(PACKAGE_FIND_VERSION "${find_version}")
  set(PACKAGE_FIND_VERSION_MAJOR "${major}")
  set(PACKAGE_FIND_VERSION_MINOR "${minor}")
  set(CMAKE_SIZEOF_VOID_P "${sizeof_void_p}")
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)
  include("${VERSION_FILE}")
  set(${out_compatible} "${PACKAGE_VERSION_COMPATIBLE}" PARENT_SCOPE)
  set(${out_unsuitable} "${PACKAGE_VERSION_UNSUITABLE}" PARENT_SCOPE)
endfunction()

# Case 1: same minor, matching arch -> compatible.
_probe(c u "0.1.0" 0 1 "${_match_void_p}")
if(NOT c)
  list(APPEND _failures "0.1.0 request should be COMPATIBLE with installed 0.1.0")
endif()
if(u)
  list(APPEND _failures "0.1.0 request on the build's own arch should NOT be UNSUITABLE")
endif()

# Case 2: a LATER minor -> NOT compatible.
# Note this case does not discriminate between SameMajorVersion and
# SameMinorVersion: every mode first rejects installed < requested, so it was
# already green before the fix. Kept as a regression control, not as evidence.
_probe(c u "0.2.0" 0 2 "${_match_void_p}")
if(c)
  list(APPEND _failures "0.2.0 request must NOT be compatible with installed 0.1.0")
endif()

# Case 3: an EARLIER minor -> NOT compatible. THIS is the SameMajorVersion bug.
# installed 0.1.0 >= requested 0.0.9 and the majors match (both 0), so
# SameMajorVersion calls it compatible -- at 0.x that makes every 0.y look
# interchangeable. SameMinorVersion rejects it because the minors differ.
_probe(c u "0.0.9" 0 0 "${_match_void_p}")
if(c)
  list(APPEND _failures "0.0.9 request must NOT be compatible with installed 0.1.0")
endif()

# Case 4: mismatched pointer size -> UNSUITABLE (this is the ARCH_INDEPENDENT bug).
_probe(c u "0.1.0" 0 1 "${_mismatch_void_p}")
if(NOT u)
  list(APPEND _failures
       "a consumer with pointer size ${_mismatch_void_p} must be UNSUITABLE for a package built with pointer size ${_match_void_p} (ARCH_INDEPENDENT must not be set on a package shipping .so files)")
endif()

if(_failures)
  string(REPLACE ";" "\n  - " _msg "${_failures}")
  message(FATAL_ERROR "package version contract FAILED:\n  - ${_msg}")
endif()
message(STATUS "package version contract OK (SameMinorVersion + arch check present)")
