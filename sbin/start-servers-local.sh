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
	QUERY_SERVER_PORT=11002
fi

if [ "$REGEX_OPT" = "" ]; then
    REGEX_OPT="TRUE"
fi

if [ "$REGEX_OPT" = "TRUE" ]; then
    OPT="-o"
fi

if [ "$SA_SAMPLING_RATE" = "" ]; then
    SA_SAMPLING_RATE="32"
fi

if [ "$ISA_SAMPLING_RATE" = "" ]; then
    ISA_SAMPLING_RATE="32"
fi

if [ "$SAMPLING_SCHEME" = "" ]; then
    SAMPLING_SCHEME="0"
fi

if [ "$NPA_SCHEME" = "" ]; then
    NPA_SCHEME="1"
fi

limit=$(($1 - 1))
for i in `seq 0 $limit`; do
	PORT=$(($QUERY_SERVER_PORT + $i))
	DATA_FILE="$SUCCINCT_DATA_PATH/data_$i"
	nohup "$bin/sserver" -m 1 -p $PORT -s $SA_SAMPLING_RATE -i $ISA_SAMPLING_RATE -x $SAMPLING_SCHEME -r $NPA_SCHEME $OPT $DATA_FILE 2>"$SUCCINCT_LOG_PATH/server_${i}.log" &
done
