// runtime/io/tests/file_resolver_test.cpp
// Task 1: FileResolver backend — SEC-3-confined local-file AssetResolver.
// Hermetic + offline; uses committed fixtures only.
#include "doctest/doctest.h"
#include "io/file/FileResolver.hpp"

using namespace x3d::runtime;
using x3d::runtime::extract::AssetKind;

TEST_CASE("FileResolver: confined existing file is Ready, others Failed") {
  auto r = io::file::makeFileResolver(FIXTURES_DIR);
  CHECK(r("f1.bin", AssetKind::Texture).ready());
  CHECK(r("missing.bin", AssetKind::Texture).failed());
  CHECK(r("../escape.bin", AssetKind::Texture).failed());
  CHECK(r("/etc/hostname", AssetKind::Texture).failed());
  CHECK(r("https://example.com/x", AssetKind::Texture).failed());
  CHECK(r("f1.bin#frag", AssetKind::Texture).ready());
  CHECK_FALSE(r("f1.bin", AssetKind::Texture).pending());
}
