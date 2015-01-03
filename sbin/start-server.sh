# Usage: start-server.sh <handler#> <shard#>
usage="Usage: start-server.sh <handler#> <shard#>"

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

if [ "$SUCCINCT_DATA_PATH" = "" ]; then
  SUCCINCT_DATA_PATH="$SUCCINCT_HOME/dat"
fi

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $SUCCINCT_LOG_PATH

if [ "$QUERY_SERVER_PORT" = "" ]; then
	QUERY_SERVER_PORT=$(( $2 + 11002 )) # Hard-coded -- fix
fi

"$bin/qserver" -m 1 -p $QUERY_SERVER_PORT 2>"$SUCCINCT_LOG_PATH/server__${1}_${2}.log" &