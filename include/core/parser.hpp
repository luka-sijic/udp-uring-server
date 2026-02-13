#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <span>

#include "models/net.hpp"

/*
op - 4 bytes
id - 4 bytes
x - 4 bytes
y - 4 bytes
color - 1 bytes
size - 4 bytes

Init Packet?


*/

struct Header {
  uint8_t magic;
  uint8_t type;
  uint16_t len;
  uint32_t seq;
};

class Parser {
public:
  Parser() = default;

  std::optional<Players> parse(std::span<const std::byte> bytes) const noexcept {
    auto payload = extract_payload(bytes);
    if (!payload.has_value()) {
      return std::nullopt;
    }

    const auto wire = payload.value();
    if (wire.size() == kWire24) {
      return decode24(wire);
    }
    if (wire.size() == kWire21) {
      return decode21(wire);
    }
    return std::nullopt;
  }

private:
  enum class Endian { Little, Big };

  static constexpr std::size_t kWire21 = 21;
  static constexpr std::size_t kWire24 = 24;
  static constexpr std::size_t kMaxAbsCoord = 1000000;

  static std::uint16_t read_u16(std::span<const std::byte> bytes,
                                std::size_t off, Endian e) noexcept {
    const auto b0 = static_cast<std::uint8_t>(bytes[off + 0]);
    const auto b1 = static_cast<std::uint8_t>(bytes[off + 1]);
    if (e == Endian::Little) {
      return static_cast<std::uint16_t>(b0 | (static_cast<std::uint16_t>(b1) << 8));
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(b0) << 8) | b1);
  }

  static std::uint32_t read_u32(std::span<const std::byte> bytes,
                                std::size_t off, Endian e) noexcept {
    const auto b0 = static_cast<std::uint8_t>(bytes[off + 0]);
    const auto b1 = static_cast<std::uint8_t>(bytes[off + 1]);
    const auto b2 = static_cast<std::uint8_t>(bytes[off + 2]);
    const auto b3 = static_cast<std::uint8_t>(bytes[off + 3]);
    if (e == Endian::Little) {
      return static_cast<std::uint32_t>(b0) |
             (static_cast<std::uint32_t>(b1) << 8) |
             (static_cast<std::uint32_t>(b2) << 16) |
             (static_cast<std::uint32_t>(b3) << 24);
    }
    return (static_cast<std::uint32_t>(b0) << 24) |
           (static_cast<std::uint32_t>(b1) << 16) |
           (static_cast<std::uint32_t>(b2) << 8) |
           static_cast<std::uint32_t>(b3);
  }

  static float read_f32(std::span<const std::byte> bytes, std::size_t off,
                        Endian e) noexcept {
    const auto bits = read_u32(bytes, off, e);
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    float out = 0.0f;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
  }

  static bool valid_coords(const Players &p) noexcept {
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
      return false;
    }
    return std::fabs(p.x) <= static_cast<float>(kMaxAbsCoord) &&
           std::fabs(p.y) <= static_cast<float>(kMaxAbsCoord);
  }

  static bool valid_op(const Players &p) noexcept { return p.op <= 2; }

  static bool looks_sane(const Players &p) noexcept {
    return valid_op(p) && valid_coords(p);
  }

  static Players decode24_with_endian(std::span<const std::byte> wire,
                                      Endian e) noexcept {
    Players p{};
    p.op = read_u32(wire, 0, e);
    p.id = read_u32(wire, 4, e);
    p.x = read_f32(wire, 8, e);
    p.y = read_f32(wire, 12, e);
    // For the 24-byte layout, color is a standalone byte at offset 16.
    // Offsets 17..19 are padding before size at offset 20.
    p.color = static_cast<std::uint8_t>(wire[16]);
    p.size = read_u32(wire, 20, e);
    return p;
  }

  static Players decode21_with_endian(std::span<const std::byte> wire,
                                      Endian e) noexcept {
    Players p{};
    p.op = read_u32(wire, 0, e);
    p.id = read_u32(wire, 4, e);
    p.x = read_f32(wire, 8, e);
    p.y = read_f32(wire, 12, e);
    p.color = static_cast<std::uint8_t>(wire[16]);
    p.size = read_u32(wire, 17, e);
    return p;
  }

  static std::optional<Players> choose_best(const Players &le,
                                            const Players &be) noexcept {
    const auto le_ok = looks_sane(le);
    const auto be_ok = looks_sane(be);

    if (le_ok && !be_ok) {
      return le;
    }
    if (be_ok && !le_ok) {
      return be;
    }
    if (le_ok && be_ok) {
      const auto le_mag = std::fabs(le.x) + std::fabs(le.y);
      const auto be_mag = std::fabs(be.x) + std::fabs(be.y);
      return (be_mag < le_mag) ? std::optional<Players>(be)
                               : std::optional<Players>(le);
    }

    if (valid_op(le) && std::isfinite(le.x) && std::isfinite(le.y)) {
      return le;
    }
    if (valid_op(be) && std::isfinite(be.x) && std::isfinite(be.y)) {
      return be;
    }
    return std::nullopt;
  }

  static std::optional<Players> decode24(std::span<const std::byte> wire) noexcept {
    const auto le = decode24_with_endian(wire, Endian::Little);
    const auto be = decode24_with_endian(wire, Endian::Big);
    return choose_best(le, be);
  }

  static std::optional<Players> decode21(std::span<const std::byte> wire) noexcept {
    const auto le = decode21_with_endian(wire, Endian::Little);
    const auto be = decode21_with_endian(wire, Endian::Big);
    return choose_best(le, be);
  }

  static std::optional<std::span<const std::byte>>
  extract_payload(std::span<const std::byte> bytes) noexcept {
    if (bytes.size() == kWire21 || bytes.size() == kWire24) {
      return bytes;
    }

    if (bytes.size() >= sizeof(Header)) {
      const auto payload_le =
          static_cast<std::size_t>(read_u16(bytes, 2, Endian::Little));
      const auto payload_be =
          static_cast<std::size_t>(read_u16(bytes, 2, Endian::Big));

      if ((payload_le == kWire21 || payload_le == kWire24) &&
          bytes.size() == sizeof(Header) + payload_le) {
        return bytes.subspan(sizeof(Header), payload_le);
      }

      if ((payload_be == kWire21 || payload_be == kWire24) &&
          bytes.size() == sizeof(Header) + payload_be) {
        return bytes.subspan(sizeof(Header), payload_be);
      }
    }

    return std::nullopt;
  }
};
