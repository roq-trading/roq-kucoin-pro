/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/fix_bridge/application.hpp"

#include "roq/logging.hpp"

#include "roq/server/fix_bridge/controller.hpp"

#include "roq/kucoin_pro/gateway/controller.hpp"

#include "roq/kucoin_pro/fix_bridge/config.hpp"
#include "roq/kucoin_pro/fix_bridge/settings.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace fix_bridge {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  Config config{settings};
  log::warn("config={}"sv, config);
  auto context = server::create_io_context(settings);
  server::fix_bridge::Controller<gateway::Controller>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace fix_bridge
}  // namespace kucoin_pro
}  // namespace roq
