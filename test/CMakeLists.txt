include(RoqAutogen)

set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES
    json_account_ack.cpp
    # json_add_order_ack.cpp
    json_cancel_order_ack.cpp
    json_currencies_ack.cpp
    json_currency_ack.cpp
    json_encoder.cpp
    json_execution_ack.cpp
    json_instrument_ack.cpp
    json_obu.cpp
    json_order_all.cpp
    json_order_book_ack.cpp
    json_orders_ack.cpp
    json_pong.cpp
    json_position_ack.cpp
    json_position_all.cpp
    json_ticker.cpp
    json_trade.cpp
    json_welcome.cpp
    json_ws_add_order_ack.cpp
    json_ws_cancel_order_ack.cpp
    json_ws_error.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-json roq-server::roq-server Catch2::Catch2)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
