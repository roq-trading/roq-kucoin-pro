/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::Balance;

TEST_CASE("simple", "[json_balance]") {
  auto message = R"({)"
                 R"("T":"balance.UNIFIED",)"
                 R"("P":1771732989773425836,)"
                 R"("d":{)"
                 R"("c":"USDT",)"
                 R"("e":"500.0000000000",)"
                 R"("b":"500.0000000000",)"
                 R"("a":"499.9700000000",)"
                 R"("h":"0.0300000000",)"
                 R"("U":1771504095649000000,)"
                 R"("l":"0.0000000000")"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::BALANCE_UNIFIED);
    CHECK(obj.push_time == 1771732989773425836ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
