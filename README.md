# cpp-trading-engine

A small C++20 limit-order-book demo with:

- an in-memory matching engine
- a single-producer/single-consumer ring buffer for client/exchange messaging
- a threaded exchange loop that consumes order messages and emits events

The executable entry point is [`src/main.cpp`]

## What It Does

The demo starts an exchange thread, sends a couple of hard-coded orders through the inbound ring, and prints the resulting events from the outbound ring.

The current sample flow in [`src/main.cpp`] is:

1. submit buy order `id=1` at `100` for `10`
2. submit sell order `id=2` at `99` for `5`
3. match the sell against the resting buy

Expected output:

```text
Ack 1
Ack 2
Fill taker=2 maker=1 px=100 qty=5
```

## Message Types

Inbound messages are defined in [`include/messages.hpp`]:

- `NewOrderMsg`
- `CancelMsg`
- `ModifyMsg`

Outbound events are:

- `Ack`
- `Reject`
- `CancelAck`
- `ReplaceAck`
- `Fill`

## Build

Requirements:

- C++20 compiler
- pthread support
- CMake 3.20+ if you want to use the CMake build

### Option 1: Build with CMake

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/engine
```

### Option 2: Build directly with the compiler

Useful if you just want to run the demo without installing CMake:

```bash
c++ -std=c++20 -O2 -pthread -Iinclude \
  src/main.cpp src/orderbook.cpp src/engine.cpp src/market_data.cpp \
  -o engine_demo
```

Run:

```bash
./engine_demo
```

## How To Use This Implementation

The easiest way to use it is to edit the client-side demo in [`src/main.cpp`]:

- push `Msg` values into the `inbox` ring
- let the exchange thread call `Engine::handle`
- read `Event` values from the `outbox` ring


Example messages you can send:

```cpp
send(NewOrderMsg{OrderType::GoodTillCancel, 10, Side::Buy, 101, 20});
send(NewOrderMsg{OrderType::FillAndKill, 11, Side::Sell, 101, 5});
send(CancelMsg{10});
send(ModifyMsg{10, Side::Buy, 102, 15});
```

## Matching Behavior

- only positive-price limit orders are accepted
- `GoodTillCancel` orders can rest on the book
- `FillAndKill` orders must cross immediately or they are rejected
- duplicate order IDs are rejected
- matching is price-time priority within each price level

