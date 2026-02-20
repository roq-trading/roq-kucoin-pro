set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES encoder.cpp map.cpp parser.cpp utils.cpp ws_parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    depth.json
    update_type.json
    obu.json
    obu_data.json
    obu_data_item.json
    account_ack_data.json
    account_ack.json
    ack.json
    add_order_ack_data.json
    add_order_ack.json
    cancel_all_orders_ack_data.json
    cancel_all_orders_ack.json
    cancel_order_ack_data.json
    cancel_order_ack.json
    channel.json
    currencies_ack_data_item_items_item.json
    currencies_ack_data_item.json
    currencies_ack.json
    currency_ack_data.json
    currency_ack_data_list_item.json
    currency_ack.json
    display_type.json
    error.json
    fee_type.json
    fills_ack_data_item.json
    fills_ack.json
    instance_server.json
    instrument_ack_data.json
    instrument_ack_data_list_item.json
    instrument_ack.json
    liquidity.json
    margin_mode.json
    market_stage.json
    order_book_ack_data_bids_asks_item.json
    order_book_ack_data.json
    order_book_ack.json
    orders_ack_data_items_item.json
    orders_ack_data.json
    orders_ack.json
    order_status.json
    order_type.json
    order_update_type.json
    pong.json
    positions_ack_data_item.json
    positions_ack.json
    position_side.json
    self_trade_prevention.json
    side.json
    status.json
    stop.json
    stop_price_type.json
    ticker_data.json
    ticker.json
    time_in_force.json
    trade_data.json
    trade.json
    trade_type.json
    type.json
    welcome.json
    ws_add_order_ack_data.json
    ws_add_order_ack.json
    ws_auth.json
    ws_cancel_order_ack_data.json
    ws_cancel_order_ack.json
    ws_error.json
    ws_message.json
    ws_op.json
    ws_pong.json
    ws_user_rate_limit.json
    ws_welcome.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} autogen-json-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
