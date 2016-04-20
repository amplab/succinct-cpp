#!/usr/bin/env bash

curdir="`dirname "$0"`"
curdir="`cd "$curdir"; pwd`"

# Install dependencies on coordinator and all servers
$curdir/install-dependencies.sh &
$curdir/../sbin/hosts.sh $curdir/install-dependencies.sh
wait

echo "Finished installing dependencies on coordinator and all servers"
