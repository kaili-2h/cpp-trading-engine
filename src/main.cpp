#include "engine.hpp"
#include "spsc_ring.hpp"
#include "messages.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono_literals;

  std::atomic<bool> running{true};

  // Client -> Exchange
  SpscRing<Msg, 4096> inbox;
  // Exchange -> Client
  SpscRing<Event, 4096> outbox;

  // Exchange thread (owns Engine + Orderbook)
  std::thread exch([&] {
    Engine engine;
    Msg msg;
    while (running.load(std::memory_order_relaxed)) {
      if (!inbox.try_pop(msg)) {
        std::this_thread::yield();
        continue;
      }
      auto events = engine.handle(msg);
      for (auto& e : events) {
        while (running.load(std::memory_order_relaxed) && !outbox.try_push(std::move(e))) {
          std::this_thread::yield();
        }
      }
    }
  });

  // ---- Client side: send some test orders ----

  auto send = [&](Msg m) {
    while (!inbox.try_push(std::move(m))) {
      std::this_thread::yield();
    }
  };

  // Place two crossing orders
  send(NewOrderMsg{OrderType::GoodTillCancel, 1, Side::Buy, 100, 10});
  send(NewOrderMsg{OrderType::GoodTillCancel, 2, Side::Sell, 99, 5});

  // Read events
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < 1s) {
    Event ev;
    while (outbox.try_pop(ev)) {
      std::visit([&](auto&& e) {
        using E = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<E, Ack>) {
          std::cout << "Ack " << e.id << "\n";
        } else if constexpr (std::is_same_v<E, Reject>) {
          std::cout << "Reject "
                    << (e.id ? std::to_string(*e.id) : std::string("n/a"))
                    << " reason=" << e.reason << "\n";
        } else if constexpr (std::is_same_v<E, CancelAck>) {
          std::cout << "CancelAck " << e.id << "\n";
        } else if constexpr (std::is_same_v<E, ReplaceAck>) {
          std::cout << "ReplaceAck " << e.id << "\n";
        } else if constexpr (std::is_same_v<E, Fill>) {
          std::cout << "Fill taker=" << e.taker_id
                    << " maker=" << e.maker_id
                    << " px=" << e.price
                    << " qty=" << e.qty << "\n";
        }
      }, ev);
    }
    std::this_thread::sleep_for(10ms);
  }

  running.store(false, std::memory_order_relaxed);
  exch.join();
  return 0;
}