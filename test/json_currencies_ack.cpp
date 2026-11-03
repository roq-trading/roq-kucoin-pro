/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/protocol/json/currencies_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::CurrenciesAck;

// note! truncated
TEST_CASE("simple", "[json_currencies_ack]") {
  auto message = R"({)"
                 R"("code":"200000",)"
                 R"("data":[{)"
                 R"("currency":"CSP",)"
                 R"("name":"CSP",)"
                 R"("fullName":"Caspian",)"
                 R"("precision":8,)"
                 R"("confirms":null,)"
                 R"("contractAddress":null,)"
                 R"("isMarginEnabled":false,)"
                 R"("isDebitEnabled":false,)"
                 R"("list":[{)"
                 R"("chainName":"ERC20",)"
                 R"("withdrawFeeRate":"0",)"
                 R"("isWithdrawEnabled":false,)"
                 R"("isDepositEnabled":false,)"
                 R"("confirms":64,)"
                 R"("preConfirms":12,)"
                 R"("contractAddress":"0xa6446d655a0c34bc4f05042ee88170d056cbaf45",)"
                 R"("depositFeeRate":null,)"
                 R"("depositTierFee":null,)"
                 R"("withdrawPrecision":8,)"
                 R"("chain":"eth",)"
                 R"("addressRegex":"^(0x)[0-9A-Fa-f]{40}$",)"
                 R"("memoRegex":"",)"
                 R"("maxWithdrawFee":null,)"
                 R"("isMemoRequired":false,)"
                 R"("minWithdrawSize":"2999",)"
                 R"("minDepositSize":null,)"
                 R"("maxWithdrawSize":null,)"
                 R"("maxDepositSize":null,)"
                 R"("fixedDepositFee":null,)"
                 R"("maxDepositFee":null,)"
                 R"("minWithdrawFee":"2999")"
                 R"(})"
                 R"(])"
                 R"(},{)"
                 R"("currency":"MHC",)"
                 R"("name":"MHC",)"
                 R"("fullName":"MetaHash",)"
                 R"("precision":4,)"
                 R"("confirms":null,)"
                 R"("contractAddress":null,)"
                 R"("isMarginEnabled":false,)"
                 R"("isDebitEnabled":true,)"
                 R"("list":[{)"
                 R"("chainName":"MHC",)"
                 R"("withdrawFeeRate":"0",)"
                 R"("isWithdrawEnabled":false,)"
                 R"("isDepositEnabled":false,)"
                 R"("confirms":32,)"
                 R"("preConfirms":32,)"
                 R"("contractAddress":"",)"
                 R"("depositFeeRate":null,)"
                 R"("depositTierFee":null,)"
                 R"("withdrawPrecision":4,)"
                 R"("chain":"mhc",)"
                 R"("addressRegex":"",)"
                 R"("memoRegex":"",)"
                 R"("maxWithdrawFee":null,)"
                 R"("isMemoRequired":false,)"
                 R"("minWithdrawSize":"100",)"
                 R"("minDepositSize":null,)"
                 R"("maxWithdrawSize":null,)"
                 R"("maxDepositSize":null,)"
                 R"("fixedDepositFee":null,)"
                 R"("maxDepositFee":null,)"
                 R"("minWithdrawFee":"50")"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.code == 200000);
    // REQUIRE(std::size(obj.data) == 3);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
