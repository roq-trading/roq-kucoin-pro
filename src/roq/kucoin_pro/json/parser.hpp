/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/ack.hpp"
#include "roq/kucoin_pro/json/error.hpp"
#include "roq/kucoin_pro/json/pong.hpp"
#include "roq/kucoin_pro/json/welcome.hpp"

#include "roq/kucoin_pro/json/obu.hpp"
#include "roq/kucoin_pro/json/ticker.hpp"
#include "roq/kucoin_pro/json/trade.hpp"

#include "roq/kucoin_pro/json/balance.hpp"
#include "roq/kucoin_pro/json/order_all.hpp"
#include "roq/kucoin_pro/json/position_all.hpp"

namespace roq {
namespace kucoin_pro {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(Trace<json::Welcome> const &) = 0;
    virtual void operator()(Trace<json::Error> const &) = 0;
    virtual void operator()(Trace<json::Pong> const &) = 0;
    virtual void operator()(Trace<json::Ack> const &) = 0;

    virtual void operator()(Trace<json::Ticker> const &) = 0;
    virtual void operator()(Trace<json::Trade> const &) = 0;
    virtual void operator()(Trace<json::OBU> const &) = 0;

    virtual void operator()(Trace<json::Balance> const &) = 0;
    virtual void operator()(Trace<json::PositionAll> const &) = 0;
    virtual void operator()(Trace<json::OrderAll> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace kucoin_pro
}  // namespace roq
