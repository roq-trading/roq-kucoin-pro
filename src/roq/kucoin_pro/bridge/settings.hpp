/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/server/bridge/settings.hpp"

#include "roq/kucoin_pro/flags/settings.hpp"

namespace roq {
namespace kucoin_pro {
namespace bridge {

struct Settings final : public flags::Settings {
  explicit Settings(args::Parser const &);

  server::bridge::Settings bridge;
};

}  // namespace bridge
}  // namespace kucoin_pro
}  // namespace roq

template <>
struct fmt::formatter<roq::kucoin_pro::bridge::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::kucoin_pro::bridge::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(gateway={}, )"
        R"(bridge={})"
        R"(}})"sv,
        static_cast<roq::kucoin_pro::flags::Settings const &>(value),
        value.bridge);
  }
};
