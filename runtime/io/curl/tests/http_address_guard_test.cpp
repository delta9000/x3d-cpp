// Exercise the private resolved-address classifier without opening a socket.
#include "doctest/doctest.h"
#include "../HttpResolver.cpp"


TEST_CASE("special-purpose IPv4 ranges are blocked in every IPv6 embedding") {
  using namespace x3d::runtime::io::curl;
  for (std::uint32_t address : {
           0xE0000000u, 0xEFFFFFFFu, // multicast endpoints
           0xF0000000u, 0xFFFFFFFFu, // reserved endpoints and limited broadcast
           0xC0000000u, 0xC00000FFu, // protocol assignments endpoints
           0xC6120000u, 0xC613FFFFu  // benchmarking endpoints
       }) {
    REQUIRE((isBlockedV4(address)));
    sockaddr_in v4{};
    v4.sin_family = AF_INET;
    v4.sin_addr.s_addr = htonl(address);
    REQUIRE((isBlockedSockaddr(reinterpret_cast<const sockaddr*>(&v4))));

    // All supported IPv4-embedding forms must apply the same IPv4 policy.
    for (int prefix = 0; prefix < 3; ++prefix) {
      sockaddr_in6 v6{};
      v6.sin6_family = AF_INET6;
      auto* bytes = v6.sin6_addr.s6_addr;
      if (prefix == 1) bytes[10] = bytes[11] = 0xFF;
      if (prefix == 2) { bytes[1] = 0x64; bytes[2] = 0xFF; bytes[3] = 0x9B; }
      for (int i = 0; i < 4; ++i)
        bytes[12 + i] = static_cast<unsigned char>(address >> (24 - 8 * i));
      REQUIRE((isBlockedSockaddr(reinterpret_cast<const sockaddr*>(&v6))));
    }
  }
}

TEST_CASE("addresses just outside the blocked ranges are allowed") {
  using namespace x3d::runtime::io::curl;
  for (std::uint32_t address : {0xDFFFFFFFu, 0xC0000100u,
                                0xC611FFFFu, 0xC6140000u, 0x08080808u})
    REQUIRE((!isBlockedV4(address)));
}
