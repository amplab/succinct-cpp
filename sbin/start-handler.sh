# Usage: start-handler.sh <handler-id>
usage="Usage: start-handler.sh <handler-id>"

if [ $# -ne 1 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$HANDLER_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$LOG_PATH" = "" ]; then
  export LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $LOG_PATH/stderr

if [ "$CONF_PATH" = "" ]; then
  export CONF_PATH="$SUCCINCT_HOME/conf"
fi

nohup "$bin/shandler" $1 >/dev/null 2>"$LOG_PATH/stderr/handler_$1.stderr" &

