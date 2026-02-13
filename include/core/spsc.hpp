#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

template <typename T>
class SPSC {
public:
  explicit SPSC(std::size_t capacity)
      : capacity_(capacity < 2 ? 2 : capacity),
        buf_(std::make_unique<T[]>(capacity_)) {}

  bool push(const T &item) noexcept {
    const auto write_idx = write_idx_.load(std::memory_order_relaxed);
    const auto read_idx = read_idx_.load(std::memory_order_acquire);
    const auto next = (write_idx + 1) % capacity_;

    if (next == read_idx) {
      return false;
    }

    buf_[write_idx] = item;
    write_idx_.store(next, std::memory_order_release);
    return true;
  }

  bool push(T &&item) noexcept {
    const auto write_idx = write_idx_.load(std::memory_order_relaxed);
    const auto read_idx = read_idx_.load(std::memory_order_acquire);
    const auto next = (write_idx + 1) % capacity_;

    if (next == read_idx) {
      return false;
    }

    buf_[write_idx] = std::move(item);
    write_idx_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T &item) noexcept {
    const auto write_idx = write_idx_.load(std::memory_order_acquire);
    const auto read_idx = read_idx_.load(std::memory_order_relaxed);

    if (read_idx == write_idx) {
      return false;
    }

    item = std::move(buf_[read_idx]);
    const auto next = (read_idx + 1) % capacity_;
    read_idx_.store(next, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool empty() const noexcept {
    const auto read_idx = read_idx_.load(std::memory_order_relaxed);
    const auto write_idx = write_idx_.load(std::memory_order_relaxed);
    return read_idx == write_idx;
  }

private:
  std::size_t capacity_;
  std::unique_ptr<T[]> buf_;
  alignas(64) std::atomic<std::size_t> read_idx_{0};
  alignas(64) std::atomic<std::size_t> write_idx_{0};
};
