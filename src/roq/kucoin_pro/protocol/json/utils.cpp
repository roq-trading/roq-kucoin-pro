/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/protocol/json/utils.hpp"

#include "roq/kucoin_pro/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace protocol {
namespace json {

Error guess_error([[maybe_unused]] int32_t code) {
  return Error::UNKNOWN;
}

bool is_auth_error(int32_t code) {
  switch (code) {
    case 400001:  // Any of KC-API-KEY, KC-API-SIGN, KC-API-TIMESTAMP, KC-API-PASSPHRASE is missing in your request header
    case 400002:  // KC-API-TIMESTAMP Invalid
    case 400003:  // KC-API-KEY does not exist
    case 400004:  // KC-API-PASSPHRASE error
    case 400005:  // Signature error
    case 400006:  // The requested ip address is not on the api whitelist
      return true;
  }
  return false;
}

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
