#pragma once
#include "messages.hpp"
#include <stdexcept>

class Order {
public:
  Order(OrderType type, OrderId id, Side side, Price price, Quantity qty)
    : type_{type}, id_{id}, side_{side}, price_{price}, initial_{qty}, remaining_{qty} {}

  OrderId id() const { return id_; }
  Side side() const { return side_; }
  Price price() const { return price_; }
  OrderType type() const { return type_; }
  Quantity initial_qty() const { return initial_; }
  Quantity remaining_qty() const { return remaining_; }
  bool filled() const { return remaining_ == 0; }

  void fill(Quantity q) {
    if (q > remaining_) throw std::logic_error("fill > remaining");
    remaining_ -= q;
  }

private:
  OrderType type_;
  OrderId id_;
  Side side_;
  Price price_;
  Quantity initial_;
  Quantity remaining_;
};