/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "ws_parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::WSAddOrderAck;

TEST_CASE("success", "[json_ws_add_order_ack]") {
  auto message = R"({)"
                 R"("id":"4wACJpW38FMAAQAAAAAA",)"
                 R"("op":"uta.order",)"
                 R"("code":"200000",)"
                 R"("data":{)"
                 R"("orderId":"414936156380143616",)"
                 R"("tradeType":"FUTURES",)"
                 R"("ts":1771741690837000000,)"
                 R"("clientOid":"4wACJpW38FMAAQAAAAAA")"
                 R"(},)"
                 R"("inTime":1771741690821,)"
                 R"("outTime":1771741690838)"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "4wACJpW38FMAAQAAAAAA"sv);
    CHECK(obj.op == protocol::json::WSOp::ADD_ORDER_ACK);
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.order_id == "414936156380143616"sv);
    CHECK(data.trade_type == protocol::json::TradeType::FUTURES);
    CHECK(data.ts == 1771741690837000000ns);
    CHECK(data.client_oid == "4wACJpW38FMAAQAAAAAA"sv);
  };
  WSParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
