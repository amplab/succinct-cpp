#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/bin/load-succinct-env.sh"

bin="$SUCCINCT_HOME/bin"
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
	QUERY_SERVER_PORT="11002"
fi

if [ "$ISA_SAMPLING_RATE" = "" ]; then
    ISA_SAMPLING_RATE="32"
fi

if [ "$SA_SAMPLING_RATE" = "" ]; then
    SA_SAMPLING_RATE="32"
fi

if [ "$OPPORTUNISTIC" = "TRUE" ]; then
    OPP="-o"
elif
    OPP=""
fi

if [ "$STANDALONE" = "TRUE" ]; then
    STD="-d"
elif
    STD=""
fi

LIMIT=$(($1 - 1))

for i in `seq 0 $LIMIT`; do
	PORT=$(($QUERY_SERVER_PORT + $i))
	DATA_FILE="$SUCCINCT_DATA_PATH/data"
    nohup "$bin/aserver" -m 1 -p $PORT -i $ISA_SAMPLING_RATE -s $SA_SAMPLING_RATE $STD $OPP $DATA_FILE 2>"$SUCCINCT_LOG_PATH/server_${i}.log" &
done
