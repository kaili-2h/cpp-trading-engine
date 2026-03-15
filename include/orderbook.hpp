#pragma once
#include "order.hpp"
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Orderbook {
public:
  struct AddResult {
    bool accepted{false};
    std::optional<std::string> reject_reason;
    std::vector<Fill> fills;
    bool resting{false};
  };

  AddResult add(const std::shared_ptr<Order>& incoming);
  bool cancel(OrderId id);
  bool exists(OrderId id) const;
  std::size_t size() const;

private:
  using OrderPtr = std::shared_ptr<Order>;
  using OrderList = std::list<OrderPtr>;

  struct OrderEntry {
    OrderPtr order;
    OrderList::iterator loc;
  };

  // best bid first
  std::map<Price, OrderList, std::greater<Price>> bids_;
  // best ask first
  std::map<Price, OrderList, std::less<Price>> asks_;
  std::unordered_map<OrderId, OrderEntry> by_id_;

  bool can_cross(Side side, Price px) const;
  void rest(const OrderPtr& o);
};