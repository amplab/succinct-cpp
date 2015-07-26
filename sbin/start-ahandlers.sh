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

NUM_HOSTS=$(echo "$HOSTLIST"|sed  "s/#.*$//;/^$/d"|wc -l)

if [ "$SHARDS_PER_SERVER" = "" ]; then
  SHARDS_PER_SERVER="1"
fi

if [ "$REPLICATION" = "" ]; then
  REPLICATION="1"
fi

if [ "$PRIMARY_SHARDS" = "" ]; then
  PRIMARY_SHARDS=$((SHARDS_PER_SERVER * NUM_HOSTS / REPLICATION))
fi

# Launch the handlers
# By default disable strict host key checking
if [ "$SUCCINCT_SSH_OPTS" = "" ]; then
  SUCCINCT_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

i=0
for host in `echo "$HOSTLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${SUCCINCT_SSH_FOREGROUND}" ]; then
    ssh $SUCCINCT_SSH_OPTS "$host" "$sbin/start-ahandler.sh" $SHARDS_PER_SERVER $PRIMARY_SHARDS $i \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $SUCCINCT_SSH_OPTS "$host" "$sbin/start-ahandler.sh" $SHARDS_PER_SERVER $PRIMARY_SHARDS $i \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$SUCCINCT_HOST_SLEEP" != "" ]; then
    sleep $SUCCINCT_HOST_SLEEP
  fi
  i=$(( $i + 1 ))
done
