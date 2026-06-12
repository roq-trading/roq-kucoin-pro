/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/application.hpp"

#include "roq/kucoin_pro/flags/settings.hpp"

#include "roq/kucoin_pro/gateway/config.hpp"
#include "roq/kucoin_pro/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  flags::Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<gateway::Controller>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kucoin_pro
}  // namespace roq
