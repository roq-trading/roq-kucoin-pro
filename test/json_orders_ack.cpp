/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/orders_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::OrdersAck;

TEST_CASE("empty", "[json_orders_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("pageNumber":1,)"
                       R"("pageSize":50,)"
                       R"("totalNum":0,)"
                       R"("totalPage":0,)"
                       R"("items":[],)"
                       R"("tradeType":"FUTURES")"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.page_number == 1);
    CHECK(data.page_size == 50);
    CHECK(data.total_num == 0);
    CHECK(data.total_page == 0);
    REQUIRE(std::empty(data.items));
    CHECK(data.trade_type == json::TradeType::FUTURES);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("simple", "[json_orders_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("pageNumber":1,)"
                       R"("pageSize":50,)"
                       R"("totalNum":1,)"
                       R"("totalPage":1,)"
                       R"("items":[{)"
                       R"("orderId":"414899660931538944",)"
                       R"("symbol":"XBTUSDTM",)"
                       R"("orderType":"LIMIT",)"
                       R"("side":"BUY",)"
                       R"("size":"1",)"
                       R"("price":"50000",)"
                       R"("timeInForce":"GTC",)"
                       R"("tags":"",)"
                       R"("orderTime":1771732989629264801,)"
                       R"("stp":"",)"
                       R"("cancelAfter":null,)"
                       R"("postOnly":false,)"
                       R"("reduceOnly":false,)"
                       R"("triggerDirection":"",)"
                       R"("triggerPrice":"",)"
                       R"("triggerPriceType":"",)"
                       R"("tpTriggerPrice":"",)"
                       R"("tpTriggerPriceType":"",)"
                       R"("slTriggerPrice":"",)"
                       R"("slTriggerPriceType":"",)"
                       R"("filledSize":"0",)"
                       R"("avgPrice":"0",)"
                       R"("fee":"0",)"
                       R"("feeCurrency":"USDT",)"
                       R"("tax":"0",)"
                       R"("updatedTime":1771732989667873233,)"
                       R"("triggerOrderId":"",)"
                       R"("cancelReason":"",)"
                       R"("cancelSize":"0",)"
                       R"("clientOid":"UgACRG2H61MAAQAAAAAA",)"
                       R"("sizeUnit":"UNIT",)"
                       R"("status":2)"
                       R"(})"
                       R"(],)"
                       R"("tradeType":"FUTURES")"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.page_number == 1);
    CHECK(data.page_size == 50);
    CHECK(data.total_num == 1);
    CHECK(data.total_page == 1);
    REQUIRE(std::size(data.items) == 1);
    auto &i0 = data.items[0];
    CHECK(i0.order_id == "414899660931538944"sv);
    // ...
    CHECK(data.trade_type == json::TradeType::FUTURES);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
