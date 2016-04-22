#!/usr/bin/env bash

curdir="`dirname "$0"`"
curdir="`cd "$curdir"; pwd`"

# Install dependencies and succinct on coordinator and all servers
$curdir/../sbin/sync $curdir/../

$curdir/install-dependencies.sh &
$curdir/../sbin/servers.sh $curdir/install-dependencies.sh
wait

echo "Finished installing dependencies on coordinator and all servers"

cd $curdir/../
./build.sh

cp $curdir/succinct-env.sh.template conf/succinct-env.sh
coordinator=`curl -s http://169.254.169.254/latest/meta-data/public-hostname`
echo "export COORDINATOR_HOSTNAME=\"$coordinator\"" >> conf/succinct-env.sh

# FIXME: Assumes the architecture on both coordinator and servers is the same
./sbin/sync .
