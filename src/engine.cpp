#include "engine.hpp"
#include <memory>

std::vector<Event> Engine::handle(const Msg& msg) {
  std::vector<Event> out;
  std::visit([&](auto&& m) { this->handle_one(m, out); }, msg);
  return out;
}

void Engine::handle_one(const NewOrderMsg& m, std::vector<Event>& out) {
  auto order = std::make_shared<Order>(m.type, m.id, m.side, m.price, m.qty);
  auto r = ob_.add(order);
  if (!r.accepted) {
    out.push_back(Reject{m.id, *r.reject_reason});
    return;
  }
  out.push_back(Ack{m.id});
  for (const auto& f : r.fills) out.push_back(f);
}

void Engine::handle_one(const CancelMsg& m, std::vector<Event>& out) {
  if (!ob_.exists(m.id)) {
    out.push_back(Reject{m.id, "unknown orderId"});
    return;
  }
  if (ob_.cancel(m.id)) out.push_back(CancelAck{m.id});
  else out.push_back(Reject{m.id, "cancel failed"});
}

void Engine::handle_one(const ModifyMsg& m, std::vector<Event>& out) {
  if (!ob_.exists(m.id)) {
    out.push_back(Reject{m.id, "unknown orderId"});
    return;
  }

  // simple replace: cancel then add new GTC with same id
  ob_.cancel(m.id);
  out.push_back(ReplaceAck{m.id});

  auto order = std::make_shared<Order>(OrderType::GoodTillCancel, m.id, m.side, m.price, m.qty);
  auto r = ob_.add(order);
  if (!r.accepted) {
    out.push_back(Reject{m.id, *r.reject_reason});
    return;
  }
  for (const auto& f : r.fills) out.push_back(f);
}