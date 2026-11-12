// HTTP fixture bytes and missing-asset contract through AssetResolver.
#include "doctest/doctest.h"
#include "AssetResolver.hpp"
#include "HttpResolver.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
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

// In-process HTTP server (serves a directory of files on localhost)
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

// Helpers
std::vector<std::uint8_t> readFile(const std::string &p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string &s = ss.str();
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

TEST_CASE("asset_resolver_swap_test") {
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
  REQUIRE((port > 0));
  std::cerr << "[swap-test] HTTP server on http://127.0.0.1:" << port << "\n";

  // 2. Resolve backend A (libcurl HTTP). The fixture server is on 127.0.0.1,
  //    so opt in to private-network destinations (the default blocks them).
  auto backendA = x3d::runtime::io::curl::makeHttpResolver(
      x3d::runtime::io::curl::HttpResolverOptions{
          /*allowPrivateNetworks=*/true});
  REQUIRE((static_cast<bool>(backendA)));

  for (const auto &p : fixtures) {
    auto fname = p.substr(p.find_last_of('/') + 1);
    std::string url = "http://127.0.0.1:" + std::to_string(port) + "/" + fname;
    AssetResult r = backendA(url, AssetKind::Texture);
    REQUIRE((r.ready()));
    auto expected = readFile(p);
    REQUIRE((r.bytes.size() == expected.size()));
    REQUIRE((std::equal(r.bytes.begin(), r.bytes.end(), expected.begin())));
  }

  AssetResult missing = backendA("http://127.0.0.1:" + std::to_string(port) +
                                     "/does-not-exist.bin",
                                 AssetKind::Texture);
  REQUIRE((missing.failed()));
  REQUIRE((missing.bytes.empty()));
}
