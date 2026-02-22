/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::OrderAll;

TEST_CASE("create", "[json_order_all]") {
  auto message = R"({)"
                 R"("T":"orderAll.UNIFIED",)"
                 R"("P":1771742294040092777,)"
                 R"("d":{)"
                 R"("tT":"FUTURES",)"
                 R"("oi":"414938686455627776",)"
                 R"("ci":"mQAC54sT8VMAAQAAAAAA",)"
                 R"("os":2,)"
                 R"("eT":"OPEN",)"
                 R"("s":"XBTUSDTM",)"
                 R"("S":"BUY",)"
                 R"("oT":"LIMIT",)"
                 R"("lR":"",)"
                 R"("oS":"USER",)"
                 R"("p":"50000",)"
                 R"("ti":"",)"
                 R"("q":"1",)"
                 R"("qU":"UNIT",)"
                 R"("fS":"0",)"
                 R"("lS":"0",)"
                 R"("ls":"0",)"
                 R"("aP":"0",)"
                 R"("f":"0",)"
                 R"("fC":"USDT",)"
                 R"("t":"0",)"
                 R"("cR":"",)"
                 R"("cS":"0",)"
                 R"("rS":"1",)"
                 R"("tD":"",)"
                 R"("tP":"",)"
                 R"("tPT":"",)"
                 R"("pP":"",)"
                 R"("pPT":"",)"
                 R"("lP":"",)"
                 R"("lPT":"",)"
                 R"("toi":"",)"
                 R"("stp":"",)"
                 R"("rO":false,)"
                 R"("tIF":"GTC",)"
                 R"("pO":false,)"
                 R"("O":1771742294039455063,)"
                 R"("U":1771742294039715653)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::ORDER_ALL_UNIFIED);
    CHECK(obj.push_time == 1771742294040092777ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("cancel", "[json_order_all]") {
  auto message = R"({)"
                 R"("T":"orderAll.UNIFIED",)"
                 R"("P":1771757876485897853,)"
                 R"("d":{)"
                 R"("tT":"FUTURES",)"
                 R"("oi":"414938686455627776",)"
                 R"("ci":"mQAC54sT8VMAAQAAAAAA",)"
                 R"("os":5,)"
                 R"("eT":"CANCEL",)"
                 R"("s":"XBTUSDTM",)"
                 R"("S":"BUY",)"
                 R"("oT":"LIMIT",)"
                 R"("lR":"",)"
                 R"("oS":"USER",)"
                 R"("p":"50000",)"
                 R"("ti":"",)"
                 R"("q":"1",)"
                 R"("qU":"UNIT",)"
                 R"("fS":"0",)"
                 R"("lS":"0",)"
                 R"("ls":"0",)"
                 R"("aP":"0",)"
                 R"("f":"0",)"
                 R"("fC":"USDT",)"
                 R"("t":"0",)"
                 R"("cR":"USER",)"
                 R"("cS":"1",)"
                 R"("rS":"0",)"
                 R"("tD":"",)"
                 R"("tP":"",)"
                 R"("tPT":"",)"
                 R"("pP":"",)"
                 R"("pPT":"",)"
                 R"("lP":"",)"
                 R"("lPT":"",)"
                 R"("toi":"",)"
                 R"("stp":"",)"
                 R"("rO":false,)"
                 R"("tIF":"GTC",)"
                 R"("pO":false,)"
                 R"("O":1771742294039455063,)"
                 R"("U":1771757876483000000)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::ORDER_ALL_UNIFIED);
    CHECK(obj.push_time == 1771757876485897853ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
