#include "engine.hpp"
#include "market_data.hpp"
#include "strategy.hpp"
#include "spsc_ring.hpp"
#include <atomic>
#include <chrono>
#include <thread>

int main() {
  using namespace std::chrono_literals;

  std::atomic<bool> running{true};

  SpscRing<Tick, 1024> md_to_strat;
  SpscRing<Msg, 4096> strat_to_exch;
  SpscRing<Event, 4096> exch_to_strat;

  MarketDataSource md(md_to_strat, running);
  Strategy strat(md_to_strat, strat_to_exch, exch_to_strat, running);

  std::thread exch([&] {
    Engine engine;
    Msg msg;
    while (running.load(std::memory_order_relaxed)) {
      if (!strat_to_exch.try_pop(msg)) {
        std::this_thread::yield();
        continue;
      }
      auto events = engine.handle(msg);
      for (auto& e : events) {
        while (running.load(std::memory_order_relaxed) && !exch_to_strat.try_push(std::move(e))) {
          std::this_thread::yield();
        }
      }
    }
  });

  md.start();
  strat.start();

  std::this_thread::sleep_for(5s);
  running.store(false, std::memory_order_relaxed);

  md.join();
  strat.join();
  exch.join();
  return 0;
}