/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::Ticker;

TEST_CASE("simple", "[json_ticker]") {
  auto message = R"({)"
                 R"("T":"ticker.FUTURES",)"
                 R"("P":1771594732074799474,)"
                 R"("d":{)"
                 R"("a":"1927.87",)"
                 R"("A":"4",)"
                 R"("q":"2",)"
                 R"("b":"1927.86",)"
                 R"("B":"810",)"
                 R"("s":"ETHUSDTM",)"
                 R"("S":"buy",)"
                 R"("E":1814374362936,)"
                 R"("l":"1927.87",)"
                 R"("M":1771594732074000000)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::TICKER_FUTURES);
    CHECK(obj.push_time == 1771594732074799474ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
