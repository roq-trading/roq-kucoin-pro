/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/protocol/json/utils.hpp"

#include "roq/kucoin_pro/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace protocol {
namespace json {

Error guess_error([[maybe_unused]] int32_t code) {
  return Error::UNKNOWN;
}

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
