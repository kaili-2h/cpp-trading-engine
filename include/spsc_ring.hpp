#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

template <typename T, std::size_t CapacityPow2>
class SpscRing {
  static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of 2.");

public:
  bool try_push(const T& v) {
    const auto h = head_.load(std::memory_order_relaxed);
    const auto next = h + 1;
    if (next - tail_.load(std::memory_order_acquire) > CapacityPow2) return false;
    buf_[h & mask_] = v;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_push(T&& v) {
    const auto h = head_.load(std::memory_order_relaxed);
    const auto next = h + 1;
    if (next - tail_.load(std::memory_order_acquire) > CapacityPow2) return false;
    buf_[h & mask_] = std::move(v);
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) {
    const auto t = tail_.load(std::memory_order_relaxed);
    if (head_.load(std::memory_order_acquire) == t) return false;
    out = std::move(buf_[t & mask_]);
    tail_.store(t + 1, std::memory_order_release);
    return true;
  }

  std::size_t size_approx() const {
    const auto h = head_.load(std::memory_order_acquire);
    const auto t = tail_.load(std::memory_order_acquire);
    return static_cast<std::size_t>(h - t);
  }

private:
  static constexpr std::size_t mask_ = CapacityPow2 - 1;

  alignas(64) std::atomic<std::uint64_t> head_{0};
  alignas(64) std::atomic<std::uint64_t> tail_{0};
  alignas(64) T buf_[CapacityPow2];
};