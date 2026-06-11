/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::PositionAll;

TEST_CASE("simple", "[json_position_all]") {
  auto message = R"({)"
                 R"("T":"positionAll.UNIFIED",)"
                 R"("P":1771761913787998066,)"
                 R"("d":{)"
                 R"("pi":"30000000000072788",)"
                 R"("s":"XBTUSDTM",)"
                 R"("mM":"CROSS",)"
                 R"("q":"1",)"
                 R"("eP":"68130",)"
                 R"("pV":"68.1304",)"
                 R"("mP":"68130.4",)"
                 R"("lP":"0.1",)"
                 R"("bP":"0.1",)"
                 R"("l":"3",)"
                 R"("uPL":"0.0004",)"
                 R"("rPL":"0.0047691",)"
                 R"("iM":"22.71013333106232",)"
                 R"("mmr":"0.004",)"
                 R"("mtM":"0.2725216",)"
                 R"("U":1771761913781000000,)"
                 R"("O":1771761913781000000)"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == protocol::json::Channel::POSITION_ALL_UNIFIED);
    CHECK(obj.push_time == 1771761913787998066ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
