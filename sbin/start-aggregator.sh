#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

if [ "$NUM_SHARDS" = "" ]; then
	NUM_SHARDS="1"
fi

mkdir -p $SUCCINCT_LOG_PATH

nohup "$AGGREGATOR" -s "$NUM_SHARDS" 2>"$SUCCINCT_LOG_PATH/aggregator.log" >/dev/null &
