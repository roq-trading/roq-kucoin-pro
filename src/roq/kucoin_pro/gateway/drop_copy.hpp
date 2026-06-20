/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/kucoin_pro/gateway/account.hpp"
#include "roq/kucoin_pro/gateway/private_token.hpp"
#include "roq/kucoin_pro/gateway/request.hpp"
#include "roq/kucoin_pro/gateway/shared.hpp"

#include "roq/kucoin_pro/protocol/json/parser.hpp"

namespace roq {
namespace kucoin_pro {
namespace gateway {

struct DropCopy final : public web::socket::Client::Handler, public protocol::json::Parser::Handler {
  struct Handler {};

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, Request &, std::string_view const &query);

  DropCopy(DropCopy const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  // cross-communication

  void operator()(PrivateToken const &);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;
  //
  std::string_view get_query() const override { return query_; }

  // helpers

  bool ready() const;

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  enum class State {
    UNDEFINED = 0,
    SUBSCRIBE,
    DONE,
  };

  uint32_t download(State);

  void subscribe();

  void subscribe_account(std::string_view const &channel);
  void subscribe_trade(std::string_view const &channel);

  void send_ping(std::chrono::nanoseconds now);

  void parse(std::string_view const &message);

  // protocol::json::Parser::Handler

  void operator()(Trace<protocol::json::Welcome> const &) override;
  void operator()(Trace<protocol::json::Error> const &) override;
  void operator()(Trace<protocol::json::Pong> const &) override;
  void operator()(Trace<protocol::json::Ack> const &) override;

  void operator()(Trace<protocol::json::Ticker> const &) override;
  void operator()(Trace<protocol::json::Trade> const &) override;
  void operator()(Trace<protocol::json::OBU> const &) override;

  void operator()(Trace<protocol::json::Balance> const &) override;
  void operator()(Trace<protocol::json::PositionAll> const &) override;
  void operator()(Trace<protocol::json::OrderAll> const &) override;

  // helpers

  void check_response_private_token();

  void request_private_token();

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::string query_;
  std::unique_ptr<web::socket::Client> const connection_;
  std::chrono::nanoseconds const ping_frequency_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse,  //
        welcome, error, pong, ack, balance, position_all, order_all;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  // account
  Account &account_;
  Shared &shared_;
  Request &request_;
  // state
  bool welcome_ = false;
  bool ready_ = false;
  ConnectionStatus connection_status_ = {};
  core::Download<State> download_;
  std::chrono::nanoseconds logon_timeout_ = {};
  std::chrono::nanoseconds next_ping_ = {};
  //
  bool download_private_token_ = false;
  //
  std::chrono::nanoseconds next_simulated_disconnect_ = {};
};

}  // namespace gateway
}  // namespace kucoin_pro
}  // namespace roq
