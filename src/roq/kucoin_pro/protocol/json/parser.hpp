/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/protocol/json/ack.hpp"
#include "roq/kucoin_pro/protocol/json/error.hpp"
#include "roq/kucoin_pro/protocol/json/pong.hpp"
#include "roq/kucoin_pro/protocol/json/welcome.hpp"

#include "roq/kucoin_pro/protocol/json/obu.hpp"
#include "roq/kucoin_pro/protocol/json/ticker.hpp"
#include "roq/kucoin_pro/protocol/json/trade.hpp"

#include "roq/kucoin_pro/protocol/json/balance.hpp"
#include "roq/kucoin_pro/protocol/json/order_all.hpp"
#include "roq/kucoin_pro/protocol/json/position_all.hpp"

namespace roq {
namespace kucoin_pro {
namespace protocol {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(Trace<protocol::json::Welcome> const &) = 0;
    virtual void operator()(Trace<protocol::json::Error> const &) = 0;
    virtual void operator()(Trace<protocol::json::Pong> const &) = 0;
    virtual void operator()(Trace<protocol::json::Ack> const &) = 0;

    virtual void operator()(Trace<protocol::json::Ticker> const &) = 0;
    virtual void operator()(Trace<protocol::json::Trade> const &) = 0;
    virtual void operator()(Trace<protocol::json::OBU> const &) = 0;

    virtual void operator()(Trace<protocol::json::Balance> const &) = 0;
    virtual void operator()(Trace<protocol::json::PositionAll> const &) = 0;
    virtual void operator()(Trace<protocol::json::OrderAll> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
