#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$SERVER_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$LOG_PATH" = "" ]; then
  export LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $LOG_PATH/stderr

if [ "$CONF_PATH" = "" ]; then
  export CONF_PATH="$SUCCINCT_HOME/conf"
fi

if [ "$SUCCINCT_DATA_PATH" = "" ]; then
  export SUCCINCT_DATA_PATH="$SUCCINCT_HOME/dat"
fi

limit=$(($1 - 1))
for i in `seq 0 $limit`; do
	DATA_FILE="$SUCCINCT_DATA_PATH/data_$i"
  SHARD_ID=$(($1 * $2 + $i))
	nohup "$bin/sserver" $i $DATA_FILE >/dev/null 2>"$LOG_PATH/stderr/server_$i.stderr" &
done
