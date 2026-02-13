#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <utility>

#include "core/log.hpp"
#include "core/parser.hpp"
#include "core/spsc.hpp"
#include "models/net.hpp"
#include "net/net_out.hpp"

class Router {
public:
  explicit Router(INetOut &out) : out_(out), q_(kQueueCapacity) {
    running_.store(true, std::memory_order_relaxed);
    worker_ = std::thread(&Router::poll, this);
  }

  ~Router() noexcept {
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  Router(const Router &) = delete;
  Router &operator=(const Router &) = delete;

  void enqueue_packet(const PacketView &pkt) noexcept {
    if (pkt.bytes.size() > kMaxPacketBytes) {
      UDP_LOGLN("packet too large for router queue: " << pkt.bytes.size());
      return;
    }

    QueuedPacket qp{};
    qp.peer = pkt.peer;
    qp.peer_len = pkt.peer_len;
    qp.len = pkt.bytes.size();
    if (qp.len > 0) {
      std::memcpy(qp.bytes.data(), pkt.bytes.data(), qp.len);
    }

    if (!q_.push(std::move(qp))) {
      UDP_LOGLN("router queue full: dropping packet");
    }
  }

  void on_packet(const PacketView &pkt) {
    auto decoded_opt = parser_.parse(pkt.bytes);
    if (!decoded_opt.has_value()) {
      UDP_LOGLN("failed to parse packet: got " << pkt.bytes.size() << " bytes");
      return;
    }
    const auto decoded = decoded_opt.value();
    UDP_LOGLN(decoded.op << " " << decoded.id << " " << decoded.x << " "
                         << decoded.y);
    switch (decoded.op) {
    case 0:
      on_register(pkt, decoded);
      break;
    case 1:
      on_player(pkt, decoded);
      break;
    case 2:
      on_update(pkt, decoded);
      break;
    default:
      UDP_LOGLN("Unknown OP recv");
      break;
    }
  }

private:
  static constexpr std::size_t kQueueCapacity = 1024;
  static constexpr std::size_t kMaxPacketBytes = 2048;

  struct QueuedPacket {
    sockaddr_storage peer{};
    socklen_t peer_len{};
    std::array<std::byte, kMaxPacketBytes> bytes{};
    std::size_t len{};
  };

  void poll() noexcept {
    while (running_.load(std::memory_order_acquire) || !q_.empty()) {
      QueuedPacket qp{};
      if (!q_.pop(qp)) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        continue;
      }

      PacketView pkt{
          qp.peer,
          qp.peer_len,
          std::span<const std::byte>(qp.bytes.data(), qp.len),
      };
      on_packet(pkt);
    }
  }

  void on_register(const PacketView &pkt, const Players &p) {
    players_[p.id] = {pkt.peer, pkt.peer_len};
    UDP_LOGLN("Player added: " << p.id << " " << p.op);
    // Register is a control message for server state; do not rebroadcast as op=0.
  }

  void on_player(const PacketView &pkt, const Players &p) {
    players_[p.id] = {pkt.peer, pkt.peer_len};
    UDP_LOGLN("Player packet: " << p.id);
    broadcast_all(pkt.bytes.data(), pkt.bytes.size());
  }

  void broadcast_all(const void *data, size_t len) {
    for (auto const &[id, peer] : players_) {
      out_.send_to(peer.addr, peer.len, data, len);
    }
  }

  void broadcast_all_except(const void *data, size_t len, int pid) {
    for (auto const &[id, peer] : players_) {
      if (id != pid) {
        out_.send_to(peer.addr, peer.len, data, len);
      }
    }
  }

  void on_update(const PacketView &pkt, const Players &p) {
    UDP_LOGLN("Sending data...");
    (void)p;
    broadcast_all(pkt.bytes.data(), pkt.bytes.size());
  }

  Parser parser_;
  INetOut &out_;
  std::unordered_map<uint32_t, PeerInfo> players_;
  SPSC<QueuedPacket> q_;
  std::atomic<bool> running_{false};
  std::thread worker_;
};
