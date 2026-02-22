#include "orderbook.hpp"
#include <algorithm>

bool Orderbook::can_cross(Side side, Price px) const {
  if (side == Side::Buy) {
    if (asks_.empty()) return false;
    return px >= asks_.begin()->first;
  } else {
    if (bids_.empty()) return false;
    return px <= bids_.begin()->first;
  }
}

void Orderbook::rest(const OrderPtr& o) {
  OrderList::iterator it;
  if (o->side() == Side::Buy) {
    auto& lvl = bids_[o->price()];
    lvl.push_back(o);
    it = std::prev(lvl.end());
  } else {
    auto& lvl = asks_[o->price()];
    lvl.push_back(o);
    it = std::prev(lvl.end());
  }
  by_id_.insert({o->id(), OrderEntry{o, it}});
}

Orderbook::AddResult Orderbook::add(const OrderPtr& incoming) {
  AddResult res;

  // validation
  if (incoming->remaining_qty() == 0) {
    res.accepted = false;
    res.reject_reason = "qty must be > 0";
    return res;
  }
  if (incoming->price() <= 0) {
    res.accepted = false;
    res.reject_reason = "price must be > 0 (limit-only engine)";
    return res;
  }
  if (by_id_.contains(incoming->id())) {
    res.accepted = false;
    res.reject_reason = "duplicate orderId";
    return res;
  }

  // IOC must cross immediately
  if (incoming->type() == OrderType::FillAndKill && !can_cross(incoming->side(), incoming->price())) {
    res.accepted = false;
    res.reject_reason = "IOC cannot cross";
    return res;
  }

  res.accepted = true;

  // Match incoming only
  while (!incoming->filled()) {
    if (incoming->side() == Side::Buy) {
      if (asks_.empty()) break;

      auto bestIt = asks_.begin();
      const Price bestPx = bestIt->first;
      if (incoming->price() < bestPx) break;

      auto& q = bestIt->second;
      while (!incoming->filled() && !q.empty()) {
        auto maker = q.front();
        const Quantity qty = std::min(incoming->remaining_qty(), maker->remaining_qty());

        incoming->fill(qty);
        maker->fill(qty);

        res.fills.push_back(Fill{incoming->id(), maker->id(), bestPx, qty});

        if (maker->filled()) {
          q.pop_front();
          by_id_.erase(maker->id());
        }
      }
      if (q.empty()) asks_.erase(bestIt);

    } else { // Sell
      if (bids_.empty()) break;

      auto bestIt = bids_.begin();
      const Price bestPx = bestIt->first;
      if (incoming->price() > bestPx) break;

      auto& q = bestIt->second;
      while (!incoming->filled() && !q.empty()) {
        auto maker = q.front();
        const Quantity qty = std::min(incoming->remaining_qty(), maker->remaining_qty());

        incoming->fill(qty);
        maker->fill(qty);

        res.fills.push_back(Fill{incoming->id(), maker->id(), bestPx, qty});

        if (maker->filled()) {
          q.pop_front();
          by_id_.erase(maker->id());
        }
      }
      if (q.empty()) bids_.erase(bestIt);
    }
  }

  // Rest remainder only if GTC
  if (!incoming->filled() && incoming->type() == OrderType::GoodTillCancel) {
    rest(incoming);
    res.resting = true;
  }

  return res;
}

bool Orderbook::cancel(OrderId id) {
  auto it = by_id_.find(id);
  if (it == by_id_.end()) return false;

  auto o = it->second.order;
  auto loc = it->second.loc;
  const Price px = o->price();
  by_id_.erase(it);

  if (o->side() == Side::Buy) {
    auto lvlIt = bids_.find(px);
    if (lvlIt == bids_.end()) return false;
    lvlIt->second.erase(loc);
    if (lvlIt->second.empty()) bids_.erase(lvlIt);
  } else {
    auto lvlIt = asks_.find(px);
    if (lvlIt == asks_.end()) return false;
    lvlIt->second.erase(loc);
    if (lvlIt->second.empty()) asks_.erase(lvlIt);
  }
  return true;
}

bool Orderbook::exists(OrderId id) const { return by_id_.contains(id); }
std::size_t Orderbook::size() const { return by_id_.size(); }