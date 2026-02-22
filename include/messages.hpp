#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

enum class OrderType { GoodTillCancel, FillAndKill }; // IOC semantics
enum class Side { Buy, Sell };

// Messages into exchange
struct NewOrderMsg {
  OrderType type;
  OrderId id;
  Side side;
  Price price;
  Quantity qty;
};

struct CancelMsg { OrderId id; };

struct ModifyMsg {
  OrderId id;
  Side side;
  Price price;
  Quantity qty;
};

using Msg = std::variant<NewOrderMsg, CancelMsg, ModifyMsg>;

// Events out of exchange
struct Ack { OrderId id; };
struct Reject { std::optional<OrderId> id; std::string reason; };
struct CancelAck { OrderId id; };
struct ReplaceAck { OrderId id; };

struct Fill {
  OrderId taker_id;
  OrderId maker_id;
  Price price;
  Quantity qty;
};

using Event = std::variant<Ack, Reject, CancelAck, ReplaceAck, Fill>;

// Market data
struct Tick {
  std::int64_t seq;
  Price mid;
  Price spread;
  std::chrono::steady_clock::time_point t_publish;
};