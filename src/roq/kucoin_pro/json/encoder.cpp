/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/json/encoder.hpp"

#include "roq/decimal.hpp"

#include "roq/kucoin_pro/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace json {

std::string_view Encoder::add_order(
    std::string &buffer,
    CreateOrder const &create_order,
    server::oms::Order const &,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id) {
  buffer.clear();
  auto side = map(create_order.side).template get<json::Side>();
  auto order_type = map(create_order.order_type).template get<json::OrderType>();
  auto reduce_only = create_order.execution_instructions.has(ExecutionInstruction::DO_NOT_INCREASE);
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("tradeType":"FUTURES",)"
      R"("clientOid":"{}",)"
      R"("symbol":"{}",)"
      R"("side":"{}",)"
      R"("orderType":"{}")"sv,
      request_id,
      create_order.symbol,
      side.as_raw_text(),
      order_type.as_raw_text());
  if (create_order.execution_instructions.has(ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"postOnly":true)"sv);
  }
  switch (create_order.order_type) {
    using enum roq::OrderType;
    case UNDEFINED:
      assert(false);
      break;
    case MARKET:
      fmt::format_to(
          std::back_inserter(buffer),
          R"(,"reduceOnly":{})"
          R"(,"size":"{}")"
          R"(,"sizeUnit":"UNIT")"sv,
          reduce_only,
          Decimal{create_order.quantity, ref_data.quantity.precision});
      break;
    case LIMIT: {
      auto time_in_force = map(create_order.time_in_force).template get<json::TimeInForce>();
      fmt::format_to(
          std::back_inserter(buffer),
          R"(,"timeInForce":"{}")"
          R"(,"reduceOnly":{})"
          R"(,"size":"{}")"
          R"(,"price":"{}")"
          R"(,"sizeUnit":"UNIT")"sv,
          time_in_force.as_raw_text(),
          reduce_only,
          Decimal{create_order.quantity, ref_data.quantity.precision},
          Decimal{create_order.price, ref_data.price.precision});
      break;
    }
  }
  fmt::format_to(std::back_inserter(buffer), R"(}})"sv);
  return buffer;
}

std::string_view Encoder::cancel_order(
    std::string &buffer,
    CancelOrder const &,
    server::oms::Order const &order,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  if (std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"({{)"
        R"("tradeType":"FUTURES",)"
        R"("clientOid":"{}",)"
        R"("symbol":"{}")"
        R"(}})"sv,
        order.client_order_id,
        order.symbol);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"({{)"
        R"("tradeType":"FUTURES",)"
        R"("orderId":"{}",)"
        R"("symbol":"{}")"
        R"(}})"sv,
        order.external_order_id,
        order.symbol);
  }
  return buffer;
}

std::string_view Encoder::cancel_all_orders(
    std::string &buffer, Event<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id, std::string_view const &symbol) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("tradeType":"FUTURES",)"
      R"("symbol":"{}")"
      R"(}})"sv,
      symbol);
  return buffer;
}

// WSAPI

std::string_view Encoder::ws_add_order(
    std::string &buffer,
    CreateOrder const &create_order,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id) {
  auto args = add_order(buffer, create_order, order, ref_data, request_id);
  // note! just overwrite
  buffer = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("op":"uta.order",)"
      R"("args":{})"
      R"(}})"sv,
      request_id,
      args);
  return buffer;
}

std::string_view Encoder::ws_cancel_order(
    std::string &buffer,
    CancelOrder const &cancel_order_2,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  buffer.clear();
  auto args = cancel_order(buffer, cancel_order_2, order, ref_data, request_id, previous_request_id);
  buffer = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("op":"uta.cancel",)"
      R"("args":{})"
      R"(}})"sv,
      request_id,
      args);
  return buffer;
}

}  // namespace json
}  // namespace kucoin_pro
}  // namespace roq
