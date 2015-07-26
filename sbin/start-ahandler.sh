# Usage: start-ahandler.sh <#local-shards> <#global-shards> <handler#>
usage="Usage: start-ahandler.sh <#local-shards> <#global-shards> <handler#>"

if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$ADAPTIVE_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

if [ "$REPLICATION" = "" ]; then
	REPLICATION="1"
fi

mkdir -p $SUCCINCT_LOG_PATH

nohup "$bin/ahandler" -m 1 -s "$1" -p "$2" -h "$SUCCINCT_CONF_DIR/hosts" -r "$REPLICATION" -i "$3" 2>"$SUCCINCT_LOG_PATH/handler_${3}.log" &
