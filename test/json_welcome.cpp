/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::Welcome;

TEST_CASE("simple", "[json_welcome]") {
  auto message = R"({)"
                 R"("sessionId":"d2c9a1c2-1f44-4ba6-b4ed-31ff3b9243af",)"
                 R"("message":"welcome",)"
                 R"("pingInterval":18000)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.session_id == "d2c9a1c2-1f44-4ba6-b4ed-31ff3b9243af"sv);
    CHECK(obj.message == "welcome"sv);
    CHECK(obj.ping_interval == 18000);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
