#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <span>

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
  // magic
  std::uint32_t op; // 0 register 1 player update
  std::uint32_t id;
  float x;
  float y;
  std::uint8_t color;
  std::uint32_t size;
};
