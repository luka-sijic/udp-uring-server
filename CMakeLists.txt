cmake_minimum_required(VERSION 3.20)
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN    "Enable AddressSanitizer/UBSan (Debug-ish builds)" OFF)
option(ENABLE_LTO     "Enable link-time optimization (Release-ish builds)" OFF)

set(APP_SOURCES
  src/main.cpp
  src/net/server.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBURING REQUIRED IMPORTED_TARGET liburing)
  list(APPEND APP_SOURCES src/net/uring_driver.cpp)
else()
  find_package(Boost REQUIRED)
  list(APPEND APP_SOURCES src/net/asio_driver.cpp)
endif()

add_executable(app ${APP_SOURCES})

target_include_directories(app PRIVATE include)

add_compile_options(
  -Wall -Wextra -Wpedantic 
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(app PRIVATE PkgConfig::LIBURING)
else()
  target_link_libraries(app PRIVATE Boost::headers)
endif()

if (ENABLE_ASAN)
  target_compile_options(app PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(app PRIVATE -fsanitize=address,undefined)
endif()

if (ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_err)
  if (ipo_ok)
    set_property(TARGET app PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_err}")
  endif()
endif()
