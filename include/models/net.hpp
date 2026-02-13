#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <span>
#include <type_traits>

struct PacketView {
  sockaddr_storage peer;
  socklen_t peer_len;
  std::span<const std::byte> bytes;
};

struct PeerInfo {
  sockaddr_storage addr{};
  socklen_t len{};
};

struct Players {
  // 0 register, 1 player, 2 update
  std::uint32_t op;
  std::uint32_t id;
  float x;
  float y;
  std::uint8_t color;
  std::uint32_t size;
};

static_assert(std::is_standard_layout_v<Players>,
              "Players must remain standard layout");
static_assert(sizeof(Players) == 24,
              "Players wire contract changed: expected 24-byte layout");
