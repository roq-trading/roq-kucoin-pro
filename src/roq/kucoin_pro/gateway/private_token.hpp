/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string_view>

namespace roq {
namespace kucoin_pro {
namespace gateway {

struct PrivateToken final {
  std::string_view account;
  std::string_view query;
};

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
