/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/map.hpp"

#include "roq/liquidity.hpp"
#include "roq/margin_mode.hpp"
#include "roq/order_status.hpp"
#include "roq/order_type.hpp"
#include "roq/security_type.hpp"
#include "roq/side.hpp"
#include "roq/time_in_force.hpp"

#include "roq/kucoin_pro/json/liquidity_role.hpp"
#include "roq/kucoin_pro/json/margin_mode.hpp"
#include "roq/kucoin_pro/json/order_status.hpp"
#include "roq/kucoin_pro/json/order_type.hpp"
#include "roq/kucoin_pro/json/side.hpp"
#include "roq/kucoin_pro/json/time_in_force.hpp"
#include "roq/kucoin_pro/json/trade_type.hpp"

namespace roq {

template <>
template <>
std::optional<OrderStatus> Map<int32_t>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<kucoin_pro::json::LiquidityRole>::helper() const;

template <>
template <>
std::optional<MarginMode> Map<kucoin_pro::json::MarginMode>::helper() const;

template <>
template <>
std::optional<OrderStatus> Map<kucoin_pro::json::OrderStatus>::helper() const;

template <>
template <>
std::optional<OrderType> Map<kucoin_pro::json::OrderType>::helper() const;

template <>
template <>
std::optional<Side> Map<kucoin_pro::json::Side>::helper() const;

template <>
template <>
std::optional<TimeInForce> Map<kucoin_pro::json::TimeInForce>::helper() const;

template <>
template <>
std::optional<SecurityType> Map<kucoin_pro::json::TradeType>::helper() const;

// ===

template <>
template <>
std::optional<kucoin_pro::json::MarginMode> Map<MarginMode>::helper() const;

template <>
template <>
std::optional<kucoin_pro::json::OrderType> Map<OrderType>::helper() const;

template <>
template <>
std::optional<kucoin_pro::json::Side> Map<Side>::helper() const;

template <>
template <>
std::optional<kucoin_pro::json::TimeInForce> Map<TimeInForce>::helper() const;

}  // namespace roq
