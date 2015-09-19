#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

if [ "$SUCCINCT_HOSTS" = "" ]; then
	if [ -f "${SUCCINCT_CONF_DIR}/hosts" ]; then
		SUCCINCT_HOSTS="${SUCCINCT_CONF_DIR}/hosts"
	else
		echo "Hosts file not found"
		exit
	fi
fi

if [ "$BLOWFISH_CONF" = "" ]; then
	if [ -f "${SUCCINCT_CONF_DIR}/blowfish.conf" ]; then
		BLOWFISH_CONF="${SUCCINCT_CONF_DIR}/blowfish.conf"
	else
		echo "Hosts file not found"
		exit
	fi
fi

if [ "$S3CMD_EXEC" = "" ]; then
	S3CMD_EXEC="s3cmd"
fi

if [ "$SUCCINCT_DATA_PATH" = "" ]; then
	SUCCINCT_DATA_PATH="$SUCCINCT_PREFIX/dat"
fi

echo "Creating download script based on configuration..."
python "$sbin/setup_cluster.py" -H "$SUCCINCT_HOSTS"  -c "$BLOWFISH_CONF" -S "$S3CMD_EXEC" -d "$SUCCINCT_DATA_PATH" > download.sh
echo "Done. The script to be executed looks as follows:"
cat download.sh
chmod a+x download.sh
echo "Running script..."
./download.sh
echo "Script execution complete; Removing script..."
rm download.sh
echo "Done. Cluster setup complete."
