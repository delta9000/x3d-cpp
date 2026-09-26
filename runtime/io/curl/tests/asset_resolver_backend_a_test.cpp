// runtime/io/curl/tests/asset_resolver_backend_a_test.cpp
// U1 per-backend test: verify HttpResolver wiring + URL prefix check +
// libcurl error path, plus the SEC-6 hardening (loopback blocked by default,
// response-size cap, redirect to a non-http scheme rejected). The success path
// is tested in U3 (swap-test) against an in-process HTTP server — here a small
// in-process server drives only the security assertions, keeping the test
// hermetic + offline.
#include "HttpResolver.hpp"

#include "AssetResolver.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::runtime::io::curl;

namespace {

// Minimal single-connection-at-a-time HTTP server on 127.0.0.1 with the few
// routes the security assertions need.
class TestServer {
 public:
  int start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return -1;
    int yes = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) <
            0 ||
        ::listen(listen_fd_, 16) < 0) {
      ::close(listen_fd_);
      return -1;
    }
    socklen_t alen = sizeof(addr);
    if (::getsockname(listen_fd_, reinterpret_cast<sockaddr *>(&addr), &alen) <
        0) {
      ::close(listen_fd_);
      return -1;
    }
    port_ = ntohs(addr.sin_port);
    running_ = true;
    thread_ = std::thread([this] { serveLoop(); });
    return port_;
  }

  void stop() {
    if (!running_.exchange(false)) return;
    ::shutdown(listen_fd_, SHUT_RDWR);
    ::close(listen_fd_);
    if (thread_.joinable()) thread_.join();
  }

  ~TestServer() { stop(); }

 private:
  void serveLoop() {
    while (running_.load()) {
      sockaddr_in cli{};
      socklen_t clen = sizeof(cli);
      int cfd = ::accept(listen_fd_, reinterpret_cast<sockaddr *>(&cli), &clen);
      if (cfd < 0) {
        if (!running_.load()) return;
        continue;
      }
      handleClient(cfd);
      ::close(cfd);
    }
  }

  void handleClient(int cfd) {
    char buf[2048];
    ssize_t n = ::recv(cfd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';
    const std::string req(buf, static_cast<std::size_t>(n));
    auto sp1 = req.find(' ');
    auto sp2 = req.find(' ', sp1 == std::string::npos ? 0 : sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) return;
    std::string path = req.substr(sp1 + 1, sp2 - sp1 - 1);

    if (path == "/ok") {
      send200(cfd, "hello");
    } else if (path == "/big") {
      // No Content-Length (connection-close framing) so curl cannot reject on
      // the size hint alone — the write callback's cap is what must trigger.
      sendRaw(cfd, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream"
                   "\r\nConnection: close\r\n\r\n");
      std::string body(65536, 'x');
      ::send(cfd, body.data(), body.size(), 0);
    } else if (path == "/redir-file") {
      send302(cfd, "file:///etc/hosts");
    } else if (path == "/redir-ftp") {
      send302(cfd, "ftp://127.0.0.1:1/x");
    } else if (path == "/redir-ok") {
      send302(cfd, "/ok");
    } else if (path == "/loop") {
      send302(cfd, "/loop");
    } else {
      sendRaw(cfd, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n"
                   "Connection: close\r\n\r\n");
    }
  }

  static void sendRaw(int cfd, const std::string &s) {
    ::send(cfd, s.data(), s.size(), 0);
  }

  static void send200(int cfd, const std::string &body) {
    std::string r = "HTTP/1.1 200 OK\r\nContent-Length: " +
                    std::to_string(body.size()) +
                    "\r\nConnection: close\r\n\r\n" + body;
    sendRaw(cfd, r);
  }

  static void send302(int cfd, const std::string &location) {
    std::string r = "HTTP/1.1 302 Found\r\nLocation: " + location +
                    "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    sendRaw(cfd, r);
  }

  std::atomic<bool> running_{false};
  std::thread thread_;
  int listen_fd_ = -1;
  int port_ = 0;
};

}  // namespace

int main() {
  // 1. URL prefix check: rejects non-http schemes with Failed, no network call.
  AssetResolver r = makeHttpResolver();
  assert(static_cast<bool>(r));
  assert(r("urn:x3d:foo", AssetKind::Texture).failed());
  assert(r("file:///etc/hosts", AssetKind::Texture).failed());
  assert(r("plain/path.txt", AssetKind::Texture).failed());
  assert(r("", AssetKind::Texture).failed());
  assert(r("HTTP://uppercase-scheme.example/x", AssetKind::Texture).failed());

  // 2. libcurl error path: unreachable host returns Failed.
  //    127.0.0.1:1 is reserved + unused; with the default options the
  //    resolved-address guard rejects loopback before the connect even starts.
  assert(r("http://127.0.0.1:1/x", AssetKind::Texture).failed());

  // 3. URL prefix check honors https:// just like http://; use a non-routable
  //    host so we exercise the prefix-accept path (DNS failure -> Failed).
  assert(r("https://nonexistent.invalid/x", AssetKind::Texture).failed());

  // 3b. The IPv6 guard classifies embedded IPv4 in forms other than the
  //     ::ffff: mapped prefix: NAT64 (64:ff9b::/96) and IPv4-compatible
  //     (::a.b.c.d). Both below embed 127.0.0.1 and are refused by default.
  assert(r("http://[64:ff9b::7f00:1]/", AssetKind::Texture).failed());
  assert(r("http://[::7f00:1]/", AssetKind::Texture).failed());

  // ── SEC-6 hardening, against an in-process loopback server ───────────────
  TestServer srv;
  const int port = srv.start();
  assert(port > 0);
  const std::string base = "http://127.0.0.1:" + std::to_string(port);

  // Opt-in resolver for the tests that must actually reach the local server.
  const AssetResolver local =
      makeHttpResolver(HttpResolverOptions{/*allowPrivateNetworks=*/true});

  // 4. Loopback blocked by default: the server is up and answers 200, yet the
  //    default resolver refuses to connect (private-address guard). The opt-in
  //    resolver fetches the same URL — proving the server really is reachable.
  assert(r(base + "/ok", AssetKind::Texture).failed());
  {
    AssetResult ok = local(base + "/ok", AssetKind::Texture);
    assert(ok.ready());
    assert(ok.bytes.size() == 5);
    assert(std::string(ok.bytes.begin(), ok.bytes.end()) == "hello");
  }

  // 5. Size cap enforced: a body past maxBytes aborts the transfer -> Failed,
  //    while a body under the same cap still succeeds.
  {
    HttpResolverOptions capped;
    capped.allowPrivateNetworks = true;
    capped.maxBytes = 4096;
    AssetResolver resolved = makeHttpResolver(capped);
    assert(resolved(base + "/ok", AssetKind::Texture).ready());
    assert(resolved(base + "/big", AssetKind::Texture).failed());
  }

  // 6. Redirect to a non-http scheme is rejected (REDIR_PROTOCOLS).
  assert(local(base + "/redir-file", AssetKind::Texture).failed());
  assert(local(base + "/redir-ftp", AssetKind::Texture).failed());

  // 7. Same-scheme redirects still follow; the redirect cap bounds a loop.
  assert(local(base + "/redir-ok", AssetKind::Texture).ready());
  {
    HttpResolverOptions capped;
    capped.allowPrivateNetworks = true;
    capped.maxRedirects = 5;
    AssetResolver resolved = makeHttpResolver(capped);
    assert(resolved(base + "/loop", AssetKind::Texture).failed());
  }

  return 0;
}
