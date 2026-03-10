/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/kucoin_pro/json/map.hpp"
#include "roq/kucoin_pro/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {

// === CONSTANTS ===

namespace {
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask{
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
    SupportType::FUNDS,
    SupportType::POSITION,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;

auto const PING_FREQUENCY = 60s;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account.name);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&settings.ws.private_uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() -> std::string { return {}; });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(
    Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, Request &request, std::string_view const &query)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account)}, query_{query},
      connection_{create_connection(*this, shared.settings, context)}, ping_frequency_{PING_FREQUENCY},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .welcome = create_metrics(shared.settings, name_, "welcome"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .pong = create_metrics(shared.settings, name_, "pong"sv),
          .ack = create_metrics(shared.settings, name_, "ack"sv),
          .balance = create_metrics(shared.settings, name_, "balance"sv),
          .position_all = create_metrics(shared.settings, name_, "position_all"sv),
          .order_all = create_metrics(shared.settings, name_, "order_all"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      account_{account}, shared_{shared}, request_{request}, download_{{}, [this](auto state) { return download(state); }} {
}

bool DropCopy::ready() const {
  return (*connection_).ready();
}

void DropCopy::operator()(Event<Start> const &) {
  (*connection_).start();
}

void DropCopy::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void DropCopy::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if ((*connection_).ready()) {
    if (welcome_) {
      if (next_ping_ < now) {
        send_ping(now);
      }
    }
  } else if (logon_timeout_.count() && logon_timeout_ < now) {
    assert(!welcome_);
    log::warn("Did not receive the welcome message, disconnecting now..."sv);
    (*connection_).close();
  }
  check_response_private_token();
  // DEBUG
  if (next_simulated_disconnect_.count() && next_simulated_disconnect_ < now) {
    next_simulated_disconnect_ = {};
    request_private_token();
  }
}

void DropCopy::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.welcome, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.pong, metrics::Type::PROFILE)
      .write(profile_.ack, metrics::Type::PROFILE)
      .write(profile_.balance, metrics::Type::PROFILE)
      .write(profile_.position_all, metrics::Type::PROFILE)
      .write(profile_.order_all, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void DropCopy::operator()(PrivateToken const &private_token) {
  if (!std::empty(private_token.query) && query_ != private_token.query) {
    query_ = private_token.query;
    log::warn(R"(DEBUG private_token="{}")"sv, query_);
    (*connection_).resume();
  }
}

void DropCopy::operator()(web::socket::Client::Connected const &) {
  assert(logon_timeout_.count() == 0);
  auto now = clock::get_system();
  logon_timeout_ = now + shared_.settings.ws.request_timeout;
  // DEBUG
  if (shared_.settings.misc.experimental_simulate_expired_token.count()) {
    next_simulated_disconnect_ = now + shared_.settings.misc.experimental_simulate_expired_token;
  }
}

void DropCopy::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
  welcome_ = false;
  logon_timeout_ = {};
  next_ping_ = {};
}

void DropCopy::operator()(web::socket::Client::Ready const &) {
  // note! wait for welcome
}

void DropCopy::operator()(web::socket::Client::Close const &) {
}

void DropCopy::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(web::socket::Client::Binary const &binary) {
  std::string_view payload{reinterpret_cast<char const *>(std::data(binary.payload)), std::size(binary.payload)};
  parse(payload);
}

void DropCopy::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::WS,
      .encoding = {Encoding::JSON},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_).get_interface(),
      .authority = (*connection_).get_current_authority(),
      .path = (*connection_).get_current_path(),
      .proxy = (*connection_).get_proxy(),
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    using enum DropCopyState;
    case UNDEFINED:
      assert(false);
      break;
    case SUBSCRIBE:
      (*this)(ConnectionStatus::DOWNLOADING, "subscribe"sv);
      subscribe();
      return 0;
    case DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0;
  }
  assert(false);
  return 0;
}

void DropCopy::subscribe() {
  subscribe_account("balance"sv);
  subscribe_trade("positionAll"sv);
  subscribe_trade("orderAll"sv);
  subscribe_trade("execution"sv);  // 2026-02-22: not currently supported by exchange
}

void DropCopy::subscribe_account(std::string_view const &channel) {
  auto now = clock::get_system();
  auto message = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("action":"SUBSCRIBE",)"
      R"("channel":"{}",)"
      R"("accountType":"UNIFIED")"
      R"(}})"sv,
      now.count(),
      channel);
  (*connection_).send_text(message);
}

void DropCopy::subscribe_trade(std::string_view const &channel) {
  auto now = clock::get_system();
  auto message = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("action":"SUBSCRIBE",)"
      R"("channel":"{}",)"
      R"("tradeType":"UNIFIED")"
      R"(}})"sv,
      now.count(),
      channel);
  (*connection_).send_text(message);
}

void DropCopy::send_ping(std::chrono::nanoseconds now) {
  assert(ping_frequency_.count() > 0);
  next_ping_ = now + ping_frequency_ / 2;
  auto message = fmt::format(
      R"({{)"
      R"("id":{},)"
      R"("type":"ping")"
      R"(}})"sv,
      now.count());
  (*connection_).send_text(message);
}

void DropCopy::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void DropCopy::operator()(Trace<json::Welcome> const &event) {
  profile_.welcome([&]() {
    auto &[trace_info, welcome] = event;
    log::info<1>("welcome={}"sv, welcome);
    welcome_ = true;
    download_.begin();
  });
}

void DropCopy::operator()(Trace<json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::error("error={}"sv, error);
    // XXX FIXME TODO this was carried over from roq-kucoin-futures -- check data
    if (error.code == 401 && error.data == "token is expired"sv) {
      request_private_token();
    }
  });
}

void DropCopy::operator()(Trace<json::Pong> const &event) {
  profile_.pong([&]() {
    auto &[trace_info, pong] = event;
    log::info<4>("pong={}"sv, pong);
  });
}

void DropCopy::operator()(Trace<json::Ack> const &event) {
  profile_.ack([&]() {
    auto &[trace_info, ack] = event;
    log::info<2>("ack={}"sv, ack);
  });
}

void DropCopy::operator()(Trace<json::Ticker> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Trade> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::OBU> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Balance> const &event) {
  profile_.balance([&]() {
    auto &[message_info, balance] = event;
    log::warn("DEBUG balance={}"sv, balance);
  });
}

void DropCopy::operator()(Trace<json::PositionAll> const &event) {
  profile_.position_all([&]() {
    auto &[message_info, position_all] = event;
    log::warn("DEBUG position_all={}"sv, position_all);
  });
}

void DropCopy::operator()(Trace<json::OrderAll> const &event) {
  profile_.order_all([&]() {
    auto &[message_info, order_all] = event;
    log::warn("DEBUG order_all={}"sv, order_all);
  });
}

void DropCopy::check_response_private_token() {
  if (download_private_token_ && request_.request_private_token < request_.respond_private_token) {
    download_private_token_ = false;
    log::warn("GOT PRIVATE TOKEN"sv);
  }
}

void DropCopy::request_private_token() {
  if (std::empty(query_)) {
    return;
  }
  log::warn("REQUEST PRIVATE TOKEN"sv);
  query_.clear();
  (*connection_).suspend(60s);
  request_.request_private_token = clock::get_system();
  download_private_token_ = true;
}

}  // namespace kucoin_pro
}  // namespace roq
