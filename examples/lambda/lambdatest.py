import file
import kvstore
import semistructuredstore
import os
from os import path
import boto3
import tempfile

# Change the filenames to succinctdi_sa, succinctdir_isa .... and upload to s3
def uploadDirectory(path,bucketname, name):
    s3 = boto3.client("s3")
    for root,dirs,files in os.walk(path):   
        for f in files:
            s3.upload_file(os.path.join(root,f), bucketname, f) 

# compress file
def call_compress (event, context):
    # # Upload a file to the bucket
    # s3 = boto3.resource("s3")
    # os.chdir("/tmp")
    # f = open("test.txt","w")
    # f.write("hello this is a test")
    # f.close()
    # s3.meta.client.upload_file("/tmp/test.txt", "succinct-datasets", "test.txt")

    # Get file content from S3 and save as "input"
    s3 = boto3.client("s3")
    obj = client.get_object(Bucket='succinct-datasets', Key=event['key1'])
    input = obj.get()['Body'].read().decode('utf-8')
    
    # with open(event['key1'], 'r') as f:
        # print(f.read())

    # Compress the input using file module
    q = file.File(input, 32, 32, 128, 0, 1)
    content = (q.GetContent().tobytes())

    # Upload content back onto S3 in .succinct file
    client.put_object(Body=content, Bucket='succinct-datasets', Key=event['key1'] + ".succinct")

    # for f in os.listdir("/tmp"):
        # print(f)

    # uploadDirectory("/tmp/" + event['key1'] + ".succinct", "succinct-datasets", event['key1'])
    
    # out = os.path.isfile("/tmp/" + event['key1'] + ".succinct")
    # print(out)
    
    # Remove compressed .succinct file contents from s3
    # s3 = boto3.resource("s3")
    # obj = s3.Object("succinct-datasets", event['key1'] + ".succinct.metadata")
    # obj.delete()
    print("File compression and upload is complete")

def call_query (event, context):
    # Download .succinct file from bucket and compress
    s3 = boto3.client("s3")
    os.chdir("/tmp")
    os.mkdir(event['key1'])
    s3.download_file("succinct-datasets", event['key1'] + ".metadata", "/tmp/" + event['key1'] + "/" + event['key1']  + ".metadata")
    # for f in os.listdir("/tmp"):
    #     print(f)
    q = file.File(event['key1'])
    print("File deserialization and querying is complete")