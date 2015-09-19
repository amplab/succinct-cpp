#!/usr/bin/python

import sys
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-H", "--hosts-file", action="store", type="string", dest="hosts_file", default="conf/hosts")
parser.add_option("-c", "--conf-file", action="store", type="string", dest="conf_file", default="conf/blowfish.conf")
parser.add_option("-d", "--data-path", action="store", type="string", dest="data_path", default="/mnt/data")
parser.add_option("-S", "--s3cmd-exec", action="store", type="string", dest="s3cmd_exec", default="s3cmd")

(options, args) = parser.parse_args()

hosts_file = options.hosts_file
conf_file = options.conf_file
data_path = options.data_path
s3cmd_exec = options.s3cmd_exec
data_sr_map =   {
                8 : "s3://succinct-datasets/nsdi16/distributed/data_8.succinct/*",
                16 : "s3://succinct-datasets/nsdi16/distributed/data_16.succinct/*",
                32 : "s3://succinct-datasets/nsdi16/distributed/data_32.succinct/*",
                64 : "s3://succinct-datasets/nsdi16/distributed/data_64.succinct/*",
                128 : "s3://succinct-datasets/nsdi16/distributed/data_128.succinct/*",
                }

# Read hosts
hosts = [host.rstrip('\n') for host in open(hosts_file)]

# Read sampling rates
sr_map = dict((int(sdata.split()[0]), int(sdata.split()[1])) for sdata in open(conf_file))
num_shards = len(sr_map)

for i in range(0, num_shards):
    if i > 0 and i % len(hosts) == 0:
        print "wait"
    data_source = data_sr_map[sr_map[i]]
    host = hosts[i % len(hosts)]
    dst = data_path + "/data_" + str(i) + ".succinct"
    cmd = "mkdir -p " + dst + "&& " + s3cmd_exec + " get " + data_source + " " + dst
    ssh_cmd = "ssh -o StrictHostKeyChecking=no " + host + " \'" + cmd + "\' 2>&1 | sed \"s/^/" + host + ": /\" &"
    print ssh_cmd
print "wait"