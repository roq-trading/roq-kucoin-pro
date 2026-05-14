/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace kucoin_pro {

enum class OrderEntryState : uint8_t {
  UNDEFINED = 0,
  PRIVATE_TOKEN,
  ACCOUNT,
  POSITION,
  ORDERS,
  EXECUTION,
  DONE,
};

}  // namespace kucoin_pro
}  // namespace roq
