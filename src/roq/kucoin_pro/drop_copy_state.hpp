/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace kucoin_pro {

enum class DropCopyState : uint8_t {
  UNDEFINED = 0,
  SUBSCRIBE,
  DONE,
};

}  // namespace kucoin_pro
}  // namespace roq
