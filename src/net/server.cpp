#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>

#include "net/server.hpp"

#ifdef __linux__
#include "net/uring_driver.hpp"
#else
#include "net/asio_driver.hpp"
#endif

int Server::make_listen_socket(uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  int one = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    ::close(fd);
    return -1;
  }

  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0) {
    perror("setsockopt(SO_REUSEPORT)");
    ::close(fd);
    return -1;
  }

  sockaddr_in addr{
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {.s_addr = htonl(INADDR_ANY)},
  };

  if (::bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    ::close(fd);
    return -1;
  }

  return fd;
}

void Server::start() {
  int fd = make_listen_socket(port_);
  std::cout << "Listening on 0.0.0.0:" << port_ << " (Ctrl+C to stop)\n";
#ifdef __linux__
  int fd = Server::make_listen_socket(port);
  UringDriver driver(fd);
  driver.start();
#else
  AsioDriver driver(port_);
  driver.start();
#endif
}
