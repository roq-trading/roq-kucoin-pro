/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "ws_parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::WSCancelOrderAck;

TEST_CASE("success", "[json_ws_cancel_order_ack]") {
  auto message = R"({)"
                 R"("id":"UQACRG2H61MAAgAAAAAA",)"
                 R"("op":"uta.cancel",)"
                 R"("code":"200000",)"
                 R"("data":{)"
                 R"("orderId":"414899660931538944",)"
                 R"("tradeType":"FUTURES",)"
                 R"("ts":1771738517838000000,)"
                 R"("clientOid":"UgACRG2H61MAAQAAAAAA")"
                 R"(},)"
                 R"("inTime":1771738517837,)"
                 R"("outTime":1771738517838)"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "UQACRG2H61MAAgAAAAAA"sv);
    CHECK(obj.op == protocol::json::WSOp::CANCEL_ORDER_ACK);
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.order_id == "414899660931538944"sv);
    CHECK(data.trade_type == protocol::json::TradeType::FUTURES);
    CHECK(data.ts == 1771738517838000000ns);
    CHECK(data.client_oid == "UgACRG2H61MAAQAAAAAA"sv);
  };
  WSParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
