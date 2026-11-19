/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/gateway/controller.hpp"

#include "roq/logging.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/kucoin_pro/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace gateway {

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto const &config) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    auto obj = std::make_unique<Account>(config, account.name);
    result.try_emplace(account.name, std::move(obj));
  }
  return result;
}

template <typename R>
R create_requests(auto &config, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    auto obj = std::make_unique<Request>(shared);
    result.try_emplace(account.name, std::move(obj));
  }
  return result;
}

Request &get_request(auto &requests, auto &account) {
  auto iter = requests.find(account);
  if (iter == std::end(requests)) {
    log::fatal("Unexpected"sv);
  }
  return *(*iter).second;
}

template <typename R>
R create_order_entry_rest(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared, auto &requests) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, item] : accounts) {
    auto &account = *item;
    auto &request = get_request(requests, account.name);
    auto obj = std::make_unique<OrderEntryREST>(gateway, context, ++stream_id, account, shared, request);
    result.try_emplace(account.name, std::move(obj));
  }
  return result;
}

template <typename R>
R create_order_entry_ws(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  if (!shared.settings.misc.disable_ws_api && !std::empty(shared.settings.ws.uri)) {
    for (auto &[_, item] : accounts) {
      auto &account = *item;
      auto obj = std::make_unique<OrderEntryWS>(gateway, context, ++stream_id, account, shared);
      result.try_emplace(account.name, std::move(obj));
    }
  }
  return result;
}

template <typename R>
R create_drop_copy(auto &accounts) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, item] : accounts) {
    auto &account = *item;
    result.try_emplace(account.name, nullptr);
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<server::Handler> Controller::create(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context) {
  return std::make_unique<Controller>(dispatcher, settings, config, context);
}

uint8_t Controller::parse_api(Settings const &) {
  return {};
}

Controller::Controller(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, context_{context}, shared_{dispatcher, settings}, accounts_{create_accounts<decltype(accounts_)>(config)},
      requests_{create_requests<decltype(requests_)>(config, shared_)}, rest_{*this, context_, ++stream_id_, shared_},
      order_entry_rest_{create_order_entry_rest<decltype(order_entry_rest_)>(*this, context_, stream_id_, accounts_, shared_, requests_)},
      order_entry_ws_{create_order_entry_ws<decltype(order_entry_ws_)>(*this, context_, stream_id_, accounts_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(accounts_)} {
}

// server::Handler

void Controller::operator()(Trace<Start> const &event) {
  log::info("Starting..."sv);
  assert(std::empty(market_data_));
  dispatch(event);
}

void Controller::operator()(Trace<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Controller::operator()(Trace<Timer> const &event) {
  dispatch(event);
}

void Controller::operator()(Trace<Connected> const &, [[maybe_unused]] uint8_t user_id) {
}

void Controller::operator()(Trace<Disconnected> const &event, [[maybe_unused]] uint8_t user_id) {
  auto &[message_info, disconnected] = event;
  if (disconnected.order_cancel_policy != OrderCancelPolicy{}) {
    log::warn("** CANCEL-ON-DISCONNECT *NOT* SUPPORTED ***"sv);
  }
}

void Controller::operator()(Trace<Subscribe> const &event) {
  auto &[trace_info, subscribe] = event;
  std::vector<Symbol> symbols;
  for (auto &item : subscribe.symbols) {
    if (shared_.all_symbols.emplace(item).second) {
      symbols.emplace_back(item);
    } else {
      log::warn(R"(*** DUPLICATE SUBSCRIPTION *** (symbol="{}")"sv, item);
    }
  }
  auto symbols_update = SymbolsUpdate{
      .symbols = symbols,
  };
  create_trace_and_dispatch_2(trace_info, symbols_update);
}

void Controller::operator()(Trace<Control> const &event) {
  auto &[trace_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Controller::operator()(Trace<server::oms::OrderActionBatchBegin> const &event) {
  dispatch_order_action<void>(event);
}

void Controller::operator()(Trace<server::oms::OrderActionBatchCommit> const &event) {
  dispatch_order_action<void>(event);
}

void Controller::operator()(Trace<server::oms::OrderActionBatchRollback> const &event) {
  dispatch_order_action<void>(event);
}

uint16_t Controller::operator()(
    Trace<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  return dispatch_order_action<uint16_t>(event, order, ref_data, request_id);
}

uint16_t Controller::operator()(
    Trace<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  return dispatch_order_action<uint16_t>(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Controller::operator()(
    Trace<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  return dispatch_order_action<uint16_t>(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Controller::operator()(Trace<CancelAllOrders> const &event, std::string_view const &request_id) {
  return dispatch_order_action<uint16_t>(event, request_id);
}

uint16_t Controller::operator()(Trace<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Trace<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
}

// Rest::Handler

void Controller::operator()(Trace<SymbolsUpdate> const &event) {
  auto &[trace_info, symbols_update] = event;
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(trace_info, size);
  for (auto &iter : market_data_) {
    (*iter).subscribe(start_from);
  }
}

// OrderEntry::Handler / DropCopy::Handler

void Controller::operator()(Trace<PrivateToken> const &event) {
  auto &[trace_info, private_token] = event;
  auto account = private_token.account;
  auto &drop_copy = drop_copy_[account];
  if (!drop_copy) {
    auto &account_2 = *accounts_.at(account);
    auto &request = get_request(requests_, account);
    auto obj = std::make_unique<DropCopy>(*this, context_, ++stream_id_, account_2, shared_, request, private_token.query);
    Start start;
    create_trace_and_dispatch(*obj, trace_info, start);
    drop_copy = std::move(obj);
  } else {
    (*drop_copy)(event);
  }
}

// utilities

void Controller::ensure_symbol_slices(TraceInfo const &trace_info, size_t size) {
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::debug("Create MarketData (stream_id={}, index={})"sv, stream_id, index);
    auto obj = std::make_unique<MarketData>(*this, context_, stream_id, shared_, index);
    Start start;
    create_trace_and_dispatch(*obj, trace_info, start);
    market_data_.emplace_back(std::move(obj));
  }
}

template <typename... Args>
void Controller::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Controller::dispatch_helper(auto &self, Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  helper(self.rest_);
  for (auto &[_, item] : self.order_entry_rest_) {
    helper(*item);
  }
  for (auto &[_, item] : self.order_entry_ws_) {
    helper(*item);
  }
  for (auto &[_, item] : self.drop_copy_) {
    if (static_cast<bool>(item)) {
      helper(*item);
    }
  }
  for (auto &item : self.market_data_) {
    helper(*item);
  }
}

template <typename Res, typename T, typename... Args>
Res Controller::dispatch_order_action(Trace<T> const &event, Args &&...args) {
  auto &[trace_info, value] = event;
  if (shared_.settings.misc.disable_ws_api || std::is_same_v<T, CancelAllOrders>) {
    auto iter = order_entry_rest_.find(value.account);
    if (iter == std::end(order_entry_rest_)) [[unlikely]] {
      throw RuntimeError{R"(Unknown account="{}")"sv, value.account};
    }
    return (*(*iter).second)(event, args...);
  } else {
    auto iter = order_entry_ws_.find(value.account);
    if (iter == std::end(order_entry_ws_)) [[unlikely]] {
      throw RuntimeError{R"(Unknown account="{}")"sv, value.account};
    }
    return (*(*iter).second)(event, args...);
  }
}

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
