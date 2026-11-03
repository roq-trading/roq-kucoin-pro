/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kucoin_pro/protocol/json/instrument_ack.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::InstrumentAck;

// note! truncated
TEST_CASE("simple", "[json_instrument_ack]") {
  auto message = R"({)"
                 R"("code":"200000",)"
                 R"("data":{)"
                 R"("tradeType":"FUTURES",)"
                 R"("list":[{)"
                 R"("baseCurrency":"XBT",)"
                 R"("buyLimit":"80672.1",)"
                 R"("displayBaseCurrency":"XBT",)"
                 R"("name":"XBTUSDTM",)"
                 R"("expiryTime":null,)"
                 R"("f":"1.3",)"
                 R"("tradingStartTime":"1585555200000",)"
                 R"("isInverse":false,)"
                 R"("quoteCurrency":"USDT",)"
                 R"("k":"490.0000000000",)"
                 R"("lotSize":"1",)"
                 R"("maxMarketOrderSize":"1000000",)"
                 R"("m":"300.0000000000",)"
                 R"("makerFeeRate":"0.00020",)"
                 R"("marketStage":"NORMAL",)"
                 R"("marketType":"CRYPTO",)"
                 R"("maxLeverage":"125",)"
                 R"("maxBaseOrderSize":"1000000",)"
                 R"("maxPrice":"1000000.0000000000",)"
                 R"("priceLimitRatio":"0.0500000000",)"
                 R"("preMarketToPerpDate":null,)"
                 R"("mmrLevConstant":"125.00",)"
                 R"("mmrLimit":"0.3",)"
                 R"("unitSize":"0.001",)"
                 R"("indexSourceExchanges":[)"
                 R"("okex",)"
                 R"("binance",)"
                 R"("kucoin",)"
                 R"("bybit",)"
                 R"("gateio")"
                 R"(],)"
                 R"("tradingStatus":"Open",)"
                 R"("symbol":"XBTUSDTM",)"
                 R"("takerFeeRate":"0.00060",)"
                 R"("tickSize":"0.1",)"
                 R"("settlementFeeRate":null,)"
                 R"("settlementTime":null,)"
                 R"("settlementCurrency":"USDT",)"
                 R"("sellLimit":"72989",)"
                 R"("indexPriceTickSize":"0.01",)"
                 R"("contractType":"0",)"
                 R"("assetClass":"CRYPTO",)"
                 R"("subMarketType":null)"
                 R"(},{)"
                 R"("baseCurrency":"ZRX",)"
                 R"("buyLimit":"0.1037",)"
                 R"("displayBaseCurrency":"ZRX",)"
                 R"("name":"ZRXUSDTM",)"
                 R"("expiryTime":null,)"
                 R"("f":"1.3",)"
                 R"("tradingStartTime":"1681459200000",)"
                 R"("isInverse":false,)"
                 R"("quoteCurrency":"USDT",)"
                 R"("k":"1359044.0000000000",)"
                 R"("lotSize":"1",)"
                 R"("maxMarketOrderSize":"100000",)"
                 R"("m":"852000.0000000000",)"
                 R"("makerFeeRate":"0.00020",)"
                 R"("marketStage":"NORMAL",)"
                 R"("marketType":"CRYPTO",)"
                 R"("maxLeverage":"30",)"
                 R"("maxBaseOrderSize":"100000",)"
                 R"("maxPrice":"1000000.0000000000",)"
                 R"("priceLimitRatio":"0.1000000000",)"
                 R"("preMarketToPerpDate":null,)"
                 R"("mmrLevConstant":"30.00",)"
                 R"("mmrLimit":"0.3",)"
                 R"("unitSize":"10.0",)"
                 R"("indexSourceExchanges":[)"
                 R"("bybit",)"
                 R"("okex",)"
                 R"("binance",)"
                 R"("kucoin")"
                 R"(],)"
                 R"("tradingStatus":"Open",)"
                 R"("symbol":"ZRXUSDTM",)"
                 R"("takerFeeRate":"0.00060",)"
                 R"("tickSize":"0.00010",)"
                 R"("settlementFeeRate":null,)"
                 R"("settlementTime":null,)"
                 R"("settlementCurrency":"USDT",)"
                 R"("sellLimit":"0.0849",)"
                 R"("indexPriceTickSize":"0.00010",)"
                 R"("contractType":"0",)"
                 R"("assetClass":"CRYPTO",)"
                 R"("subMarketType":null)"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.code == 200000);
    // REQUIRE(std::size(obj.data) == 3);
  };
  core::json::BufferStack buffers{65536, 2};
  value_type obj{message, buffers};
  helper(obj);
}
