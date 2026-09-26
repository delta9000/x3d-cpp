// runtime/io/curl/HttpResolver.cpp — libcurl HTTP backend for the
// AssetResolver seam. The single TU where libcurl meets the seam (mirrors the
// QuickJsBackend.cpp isolation discipline).
#include "HttpResolver.hpp"

#include <curl/curl.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace x3d::runtime::io::curl {

namespace {

bool isHttpUrl(const std::string &url) {
  return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
}

// ── Resolved-address guard (SEC-6) ──────────────────────────────────────────
// Runs in CURLOPT_OPENSOCKETFUNCTION, i.e. on the actual sockaddr libcurl is
// about to connect to — after DNS resolution and for every redirect hop. A
// hostname or a redirect therefore cannot bypass the check.

bool isBlockedV4(std::uint32_t a /* host byte order */) {
  if ((a >> 24) == 0) return true;     // 0.0.0.0/8  "this network" / unspecified
  if ((a >> 24) == 127) return true;   // 127.0.0.0/8   loopback
  if ((a >> 24) == 10) return true;    // 10.0.0.0/8    RFC1918
  if ((a >> 16) == 0xA9FE) return true;  // 169.254.0.0/16 link-local
  if ((a >> 20) == 0xAC1) return true;   // 172.16.0.0/12  RFC1918
  if ((a >> 16) == 0xC0A8) return true;  // 192.168.0.0/16 RFC1918
  if ((a >> 22) == 0x191) return true;   // 100.64.0.0/10  CGNAT
  return false;
}

// Classifies the IPv4 address embedded in the low 4 bytes of either an
// IPv4-mapped (::ffff:a.b.c.d), IPv4-compatible (::a.b.c.d) or NAT64
// (64:ff9b::/96) IPv6 address.
bool isBlockedEmbeddedV4(const unsigned char *b) {
  const std::uint32_t v4 = (static_cast<std::uint32_t>(b[12]) << 24) |
                           (static_cast<std::uint32_t>(b[13]) << 16) |
                           (static_cast<std::uint32_t>(b[14]) << 8) |
                           static_cast<std::uint32_t>(b[15]);
  return isBlockedV4(v4);
}

bool isBlockedV6(const unsigned char *b) {
  static const unsigned char kZero[16] = {};
  static const unsigned char kNat64[12] = {0x00, 0x64, 0xFF, 0x9B};  // 64:ff9b::/96
  if (std::memcmp(b, kZero, 16) == 0) return true;              // ::
  if (std::memcmp(b, kZero, 15) == 0 && b[15] == 1) return true;  // ::1
  if (b[0] == 0xFE && (b[1] & 0xC0) == 0x80) return true;      // fe80::/10
  if ((b[0] & 0xFE) == 0xFC) return true;                      // fc00::/7
  if (std::memcmp(b, kZero, 10) == 0 && b[10] == 0xFF &&
      b[11] == 0xFF) {  // ::ffff:a.b.c.d — classify as IPv4
    return isBlockedEmbeddedV4(b);
  }
  if (std::memcmp(b, kNat64, 12) == 0) {  // 64:ff9b::/96 NAT64 — classify the
    return isBlockedEmbeddedV4(b);        // embedded IPv4 (b[12..15])
  }
  if (std::memcmp(b, kZero, 12) == 0) {  // ::a.b.c.d IPv4-compatible (deprecated,
    return isBlockedEmbeddedV4(b);       // but still routable) — classify as IPv4
  }
  return false;
}

bool isBlockedSockaddr(const struct sockaddr *sa) {
  if (sa == nullptr) return true;
  if (sa->sa_family == AF_INET) {
    const auto *in4 = reinterpret_cast<const struct sockaddr_in *>(sa);
    return isBlockedV4(ntohl(in4->sin_addr.s_addr));
  }
  if (sa->sa_family == AF_INET6) {
    const auto *in6 = reinterpret_cast<const struct sockaddr_in6 *>(sa);
    return isBlockedV6(in6->sin6_addr.s6_addr);
  }
  return false;  // not an IP family we can classify; leave it to libcurl
}

curl_socket_t openSocketCb(void *clientp, curlsocktype /*purpose*/,
                           struct curl_sockaddr *address) {
  const auto *opts = static_cast<const HttpResolverOptions *>(clientp);
  if (!opts->allowPrivateNetworks && isBlockedSockaddr(&address->addr)) {
    return CURL_SOCKET_BAD;  // abort the connect -> CURLE_COULDNT_CONNECT
  }
  return ::socket(address->family, address->socktype, address->protocol);
}

// ── Size-capped write sink ──────────────────────────────────────────────────

struct WriteSink {
  std::vector<std::uint8_t> bytes;
  std::uint64_t maxBytes = 0;
};

std::size_t writeCb(char *ptr, std::size_t size, std::size_t nmemb,
                    void *userdata) {
  auto *sink = static_cast<WriteSink *>(userdata);
  const std::uint64_t total =
      static_cast<std::uint64_t>(size) * static_cast<std::uint64_t>(nmemb);
  if (static_cast<std::uint64_t>(sink->bytes.size()) + total > sink->maxBytes) {
    return 0;  // abort the transfer -> CURLE_WRITE_ERROR -> Failed
  }
  sink->bytes.insert(sink->bytes.end(), reinterpret_cast<std::uint8_t *>(ptr),
                     reinterpret_cast<std::uint8_t *>(ptr) + total);
  return static_cast<std::size_t>(total);
}

}  // namespace

x3d::runtime::extract::AssetResolver makeHttpResolver(
    HttpResolverOptions options) {
  return [options](const std::string &url,
                   x3d::runtime::extract::AssetKind /*kind*/)
             -> x3d::runtime::extract::AssetResult {
    if (!isHttpUrl(url)) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    WriteSink sink;
    sink.maxBytes = options.maxBytes;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, options.maxRedirects);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE,
                     static_cast<curl_off_t>(options.maxBytes));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, options.timeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // Block private/loopback/link-local resolved addresses unless opted in.
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, &openSocketCb);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &options);

    // SEC-6: the guard above runs on the sockaddr libcurl actually connects to.
    // With http_proxy/https_proxy/all_proxy in the environment libcurl connects
    // to the *proxy*, so the guard would inspect the proxy, not the target, and
    // a private target is reachable through it. Disable environment proxies in
    // the hardened default so the guard always sees the real destination.
    if (!options.allowPrivateNetworks) {
      curl_easy_setopt(curl, CURLOPT_PROXY, "");
    }

    // Restrict both the request and every redirect to http(s).
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
    const long http_protos =
        static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, http_protos);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, http_protos);
#endif

    const CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    if (rc == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    if (http_code < 200 || http_code >= 300) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    if (sink.bytes.empty()) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    return x3d::runtime::extract::AssetResult::makeReady(
        std::move(sink.bytes));
  };
}

}  // namespace x3d::runtime::io::curl
