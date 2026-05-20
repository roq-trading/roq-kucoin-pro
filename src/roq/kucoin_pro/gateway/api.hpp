/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/kucoin_pro/gateway/settings.hpp"

namespace roq {
namespace kucoin_pro {
namespace gateway {

struct API final {
  struct {
    std::string_view asset_currencies;
    std::string_view market_currency;
    std::string_view market_instrument;
    std::string_view market_orderbook;
    std::string_view server_status;
  } rest_public;
  struct {
    std::string_view bullet_private;
    std::string_view account_mode;
    std::string_view account_balance;
    std::string_view position_open_list;
    std::string_view order_open_list;
    std::string_view order_execution;
    std::string_view order_place;
    std::string_view order_cancel;
    std::string_view order_cancel_all;
  } rest_private;
  // ws
  // -- public
  std::string_view ticker;
  std::string_view trade;
  std::string_view obu;
  // -- account
  std::string_view get_balance;
  std::string_view get_account;
  std::string_view get_open_orders;
  std::string_view order;
  std::string_view all_open_orders;
  std::string_view countdown_cancel_all;

  // factory
  static API create(Settings const &);
};

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
