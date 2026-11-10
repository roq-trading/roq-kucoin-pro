/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/gateway/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/kucoin_pro/protocol/json/map.hpp"
#include "roq/kucoin_pro/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask{
    SupportType::FUNDS,
    SupportType::POSITION,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
};

uint64_t const REQUEST_ID = 1'000'000;

size_t const MAX_DECODE_BUFFER_DEPTH = 1;

auto const PING_FREQUENCY = 300;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account.name);
}

auto create_connection(auto &handler, auto &settings, auto &context, auto &shared) {
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
  return web::socket::Client::create(handler, context, config, shared.throttle, []() -> std::string { return {}; });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(
    Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, Request &request, std::string_view const &query)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account)}, query_{query},
      connection_{create_connection(*this, shared.settings, context, shared)}, ping_frequency_{PING_FREQUENCY},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH}, request_id_{stream_id_ * REQUEST_ID},
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
      account_{account}, shared_{shared}, request_{request}, download_{{}, [this](auto &event) { return download(event); }} {
}

void DropCopy::operator()(Trace<PrivateToken> const &event) {
  auto &[trace_info, private_token] = event;
  if (!std::empty(private_token.query) && query_ != private_token.query) {
    query_ = private_token.query;
    log::warn(R"(DEBUG private_token="{}")"sv, query_);
    (*connection_).resume();
  }
}

// server::Stream

void DropCopy::operator()(Trace<Start> const &) {
  (*connection_).start();
}

void DropCopy::operator()(Trace<Stop> const &) {
  (*connection_).stop();
}

void DropCopy::operator()(Trace<Timer> const &event) {
  auto &[trace_info, timer] = event;
  if ((*connection_).refresh(trace_info)) {
    check_request_queue(trace_info);
    if (ready()) {
      if (next_ping_ < trace_info.origin_create_time) {
        assert(ping_frequency_.count() > 0);
        next_ping_ = trace_info.origin_create_time + ping_frequency_ / 2;
        send_ping(timer.now);
      }
      check_response_private_token();
      check_stale_orders(trace_info);
      // DEBUG
      if (next_simulated_disconnect_.count() && next_simulated_disconnect_ < timer.now) {
        next_simulated_disconnect_ = {};
        request_private_token(trace_info);
      }
    }
  } else if (logon_timeout_.count() && logon_timeout_ < timer.now) {
    assert(!welcome_);
    log::warn("Did not receive the welcome message, disconnecting now..."sv);
    (*connection_).close();
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

void DropCopy::operator()(Trace<ConnectionStatus> const &event, std::string_view const &reason) {
  auto &[trace_info, connection_status] = event;
  connection_status_ = connection_status;
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
  create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
}

// web::socket::Client::Handler

void DropCopy::operator()(Trace<web::socket::Connected> const &event) {
  auto &[trace_info, connected] = event;
  assert(logon_timeout_.count() == 0);
  logon_timeout_ = trace_info.origin_create_time + shared_.settings.ws.request_timeout;
  // DEBUG
  if (shared_.settings.misc.experimental_simulate_expired_token.count()) {
    next_simulated_disconnect_ = trace_info.origin_create_time + shared_.settings.misc.experimental_simulate_expired_token;
  }
}

void DropCopy::operator()(Trace<web::socket::Disconnected> const &event) {
  auto &[trace_info, disconnected] = event;
  ++counter_.disconnect;
  ready_ = false;
  create_trace_and_dispatch_2(trace_info, ConnectionStatus::DISCONNECTED);
  download_.reset();
  welcome_ = false;
  logon_timeout_ = {};
  next_ping_ = {};
}

void DropCopy::operator()(Trace<web::socket::Ready> const &) {
  // note! wait for welcome
}

void DropCopy::operator()(Trace<web::socket::Close> const &event) {
  auto &[trace_info, close] = event;
  log::warn("close={}"sv, close);
}

void DropCopy::operator()(Trace<web::socket::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(Trace<web::socket::Text> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<web::socket::Binary> const &event) {
  auto &[trace_info, binary] = event;
  std::string_view payload{reinterpret_cast<char const *>(std::data(binary.payload)), std::size(binary.payload)};
  parse(trace_info, payload);
}

// core::Download

int32_t DropCopy::download(Trace<State> const &event) {
  auto &[trace_info, state] = event;
  auto publish_downloading = [&]() { create_trace_and_dispatch_2(trace_info, ConnectionStatus::DOWNLOADING, magic_enum::enum_name(state)); };
  switch (state) {
    using enum State;
    case UNDEFINED:
      assert(false);
      break;
    case SUBSCRIBE_BALANCES:
      publish_downloading();
      subscribe_account(state, "balance"sv);
      return 1;
    case GET_BALANCES:
      publish_downloading();
      request_.balances.request(trace_info);
      return 1;
    case SUBSCRIBE_POSITIONS:
      publish_downloading();
      subscribe_trade(state, "positionAll"sv);
      return 1;
    case GET_POSITIONS:
      publish_downloading();
      request_.positions.request(trace_info);
      return 1;
    case SUBSCRIBE_ORDERS:
      publish_downloading();
      subscribe_trade(state, "orderAll"sv);
      return 1;
    case GET_OPEN_ORDERS:
      publish_downloading();
      request_.open_orders.request(trace_info);
      return 1;
    case GET_STALE_ORDERS:
      if (std::empty(request_.details_stale_orders)) {
        return 0;
      } else {
        publish_downloading();
        request_.stale_orders.request(trace_info);
        return 1;
      }
    case SUBSCRIBE_TRADES:
      publish_downloading();
      subscribe_trade(state, "execution"sv);  // 2026-02-22: not currently supported by exchange
      return 1;
    case GET_TRADES:
      if (shared_.settings.download.trades_lookback.count()) {
        publish_downloading();
        request_.trades.request(trace_info);
        return 1;
      } else {
        return 0;
      }
    case DONE:
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0;
  }
  assert(false);
  return 0;
}

// protocol::json::Parser::Handler

void DropCopy::operator()(Trace<protocol::json::Welcome> const &event) {
  profile_.welcome([&]() {
    auto &[trace_info, welcome] = event;
    log::info<1>("welcome={}"sv, welcome);
    welcome_ = true;
    download_.begin(trace_info);
  });
}

void DropCopy::operator()(Trace<protocol::json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::error("error={}"sv, error);
    // XXX FIXME TODO this was carried over from roq-kucoin-futures -- check data
    if (error.code == 401 && error.data == "token is expired"sv) {
      request_private_token(trace_info);
    }
  });
}

void DropCopy::operator()(Trace<protocol::json::Pong> const &event) {
  profile_.pong([&]() {
    auto &[trace_info, pong] = event;
    log::info<4>("pong={}"sv, pong);
  });
}

void DropCopy::operator()(Trace<protocol::json::Ack> const &event) {
  profile_.ack([&]() {
    auto &[trace_info, ack] = event;
    log::info<2>("ack={}"sv, ack);
    auto request_id = magic_enum::enum_cast<State>(ack.id);
    if (request_id.has_value()) {
      auto state = request_id.value();
      download_.check(trace_info, state);
    } else {
      log::warn(R"(Unexpected: request_id="{}")"sv, ack.id);
    }
  });
}

void DropCopy::operator()(Trace<protocol::json::Ticker> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<protocol::json::Trade> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<protocol::json::OBU> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<protocol::json::Balance> const &event) {
  profile_.balance([&]() {
    auto &[message_info, balance] = event;
    log::warn("DEBUG balance={}"sv, balance);
  });
}

void DropCopy::operator()(Trace<protocol::json::PositionAll> const &event) {
  profile_.position_all([&]() {
    auto &[message_info, position_all] = event;
    log::warn("DEBUG position_all={}"sv, position_all);
  });
}

void DropCopy::operator()(Trace<protocol::json::OrderAll> const &event) {
  profile_.order_all([&]() {
    auto &[message_info, order_all] = event;
    log::warn("DEBUG order_all={}"sv, order_all);
  });
}

// helpers

void DropCopy::check_response_private_token() {
  if (request_.private_token.has_response()) {
    log::warn("GOT PRIVATE TOKEN"sv);
  }
}

void DropCopy::request_private_token(TraceInfo const &trace_info) {
  if (std::empty(query_)) {
    return;
  }
  log::warn("REQUEST PRIVATE TOKEN"sv);
  query_.clear();
  (*connection_).suspend_for(60s);
  request_.private_token.request(trace_info);
}

void DropCopy::subscribe_account(State state, std::string_view const &channel) {
  auto message = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("action":"SUBSCRIBE",)"
      R"("channel":"{}",)"
      R"("accountType":"UNIFIED")"
      R"(}})"sv,
      state,
      channel);
  (*connection_).send_text(message);
}

void DropCopy::subscribe_trade(State state, std::string_view const &channel) {
  auto message = fmt::format(
      R"({{)"
      R"("id":"{}",)"
      R"("action":"SUBSCRIBE",)"
      R"("channel":"{}",)"
      R"("tradeType":"UNIFIED")"
      R"(}})"sv,
      state,
      channel);
  (*connection_).send_text(message);
}

void DropCopy::send_ping(std::chrono::nanoseconds now) {
  auto message = fmt::format(
      R"({{)"
      R"("id":{},)"
      R"("type":"ping")"
      R"(}})"sv,
      now.count());
  (*connection_).send_text(message);
}

void DropCopy::check_request_queue(TraceInfo const &trace_info) {
  auto state = download_.state();
  switch (state) {
    using enum State;
    case UNDEFINED:
      break;
    case SUBSCRIBE_BALANCES:
      break;
    case GET_BALANCES:
      if (request_.balances.has_response()) {
        download_.check(trace_info, state);
      }
      break;
    case SUBSCRIBE_POSITIONS:
      break;
    case GET_POSITIONS:
      if (request_.positions.has_response()) {
        download_.check(trace_info, state);
      }
      break;
    case SUBSCRIBE_ORDERS:
      break;
    case GET_OPEN_ORDERS:
      if (request_.open_orders.has_response()) {
        check_stale_orders();  // note! *before* updating download
        download_.check(trace_info, state);
      }
      break;
    case GET_STALE_ORDERS:
      if (request_.stale_orders.has_response()) {
        if (!std::empty(request_.details_stale_orders)) {
          log::fatal("Unexpected"sv);
        }
        download_.check(trace_info, state);
      }
      break;
    case SUBSCRIBE_TRADES:
      break;
    case GET_TRADES:
      if (request_.trades.has_response()) {
        download_.check(trace_info, state);
      }
      break;
    case DONE:
      break;
  }
}

bool DropCopy::check_stale_orders() {
  size_t counter = 0;
  auto helper = [&](auto &order) {
    auto key = std::make_pair(order.user_id, order.order_id);
    auto res = request_.details_stale_orders.insert(key);
    if (res.second) {
      log::warn(
          R"(DEBUG stale order: )"
          R"(order_id={}, client_order_id="{}", external_order_id="{}")"sv,
          order.order_id,
          order.client_order_id,
          order.external_order_id);
      ++counter;
    }
  };
  if (shared_.dispatcher.get_all_stale_orders(helper, account_.name)) {
  } else {
  }
  if (counter) {
    log::info("Found {} new lost order(s)"sv, counter);
    return true;
  } else {
    return false;
  }
}

void DropCopy::check_stale_orders(TraceInfo const &trace_info) {
  if (next_check_stale_orders_ < trace_info.origin_create_time) {
    next_check_stale_orders_ = trace_info.origin_create_time + shared_.settings.oms.stale_order_check_freq;
    if (check_stale_orders()) {
      request_.stale_orders.request(trace_info);
    }
  }
}

void DropCopy::parse(TraceInfo const &trace_info, std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      if (!protocol::json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
