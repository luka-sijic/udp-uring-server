# Low latency real-time UDP telemetry server (C++23, io_uring, boost asio)

## Build Instructions
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build/debug -j && ./build/debug/app