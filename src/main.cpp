#include <exception>
#include <iostream>

#include "net/server.hpp"

#ifdef __linux__
#include "net/uring_driver.hpp"
#else
#include "net/asio_driver.hpp"
#endif

static const auto fast_io = []() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  return 0;
}();

int main() {
  try {
    const std::uint16_t port = 9000;

#ifdef __linux__
    int fd = Server::make_listen_socket(port);
    UringDriver driver(fd);
    driver.start();
#else
    AsioDriver driver(port);
    driver.start();
#endif

    return 0;

  } catch (const std::exception &e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}
