#include "market_data.hpp"
#include <algorithm>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

MarketDataSource::MarketDataSource(SpscRing<Tick, 1024>& out, std::atomic<bool>& running)
  : out_{out}, running_{running} {}

void MarketDataSource::start() { th_ = std::thread(&MarketDataSource::run, this); }
void MarketDataSource::join() { if (th_.joinable()) th_.join(); }

void MarketDataSource::run() {
  std::mt19937 rng(123);
  std::normal_distribution<double> step(0.0, 0.5);

  Price mid = 10000;
  Price spread = 2;

  for (std::int64_t seq = 1; running_.load(std::memory_order_relaxed); ++seq) {
    mid = static_cast<Price>(std::max<Price>(1, mid + static_cast<Price>(step(rng))));
    Tick t{seq, mid, spread, std::chrono::steady_clock::now()};

    while (running_.load(std::memory_order_relaxed) && !out_.try_push(t)) {
      std::this_thread::yield();
    }

    std::this_thread::sleep_for(200us);
  }
}