#! /bin/bash
FILES=/home/ec2-user/file_chunks/sample.txt-chunk-*.succinct
for f in $FILES
do
        aws s3 cp $f s3://succinct-datasets
done