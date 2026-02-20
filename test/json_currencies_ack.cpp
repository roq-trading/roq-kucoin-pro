/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/json/currencies_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = json::CurrenciesAck;

// note! truncated
TEST_CASE("simple", "[json_currencies_ack]") {
  auto message = R"({)"
                 R"("code":"200000",)"
                 R"("data":[{)"
                 R"("currency":"CSP",)"
                 R"("name":"CSP",)"
                 R"("fullName":"Caspian",)"
                 R"("precision":8,)"
                 R"("isMarginEnabled":false,)"
                 R"("isDebitEnabled":false,)"
                 R"("items":[{)"
                 R"("chainName":"ERC20",)"
                 R"("minWithdrawSize":"2999",)"
                 R"("minDepositSize":null,)"
                 R"("withdrawFeeRate":"0",)"
                 R"("minWithdrawFee":"2999",)"
                 R"("isWithdrawEnabled":false,)"
                 R"("isDepositEnabled":false,)"
                 R"("confirms":96,)"
                 R"("preConfirms":32,)"
                 R"("contractAddress":"0xa6446d655a0c34bc4f05042ee88170d056cbaf45",)"
                 R"("withdrawPrecision":8,)"
                 R"("maxWithdrawSize":null,)"
                 R"("maxDepositSize":null,)"
                 R"("isMemoRequired":false,)"
                 R"("chainId":"eth")"
                 R"(})"
                 R"(])"
                 R"(},{)"
                 R"("currency":"LOKI",)"
                 R"("name":"OXEN",)"
                 R"("fullName":"Oxen",)"
                 R"("precision":8,)"
                 R"("isMarginEnabled":false,)"
                 R"("isDebitEnabled":true,)"
                 R"("items":[{)"
                 R"("chainName":"LOKI",)"
                 R"("minWithdrawSize":"1.1",)"
                 R"("minDepositSize":null,)"
                 R"("withdrawFeeRate":"0",)"
                 R"("minWithdrawFee":"1",)"
                 R"("isWithdrawEnabled":false,)"
                 R"("isDepositEnabled":false,)"
                 R"("confirms":10,)"
                 R"("preConfirms":10,)"
                 R"("contractAddress":"",)"
                 R"("withdrawPrecision":8,)"
                 R"("maxWithdrawSize":null,)"
                 R"("maxDepositSize":null,)"
                 R"("isMemoRequired":false,)"
                 R"("chainId":"loki")"
                 R"(})"
                 R"(])"
                 R"(},{)"
                 R"("currency":"MHC",)"
                 R"("name":"MHC",)"
                 R"("fullName":"MetaHash",)"
                 R"("precision":4,)"
                 R"("isMarginEnabled":false,)"
                 R"("isDebitEnabled":true,)"
                 R"("items":[{)"
                 R"("chainName":"MHC",)"
                 R"("minWithdrawSize":"100",)"
                 R"("minDepositSize":null,)"
                 R"("withdrawFeeRate":"0",)"
                 R"("minWithdrawFee":"50",)"
                 R"("isWithdrawEnabled":false,)"
                 R"("isDepositEnabled":false,)"
                 R"("confirms":32,)"
                 R"("preConfirms":32,)"
                 R"("contractAddress":"",)"
                 R"("withdrawPrecision":4,)"
                 R"("maxWithdrawSize":null,)"
                 R"("maxDepositSize":null,)"
                 R"("isMemoRequired":false,)"
                 R"("chainId":"mhc")"
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
