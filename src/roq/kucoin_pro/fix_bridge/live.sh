#!/usr/bin/env bash

if [ "$1" == "debug" ]; then
  PREFIX="gdb --args"
else
  PREFIX=
fi

NAME="kucoin-pro"

CONFIG="${CONFIG:-$NAME}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-kucoin-pro/$CONFIG.toml"

FLAGFILE="../../../../share/flags/prod/flags.cfg"

DOWNLOAD_TRADES_LOOKBACK="24h"
WS_API=true

$PREFIX ./roq-kucoin-pro-fix-bridge \
  --name "$NAME" \
  --config_file "$CONFIG_FILE" \
  --flagfile "$FLAGFILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --client_listen_address "$HOME/run/$NAME.sock" \
  --service_listen_address "$HOME/run/metrics/${NAME}.sock" \
  --download_trades_lookback="$DOWNLOAD_TRADES_LOOKBACK" \
  --ws_api=$WS_API \
  $@
