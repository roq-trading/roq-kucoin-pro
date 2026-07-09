/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/service.hpp"

namespace roq {
namespace kucoin_pro {
namespace bridge {

struct Application final : public Service {
  using Service::Service;

 protected:
  int main(args::Parser const &) override;
};

}  // namespace bridge
}  // namespace kucoin_pro
}  // namespace roq
