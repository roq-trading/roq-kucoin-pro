/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/order_entry_rest.hpp"

#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/kucoin_pro/json/encoder.hpp"
#include "roq/kucoin_pro/json/map.hpp"
#include "roq/kucoin_pro/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::TRADE,
    SupportType::FUNDS,
    SupportType::POSITION,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;

int32_t const SYSTEM_CODE_SUCCESS = 200'000;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account.name);
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

OrderEntryREST::OrderEntryREST(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, Request &request)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account)}, master_{account.master},
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .private_token = create_metrics(shared.settings, name_, "private_token"sv),
          .private_token_ack = create_metrics(shared.settings, name_, "private_token_ack"sv),
          .account = create_metrics(shared.settings, name_, "account"sv),
          .account_ack = create_metrics(shared.settings, name_, "account_ack"sv),
          .position = create_metrics(shared.settings, name_, "position"sv),
          .position_ack = create_metrics(shared.settings, name_, "position_ack"sv),
          .orders = create_metrics(shared.settings, name_, "orders"sv),
          .orders_ack = create_metrics(shared.settings, name_, "orders_ack"sv),
          .execution = create_metrics(shared.settings, name_, "execution"sv),
          .execution_ack = create_metrics(shared.settings, name_, "execution_ack"sv),
          .create_order = create_metrics(shared.settings, name_, "create_order"sv),
          .create_order_ack = create_metrics(shared.settings, name_, "create_order_ack"sv),
          .cancel_order = create_metrics(shared.settings, name_, "cancel_order"sv),
          .cancel_order_ack = create_metrics(shared.settings, name_, "cancel_order_ack"sv),
          .cancel_all_orders = create_metrics(shared.settings, name_, "cancel_all_orders"sv),
          .cancel_all_orders_ack = create_metrics(shared.settings, name_, "cancel_all_orders_ack"sv),
          .order_book = create_metrics(shared.settings, name_, "order_book"sv),
          .order_book_ack = create_metrics(shared.settings, name_, "order_book_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, shared_{shared}, request_{request}, download_{shared.settings.rest.request_timeout, [this](auto state) { return download(state); }} {
}

void OrderEntryREST::operator()(Event<Start> const &) {
  (*connection_).start();
}

void OrderEntryREST::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void OrderEntryREST::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if (!ready()) {
    return;
  }
  if (master_) {
    check_request_queue(now);
  }
  if (!downloading() && request_.respond_private_token < request_.request_private_token) {
    get_private_token();
    download_private_token_ = true;
  }
}

void OrderEntryREST::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.private_token, metrics::Type::PROFILE)
      .write(profile_.private_token_ack, metrics::Type::PROFILE)
      .write(profile_.account, metrics::Type::PROFILE)
      .write(profile_.account_ack, metrics::Type::PROFILE)
      .write(profile_.position, metrics::Type::PROFILE)
      .write(profile_.position_ack, metrics::Type::PROFILE)
      .write(profile_.orders, metrics::Type::PROFILE)
      .write(profile_.orders_ack, metrics::Type::PROFILE)
      .write(profile_.execution, metrics::Type::PROFILE)
      .write(profile_.execution_ack, metrics::Type::PROFILE)
      .write(profile_.create_order, metrics::Type::PROFILE)
      .write(profile_.create_order_ack, metrics::Type::PROFILE)
      .write(profile_.cancel_order, metrics::Type::PROFILE)
      .write(profile_.cancel_order_ack, metrics::Type::PROFILE)
      .write(profile_.cancel_all_orders, metrics::Type::PROFILE)
      .write(profile_.cancel_all_orders_ack, metrics::Type::PROFILE)
      .write(profile_.order_book, metrics::Type::PROFILE)
      .write(profile_.order_book_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

uint16_t OrderEntryREST::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  create_order(event, order, ref_data, request_id);
  return stream_id_;
}

uint16_t OrderEntryREST::operator()(
    Event<ModifyOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t OrderEntryREST::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  cancel_order(event, order, ref_data, request_id, previous_request_id);
  return stream_id_;
}

uint16_t OrderEntryREST::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  cancel_all_orders(event, request_id);
  return stream_id_;
}

void OrderEntryREST::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    download_.begin();
  }
}

void OrderEntryREST::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
  download_private_token_ = false;
}

void OrderEntryREST::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void OrderEntryREST::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
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
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

uint32_t OrderEntryREST::download(OrderEntryState state) {
  switch (state) {
    using enum OrderEntryState;
    case UNDEFINED:
      assert(false);
      break;
    case PRIVATE_TOKEN:
      if (!has_downloaded_private_token_) {
        (*this)(ConnectionStatus::DOWNLOADING, "private-token"sv);
        get_private_token();
        return 1;
      } else {
        return 0;
      }
    case ACCOUNT:
      (*this)(ConnectionStatus::DOWNLOADING, "account"sv);
      get_account();
      return 1;
    case POSITION:
      (*this)(ConnectionStatus::DOWNLOADING, "position"sv);
      get_position();
      return 1;
    case ORDERS:
      (*this)(ConnectionStatus::DOWNLOADING, "orders"sv);
      get_orders();
      return 1;
    case EXECUTION:
      if (shared_.settings.download.trades_lookback.count()) {
        (*this)(ConnectionStatus::DOWNLOADING, "execution"sv);
        get_execution();
        return 1;
      } else {
        return 0;
      }
    case DONE:
      (*this)(ConnectionStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

// private-token

void OrderEntryREST::get_private_token() {
  profile_.private_token([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.rest_private.bullet_private;
    auto headers = account_.create_headers(method, path, {}, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_private_token_ack(event, sequence);
    };
    (*connection_)("private-token", request, callback);
  });
}

void OrderEntryREST::get_private_token_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = OrderEntryState::PRIVATE_TOKEN;
  profile_.private_token_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      if (download_.downloading()) {
        download_.retry(STATE);
      }
      request_.respond_private_token = clock::get_system();  // completion
      download_private_token_ = false;
    };
    auto handle_success = [&](auto &body) {
      if (download_.downloading() && download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::Token token{body, decode_buffer_};
        if (token.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, token};
          (*this)(event_2);
        } else {
          log::fatal("Unexpected: token={}"sv, token);
        }
        if (download_.downloading()) {
          download_.check(STATE);
        }
        request_.respond_private_token = clock::get_system();  // completion
        download_private_token_ = false;
        has_downloaded_private_token_ = true;
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::Token> const &event) {
  auto &[trace_info, token] = event;
  log::info<2>("token={}"sv, token);
  auto query = fmt::format("?token={}"sv, token.data.token);
  auto private_token = PrivateToken{
      .account = account_.name,
      .query = query,
  };
  handler_(private_token);
}

// account

void OrderEntryREST::get_account() {
  profile_.account([&]() {
    auto method = web::http::Method::GET;
    auto path = shared_.api.rest_private.account_balance;
    auto query = "?accountMode=UNIFIED"sv;
    auto headers = account_.create_headers(method, path, query, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_account_ack(event, sequence);
    };
    (*connection_)("account"sv, request, callback);
  });
}

void OrderEntryREST::get_account_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = OrderEntryState::ACCOUNT;
  profile_.account_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::AccountAck account_ack{body, decode_buffer_};
        if (account_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, account_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(account_ack.code), account_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::AccountAck> const &event) {
  auto &[trace_info, account_ack] = event;
  log::info<2>("account_ack={}"sv, account_ack);
  auto &data = account_ack.data;
  for (auto &account : data.accounts) {
    for (auto &item : account.currencies) {
      auto funds_update = FundsUpdate{
          .stream_id = stream_id_,
          .account = account_.name,
          .currency = item.currency,
          .margin_mode = {},
          .balance = item.balance,
          .hold = item.hold,
          .borrowed = NaN,
          .unrealized_pnl = NaN,
          .external_account = {},
          .update_type = UpdateType::SNAPSHOT,
          .exchange_time_utc = {},  // ???
          .exchange_sequence = {},  // ???
          .sending_time_utc = {},
      };
      create_trace_and_dispatch(handler_, trace_info, funds_update, true);
    }
  }
}

// position

void OrderEntryREST::get_position() {
  profile_.position([&]() {
    auto method = web::http::Method::GET;
    auto path = shared_.api.rest_private.position_open_list;
    auto query = "?accountMode=UNIFIED"sv;
    auto headers = account_.create_headers(method, path, query, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_position_ack(event, sequence);
    };
    (*connection_)("position", request, callback);
  });
}

void OrderEntryREST::get_position_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = OrderEntryState::POSITION;
  profile_.position_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::PositionAck position_ack{body, decode_buffer_};
        if (position_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, position_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(position_ack.code), position_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::PositionAck> const &event) {
  auto &[trace_info, position_ack] = event;
  log::info<2>("position_ack={}"sv, position_ack);
  for (auto &item : position_ack.data) {
    log::debug("item={}"sv, item);
    /*
    auto position_update = PositionUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .margin_mode = map(item.margin_mode),
        .external_account = {},
        .long_quantity = std::max(item.current_qty, 0.0),    // XXX FIXME TODO qty ???
        .short_quantity = std::max(-item.current_qty, 0.0),  // XXX FIXME TODO qty ???
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},  // ???
        .exchange_sequence = {},
        .sending_time_utc = item.current_timestamp,
    };
    create_trace_and_dispatch(handler_, trace_info, position_update, true);
    */
  }
}

// orders

void OrderEntryREST::get_orders() {
  profile_.orders([&]() {
    auto method = web::http::Method::GET;
    auto path = shared_.api.rest_private.order_open_list;
    auto query = "?accountMode=UNIFIED&tradeType=FUTURES"sv;
    auto headers = account_.create_headers(method, path, query, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_orders_ack(event, sequence);
    };
    (*connection_)("orders", request, callback);
  });
}

void OrderEntryREST::get_orders_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = OrderEntryState::ORDERS;
  profile_.orders_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::OrdersAck orders_ack{body, decode_buffer_};
        if (orders_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, orders_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(orders_ack.code), orders_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::OrdersAck> const &event) {
  auto &[trace_info, orders_ack] = event;
  log::info<2>("orders_ack={}"sv, orders_ack);
  for (auto &item : orders_ack.data.items) {
    log::debug("item={}"sv, item);
    // note! following shouldn't be necessary due to only downloading active orders -- just being safe
    auto order_status = [&]() -> OrderStatus {
      auto result = map(item.status).template get<OrderStatus>();
      if (result == OrderStatus::COMPLETED) {
        if (!utils::is_equal(item.size, item.filled_size)) {
          return OrderStatus::CANCELED;
        }
      }
      return result;
    }();
    auto remaining_quantity = [&]() -> double {
      if (order_status == OrderStatus::CANCELED) {
        return 0.0;
      }
      return item.size - item.filled_size;
    }();
    auto order_update = server::oms::OrderUpdate{
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .side = map(item.side),
        .position_effect = {},
        .margin_mode = {},
        .max_show_quantity = NaN,
        .order_type = map(item.order_type),
        .time_in_force = map(item.time_in_force),
        .execution_instructions = {},
        .create_time_utc = item.order_time,
        .update_time_utc = item.updated_time,
        .external_account = {},
        .external_order_id = item.order_id,
        .client_order_id = item.client_oid,
        .order_status = order_status,
        .error = {},
        .text = {},
        .quantity = item.size,
        .price = item.price,
        .stop_price = NaN,
        .leverage = NaN,
        .remaining_quantity = remaining_quantity,
        .traded_quantity = item.filled_size,
        .average_traded_price = item.avg_price,
        .last_traded_quantity = NaN,
        .last_traded_price = NaN,
        .last_liquidity = {},
        .routing_id = {},
        .max_request_version = {},
        .max_response_version = {},
        .max_accepted_version = {},
        .update_type = UpdateType::SNAPSHOT,
        .sending_time_utc = {},
    };
    Trace event_2{trace_info, order_update};
    (*this)(event_2, item.client_oid);
  }
}

// execution

void OrderEntryREST::get_execution() {
  profile_.execution([&]() {
    auto method = web::http::Method::GET;
    auto path = shared_.api.rest_private.order_execution;
    auto query = "?accountMode=UNIFIED&tradeType=FUTURES"sv;
    auto headers = account_.create_headers(method, path, query, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_execution_ack(event, sequence);
    };
    (*connection_)("execution", request, callback);
  });
}

void OrderEntryREST::get_execution_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = OrderEntryState::EXECUTION;
  profile_.execution_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::ExecutionAck execution_ack{body, decode_buffer_};
        if (execution_ack.code == SYSTEM_CODE_SUCCESS) {
          Trace event_2{event, execution_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(execution_ack.code), execution_ack.msg);
        }
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

// note! we don't get a client_oid
void OrderEntryREST::operator()(Trace<json::ExecutionAck> const &event) {
  auto &[trace_info, execution_ack] = event;
  log::info<2>("execution_ack={}"sv, execution_ack);
  /*
  for (auto &item : execution_ack.data) {
    log::debug("item={}"sv, item);
    auto fill = Fill{
        .exchange_time_utc = {},
        .external_trade_id = item.trade_id,
        .quantity = item.size,
        .price = item.price,
        .liquidity = map(item.liquidity),
        .commission_amount = NaN,  // XXX FIXME TODO open_fee_pay + close_fee_pay
        .commission_currency = {},
        .base_amount = NaN,
        .quote_amount = NaN,
        .profit_loss_amount = NaN,
    };
    auto trade_update = TradeUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .order_id = {},  // XXX FIXME TODO this is an issue !!!
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .side = map(item.side),
        .position_effect = map(item.position_side, item.side),
        .margin_mode = map(item.margin_mode),
        .create_time_utc = item.trade_time,
        .update_time_utc = item.trade_time,
        .external_account = {},
        .external_order_id = item.order_id,
        .client_order_id = {},  // XXX FIXME TODO this is an issue !!!
        .execution = {&fill, 1},
        .routing_id = {},
        .update_type = UpdateType::SNAPSHOT,
        .sending_time_utc = {},
        .user = {},
        .strategy_id = {},
    };
    // create_trace_and_dispatch(handler_, trace_info, trade_update, true, order.user_id, order.client_order_id);
  }
  */
}

// create-order

void OrderEntryREST::create_order(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  profile_.create_order([&]() {
    if (!ready()) {
      throw server::oms::NotReady{"not ready"sv};
    }
    auto &[message_info, create_order] = event;
    auto method = web::http::Method::POST;
    auto path = shared_.api.rest_private.order_place;
    auto body = json::Encoder::add_order(encode_buffer_, create_order, order, ref_data, request_id);
    auto headers = account_.create_headers(method, path, {}, body);
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    log::warn(R"(DEBUG request="{}")"sv, request);
    auto callback = [this, user_id = message_info.source, order_id = create_order.order_id]([[maybe_unused]] auto &request_id, auto &response) {
      auto version = 1;
      TraceInfo trace_info;
      Trace event{trace_info, response};
      create_order_ack(event, user_id, order_id, version);
    };
    (*connection_)(request_id, request, callback);
  });
}

void OrderEntryREST::create_order_ack(Trace<web::rest::Response> const &event, uint8_t user_id, uint64_t order_id, uint32_t version) {
  profile_.create_order_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(DEBUG origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      auto response = server::oms::Response{
          .request_type = RequestType::CREATE_ORDER,
          .origin = origin,
          .request_status = status,
          .error = error,
          .text = text,
          .version = version,
          .request_id = {},
          .external_order_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      log::warn("response={}, user_id={}, order_id={}"sv, response, user_id, order_id);
      Trace event_2{event, response};
      (*this)(event_2, user_id, order_id);
    };
    auto handle_success = [&](auto &body) {
      json::AddOrderAck add_order_ack{body, decode_buffer_};
      if (add_order_ack.code == SYSTEM_CODE_SUCCESS) {
        Trace event_2{event, add_order_ack};
        (*this)(event_2, user_id, order_id, version);
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(add_order_ack.code), add_order_ack.msg);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::AddOrderAck> const &event, uint8_t user_id, uint64_t order_id, uint32_t version) {
  auto &[trace_info, add_order_ack] = event;
  log::info<2>("add_order_ack={}, user_id={}, order_id={}, version={}"sv, add_order_ack, user_id, order_id, version);
  // XXX FIXME TODO we only have the order-id, then what?
}

// cancel_order

void OrderEntryREST::cancel_order(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  profile_.cancel_order([&]() {
    if (!ready()) {
      throw server::oms::NotReady{"not ready"sv};
    }
    auto &[message_info, cancel_order] = event;
    auto method = web::http::Method::POST;
    auto path = shared_.api.rest_private.order_cancel;
    // auto query = "?tradeType=FUTURES"sv;
    std::string_view query;
    auto body = json::Encoder::cancel_order(encode_buffer_, cancel_order, order, ref_data, request_id, previous_request_id);
    auto headers = account_.create_headers(method, path, query, body);
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = body,
        .quality_of_service = io::QualityOfService::IMMEDIATE,
    };
    log::warn(R"(DEBUG request="{}")"sv, request);
    auto callback = [this, user_id = message_info.source, order_id = cancel_order.order_id, version = cancel_order.version](
                        [[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      cancel_order_ack(event, user_id, order_id, version);
    };
    (*connection_)(request_id, request, callback);
  });
}

void OrderEntryREST::cancel_order_ack(Trace<web::rest::Response> const &event, uint8_t user_id, uint64_t order_id, uint32_t version) {
  profile_.cancel_order_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(DEBUG origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      auto response = server::oms::Response{
          .request_type = RequestType::CANCEL_ORDER,
          .origin = origin,
          .request_status = status,
          .error = error,
          .text = text,
          .version = version,
          .request_id = {},
          .external_order_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      log::warn("response={}, user_id={}, order_id={}"sv, response, user_id, order_id);
      Trace event_2{event, response};
      (*this)(event_2, user_id, order_id);
    };
    auto handle_success = [&](auto &body) {
      json::CancelOrderAck cancel_order_ack{body, decode_buffer_};
      if (cancel_order_ack.code == SYSTEM_CODE_SUCCESS) {
        if (std::empty(cancel_order_ack.data.cancelled_order_ids)) {
          auto error = Error::UNKNOWN_ORDER_ID;
          auto msg = std::empty(cancel_order_ack.msg) ? magic_enum::enum_name(error) : cancel_order_ack.msg;
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN_ORDER_ID, msg);
        } else {
          Trace event_2{event, cancel_order_ack};
          (*this)(event_2, user_id, order_id, version);
        }
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(cancel_order_ack.code), cancel_order_ack.msg);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::CancelOrderAck> const &event, uint8_t user_id, uint64_t order_id, uint32_t version) {
  auto &[trace_info, cancel_order_ack] = event;
  log::info<2>("cancel_order_ack={}, user_id={}, order_id={}, version={}"sv, cancel_order_ack, user_id, order_id, version);
}

// cancel_all_orders

void OrderEntryREST::cancel_all_orders(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  profile_.cancel_all_orders([&]() {
    if (!ready()) [[unlikely]] {
      throw server::oms::NotReady{"not ready"sv};
    }
    auto &cancel_all_orders = event.value;
    auto send_ack = [&](auto &symbol) {
      auto cancel_all_orders_ack = CancelAllOrdersAck{
          .stream_id = stream_id_,
          .account = account_.name,
          .order_id = cancel_all_orders.order_id,
          .exchange = cancel_all_orders.exchange,
          .symbol = symbol,
          .side = cancel_all_orders.side,
          .origin = Origin::GATEWAY,
          .request_status = RequestStatus::FORWARDED,
          .error = {},
          .text = {},
          .request_id = request_id,
          .external_account = {},
          .number_of_affected_orders = {},
          .round_trip_latency = {},
          .user = {},
          .strategy_id = cancel_all_orders.strategy_id,
      };
      TraceInfo trace_info{event};
      Trace event_2{trace_info, cancel_all_orders_ack};
      shared_(event_2);
    };
    //
    if (shared_.dispatcher_.get_all_order_symbols(
            [&](auto &symbol) {
              if (!std::empty(cancel_all_orders.symbol) && symbol != cancel_all_orders.symbol) {
                return;
              }
              auto method = web::http::Method::POST;
              auto path = shared_.api.rest_private.order_cancel_all;
              auto body = json::Encoder::cancel_all_orders(encode_buffer_, event, request_id, symbol);
              auto headers = account_.create_headers(method, path, {}, body);
              auto request = web::rest::Request{
                  .method = method,
                  .path = path,
                  .query = {},
                  .accept = web::http::Accept::APPLICATION_JSON,
                  .content_type = {},
                  .headers = headers,
                  .body = body,
                  .quality_of_service = io::QualityOfService::IMMEDIATE,
              };
              log::warn("DEBUG request={}"sv, request);
              auto callback = [this](auto &request_id, auto &response) {
                TraceInfo trace_info;
                Trace event{trace_info, response};
                cancel_all_orders_ack(event, request_id);
              };
              (*connection_)(request_id, request, callback);
              send_ack(symbol);
            },
            account_.name)) {
    } else {
      log::warn("*** NOT POSSIBLE TO CANCEL ALL OPEN ORDERS (NO SYMBOLS) ***"sv);
    }
  });
}

void OrderEntryREST::cancel_all_orders_ack(Trace<web::rest::Response> const &event, std::string_view const &request_id) {
  profile_.cancel_all_orders_ack([&]() {
    auto send_ack = [&](auto origin, auto status, Error error, std::string_view const &text) {
      auto cancel_all_orders_ack = CancelAllOrdersAck{
          .stream_id = stream_id_,
          .account = account_.name,
          .order_id = {},
          .exchange = {},
          .symbol = {},
          .side = {},
          .origin = origin,
          .request_status = status,
          .error = error,
          .text = text,
          .request_id = request_id,
          .external_account = {},
          .number_of_affected_orders = {},
          .round_trip_latency = {},
          .user = {},
          .strategy_id = {},
      };
      Trace event_2{event, cancel_all_orders_ack};
      shared_(event_2);
    };
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(DEBUG origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      send_ack(origin, status, error, text);
    };
    auto handle_success = [&](auto &body) {
      json::CancelAllOrdersAck cancel_all_orders_ack{body, decode_buffer_};
      if (cancel_all_orders_ack.code == SYSTEM_CODE_SUCCESS) {
        Trace event_2{event, cancel_all_orders_ack};
        (*this)(event_2);
      } else {
        handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(cancel_all_orders_ack.code), cancel_all_orders_ack.msg);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::CancelAllOrdersAck> const &event) {
  auto &[trace_info, cancel_all_orders_ack] = event;
  log::info<2>("cancel_all_orders_ack={}"sv, cancel_all_orders_ack);
}

// order-book

// XXX FIXME TODO this needs signing
void OrderEntryREST::get_order_book(std::string_view const &symbol) {
  profile_.order_book([&]() {
    auto method = web::http::Method::GET;
    auto path = shared_.api.rest_public.market_orderbook;
    auto query = fmt::format("?tradeType=FUTURES&symbol={}&limit=FULL"sv, symbol);
    auto headers = account_.create_headers(method, path, query, {});
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, symbol = std::string{symbol}]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_order_book_ack(event, symbol);
    };
    (*connection_)("order_book"sv, request, callback);
  });
}

// XXX TODO FIXME we need the symbol for retrying !!!
void OrderEntryREST::get_order_book_ack(Trace<web::rest::Response> const &event, [[maybe_unused]] std::string_view const &symbol) {
  profile_.order_book_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      // XXX WHAT ???
    };
    auto handle_success = [&](auto &body) {
      json::OrderBookAck order_book_ack{body, decode_buffer_};
      if (order_book_ack.code == SYSTEM_CODE_SUCCESS) {
        Trace event_2{event, order_book_ack};
        (*this)(event_2);
      };
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntryREST::operator()(Trace<json::OrderBookAck> const &event) {
  auto &[trace_info, order_book_ack] = event;
  log::info<4>("order_book_ack={}"sv, order_book_ack);
  auto &data = order_book_ack.data;
  auto sequence = data.sequence;
  auto symbol = data.symbol;
  auto &sequencer = shared_.mbp_sequencer[symbol];
  auto &mbp = shared_.get_mbp();
  auto emplace_back = [](auto &result, auto &item) {
    auto mbp_update = MBPUpdate{
        .price = item.price,
        .quantity = item.size,
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
    auto publish_snapshot = [&](auto &bids, auto &asks, auto sequence, [[maybe_unused]] auto retries, [[maybe_unused]] auto delay) {
      log::info(R"(DEBUG PUBLISH SNAPSHOT symbol="{}", sequence={})"sv, symbol, sequence);
      auto market_by_price_update = MarketByPriceUpdate{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = symbol,
          .bids = bids,
          .asks = asks,
          .update_type = UpdateType::SNAPSHOT,
          .exchange_time_utc = data.ts,
          .exchange_sequence = sequencer.last_sequence(),
          .sending_time_utc = data.ts,
          .price_precision = {},
          .quantity_precision = {},
          .max_depth = {},
          .checksum = {},
      };
      Trace event{trace_info, market_by_price_update};
      shared_(event, true, [&](auto &market_by_price) { sequencer.apply(market_by_price, sequence, false); });
    };
    auto request_snapshot = [&](auto retries) {
      log::info(R"(DEBUG REQUEST symbol="{}" (retries={}))"sv, symbol, retries);
      if (shared_.settings.ws.mbp_request_max_retries && shared_.settings.ws.mbp_request_max_retries < retries) {
        log::fatal(R"(Unexpected: symbol="{}", retries={})"sv, symbol, retries);
      }
      shared_.depth_request_queue.emplace_back(symbol);
    };
    sequencer(mbp.bids, mbp.asks, sequence, false, publish_snapshot, request_snapshot);
  } catch (BadState &) {
    log::warn(R"(RESUBSCRIBE symbol="{}")"sv, symbol);
    // XXX HANS publish stale
    sequencer.clear();
    shared_.depth_request_queue.emplace_back(symbol);
  }
}

// helpers

void OrderEntryREST::check_request_queue(std::chrono::nanoseconds now) {
  shared_.depth_request_queue.dispatch(
      [&](auto now) { return shared_.rate_limiter.can_request(now); },
      [&](auto &symbol) {
        log::info(R"(DEBUG Requesting order book snapshot symbol="{}")"sv, symbol);
        get_order_book(symbol);
      },
      now);
}

void OrderEntryREST::process_response(web::rest::Response const &response, auto error_handler, auto success_handler) {
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
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
        success_handler(body);
        break;
      case SERVER_ERROR: {
        auto message = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, message);
        break;
      }
    }
  } catch (server::oms::Exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.origin, e.status, e.error, e.what());
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

template <typename... Args>
void OrderEntryREST::operator()(Trace<server::oms::Response> const &event, uint8_t user_id, uint64_t order_id, Args &&...args) {
  auto &[trace_info, response] = event;
  if (shared_.update_order(user_id, order_id, stream_id_, trace_info, response, std::forward<Args>(args)..., []([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("Did not find order: user_id={}, order_id={}"sv, user_id, order_id);
  }
}

void OrderEntryREST::operator()(Trace<server::oms::OrderUpdate> const &event, std::string_view const &client_order_id) {
  auto &[trace_info, order_update] = event;
  if (shared_.update_order(client_order_id, stream_id_, trace_info, order_update, [&]([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
  }
}

}  // namespace kucoin_pro
}  // namespace roq
