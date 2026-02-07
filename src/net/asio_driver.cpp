#include "net/asio_driver.hpp"

#include <csignal>
#include <cstring>
#include <iostream>
#include <span>
#include <vector>

#include <arpa/inet.h>

using boost::asio::ip::udp;

AsioDriver::AsioDriver(std::uint16_t port)
    : io_(), socket_(io_), remote_(), router_(*this)
#if !defined(_WIN32)
      ,
      signals_(io_, SIGINT, SIGTERM)
#endif
{
  udp::endpoint ep(udp::v4(), port);
  socket_.open(ep.protocol());
  socket_.set_option(boost::asio::socket_base::reuse_address(true));
  socket_.bind(ep);

#if !defined(_WIN32)
  signals_.async_wait(
      [this](const boost::system::error_code &, int) { io_.stop(); });
#endif
}

void AsioDriver::start() {
  UDP_LOGLN("Starting Boost ASIO service...");
  start_receive();
  io_.run();
}

void AsioDriver::start_receive() {
  socket_.async_receive_from(
      boost::asio::buffer(buf_), remote_,
      [this](const boost::system::error_code &ec, std::size_t bytes) {
        if (!ec && bytes > 0) {
          sockaddr_storage ss{};
          std::memcpy(&ss, remote_.data(), remote_.size());
          socklen_t len = static_cast<socklen_t>(remote_.size());
          std::span<const std::byte> span(buf_.data(), bytes);
          PacketView pkt{ss, len, span};
          router_.on_packet(pkt);
        } else if (ec != boost::asio::error::operation_aborted) {
          UDP_LOGLN("asio recv error: " << ec.message());
        }
        start_receive();
      });
}

boost::asio::ip::udp::endpoint
AsioDriver::to_endpoint(const sockaddr_storage &dst,
                        socklen_t dst_len) noexcept {
  (void)dst_len;

  if (dst.ss_family == AF_INET) {
    auto *sin = reinterpret_cast<const sockaddr_in *>(&dst);
    auto addr = boost::asio::ip::address_v4(ntohl(sin->sin_addr.s_addr));
    return udp::endpoint(addr, ntohs(sin->sin_port));
  }

  if (dst.ss_family == AF_INET6) {
    auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(&dst);
    boost::asio::ip::address_v6::bytes_type bytes{};
    std::memcpy(bytes.data(), &sin6->sin6_addr, bytes.size());
    auto addr = boost::asio::ip::address_v6(bytes, sin6->sin6_scope_id);
    return udp::endpoint(addr, ntohs(sin6->sin6_port));
  }

  return udp::endpoint();
}

void AsioDriver::send_to(const sockaddr_storage &dst, socklen_t dst_len,
                         const void *data, size_t len) noexcept {
  if (len == 0)
    return;

  udp::endpoint ep = to_endpoint(dst, dst_len);
  if (ep.address().is_unspecified() || ep.port() == 0)
    return;

  auto payload = std::make_shared<std::vector<std::byte>>(len);
  std::memcpy(payload->data(), data, len);

  socket_.async_send_to(
      boost::asio::buffer(*payload), ep,
      [payload](const boost::system::error_code &ec, std::size_t) {
        if (ec) {
          UDP_LOGLN("asio send error: " << ec.message());
        }
      });
}
