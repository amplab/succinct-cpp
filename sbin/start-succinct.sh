# Starts the master on this node.
# Starts a handler on each node specified in conf/hosts

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# Sync succinct directory
sync_dir="`cd "$SUCCINT_HOME"; pwd`"
"$sbin"/sync "$sync_dir"

# Start Servers
echo "Starting servers..."
"$sbin"/start-servers.sh

# Wait for some time
sleep 1

# Start Handlers
echo "Starting handlers..."
"$sbin"/start-handlers.sh

# Wait for some time
sleep 1

# Start Master
echo "Starting master..."
"$sbin"/start-master.sh
