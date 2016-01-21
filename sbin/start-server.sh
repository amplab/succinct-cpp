# Usage: start-server.sh <server-id> <data-file>
usage="Usage: start-server.sh <server-id> <data-file>"

if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$SERVER_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$LOG_PATH" == "" ]; then
    export LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $LOG_PATH/stderr

if [ "$CONF_PATH" = "" ]; then
    export CONF_PATH="$SUCCINCT_HOME/conf"
fi

nohup "$bin/sserver" $1 $2 >/dev/null 2>"$LOG_PATH/stderr/server_$2.stderr" &
