/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::Ack;

TEST_CASE("simple", "[json_ack]") {
  auto message = R"({)"
                 R"("id":"4000001",)"
                 R"("result":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "4000001"sv);
    CHECK(obj.result == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
