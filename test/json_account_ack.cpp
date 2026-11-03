/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/protocol/json/account_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::AccountAck;

TEST_CASE("empty", "[json_account_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("accountType":"UNIFIED",)"
                       R"("ts":1793698524549,)"
                       R"("accounts":[{)"
                       R"("currencies":[])"
                       R"(})"
                       R"(])"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.account_type == protocol::json::AccountType::UNIFIED);
    CHECK(data.ts == 1793698524549ms);
    REQUIRE(std::size(data.accounts) == 1);
    auto &a0 = data.accounts[0];
    REQUIRE(std::empty(a0.currencies));
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
