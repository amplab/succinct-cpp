# Starts the master on the machine this script is executed on.

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$SUCCINCT_HOME/bin"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $SUCCINCT_LOG_PATH

"$bin/smaster" -h "$SUCCINCT_CONF_DIR/hosts" 2>"$SUCCINCT_LOG_PATH/master.log" &
