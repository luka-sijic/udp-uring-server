#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <boost/asio.hpp>

#include "core/router.hpp"
#include "net/net_out.hpp"

class AsioDriver : public INetOut {
public:
  explicit AsioDriver(std::uint16_t port);
  ~AsioDriver() override = default;

  void start();
  void send_to(const sockaddr_storage &dst, socklen_t dst_len, const void *data,
               size_t len) noexcept override;

private:
  void start_receive();
  static boost::asio::ip::udp::endpoint to_endpoint(
      const sockaddr_storage &dst, socklen_t dst_len) noexcept;

  boost::asio::io_context io_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remote_;
  std::array<std::byte, 2048> buf_{};
  Router router_;

#if !defined(_WIN32)
  boost::asio::signal_set signals_;
#endif
};
