/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::OBU;

TEST_CASE("bbo", "[json_obu]") {
  auto message = R"({)"
                 R"("T":"obu.FUTURES",)"
                 R"("dp":"1",)"
                 R"("t":"snapshot",)"
                 R"("P":1771602678625085376,)"
                 R"("d":{)"
                 R"("a":[)"
                 R"(["1952.13","840"])"
                 R"(],)"
                 R"("b":[)"
                 R"(["1952.12","4"])"
                 R"(],)"
                 R"("s":"ETHUSDTM",)"
                 R"("C":1727360837858,)"
                 R"("M":1771602678623000000,)"
                 R"("O":1727360837858)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::OBU_FUTURES);
    CHECK(obj.depth == json::Depth::ONE);
    CHECK(obj.update_type == json::UpdateType::SNAPSHOT);
    CHECK(obj.push_time == 1771602678625085376ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("increment", "[json_obu]") {
  auto message = R"({)"
                 R"("T":"obu.FUTURES",)"
                 R"("dp":"increment",)"
                 R"("t":"delta",)"
                 R"("P":1771600803304460569,)"
                 R"("d":{)"
                 R"("a":[],)"
                 R"("b":[)"
                 R"(["67135.5","1"],)"
                 R"(["67135.5","0"])"
                 R"(],)"
                 R"("C":1736202668968,)"
                 R"("s":"XBTUSDTM",)"
                 R"("M":1771600803257000000,)"
                 R"("O":1736202668967)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == json::Channel::OBU_FUTURES);
    CHECK(obj.depth == json::Depth::INCREMENT);
    CHECK(obj.update_type == json::UpdateType::DELTA);
    CHECK(obj.push_time == 1771600803304460569ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
