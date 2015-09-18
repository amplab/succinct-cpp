#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$THRIFT_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$SUCCINCT_DATA_PATH" = "" ]; then
  SUCCINCT_DATA_PATH="$SUCCINCT_HOME/dat"
fi

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $SUCCINCT_LOG_PATH

if [ "$QUERY_SERVER_PORT" = "" ]; then
	QUERY_SERVER_PORT=12002
fi

if [ "$CONF_FILE" = "" ]; then
    CONF_FILE="$SUCCINCT_PREFIX/conf/blowfish.conf"
fi

limit=$(($1 - 1))
for i in `seq 0 $limit`; do
	PORT=$(($QUERY_SERVER_PORT + $i))
	DATA_FILE="$SUCCINCT_DATA_PATH/data_$i"
	nohup "$bin/sserver" -p "$PORT" -c "$CONF_FILE" "$DATA_FILE" 2>"$SUCCINCT_LOG_PATH/server_${i}.log" &
done
