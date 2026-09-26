// scheme_router_test.cpp — makeSchemeRouter dispatch table (fake resolvers).
//
// Pins the routing rule (mirrors makeMultiFormatTextureResolver): dispatch by
// the URL's scheme BEFORE calling a backend, so each backend only sees urls it
// owns and Failed keeps meaning "real failure". Unknown scheme -> Failed
// without calling anything; scheme-less / relative urls go to the optional
// fallback, or Failed when none was supplied.
#include "AssetResolver.hpp"
#include "SchemeRouter.hpp"

#include "doctest/doctest.h"
#include <cstdint>
#include <map>
#include <string>

using namespace x3d::runtime::extract;

// A backend that records every call into `log` (tag:url;) and answers Ready with
// a one-byte payload encoding the tag.
static AssetResolver tagged(const std::string &tag, std::string *log) {
  return [tag, log](const std::string &url, AssetKind) -> AssetResult {
    if (log)
      *log += tag + ":" + url + ";";
    return AssetResult::makeReady({static_cast<std::uint8_t>(tag[0])});
  };
}

TEST_CASE("scheme_router_test") {
  SUBCASE("dispatches by scheme, case-insensitively") {
    std::string log;
    AssetResolver router = makeSchemeRouter(
        {{"http", tagged("H", &log)},
         {"https", tagged("S", &log)},
         {"s3", tagged("B", &log)}});

    CHECK((router("http://host/a.png", AssetKind::Texture).bytes ==
           std::vector<std::uint8_t>{static_cast<std::uint8_t>('H')}));
    CHECK((log == "H:http://host/a.png;"));
    log.clear();

    // Scheme match is case-insensitive (RFC 3986).
    CHECK((router("HTTPS://host/a.png", AssetKind::Texture).bytes ==
           std::vector<std::uint8_t>{static_cast<std::uint8_t>('S')}));
    CHECK((log == "S:HTTPS://host/a.png;"));
  }

  SUBCASE("unknown scheme -> Failed, nothing called") {
    std::string log;
    AssetResolver router = makeSchemeRouter({{"http", tagged("H", &log)}});
    AssetResult r = router("ftp://host/a", AssetKind::Texture);
    CHECK(r.failed());
    CHECK(log.empty());
  }

  SUBCASE("scheme-less url -> Failed without a fallback") {
    std::string log;
    AssetResolver router = makeSchemeRouter({{"http", tagged("H", &log)}});
    CHECK(router("protos/MyProto.x3d", AssetKind::ExternProto).failed());
    CHECK(log.empty());
  }

  SUBCASE("Windows drive paths are paths, not schemes -> fallback") {
    std::string log;
    AssetResolver router =
        makeSchemeRouter({{"http", tagged("H", &log)}}, tagged("F", &log));

    // "C:" parses lexically as scheme "c"; a drive designator must instead take
    // the scheme-less fallback rather than route to a non-existent "c" backend.
    CHECK(router("C:\\models\\a.x3d", AssetKind::Texture).ready());
    CHECK(router("C:/models/a.x3d", AssetKind::Texture).ready());
    CHECK(router("c:a.x3d", AssetKind::Texture).ready());
    CHECK(router("c:", AssetKind::Texture).ready());
    CHECK(log == "F:C:\\models\\a.x3d;F:C:/models/a.x3d;F:c:a.x3d;F:c:;");

    // Without a fallback a drive path is Failed (never handed to a backend).
    log.clear();
    AssetResolver noFallback =
        makeSchemeRouter({{"http", tagged("H", &log)}});
    CHECK(noFallback("C:\\models\\a.x3d", AssetKind::Texture).failed());
    CHECK(log.empty());
  }

  SUBCASE("scheme-less url -> fallback; unknown scheme ignores the fallback") {
    std::string log;
    AssetResolver router =
        makeSchemeRouter({{"http", tagged("H", &log)}}, tagged("F", &log));

    CHECK(router("protos/MyProto.x3d", AssetKind::ExternProto).ready());
    CHECK(log == "F:protos/MyProto.x3d;");
    log.clear();

    // A named-but-unregistered scheme is still Failed, not the fallback's job.
    CHECK(router("urn:x3d:foo", AssetKind::ExternProto).failed());
    CHECK(log.empty());

    // The routed backend still wins for a matching scheme.
    CHECK(router("http://host/a", AssetKind::Texture).ready());
    CHECK(log == "H:http://host/a;");
  }

  SUBCASE("routing keys tolerate scheme delimiters and case") {
    std::string log;
    AssetResolver router = makeSchemeRouter(
        {{"HTTP://", tagged("H", &log)}, {"S3:", tagged("B", &log)}});
    CHECK(router("http://host/a", AssetKind::Texture).ready());
    CHECK(router("s3://bucket/key", AssetKind::Texture).ready());
    CHECK(log == "H:http://host/a;B:s3://bucket/key;");
  }

  SUBCASE("empty backend slot -> Failed, nothing called") {
    std::string log;
    AssetResolver router = makeSchemeRouter({{"http", AssetResolver{}}});
    CHECK(router("http://host/a", AssetKind::Texture).failed());
    CHECK(log.empty());
  }

  SUBCASE("kind is forwarded verbatim") {
    AssetKind seen = AssetKind::Texture;
    AssetResolver router = makeSchemeRouter(
        {{"http", [&seen](const std::string &, AssetKind k) -> AssetResult {
           seen = k;
           return AssetResult::makeReady({});
         }}});
    (void)router("http://host/a", AssetKind::ExternProto);
    CHECK(seen == AssetKind::ExternProto);
  }
}
