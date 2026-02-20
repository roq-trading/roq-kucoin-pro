/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/kucoin_pro/json/channel.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const KEY_MESSAGE = "message"sv;
constexpr auto const KEY_TYPE = "type"sv;
constexpr auto const KEY_RESULT = "result"sv;
constexpr auto const KEY_T = "T"sv;

constexpr auto const VALUE_WELCOME = "welcome"sv;
constexpr auto const VALUE_PONG = "pong"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T>
bool dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  auto result = false;
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_MESSAGE):
        if (std::get<std::string_view>(value) == VALUE_WELCOME) {
          result = dispatch_helper<Welcome>(handler, message, buffer_stack, trace_info);
          return true;
        }
        break;
      case utils::hash::FNV::compute(KEY_TYPE):
        if (std::get<std::string_view>(value) == VALUE_PONG) {
          result = dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
          return true;
        }
        break;
      case utils::hash::FNV::compute(KEY_RESULT):
        result = true;  // silent
        break;
      case utils::hash::FNV::compute(KEY_T): {
        Channel channel{value};
        switch (channel) {
          using enum Channel::type_t;
          case UNDEFINED_INTERNAL:
            log::fatal("Unexpected"sv);
          case UNKNOWN_INTERNAL:
            return true;
          case TICKER_FUTURES:
            result = dispatch_helper<Ticker>(handler, message, buffer_stack, trace_info);
            return true;
          case TRADE_FUTURES:
            result = dispatch_helper<Trade>(handler, message, buffer_stack, trace_info);
            return true;
          case OBU_FUTURES:
            result = dispatch_helper<OBU>(handler, message, buffer_stack, trace_info);
            return true;
        }
        break;
      }
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  /*
  Message message_2{message, buffer_stack};
  switch (message_2.type) {
  using enum json::Type::type_t;
  case UNDEFINED_INTERNAL:
  if (allow_unknown_event_types) {
    return false;
  }
  break;
  case UNKNOWN_INTERNAL:
  break;
  case WELCOME:
  dispatch_helper<Welcome>(handler, message, buffer_stack, trace_info);
  return true;
  case ERROR:
  dispatch_helper<Error>(handler, message, buffer_stack, trace_info);
  return true;
  case PONG:
  dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
  return true;
  case ACK:
  dispatch_helper<Ack>(handler, message, buffer_stack, trace_info);
  return true;
  case MESSAGE:
  switch (message_2.subject) {
    using enum json::Subject::type_t;
    case UNDEFINED_INTERNAL:
      break;
    case UNKNOWN_INTERNAL:
      if (allow_unknown_event_types) {
        return false;
      }
      break;
    case TICKER_V2:
      dispatch_helper<TickerV2>(handler, message, buffer_stack, trace_info);
      return true;
    case MATCH:
      dispatch_helper<Match>(handler, message, buffer_stack, trace_info);
      return true;
    case EXECUTION:
      dispatch_helper<Execution>(handler, message, buffer_stack, trace_info);
      return true;
    case MARK_INDEX_PRICE:
      dispatch_helper<MarkIndexPrice>(handler, message, buffer_stack, trace_info);
      return true;
    case FUNDING_RATE:
      dispatch_helper<FundingRate>(handler, message, buffer_stack, trace_info);
      return true;
    case LEVEL2:
      dispatch_helper<Level2>(handler, message, buffer_stack, trace_info);
      return true;
    case FUNDING_BEGIN:
      dispatch_helper<FundingBegin>(handler, message, buffer_stack, trace_info);
      return true;
    case FUNDING_END:
      dispatch_helper<FundingEnd>(handler, message, buffer_stack, trace_info);
      return true;
    case SNAPSHOT_24H:
      dispatch_helper<Snapshot24h>(handler, message, buffer_stack, trace_info);
      return true;
    case WALLET_BALANCE_CHANGE:
      dispatch_helper<WalletBalanceChange>(handler, message, buffer_stack, trace_info);
      return true;
    case ORDER_MARGIN_CHANGE:
      dispatch_helper<OrderMarginChange>(handler, message, buffer_stack, trace_info);
      return true;
    case AVAILABLE_BALANCE_CHANGE:
      dispatch_helper<AvailableBalanceChange>(handler, message, buffer_stack, trace_info);
      return true;
    case WITHDRAW_HOLD_CHANGE:
      dispatch_helper<WithdrawHoldChange>(handler, message, buffer_stack, trace_info);
      return true;
    case POSITION_CHANGE:
      dispatch_helper<PositionChange>(handler, message, buffer_stack, trace_info);
      return true;
    case POSITION_SETTLEMENT:
      dispatch_helper<PositionSettlement>(handler, message, buffer_stack, trace_info);
      return true;
    case POSITION_ADJUST_RISK_LIMIT:
      dispatch_helper<PositionAdjustRiskLimit>(handler, message, buffer_stack, trace_info);
      return true;
    case SYMBOL_ORDER_CHANGE:
      dispatch_helper<SymbolOrderChange>(handler, message, buffer_stack, trace_info);
      return true;
    case ORDER_CHANGE:
      dispatch_helper<OrderChange>(handler, message, buffer_stack, trace_info);
      return true;
  }
  break;
  }
  */
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace kucoin_pro
}  // namespace roq
