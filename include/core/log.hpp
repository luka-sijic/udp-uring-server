#pragma once
#include <iostream>

#ifndef UDP_LOG_ENABLED
#define UDP_LOG_ENABLED 1
#endif

// clangd auto formatter breaking this?
#if UDP_LOG_ENABLED
#define UDP_LOG(expr)                                                          \
  do {                                                                         \
    std::cerr << expr;                                                         \
  } while (0)
#define UDP_LOGLN(expr)                                                        \
  do {                                                                         \
    std::cerr << expr << '\n';                                                 \
  } while (0)
#else
#define UDP_LOG(expr)                                                          \
  do {                                                                         \
  } while (0)
#define UDP_LOGLN(expr)                                                        \
  do {                                                                         \
  } while (0)
#endif
