# Starts the coordinator on this node.
# Starts a server on each node specified in conf/hosts

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# # Sync succinct directory
# sync_dir="`cd "$SUCCINT_HOME"; pwd`"
# "$sbin"/sync "$sync_dir"

# Start Servers
echo "Starting servers..."
"$sbin"/start-servers.sh

# Wait for some time
sleep 1

# Start Coordinator
echo "Starting coordinator..."
"$sbin"/start-coordinator.sh