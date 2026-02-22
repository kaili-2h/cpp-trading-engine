#pragma once
#include "messages.hpp"
#include "orderbook.hpp"
#include <vector>

class Engine {
public:
  std::vector<Event> handle(const Msg& msg);

private:
  Orderbook ob_;

  void handle_one(const NewOrderMsg& m, std::vector<Event>& out);
  void handle_one(const CancelMsg& m, std::vector<Event>& out);
  void handle_one(const ModifyMsg& m, std::vector<Event>& out);
};