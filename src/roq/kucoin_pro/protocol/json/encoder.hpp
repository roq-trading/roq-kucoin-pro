/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/cancel_all_orders.hpp"
#include "roq/cancel_order.hpp"
#include "roq/create_order.hpp"

#include "roq/server/oms/order.hpp"
#include "roq/server/oms/ref_data.hpp"

namespace roq {
namespace kucoin_pro {
namespace protocol {
namespace json {

struct Encoder final {
  static std::string_view add_order(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  static std::string_view cancel_order(
      std::string &buffer,
      CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  static std::string_view cancel_all_orders(
      std::string &buffer, Event<CancelAllOrders> const &, std::string_view const &request_id, std::string_view const &symbol);

  static std::string_view ws_add_order(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  static std::string_view ws_cancel_order(
      std::string &buffer,
      CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
};

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
