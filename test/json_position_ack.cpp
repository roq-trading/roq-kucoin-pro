/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/protocol/json/position_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::PositionAck;

TEST_CASE("empty", "[json_position_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":[])"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    REQUIRE(std::empty(data));
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("simple", "[json_position_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":[{)"
                       R"("symbol":"XBTUSDTM",)"
                       R"("id":"30000000000072788",)"
                       R"("marginMode":"CROSS",)"
                       R"("size":"1",)"
                       R"("entryPrice":"68130",)"
                       R"("positionValue":"68.06725",)"
                       R"("markPrice":"68067.25",)"
                       R"("leverage":"3",)"
                       R"("unrealizedPnL":"-0.06275",)"
                       R"("realizedPnL":"0.0047691",)"
                       R"("initialMargin":"22.689083331064425",)"
                       R"("mmr":"0.004",)"
                       R"("maintenanceMargin":"0.272269",)"
                       R"("creationTime":1771761913781000000)"
                       R"(})"
                       R"(])"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    REQUIRE(std::size(data) == 1);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
