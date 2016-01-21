#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$SERVER_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$LOG_PATH" = "" ]; then
  LOG_PATH="$SUCCINCT_HOME/log/"
fi

limit=$(($1 - 1))
for i in `seq 0 $limit`; do
	DATA_FILE="$SUCCINCT_DATA_PATH/data_$i"
	nohup "$bin/succinct-server" $i $DATA_FILE &
done
