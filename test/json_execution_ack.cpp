/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/execution_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::ExecutionAck;

TEST_CASE("empty", "[json_execution_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("tradeType":"FUTURES",)"
                       R"("lastId":null,)"
                       R"("items":[])"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.trade_type == json::TradeType::FUTURES);
    // CHECK(data.last_id ==
    REQUIRE(std::empty(data.items));
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("simple", "[json_execution_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("tradeType":"FUTURES",)"
                       R"("lastId":30000000000459208,)"
                       R"("items":[{)"
                       R"("orderId":"415020867832070144",)"
                       R"("symbol":"XBTUSDTM",)"
                       R"("orderType":"LIMIT",)"
                       R"("side":"BUY",)"
                       R"("tradeId":"1918560222926",)"
                       R"("size":"1",)"
                       R"("price":"68130",)"
                       R"("value":"68.13",)"
                       R"("executionTime":1771761913781000000,)"
                       R"("fee":"-0.0047691",)"
                       R"("feeCurrency":"USDT",)"
                       R"("tax":"",)"
                       R"("liquidityRole":"MAKER",)"
                       R"("fillType":"NORMAL")"
                       R"(})"
                       R"(])"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.trade_type == json::TradeType::FUTURES);
    // CHECK(data.last_id ==
    REQUIRE(std::size(data.items) == 1);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
