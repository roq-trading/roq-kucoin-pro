/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/api.hpp"

#include "roq/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {

// === IMPLEMENTATION ===

API API::create(Settings const &) {
  return {
      .rest_public{
          .asset_currencies = "/api/ua/v1/asset/currencies"sv,
          .market_currency = "/api/ua/v1/market/currency"sv,
          .market_instrument = "/api/ua/v1/market/instrument"sv,
          .market_orderbook = "/api/ua/v1/market/orderbook"sv,
          .server_status = "/api/ua/v1/server/status"sv,
      },
      .rest_private{
          .bullet_private = "/api/v2/bullet-private"sv,
          .account_mode = "/api/ua/v1/account/mode"sv,
          .account_balance = "/api/ua/v1/unified/account/balance"sv,
          .position_open_list = "/api/ua/v1/unified/position/open-list"sv,
          .order_open_list = "/api/ua/v1/unified/order/open-list"sv,
          .order_execution = "/api/ua/v1/unified/order/execution"sv,
          .order_place = "/api/ua/v1/unified/order/place"sv,
          .order_cancel = "/api/ua/v1/unified/order/cancel"sv,
          .order_cancel_all = "/api/ua/v1/unified/order/cancel-all"sv,
      },
      // ws
      // -- public
      .ticker = "ticker"sv,
      .trade = "trade"sv,
      .obu = "obu"sv,
      // account
      .get_balance = "/fapi/v2/balance"sv,
      .get_account = "/fapi/v2/account"sv,
      .get_open_orders = "/fapi/v1/openOrders"sv,
      .order = "/fapi/v1/order"sv,
      .all_open_orders = "/fapi/v1/allOpenOrders"sv,
      .countdown_cancel_all = "/fapi/v1/countdownCancelAll"sv,
  };
}

}  // namespace kucoin_pro
}  // namespace roq
