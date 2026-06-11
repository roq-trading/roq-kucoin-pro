set(TARGET_NAME ${PROJECT_NAME}-protocol-json)

set(SOURCES encoder.cpp map.cpp parser.cpp utils.cpp ws_parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    account_ack_data_accounts_item_currencies_item.json
    account_ack_data_accounts_item.json
    account_ack_data.json
    account_ack.json
    account_type.json
    ack.json
    add_order_ack_data.json
    add_order_ack.json
    balance_data.json
    balance.json
    business_type.json
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
    depth.json
    display_type.json
    error.json
    event_type.json
    execution_ack_data_items_item.json
    execution_ack_data.json
    execution_ack.json
    fee_type.json
    fill_type.json
    instance_server.json
    instrument_ack_data.json
    instrument_ack_data_list_item.json
    instrument_ack.json
    liquidity_role.json
    margin_mode.json
    market_stage.json
    obu_data_item.json
    obu_data.json
    obu.json
    order_all_data.json
    order_all.json
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
    position_ack_data_item.json
    position_ack.json
    position_all_data.json
    position_all.json
    position_side.json
    price_type.json
    quantity_unit.json
    self_trade_prevention.json
    side.json
    status.json
    stop.json
    stop_price_type.json
    ticker_data.json
    ticker.json
    time_in_force.json
    token_data.json
    token.json
    trade_data.json
    trade.json
    trade_type.json
    trigger_direction.json
    type.json
    update_type.json
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

add_custom_target(protocol-json-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

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

add_dependencies(${TARGET_NAME} protocol-json-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
