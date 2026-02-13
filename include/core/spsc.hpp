#include <atomic>
#include <memory>

template <typename T>
class SPSC {
public:
    SPSC(size_t capacity) : capacity_(capacity), buf_{std::make_unique<T[]>(capacity)} {}
    
    bool push(const T& item) noexcept {
        auto writeIdx = writeIdx_.load(std::memory_order_relaxed);
        auto readIdx = readIdx_.load(std::memory_order_acquire);

        const auto next = (writeIdx + 1) & (capacity_ - 1);
        const auto isFull = next == readIdx;

        if (isFull) {
            return false;
        }

        buf_[writeIdx] = item;
        writeIdx_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        auto writeIdx = writeIdx_.load(std::memory_order_acquire);
        auto readIdx = readIdx_.load(std::memory_order_relaxed); 
        
        const auto isEmpty = readIdx == writeIdx;
        if (isEmpty) {
            return false;
        }

        item = std::move(buf_[readIdx]);
        const auto nextReadIdx = (readIdx + 1) & (capacity_ - 1);
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

private:
    size_t capacity_;
    std::unique_ptr<T[]> buf_;
    alignas(64) std::atomic<size_t> readIdx_;
    alignas(64) std::atomic<size_t> writeIdx_;
};