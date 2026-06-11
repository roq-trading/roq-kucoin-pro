/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::Trade;

TEST_CASE("simple", "[json_trade]") {
  auto message = R"({)"
                 R"("T":"trade.FUTURES",)"
                 R"("P":1771596007956145319,)"
                 R"("d":{)"
                 R"("p":"1933.1",)"
                 R"("q":"5",)"
                 R"("s":"ETHUSDTM",)"
                 R"("S":"sell",)"
                 R"("E":1814377247397,)"
                 R"("ti":"1814377247397",)"
                 R"("rpi":false,)"
                 R"("M":1771596007940000000)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == protocol::json::Channel::TRADE_FUTURES);
    CHECK(obj.push_time == 1771596007956145319ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
