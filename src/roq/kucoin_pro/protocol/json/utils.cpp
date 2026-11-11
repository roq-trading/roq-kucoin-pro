/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kucoin_pro/protocol/json/utils.hpp"

#include "roq/kucoin_pro/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kucoin_pro {
namespace protocol {
namespace json {

bool is_auth_error(int32_t code) {
  switch (code) {
    case 400'001:  // Any of KC-API-KEY, KC-API-SIGN, KC-API-TIMESTAMP, KC-API-PASSPHRASE is missing in your request header
    case 400'002:  // KC-API-TIMESTAMP Invalid
    case 400'003:  // KC-API-KEY does not exist
    case 400'004:  // KC-API-PASSPHRASE error
    case 400'005:  // Signature error
    case 400'006:  // The requested ip address is not on the api whitelist
      return true;
  }
  return false;
}

}  // namespace json
}  // namespace protocol
}  // namespace kucoin_pro
}  // namespace roq
