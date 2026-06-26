// runtime/io/tests/asset_resolver_swap_test.cpp
// U3 swap-test: the AssetResolver/IO genericity proof. Drives identical
// fixture bytes through Backend A (libcurl HTTP) and Backend B (AWS S3 SDK)
// and asserts byte-equal AssetResult.bytes, plus equal AssetStatus::Failed
// for missing keys.
//
// Self-contained: starts an in-process HTTP server on localhost serving the
// fixture directory, and (when $X3D_S3_ENDPOINT is set, i.e. docker minio is
// reachable) uploads the same fixtures to a minio bucket via the AWS SDK.
// Skips with a status message when minio is not available so local dev
// without docker doesn't get a spurious failure.
#include "AssetResolver.hpp"
#include "HttpResolver.hpp"
#include "S3Resolver.hpp"
#include "S3TestSupport.hpp"  // AWS-free fixture-seeding helper (x3d_s3_testsupport)

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace x3d::runtime::extract;
using x3d::runtime::extract::AssetResolver;

// ── In-process HTTP server (serves a directory of files on localhost) ──────
class FixtureServer {
 public:
  explicit FixtureServer(std::string root)
      : root_(std::move(root)), running_(false) {}

  ~FixtureServer() { stop(); }

  // Start listening on 127.0.0.1; assigns a free port. Returns the port.
  int start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
      std::perror("socket");
      return -1;
    }
    int yes = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  // ephemeral
    if (::bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) <
        0) {
      std::perror("bind");
      ::close(listen_fd_);
      return -1;
    }
    if (::listen(listen_fd_, 16) < 0) {
      std::perror("listen");
      ::close(listen_fd_);
      return -1;
    }

    socklen_t alen = sizeof(addr);
    if (::getsockname(listen_fd_, reinterpret_cast<sockaddr *>(&addr), &alen) <
        0) {
      std::perror("getsockname");
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

    // Parse request line: "GET /path HTTP/1.x"
    std::string req(buf, n);
    auto sp1 = req.find(' ');
    auto sp2 = req.find(' ', sp1 == std::string::npos ? 0 : sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) return;
    std::string path = req.substr(sp1 + 1, sp2 - sp1 - 1);

    // Strip query string.
    if (auto q = path.find('?'); q != std::string::npos) path = path.substr(0, q);
    if (path.empty() || path[0] != '/') {
      send404(cfd);
      return;
    }
    std::string rel = path.substr(1);
    // No path traversal.
    if (rel.find("..") != std::string::npos) {
      send404(cfd);
      return;
    }
    std::string full = root_ + "/" + rel;

    std::ifstream in(full, std::ios::binary);
    if (!in) {
      send404(cfd);
      return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string body = ss.str();
    send200(cfd, body);
  }

  static void send200(int cfd, const std::string &body) {
    std::ostringstream r;
    r << "HTTP/1.1 200 OK\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Content-Type: application/octet-stream\r\n"
      << "Connection: close\r\n\r\n"
      << body;
    auto s = r.str();
    ::send(cfd, s.data(), s.size(), 0);
  }

  static void send404(int cfd) {
    const char *r = "HTTP/1.1 404 Not Found\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
    ::send(cfd, r, std::strlen(r), 0);
  }

  std::string root_;
  std::atomic<bool> running_;
  std::thread thread_;
  int listen_fd_ = -1;
  int port_ = 0;
};

// ── Helpers ────────────────────────────────────────────────────────────────
std::vector<std::uint8_t> readFile(const std::string &p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string &s = ss.str();
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

int main() {
  // Locate fixtures relative to the binary's working dir (ctest sets cwd to
  // the build dir). The CMake target passes the source dir as a compile def.
#ifndef FIXTURES_DIR
#define FIXTURES_DIR "runtime/io/tests/fixtures"
#endif

  const std::vector<std::string> fixtures = {
      std::string(FIXTURES_DIR) + "/f1.bin",
      std::string(FIXTURES_DIR) + "/f2.bin",
      std::string(FIXTURES_DIR) + "/f3.bin",
  };

  // 1. Start HTTP server.
  FixtureServer srv(FIXTURES_DIR);
  int port = srv.start();
  assert(port > 0);
  std::cerr << "[swap-test] HTTP server on http://127.0.0.1:" << port << "\n";

  // 2. Resolve backend A (libcurl HTTP).
  auto backendA = x3d::runtime::io::curl::makeHttpResolver();
  assert(static_cast<bool>(backendA));

  // 3. Resolve backend B (AWS S3). Skip the swap loop if minio isn't up.
  const char *endpoint_env = std::getenv("X3D_S3_ENDPOINT");
  if (!endpoint_env || !*endpoint_env) {
    std::cerr << "[swap-test] SKIP: $X3D_S3_ENDPOINT unset (no minio). "
                 "Set it (e.g. http://localhost:9000) and create bucket "
                 "'fixtures' to run the swap-test.\n";
    // Still validate HTTP success path so the wiring is exercised.
    for (const auto &p : fixtures) {
      auto fname = p.substr(p.find_last_of('/') + 1);
      std::string url = "http://127.0.0.1:" + std::to_string(port) + "/" + fname;
      AssetResult r = backendA(url, AssetKind::Texture);
      assert(r.ready());
      auto expected = readFile(p);
      assert(r.bytes.size() == expected.size());
      assert(std::equal(r.bytes.begin(), r.bytes.end(), expected.begin()));
    }
    std::cerr << "[swap-test] HTTP-only path OK (3 fixtures).\n";
    return 0;
  }

  // 4. Upload fixtures to minio.
  const std::string endpoint = endpoint_env;
  const std::string bucket = "x3d-swap-fixtures";

  // For localhost endpoints (docker minio), default to minioadmin/minioadmin
  // so local dev doesn't need to set env vars. For non-localhost endpoints
  // (real AWS), require AWS_ACCESS_KEY_ID + AWS_SECRET_ACCESS_KEY in env.
  if (endpoint.find("localhost") != std::string::npos ||
      endpoint.find("127.0.0.1") != std::string::npos) {
    if (!std::getenv("AWS_ACCESS_KEY_ID"))
      ::setenv("AWS_ACCESS_KEY_ID", "minioadmin", 0);
    if (!std::getenv("AWS_SECRET_ACCESS_KEY"))
      ::setenv("AWS_SECRET_ACCESS_KEY", "minioadmin", 0);
  }

  if (!x3d::runtime::io::s3::s3EnsureBucket(endpoint, bucket)) {
    std::cerr << "[swap-test] FAIL: could not create/access bucket on minio\n";
    return 1;
  }
  for (const auto &p : fixtures) {
    auto fname = p.substr(p.find_last_of('/') + 1);
    if (!x3d::runtime::io::s3::s3PutObject(endpoint, bucket, fname,
                                           readFile(p))) {
      std::cerr << "[swap-test] FAIL: upload " << fname << " to minio failed\n";
      return 1;
    }
  }
  std::cerr << "[swap-test] Uploaded 3 fixtures to s3://" << bucket << "/\n";

  auto backendB = x3d::runtime::io::s3::makeS3Resolver(endpoint);

  // 5. Swap-test loop: same logical content via A and B → bytes equal.
  for (const auto &p : fixtures) {
    auto fname = p.substr(p.find_last_of('/') + 1);
    std::string urlA = "http://127.0.0.1:" + std::to_string(port) + "/" + fname;
    std::string urlB = "s3://" + bucket + "/" + fname;

    AssetResult rA = backendA(urlA, AssetKind::Texture);
    AssetResult rB = backendB(urlB, AssetKind::Texture);

    assert(rA.ready());
    assert(rB.ready());
    assert(rA.bytes.size() == rB.bytes.size());
    assert(std::equal(rA.bytes.begin(), rA.bytes.end(), rB.bytes.begin()));
    std::cerr << "[swap-test] OK " << fname << " (" << rA.bytes.size()
              << " bytes, A==B)\n";
  }

  // 6. Failure parity: missing key returns Failed on both backends.
  {
    AssetResult rA = backendA("http://127.0.0.1:" + std::to_string(port) +
                                  "/does-not-exist.bin",
                              AssetKind::Texture);
    AssetResult rB = backendB("s3://" + bucket + "/does-not-exist.bin",
                              AssetKind::Texture);
    assert(rA.failed());
    assert(rB.failed());
    assert(rA.bytes.empty());
    assert(rB.bytes.empty());
    std::cerr << "[swap-test] OK missing-key parity (both Failed)\n";
  }

  std::cerr << "[swap-test] ALL OK\n";
  return 0;
}
