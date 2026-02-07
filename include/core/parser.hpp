#pragma once
#include <cstring>
#include <iostream>
#include <models/net.hpp>
#include <optional>

/*
op - 4 bytes
id - 4 bytes
x - 4 bytes
y - 4 bytes
color - 4 bytes
size - 4 bytes

*/

struct Header {
  uint8_t magic;
  uint8_t type;
  uint16_t len;
  uint32_t seq;
};

class Parser {
public:
  Parser() {}

  std::optional<Players> parse(std::span<const std::byte> bytes) {
    if (bytes.size() < sizeof(Header))
      return std::nullopt;

    Players ply{};
    if (bytes.size() == (int)sizeof(Players)) {
      std::memcpy(&ply, bytes.data(), sizeof(Players));
    }
    return ply;
  }

private:
  int res_;
};
