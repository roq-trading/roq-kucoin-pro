/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/gateway/shared.hpp"

#include "roq/logging.hpp"

namespace roq {
namespace kucoin_pro {
namespace gateway {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher{dispatcher}, settings{settings}, api{API::create(settings)}, rate_limiter{settings.request.limit, settings.request.limit_interval},
      symbols{settings.ws.max_subscriptions_per_stream}, depth_request_queue{settings.ws.mbp_request_delay} {
}

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
