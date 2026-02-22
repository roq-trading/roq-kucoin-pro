/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/account_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::AccountAck;

TEST_CASE("simple", "[json_account_ack]") {
  auto const message = R"({)"
                       R"("code":"200000",)"
                       R"("data":{)"
                       R"("accountType":"UNIFIED",)"
                       R"("ts":1771667355929,)"
                       R"("accounts":[{)"
                       R"("currencies":[{)"
                       R"("currency":"USDT",)"
                       R"("equity":"500.0000000000",)"
                       R"("hold":"0.0000000000",)"
                       R"("balance":"500.0000000000",)"
                       R"("available":"500.0000000000",)"
                       R"("liability":"0.0000000000")"
                       R"(})"
                       R"(])"
                       R"(})"
                       R"(])"
                       R"(})"
                       R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200000);
    auto &data = obj.data;
    CHECK(data.account_type == json::AccountType::UNIFIED);
    CHECK(data.ts == 1771667355929ms);
    REQUIRE(std::size(data.accounts) == 1);
    auto &a0 = data.accounts[0];
    REQUIRE(std::size(a0.currencies) == 1);
    auto &c0 = a0.currencies[0];
    CHECK(c0.currency == "USDT"sv);
    CHECK(c0.equity == 500.0_a);
    CHECK(c0.hold == 0.0_a);
    CHECK(c0.balance == 500.0_a);
    CHECK(c0.available == 500.0_a);
    CHECK(c0.liability == 0.0_a);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
