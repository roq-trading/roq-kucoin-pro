/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/order_book_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::OrderBookAck;

// note! truncated
TEST_CASE("simple", "[json_order_book_ack]") {
  auto message = R"({)"
                 R"("code":"200000",)"
                 R"("data":{)"
                 R"("tradeType":"FUTURES",)"
                 R"("sequence":1696964929551,)"
                 R"("symbol":"XBTUSDCM",)"
                 R"("bids":[)"
                 R"(["68205.0","1"],)"
                 R"(["68186.9","11"],)"
                 R"(["0.2","3250000"],)"
                 R"(["0.1","15601000"])"
                 R"(],)"
                 R"("asks":[)"
                 R"(["68217.2","11"],)"
                 R"(["68230.9","13"],)"
                 R"(["125681.0","1"],)"
                 R"(["135681.0","1"])"
                 R"(],)"
                 R"("ts":1771674207834000000)"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.trade_type == json::TradeType::FUTURES);
    CHECK(data.sequence == 1696964929551);
    CHECK(data.symbol == "XBTUSDCM"sv);
    auto &bids = data.bids;
    REQUIRE(std::size(bids) == 4);
    auto &asks = data.asks;
    REQUIRE(std::size(asks) == 4);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
