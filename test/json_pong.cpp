/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kucoin_pro;

using namespace std::literals;

using value_type = protocol::json::Pong;

TEST_CASE("simple", "[json_pong]") {
  auto message = R"({)"
                 R"("type":"pong",)"
                 R"("id":"366142583719543",)"
                 R"("ts":1771580331134146844)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.type == protocol::json::Type::PONG);
    CHECK(obj.id == 366142583719543);
    CHECK(obj.ts == 1771580331134146844ns);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
