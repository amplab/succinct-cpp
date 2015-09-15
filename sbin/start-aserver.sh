# Usage: start-server.sh <datafile> <server#>
usage="Usage: start-server.sh <datafile> <server#>"

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

if [ "$OPPORTUNISTIC" = "TRUE" ]; then
    OPP="-o"
else
    OPP=""
fi

if [ "$STANDALONE" = "TRUE" ]; then
    BOOTSTRAP="-d"
else
    BOOTSTRAP="-d"
fi

port=$(( $QUERY_SERVER_PORT + $2 ))

nohup "$bin/aserver" -m 1 $BOOTSTRAP $OPP -p $port ${1} 2>"$SUCCINCT_LOG_PATH/server_${2}.log" > /dev/null &
