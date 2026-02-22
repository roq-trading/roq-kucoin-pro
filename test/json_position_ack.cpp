/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/position_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::PositionAck;

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
