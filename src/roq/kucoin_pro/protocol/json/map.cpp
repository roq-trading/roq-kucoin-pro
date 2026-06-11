/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// kucoin_pro::json => roq

// int32_t => roq::OrderStatus

template <>
template <>
constexpr Helper<int32_t>::operator std::optional<roq::OrderStatus>() const {
  switch (std::get<0>(args_)) {
    case 0:  // notTriggered
      return roq::OrderStatus::SUSPENDED;
    case 1:  // triggered
      return roq::OrderStatus::ACCEPTED;
    case 2:  // live
      return roq::OrderStatus::WORKING;
    case 3:  // filled
      return roq::OrderStatus::COMPLETED;
    case 4:  // partial filled
      return roq::OrderStatus::WORKING;
    case 5:  // canceled
      return roq::OrderStatus::CANCELED;
    case 6:  // partial canceld
      return roq::OrderStatus::CANCELED;
  }
  return {};
}

static_assert(Helper{int32_t{0}} == roq::OrderStatus::SUSPENDED);
static_assert(Helper{int32_t{1}} == roq::OrderStatus::ACCEPTED);
static_assert(Helper{int32_t{2}} == roq::OrderStatus::WORKING);
static_assert(Helper{int32_t{3}} == roq::OrderStatus::COMPLETED);
static_assert(Helper{int32_t{4}} == roq::OrderStatus::WORKING);
static_assert(Helper{int32_t{5}} == roq::OrderStatus::CANCELED);
static_assert(Helper{int32_t{6}} == roq::OrderStatus::CANCELED);

template <>
template <>
std::optional<roq::OrderStatus> Map<int32_t>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::LiquidityRole => roq::Liquidity

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::LiquidityRole>::operator std::optional<roq::Liquidity>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::LiquidityRole::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case TAKER:
      return roq::Liquidity::TAKER;
    case MAKER:
      return roq::Liquidity::MAKER;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::LiquidityRole{kucoin_pro::protocol::json::LiquidityRole::UNDEFINED_INTERNAL}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::LiquidityRole{kucoin_pro::protocol::json::LiquidityRole::TAKER}} == roq::Liquidity::TAKER);
static_assert(Helper{kucoin_pro::protocol::json::LiquidityRole{kucoin_pro::protocol::json::LiquidityRole::MAKER}} == roq::Liquidity::MAKER);

template <>
template <>
std::optional<roq::Liquidity> Map<kucoin_pro::protocol::json::LiquidityRole>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::MarginMode => roq::MarginMode

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::MarginMode>::operator std::optional<roq::MarginMode>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::MarginMode::type_t;
    case UNDEFINED_INTERNAL:
      return roq::MarginMode::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::MarginMode::UNDEFINED;
    case CROSS:
      return roq::MarginMode::CROSS;
    case ISOLATED:
      return roq::MarginMode::ISOLATED;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::UNDEFINED_INTERNAL}} == roq::MarginMode::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::CROSS}} == roq::MarginMode::CROSS);
static_assert(Helper{kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::ISOLATED}} == roq::MarginMode::ISOLATED);

template <>
template <>
std::optional<roq::MarginMode> Map<kucoin_pro::protocol::json::MarginMode>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::OrderStatus => roq::OrderStatus

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::OrderStatus>::operator std::optional<roq::OrderStatus>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::OrderStatus::type_t;
    case UNDEFINED_INTERNAL:
      return roq::OrderStatus::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::OrderStatus::UNDEFINED;
    case OPEN:
      return roq::OrderStatus::WORKING;
    case MATCH:
      return roq::OrderStatus::WORKING;
    case DONE:
      return roq::OrderStatus::COMPLETED;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::OrderStatus{kucoin_pro::protocol::json::OrderStatus::UNDEFINED_INTERNAL}} == roq::OrderStatus::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::OrderStatus{kucoin_pro::protocol::json::OrderStatus::OPEN}} == roq::OrderStatus::WORKING);
static_assert(Helper{kucoin_pro::protocol::json::OrderStatus{kucoin_pro::protocol::json::OrderStatus::MATCH}} == roq::OrderStatus::WORKING);
static_assert(Helper{kucoin_pro::protocol::json::OrderStatus{kucoin_pro::protocol::json::OrderStatus::DONE}} == roq::OrderStatus::COMPLETED);

template <>
template <>
std::optional<roq::OrderStatus> Map<kucoin_pro::protocol::json::OrderStatus>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::OrderType => roq::OrderType

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::OrderType>::operator std::optional<roq::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::OrderType::type_t;
    case UNDEFINED_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case MARKET:
      return roq::OrderType::MARKET;
    case LIMIT:
      return roq::OrderType::LIMIT;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::UNDEFINED_INTERNAL}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::MARKET}} == roq::OrderType::MARKET);
static_assert(Helper{kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::LIMIT}} == roq::OrderType::LIMIT);

template <>
template <>
std::optional<roq::OrderType> Map<kucoin_pro::protocol::json::OrderType>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::Side => roq::Side

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::Side>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::Side::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Side::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Side::UNDEFINED;
    case BUY_LC:
      return roq::Side::BUY;
    case SELL_LC:
      return roq::Side::SELL;
    case BUY:
      return roq::Side::BUY;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::BUY_LC}} == roq::Side::BUY);
static_assert(Helper{kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::SELL_LC}} == roq::Side::SELL);
static_assert(Helper{kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<kucoin_pro::protocol::json::Side>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::TimeInForce => roq::TimeInForce

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::TimeInForce>::operator std::optional<roq::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::TimeInForce::type_t;
    case UNDEFINED_INTERNAL:
      return roq::TimeInForce::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::TimeInForce::UNDEFINED;
    case GTC:
      return roq::TimeInForce::GTC;
    case IOC:
      return roq::TimeInForce::IOC;
    case FOK:
      return roq::TimeInForce::FOK;
    case GTT:
      return roq::TimeInForce::UNDEFINED;
    case RTI:
      return roq::TimeInForce::UNDEFINED;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL}} == roq::TimeInForce::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::GTC}} == roq::TimeInForce::GTC);
static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::IOC}} == roq::TimeInForce::IOC);
static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::FOK}} == roq::TimeInForce::FOK);
static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::GTT}} == roq::TimeInForce::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::RTI}} == roq::TimeInForce::UNDEFINED);

template <>
template <>
std::optional<roq::TimeInForce> Map<kucoin_pro::protocol::json::TimeInForce>::helper() const {
  return Helper{args_};
}

// kucoin_pro::protocol::json::TradeType => roq::SecurityType

template <>
template <>
constexpr Helper<kucoin_pro::protocol::json::TradeType>::operator std::optional<roq::SecurityType>() const {
  switch (std::get<0>(args_)) {
    using enum kucoin_pro::protocol::json::TradeType::type_t;
    case UNDEFINED_INTERNAL:
      return roq::SecurityType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::SecurityType::UNDEFINED;
    case FUTURES:
      return roq::SecurityType::SWAP;
  }
  return {};
}

static_assert(Helper{kucoin_pro::protocol::json::TradeType{kucoin_pro::protocol::json::TradeType::UNDEFINED_INTERNAL}} == roq::SecurityType::UNDEFINED);
static_assert(Helper{kucoin_pro::protocol::json::TradeType{kucoin_pro::protocol::json::TradeType::FUTURES}} == roq::SecurityType::SWAP);

template <>
template <>
std::optional<roq::SecurityType> Map<kucoin_pro::protocol::json::TradeType>::helper() const {
  return Helper{args_};
}

// roq => kucoin_pro::json

// roq::MarginMode ==> kucoin_pro::protocol::json::MarginMode

template <>
template <>
constexpr Helper<roq::MarginMode>::operator std::optional<kucoin_pro::protocol::json::MarginMode>() const {
  switch (std::get<0>(args_)) {
    using enum roq::MarginMode;
    case UNDEFINED:
      return kucoin_pro::protocol::json::MarginMode::ISOLATED;  // note! default
    case ISOLATED:
      return kucoin_pro::protocol::json::MarginMode::ISOLATED;
    case CROSS:
      return kucoin_pro::protocol::json::MarginMode::CROSS;
    case PORTFOLIO:
      return kucoin_pro::protocol::json::MarginMode::ISOLATED;  // note! default
  }
  return {};
}

static_assert(Helper{roq::MarginMode::UNDEFINED} == kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::ISOLATED});
static_assert(Helper{roq::MarginMode::ISOLATED} == kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::ISOLATED});
static_assert(Helper{roq::MarginMode::CROSS} == kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::CROSS});
static_assert(Helper{roq::MarginMode::PORTFOLIO} == kucoin_pro::protocol::json::MarginMode{kucoin_pro::protocol::json::MarginMode::ISOLATED});

template <>
template <>
std::optional<kucoin_pro::protocol::json::MarginMode> Map<roq::MarginMode>::helper() const {
  return Helper{args_};
}

// roq::OrderType ==> kucoin_pro::protocol::json::OrderType

template <>
template <>
constexpr Helper<roq::OrderType>::operator std::optional<kucoin_pro::protocol::json::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum roq::OrderType;
    case UNDEFINED:
      return kucoin_pro::protocol::json::OrderType::UNDEFINED_INTERNAL;
    case MARKET:
      return kucoin_pro::protocol::json::OrderType::MARKET;
    case LIMIT:
      return kucoin_pro::protocol::json::OrderType::LIMIT;
  }
  return {};
}

static_assert(Helper{roq::OrderType::UNDEFINED} == kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::UNDEFINED_INTERNAL});
static_assert(Helper{roq::OrderType::MARKET} == kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::MARKET});
static_assert(Helper{roq::OrderType::LIMIT} == kucoin_pro::protocol::json::OrderType{kucoin_pro::protocol::json::OrderType::LIMIT});

template <>
template <>
std::optional<kucoin_pro::protocol::json::OrderType> Map<roq::OrderType>::helper() const {
  return Helper{args_};
}

// roq::Side ==> kucoin_pro::protocol::json::Side

template <>
template <>
constexpr Helper<roq::Side>::operator std::optional<kucoin_pro::protocol::json::Side>() const {
  switch (std::get<0>(args_)) {
    using enum roq::Side;
    case UNDEFINED:
      return kucoin_pro::protocol::json::Side::UNDEFINED_INTERNAL;
    case BUY:
      return kucoin_pro::protocol::json::Side::BUY;
    case SELL:
      return kucoin_pro::protocol::json::Side::SELL;
  }
  return {};
}

static_assert(Helper{roq::Side::UNDEFINED} == kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::UNDEFINED_INTERNAL});
static_assert(Helper{roq::Side::BUY} == kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::BUY});
static_assert(Helper{roq::Side::SELL} == kucoin_pro::protocol::json::Side{kucoin_pro::protocol::json::Side::SELL});

template <>
template <>
std::optional<kucoin_pro::protocol::json::Side> Map<roq::Side>::helper() const {
  return Helper{args_};
}

// roq::TimeInForce ==> kucoin_pro::protocol::json::TimeInForce

template <>
template <>
constexpr Helper<roq::TimeInForce>::operator std::optional<kucoin_pro::protocol::json::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum roq::TimeInForce;
    case UNDEFINED:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFD:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTC:
      return kucoin_pro::protocol::json::TimeInForce::GTC;
    case OPG:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case IOC:
      return kucoin_pro::protocol::json::TimeInForce::IOC;
    case FOK:
      return kucoin_pro::protocol::json::TimeInForce::FOK;
    case GTX:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTD:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case AT_THE_CLOSE:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_THROUGH_CROSSING:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case AT_CROSSING:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_FOR_TIME:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFA:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFM:
      return kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
  }
  return {};
}

static_assert(Helper{roq::TimeInForce::UNDEFINED} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFD} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTC} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::GTC});
static_assert(Helper{roq::TimeInForce::OPG} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::IOC} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::IOC});
static_assert(Helper{roq::TimeInForce::FOK} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::FOK});
static_assert(Helper{roq::TimeInForce::GTX} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTD} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::AT_THE_CLOSE} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(
    Helper{roq::TimeInForce::GOOD_THROUGH_CROSSING} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::AT_CROSSING} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GOOD_FOR_TIME} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFA} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFM} == kucoin_pro::protocol::json::TimeInForce{kucoin_pro::protocol::json::TimeInForce::UNDEFINED_INTERNAL});

template <>
template <>
std::optional<kucoin_pro::protocol::json::TimeInForce> Map<roq::TimeInForce>::helper() const {
  return Helper{args_};
}

}  // namespace roq
