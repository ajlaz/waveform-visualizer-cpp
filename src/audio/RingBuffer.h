#pragma once
#include <atomic>
#include <vector>
#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// RingBuffer<T>
// Lock-free single-producer / single-consumer ring buffer.
// Capacity MUST be a power of two. The audio callback is the sole producer;
// the DSP thread is the sole consumer.
// ---------------------------------------------------------------------------
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : buf_(capacity), mask_(capacity - 1), head_(0), tail_(0)
    {
        assert(capacity >= 2 && (capacity & (capacity - 1)) == 0 &&
               "RingBuffer capacity must be a power of two");
    }

    // Producer side — called from the audio callback (real-time thread).
    // Returns the number of elements actually written (may be less than n
    // if the buffer is nearly full).
    size_t write(const T* data, size_t n) noexcept {
        const size_t head  = head_.load(std::memory_order_relaxed);
        const size_t avail = buf_.size() - (head - tail_.load(std::memory_order_acquire));
        n = std::min(n, avail);
        for (size_t i = 0; i < n; ++i)
            buf_[(head + i) & mask_] = data[i];
        head_.store(head + n, std::memory_order_release);
        return n;
    }

    // Consumer side — called from the DSP thread.
    // Returns the number of elements actually read.
    size_t read(T* out, size_t n) noexcept {
        const size_t tail  = tail_.load(std::memory_order_relaxed);
        const size_t avail = head_.load(std::memory_order_acquire) - tail;
        n = std::min(n, avail);
        for (size_t i = 0; i < n; ++i)
            out[i] = buf_[(tail + i) & mask_];
        tail_.store(tail + n, std::memory_order_release);
        return n;
    }

    // Number of elements currently available for reading.
    size_t available() const noexcept {
        return head_.load(std::memory_order_acquire) -
               tail_.load(std::memory_order_acquire);
    }

    size_t capacity() const noexcept { return buf_.size(); }

private:
    std::vector<T>       buf_;
    size_t               mask_;
    std::atomic<size_t>  head_;
    // Pad to separate cache lines and avoid false sharing between threads.
    char                 pad_[64 - sizeof(std::atomic<size_t>)];
    std::atomic<size_t>  tail_;
};
