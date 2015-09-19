#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# If the hosts file is specified in the command line,
# then it takes precedence over the definition in
# succinct-env.sh. Save it here.
if [ -f "$SUCCINCT_HOSTS" ]; then
  HOSTLIST=`cat "$SUCCINCT_HOSTS"`
fi

if [ "$SHARDS_PER_SERVER" = "" ]; then
  SHARDS_PER_SERVER="1"
fi

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

if [ "$HOSTLIST" = "" ]; then
  if [ "$SUCCINCT_HOSTS" = "" ]; then
    if [ -f "${SUCCINCT_CONF_DIR}/hosts" ]; then
      HOSTLIST=`cat "${SUCCINCT_CONF_DIR}/hosts"`
    else
      HOSTLIST=localhost
    fi
  else
    HOSTLIST=`cat "${SUCCINCT_HOSTS}"`
  fi
fi

# Launch the handlers
# By default disable strict host key checking
if [ "$SUCCINCT_SSH_OPTS" = "" ]; then
  SUCCINCT_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

i=0
NUM_SHARDS=$(cat "${SUCCINCT_CONF_DIR}/blowfish.conf"|wc -l)
NUM_HOSTS=$(echo "$HOSTLIST"|sed  "s/#.*$//;/^$/d"|wc -l)
NUM_SERVER_SHARDS_MIN=$((NUM_SHARDS/NUM_HOSTS))
for host in `echo "$HOSTLIST"|sed  "s/#.*$//;/^$/d"`; do

  # Compute number of shards on this server
  LAST_SHARD_ID=$((NUM_HOSTS*NUM_SERVER_SHARDS_MIN+i))
  if [ "$LAST_SHARD_ID" -lt "$NUM_SHARDS" ]; then
    NUM_SERVER_SHARDS=$((NUM_SERVER_SHARDS_MIN+1))
  else
    NUM_SERVER_SHARDS=$((NUM_SERVER_SHARDS_MIN))
  fi

  if [ -n "${SUCCINCT_SSH_FOREGROUND}" ]; then
    ssh $SUCCINCT_SSH_OPTS "$host" "$sbin/start-handler.sh" $NUM_SERVER_SHARDS $i \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $SUCCINCT_SSH_OPTS "$host" "$sbin/start-handler.sh" $NUM_SERVER_SHARDS $i \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$SUCCINCT_HOST_SLEEP" != "" ]; then
    sleep $SUCCINCT_HOST_SLEEP
  fi
  i=$(( $i + 1 ))
done
