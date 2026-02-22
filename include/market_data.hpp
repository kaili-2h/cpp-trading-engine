#pragma once
#include "messages.hpp"
#include "spsc_ring.hpp"
#include <atomic>
#include <thread>

class MarketDataSource {
public:
  MarketDataSource(SpscRing<Tick, 1024>& out, std::atomic<bool>& running);

  void start();
  void join();

private:
  void run();

  SpscRing<Tick, 1024>& out_;
  std::atomic<bool>& running_;
  std::thread th_;
};