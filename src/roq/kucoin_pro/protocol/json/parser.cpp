/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/protocol/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/kucoin_pro/protocol/json/channel.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace protocol {
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
          //
          case BALANCE_UNIFIED:
            result = dispatch_helper<Balance>(handler, message, buffer_stack, trace_info);
            return true;
          case POSITION_ALL_UNIFIED:
            result = dispatch_helper<PositionAll>(handler, message, buffer_stack, trace_info);
            return true;
          case ORDER_ALL_UNIFIED:
            result = dispatch_helper<OrderAll>(handler, message, buffer_stack, trace_info);
            return true;
        }
        break;
      }
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
