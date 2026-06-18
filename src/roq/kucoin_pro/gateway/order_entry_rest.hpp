/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/kucoin_pro/gateway/account.hpp"
#include "roq/kucoin_pro/gateway/order_entry.hpp"
#include "roq/kucoin_pro/gateway/private_token.hpp"
#include "roq/kucoin_pro/gateway/request.hpp"
#include "roq/kucoin_pro/gateway/shared.hpp"

#include "roq/kucoin_pro/protocol/json/token.hpp"

#include "roq/kucoin_pro/protocol/json/account_ack.hpp"
#include "roq/kucoin_pro/protocol/json/execution_ack.hpp"
#include "roq/kucoin_pro/protocol/json/orders_ack.hpp"
#include "roq/kucoin_pro/protocol/json/position_ack.hpp"

#include "roq/kucoin_pro/protocol/json/add_order_ack.hpp"
#include "roq/kucoin_pro/protocol/json/cancel_all_orders_ack.hpp"
#include "roq/kucoin_pro/protocol/json/cancel_order_ack.hpp"

#include "roq/kucoin_pro/protocol/json/order_book_ack.hpp"

namespace roq {
namespace kucoin_pro {
namespace gateway {

struct OrderEntryREST final : public OrderEntry, public web::rest::Client::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<TradeUpdate> const &, bool is_last, uint8_t user_id) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(PrivateToken const &) = 0;
  };

  OrderEntryREST(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, Request &);

  OrderEntryREST(OrderEntryREST const &) = delete;

  bool ready() const override { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

 protected:
  // web::rest::Client::Handler

  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  enum class State {
    UNDEFINED = 0,
    PRIVATE_TOKEN,
    ACCOUNT,
    POSITION,
    ORDERS,
    EXECUTION,
    DONE,
  };

  uint32_t download(State);

  // private-token

  void get_private_token();
  void get_private_token_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::Token> const &);

  // account

  void get_account();
  void get_account_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::AccountAck> const &);

  // position

  void get_position();
  void get_position_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::PositionAck> const &);

  // orders

  void get_orders();
  void get_orders_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::OrdersAck> const &);

  // execution

  void get_execution();
  void get_execution_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::ExecutionAck> const &);

  // add-order

  void create_order(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  void create_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<protocol::json::AddOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-order

  void cancel_order(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  void cancel_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<protocol::json::CancelOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-all-orders

  void cancel_all_orders(Event<CancelAllOrders> const &, std::string_view const &request_id);
  void cancel_all_orders_ack(Trace<web::rest::Response> const &, std::string_view const &request_id);
  void operator()(Trace<protocol::json::CancelAllOrdersAck> const &);

  // order-book

  void get_order_book(std::string_view const &symbol);
  void get_order_book_ack(Trace<web::rest::Response> const &, std::string_view const &symbol);
  void operator()(Trace<protocol::json::OrderBookAck> const &);

  // helpers

  bool downloading() const { return download_private_token_; }

  void check_request_queue(std::chrono::nanoseconds now);

  void process_response(web::rest::Response const &, auto error_handler, auto success_handler);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  bool const master_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile private_token, private_token_ack,  //
        account, account_ack,                                  //
        position, position_ack,                                //
        orders, orders_ack,                                    //
        execution, execution_ack,                              //
        create_order, create_order_ack,                        //
        cancel_order, cancel_order_ack,                        //
        cancel_all_orders, cancel_all_orders_ack,              //
        order_book, order_book_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  Request &request_;
  // state
  ConnectionStatus connection_status_ = {};
  core::Download<State> download_;
  //
  std::string encode_buffer_;
  //
  bool download_private_token_ = false;
  bool has_downloaded_private_token_ = false;
};

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
