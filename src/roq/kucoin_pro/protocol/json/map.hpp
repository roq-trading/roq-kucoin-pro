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

#include "roq/kucoin_pro/protocol/json/liquidity_role.hpp"
#include "roq/kucoin_pro/protocol/json/margin_mode.hpp"
#include "roq/kucoin_pro/protocol/json/order_status.hpp"
#include "roq/kucoin_pro/protocol/json/order_type.hpp"
#include "roq/kucoin_pro/protocol/json/side.hpp"
#include "roq/kucoin_pro/protocol/json/time_in_force.hpp"
#include "roq/kucoin_pro/protocol/json/trade_type.hpp"

namespace roq {

template <>
template <>
std::optional<OrderStatus> Map<int32_t>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<kucoin_pro::protocol::json::LiquidityRole>::helper() const;

template <>
template <>
std::optional<MarginMode> Map<kucoin_pro::protocol::json::MarginMode>::helper() const;

template <>
template <>
std::optional<OrderStatus> Map<kucoin_pro::protocol::json::OrderStatus>::helper() const;

template <>
template <>
std::optional<OrderType> Map<kucoin_pro::protocol::json::OrderType>::helper() const;

template <>
template <>
std::optional<Side> Map<kucoin_pro::protocol::json::Side>::helper() const;

template <>
template <>
std::optional<TimeInForce> Map<kucoin_pro::protocol::json::TimeInForce>::helper() const;

template <>
template <>
std::optional<SecurityType> Map<kucoin_pro::protocol::json::TradeType>::helper() const;

// ===

template <>
template <>
std::optional<kucoin_pro::protocol::json::MarginMode> Map<MarginMode>::helper() const;

template <>
template <>
std::optional<kucoin_pro::protocol::json::OrderType> Map<OrderType>::helper() const;

template <>
template <>
std::optional<kucoin_pro::protocol::json::Side> Map<Side>::helper() const;

template <>
template <>
std::optional<kucoin_pro::protocol::json::TimeInForce> Map<TimeInForce>::helper() const;

}  // namespace roq
