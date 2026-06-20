/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/gateway/rest.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/kucoin_pro/protocol/json/map.hpp"
#include "roq/kucoin_pro/protocol/json/utils.hpp"

#include "roq/kucoin_pro/tools/splitter.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;

int32_t const SYSTEM_CODE_SUCCESS = 200000;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .currencies = create_metrics(shared.settings, name_, "currencies"sv),
          .currencies_ack = create_metrics(shared.settings, name_, "currencies_ack"sv),
          .instrument = create_metrics(shared.settings, name_, "instrument"sv),
          .instrument_ack = create_metrics(shared.settings, name_, "instrument_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto state) { return download(state); }} {
}

void Rest::operator()(Event<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
}

void Rest::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.currencies, metrics::Type::PROFILE)
      .write(profile_.currencies_ack, metrics::Type::PROFILE)
      .write(profile_.instrument, metrics::Type::PROFILE)
      .write(profile_.instrument_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void Rest::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::HTTP,
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

void Rest::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    download_.begin();
  }
}

void Rest::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
}

void Rest::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t Rest::download(State state) {
  switch (state) {
    using enum State;
    case UNDEFINED:
      assert(false);
      break;
    case CURRENCIES:
      (*this)(ConnectionStatus::DOWNLOADING, "currencies"sv);
      get_currencies();
      return 1;
    case INSTRUMENT:
      (*this)(ConnectionStatus::DOWNLOADING, "instrument"sv);
      get_instrument();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

// currencies

void Rest::get_currencies() {
  profile_.currencies([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.rest_public.asset_currencies,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_currencies_ack(event, sequence);
    };
    (*connection_)("currencies"sv, request, callback);
  });
}

void Rest::get_currencies_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::CURRENCIES;
  profile_.currencies_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::CurrenciesAck currencies_ack{body, decode_buffer_};
        if (currencies_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, currencies_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, protocol::json::guess_error(currencies_ack.code), currencies_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<protocol::json::CurrenciesAck> const &event) {
  auto &[trace_info, currencies_ack] = event;
  log::info<4>("currencies_ack={}"sv, currencies_ack);
  /*
  for (auto &item : currencies_ack.data) {
  }
  */
}

// instrument

void Rest::get_instrument() {
  profile_.instrument([&]() {
    auto query = "?tradeType=FUTURES"sv;
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.rest_public.market_instrument,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_instrument_ack(event, sequence);
    };
    (*connection_)("instrument"sv, request, callback);
  });
}

void Rest::get_instrument_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::INSTRUMENT;
  profile_.instrument_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::InstrumentAck instrument_ack{body, decode_buffer_};
        if (instrument_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, instrument_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, protocol::json::guess_error(instrument_ack.code), instrument_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<protocol::json::InstrumentAck> const &event) {
  auto &[trace_info, instrument_ack] = event;
  log::info<4>("instrument_ack={}"sv, instrument_ack);
  std::vector<Symbol> symbols;
  size_t counter = 0;
  for (auto &item : instrument_ack.data.list) {
    log::info<2>("item={}"sv, item);
    auto discard = shared_.dispatcher.discard_symbol(item.symbol);
    auto security_type = map(instrument_ack.data.trade_type).template get<SecurityType>();
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .description = {},
        .security_type = security_type,
        .external_security_id = {},
        .cfi_code = {},
        .base_currency = item.base_currency,
        .quote_currency = item.quote_currency,
        .settlement_currency = {},  // XXX isInverse
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.tick_size,
        .tick_size_steps = {},
        .multiplier = NaN,
        .min_notional = NaN,
        .min_trade_vol = item.lot_size,
        .max_trade_vol = item.max_base_order_size,
        .trade_vol_step_size = item.lot_size,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = utils::safe_cast(item.launch_time),
        .settlement_date = utils::safe_cast(item.settlement_time),
        .expiry_datetime = {},
        .expiry_datetime_utc = utils::safe_cast(item.expiry_time),
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, reference_data, true);
    if (discard) {
      log::info<1>(R"(Drop symbol="{}")"sv, item.symbol);
      continue;
    }
    if (shared_.all_symbols.emplace(item.symbol).second) {  // only include new
      symbols.emplace_back(item.symbol);
    }
    ++counter;
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0) {
    log::info("Contracts {} / {}"sv, counter, std::size(instrument_ack.data.list));
  }
  // XXX FIXME TODO trading_status
}

void Rest::process_response(web::rest::Response const &response, auto error_handler, auto success_handler) {
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case UNKNOWN:
      case INFORMATIONAL_RESPONSE:
        response.expect(web::http::Status::OK);  // throws
        break;
      case SUCCESS:
        success_handler(body);
        break;
      case REDIRECTION:
        log::fatal("Unexpected: URL is being redirected"sv);
      case CLIENT_ERROR:
        success_handler(body);  // throws
        break;
      case SERVER_ERROR: {
        auto message = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, message);
        break;
      }
    }
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
