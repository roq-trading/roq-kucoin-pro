/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/service.hpp"

namespace roq {
namespace kucoin_pro {

struct Application final : public roq::Service {
  using roq::Service::Service;

 protected:
  int main(args::Parser const &) override;
};

}  // namespace kucoin_pro
}  // namespace roq
