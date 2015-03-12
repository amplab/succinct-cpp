# Usage: start-ahandler.sh <#shards> <handler#>
usage="Usage: start-ahandler.sh <#shards> <handler#>"

if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

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

if [ "$REPLICATION" = "" ]; then
	REPLICATION=1
fi

mkdir -p $SUCCINCT_LOG_PATH

nohup "$bin/ahandler" -d -m 1 -s "$1" -h "$SUCCINCT_CONF_DIR/hosts" -r "$REPLICATION" -i "$2" "$SUCCINCT_DATA_PATH/data" 2>"$SUCCINCT_LOG_PATH/handler_${2}.log" &
