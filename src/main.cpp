#include <exception>
#include <iostream>

#include "net/server.hpp"

static const auto fast_io = []() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  return 0;
}();

int main() {
  try {
    ServerConfig cfg{.port = 9000, .threads = 4};
    Server srv(cfg);
    srv.start();

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}
