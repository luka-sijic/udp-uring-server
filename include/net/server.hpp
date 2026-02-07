#pragma once

#include <cstdint>

struct ServerConfig {
  uint16_t port;
  uint16_t threads;
};

class Server {
public:
  explicit Server(ServerConfig cfg) : port_(cfg.port), threads_(cfg.threads) {}
  int init();
  //~Server();

  Server(Server &&) = delete;
  Server &operator=(Server &&) = delete;

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  void start();

private:
  uint16_t port_;
  uint16_t threads_;
};
