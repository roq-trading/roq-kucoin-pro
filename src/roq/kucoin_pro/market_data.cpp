/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/market_data.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/kucoin_pro/json/map.hpp"
#include "roq/kucoin_pro/json/utils.hpp"

#include "roq/kucoin_pro/tools/splitter.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&settings.ws.public_uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
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
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
          .total_bytes_received = create_metrics(shared.settings, name_, "total_bytes_received"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .welcome = create_metrics(shared.settings, name_, "welcome"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .pong = create_metrics(shared.settings, name_, "pong"sv),
          .ack = create_metrics(shared.settings, name_, "ack"sv),
          .ticker = create_metrics(shared.settings, name_, "ticker"sv),
          .trade = create_metrics(shared.settings, name_, "trade"sv),
          .obu = create_metrics(shared.settings, name_, "obu"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      shared_{shared} {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if ((*connection_).ready()) {
    if (welcome_) {
      if (next_ping_ < now) {
        send_ping(now);
      }
      check_subscribe_queue(now);
    }
  } else if (logon_timeout_.count() && logon_timeout_ < now) {
    assert(!welcome_);
    log::warn("Did not receive the welcome message, disconnecting now..."sv);
    (*connection_).close();
  }
}

void MarketData::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(counter_.total_bytes_received, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.welcome, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.pong, metrics::Type::PROFILE)
      .write(profile_.ack, metrics::Type::PROFILE)
      .write(profile_.ticker, metrics::Type::PROFILE)
      .write(profile_.trade, metrics::Type::PROFILE)
      .write(profile_.obu, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
}

void MarketData::operator()(web::socket::Client::Connected const &) {
  assert(logon_timeout_.count() == 0);
  auto now = clock::get_system();
  logon_timeout_ = now + shared_.settings.ws.request_timeout;
}

void MarketData::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  welcome_ = false;
  logon_timeout_ = {};
  next_ping_ = {};
  // experimental
  shared_.mbp_sequencer.clear();  // XXX HANS this is SHARED !!!
}

void MarketData::operator()(web::socket::Client::Ready const &) {
  // note! wait for welcome
}

void MarketData::operator()(web::socket::Client::Close const &) {
}

void MarketData::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(web::socket::Client::Binary const &binary) {
  std::string_view payload{reinterpret_cast<char const *>(std::data(binary.payload)), std::size(binary.payload)};
  parse(payload);
  counter_.total_bytes_received.update((*connection_).total_bytes_received());
}

void MarketData::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
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

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  if (shared_.settings.misc.top_of_book_from_ticker) {
    subscribe(shared_.api.ticker, symbols);
  } else {
    subscribe(shared_.api.obu, symbols, "1"sv);  // bbo
  }
  subscribe(shared_.api.trade, symbols);
  // XXX FIXME TODO only if has master account !!!
  if (shared_.settings.misc.experimental_enable_mbp) {
    subscribe(shared_.api.obu, symbols, "increment"sv);  // full depth
  }
}

void MarketData::subscribe(std::string_view const &topic, std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  for (auto &item : symbols) {
    auto now = clock::get_system();
    auto message = fmt::format(
        R"({{)"
        R"("id":"{}",)"
        R"("action":"SUBSCRIBE",)"
        R"("channel":"{}",)"
        R"("tradeType":"FUTURES",)"
        R"("symbol":"{}")"
        R"(}})"sv,
        now.count(),
        topic,
        item);
    subscribe_queue_.emplace_back(message);
  }
}

void MarketData::subscribe(std::string_view const &topic, std::span<Symbol const> const &symbols, std::string_view const &depth) {
  assert(!std::empty(symbols));
  for (auto &item : symbols) {
    auto now = clock::get_system();
    auto message = fmt::format(
        R"({{)"
        R"("id":"{}",)"
        R"("action":"SUBSCRIBE",)"
        R"("channel":"{}",)"
        R"("tradeType":"FUTURES",)"
        R"("symbol":"{}",)"
        R"("depth":"{}",)"
        R"("rpiFilter":0)"
        R"(}})"sv,
        now.count(),
        topic,
        item,
        depth);
    subscribe_queue_.emplace_back(message);
  }
}

void MarketData::send_ping(std::chrono::nanoseconds now) {
  assert(shared_.settings.ws.ping_freq.count() > 0);
  next_ping_ = now + shared_.settings.ws.ping_freq / 2;
  auto message = fmt::format(
      R"({{)"
      R"("id":{},)"
      R"("type":"ping")"
      R"(}})"sv,
      now.count());
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
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

void MarketData::operator()(Trace<json::Welcome> const &event) {
  profile_.welcome([&]() {
    auto &[trace_info, welcome] = event;
    log::info<1>("welcome={}"sv, welcome);
    log::warn("DEBUG welcome={}"sv, welcome);
    (*connection_).touch(trace_info.source_receive_time);
    welcome_ = true;
    (*this)(ConnectionStatus::READY);
    subscribe();
  });
}

void MarketData::operator()(Trace<json::Error> const &event) {
  profile_.error([&]() {
    // XXX HANS DEBUG
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
    // log::fatal("event={{error={}, trace_info={}}}"sv, error, trace_info);
  });
}

void MarketData::operator()(Trace<json::Pong> const &event) {
  profile_.pong([&]() {
    auto &[trace_info, pong] = event;
    log::info<4>("pong={}"sv, pong);
  });
}

void MarketData::operator()(Trace<json::Ack> const &event) {
  profile_.ack([&]() {
    auto &[trace_info, ack] = event;
    log::info<2>("ack={}"sv, ack);
  });
}

// note! only updated on trade
void MarketData::operator()(Trace<json::Ticker> const &event) {
  profile_.ticker([&]() {
    auto &[trace_info, ticker] = event;
    log::info<4>("ticker={}"sv, ticker);
    (*connection_).touch(trace_info.source_receive_time);
    auto &data = ticker.data;
    auto top_of_book = TopOfBook{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = data.symbol,
        .layer{
            .bid_price = data.best_bid_price,
            .bid_quantity = data.best_bid_size,
            .ask_price = data.best_ask_price,
            .ask_quantity = data.best_ask_size,
        },
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = data.timestamp,
        .exchange_sequence = data.sequence_number,
        .sending_time_utc = ticker.push_time,
    };
    create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
  });
}

void MarketData::operator()(Trace<json::Trade> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, trade] = event;
    log::info<4>("trade={}"sv, trade);
    (*connection_).touch(trace_info.source_receive_time);
    auto &data = trade.data;
    auto trade_2 = Trade{
        .side = map(data.side),
        .price = data.price,
        .quantity = data.quantity,
        .trade_id = data.trade_id,
        .taker_order_id = {},
        .maker_order_id = {},
    };
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = data.symbol,
        .trades = {&trade_2, 1},
        .exchange_time_utc = data.timestamp,
        .exchange_sequence = data.sequence_number,
        .sending_time_utc = trade.push_time,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  });
}

void MarketData::operator()(Trace<json::OBU> const &event) {
  profile_.obu([&]() {
    auto &[trace_info, obu] = event;
    log::info<4>("obu={}"sv, obu);
    (*connection_).touch(trace_info.source_receive_time);
    auto get_price = [&](auto &items) -> double {
      if (std::empty(items)) {
        return NaN;
      }
      return items[0].price;
    };
    auto get_quantity = [&](auto &items) -> double {
      if (std::empty(items)) {
        return NaN;
      }
      return items[0].quantity;
    };
    auto &data = obu.data;
    auto dispatch_top_of_book = [&]() {
      auto top_of_book = TopOfBook{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = data.symbol,
          .layer{
              .bid_price = get_price(data.bids),
              .bid_quantity = get_quantity(data.bids),
              .ask_price = get_price(data.asks),
              .ask_quantity = get_quantity(data.asks),
          },
          .update_type = UpdateType::SNAPSHOT,
          .exchange_time_utc = data.timestamp,
          .exchange_sequence = data.end_sequence,
          .sending_time_utc = obu.push_time,
      };
      create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
    };
    auto dispatch_market_by_price = [&]() {
      auto first_sequence = data.start_sequence;
      auto last_sequence = data.end_sequence;
      auto previous_sequence = data.start_sequence - 1;  // ???
      auto &sequencer = shared_.mbp_sequencer[data.symbol];
      auto &mbp = shared_.get_mbp();
      auto emplace_back = [](auto &result, auto &value) {
        auto mbp_update = MBPUpdate{
            .price = value.price,
            .quantity = value.quantity,
            .implied_quantity = NaN,
            .number_of_orders = {},
            .update_action = {},
            .price_level = {},
        };
        result.emplace_back(std::move(mbp_update));
      };
      for (auto &item : data.bids) {
        emplace_back(mbp.bids, item);
      }
      for (auto &item : data.asks) {
        emplace_back(mbp.asks, item);
      }
      try {
        auto create_update = [&](auto &bids, auto &asks, auto update_type, auto exchange_sequence) -> MarketByPriceUpdate {
          return {
              .stream_id = stream_id_,
              .exchange = shared_.settings.exchange,
              .symbol = data.symbol,
              .bids = bids,
              .asks = asks,
              .update_type = update_type,
              .exchange_time_utc = data.timestamp,
              .exchange_sequence = exchange_sequence,
              .sending_time_utc = {},
              .price_precision = {},
              .quantity_precision = {},
              .max_depth = {},
              .checksum = {},
          };
        };
        auto publish_update = [&](auto &bids, auto &asks) {
          auto market_by_price_update = create_update(bids, asks, UpdateType::INCREMENTAL, last_sequence);
          create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
        };
        auto publish_snapshot = [&](auto &bids, auto &asks, auto sequence, [[maybe_unused]] auto retries, [[maybe_unused]] auto delay) {
          log::info(R"(DEBUG PUBLISH SNAPSHOT symbol="{}", sequence={})"sv, data.symbol, sequence);
          auto market_by_price_update = create_update(bids, asks, UpdateType::SNAPSHOT, sequencer.last_sequence());
          Trace event{trace_info, market_by_price_update};
          shared_(event, true, [&](auto &market_by_price) { sequencer.apply(market_by_price, sequence, false); });
        };
        auto request_snapshot = [&](auto retries) {
          log::info(R"(DEBUG REQUEST symbol="{}" (retries={}))"sv, data.symbol, retries);
          if (shared_.settings.ws.mbp_request_max_retries && shared_.settings.ws.mbp_request_max_retries < retries) {
            log::fatal(R"(Unexpected: symbol="{}", retries={})"sv, data.symbol, retries);
          }
          shared_.depth_request_queue.emplace_back(data.symbol);
        };
        sequencer(mbp.bids, mbp.asks, data.start_sequence, data.end_sequence, previous_sequence, publish_update, publish_snapshot, request_snapshot);
      } catch (BadState &) {
        log::warn(R"(RESUBSCRIBE symbol="{}")"sv, data.symbol);
        // XXX HANS publish stale
        sequencer.clear();
        shared_.depth_request_queue.emplace_back(data.symbol);
      }
    };
    switch (obu.depth) {
      using enum json::Depth::type_t;
      case UNDEFINED_INTERNAL:
      case UNKNOWN_INTERNAL:
        log::fatal("Unexpected"sv);
      case ONE:
        dispatch_top_of_book();
        break;
      case INCREMENT:
        dispatch_market_by_price();
        break;
    }
  });
}

void MarketData::operator()(Trace<json::Balance> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::PositionAll> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::OrderAll> const &) {
  log::fatal("Unexpected"sv);
}

// helpers

void MarketData::check_subscribe_queue(std::chrono::nanoseconds now) {
  subscribe_queue_.dispatch([&](auto now) { return shared_.rate_limiter.can_request(now); }, [&](auto &message) { (*connection_).send_text(message); }, now);
}

}  // namespace kucoin_pro
}  // namespace roq
