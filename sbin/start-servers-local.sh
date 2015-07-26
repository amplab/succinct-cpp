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
    opt="-o"
fi

i=0
while read sr dist; do
	((sampling_rates[$i] = $sr))
	i=$(($i + 1))
done < ${SUCCINCT_CONF_DIR}/repl

num_shards=$1
num_hosts=$2
local_host_id=$3
num_replicas=$( wc -l < ${SUCCINCT_CONF_DIR}/repl)
limit=$(($1 - 1))

for i in `seq 0 $limit`; do
	port=$(($QUERY_SERVER_PORT + $i))
	shard_id=$(($i * $num_hosts + local_host_id))
	shard_type=$(($shard_id % $num_replicas))
	data_file="$SUCCINCT_DATA_PATH/data_${shard_type}"
	nohup "$bin/sserver" -m 1 -p $port -i ${sampling_rates[$shard_type]} $opt $data_file 2>"$SUCCINCT_LOG_PATH/server_${i}.log" &
done
