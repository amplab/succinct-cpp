import file
import kvstore
import semistructuredstore
import os
from os import path
import boto3
import tempfile
import time
from threading import Thread

# Compress time global
compress_time = 0
read_time = 0
upload_time = 0


def read_chunk(i, chunk_string, read_chunks, s3):
    print("reading chunk " + str(i))
    global read_time
    start = time.time()
    obj = s3.get_object(Bucket='succinct-datasets', Key=chunk_string + str(i) + ".succinct")
    read_chunks.append(obj['Body'].read().decode('utf-8'))
    del obj
    read_time += time.time() - start

def compress_chunk(i, read_chunks, compressed_chunks):
    print("compressing chunk " + str(i))
    global compress_time
    start = time.time()
    q = file.File(0, read_chunks[i], 32, 32, 128, 0, 1)
    compressed_chunks.append(q.GetContent().tobytes())
    q.DeleteContent()
    del q
    read_chunks[i] = None
    compress_time += time.time() - start

def upload_chunk(i, chunk_string, compressed_chunks, s3):
    print("uploading chunk " + str(i))
    global upload_time
    start = time.time()
    s3.put_object(Body=compressed_chunks[i], Bucket='succinct-datasets', Key=chunk_string + "compressed-" + str(i) + ".succinct")
    # Remove chunk here
    compressed_chunks[i] = None
    upload_time += time.time() - start



# compress file
def call_compress (event, context):
    # Define variables
    chunk_string = event['key1'] + "-chunk-"
    s3 = boto3.client("s3")
    # Depends on number of chunks file is split
    num_chunks = 1167
    read_chunks = []
    compressed_chunks = []

    # # **** QUERY FOR NUMBER OF CHUNK FILES ****
    # s3 = boto3.resource("s3")
    # bucket = s3.Bucket('succinct-datasets')
    # for bucket_obj in bucket.objects.all():
    #     if (chunk_string in str(bucket_obj)):
    #         num_chunks += 1
    # print("The num chunks is: " + str(num_chunks))

    # **** PARALLEL EXECUTION ****
    read_threads = []
    compress_threads = []
    upload_threads = []

    for i in range(0, num_chunks + 2):
        if (i == 0):
            # Create read thread
            read_thread = Thread(target=read_chunk, args=(0, chunk_string, read_chunks, s3))
            read_thread.start()
            read_threads.append(read_thread)
        elif (i == 1):
             # Create read thread
            read_thread = Thread(target=read_chunk, args=(1, chunk_string, read_chunks, s3))
            read_thread.start()
            read_threads.append(read_thread)

            # If read_threads[0] isn't done, finish
            if read_threads[0].is_alive():
                read_threads[0].join()
            # Create compress thread
            compress_thread = Thread(target=compress_chunk, args=(0, read_chunks, compressed_chunks))
            compress_thread.start()
            compress_threads.append(compress_thread)
        elif (i == num_chunks):
            # If read_threads[num_chunks - 1] isn't done, finish
            if read_threads[i - 1].is_alive():
                read_threads[i - 1].join()
            # Create compress thread
            compress_thread = Thread(target=compress_chunk, args=(i - 1, read_chunks, compressed_chunks))
            compress_thread.start()
            compress_threads.append(compress_thread)

            # If compress_threads[num_chunks - 2] isn't done, finish
            if compress_threads[i - 2].is_alive():
                compress_threads[i - 2].join()
            # Create upload thread
            upload_thread = Thread(target=upload_chunk, args=(i - 2, chunk_string, compressed_chunks, s3))
            upload_thread.start()
            upload_threads.append(upload_thread)
        elif (i == num_chunks + 1):
            # If compress_threads[num_chunks - 1] isn't done, finish
            if compress_threads[i - 2].is_alive():
                compress_threads[i - 2].join()
            # Create upload thread
            upload_thread = Thread(target=upload_chunk, args=(i - 2, chunk_string, compressed_chunks, s3))
            upload_thread.start()
            upload_threads.append(upload_thread)
        else:
            # Create read thread
            read_thread = Thread(target=read_chunk, args=(i, chunk_string, read_chunks, s3))
            read_thread.start()
            read_threads.append(read_thread)

            # If read_threads[i - 1] isn't done, finish
            if read_threads[i - 1].is_alive():
                read_threads[i - 1].join()
            # Create compress thread
            compress_thread = Thread(target=compress_chunk, args=(i - 1, read_chunks, compressed_chunks))
            compress_thread.start()
            compress_threads.append(compress_thread)

            # If compress_threads[i - 2] isn't done, finish
            if compress_threads[i - 2].is_alive():
                compress_threads[i - 2].join()
            # Create upload thread
            upload_thread = Thread(target=upload_chunk, args=(i - 2, chunk_string, compressed_chunks, s3))
            upload_thread.start()
            upload_threads.append(upload_thread)


    # # **** SEQUENTIAL EXECUTION ****
    # for i in range(0, num_chunks + 2):
    #     if (i == 0):
    #         read_chunk(0, chunk_string, read_chunks, s3)
    #     elif (i == 1):
    #         read_chunk(1, chunk_string, read_chunks, s3)
    #         compress_chunk(0, read_chunks, compressed_chunks)
    #     elif (i == num_chunks):
    #         compress_chunk(num_chunks - 1, read_chunks, compressed_chunks)
    #         upload_chunk(num_chunks - 2, chunk_string, compressed_chunks, s3)
    #     elif (i == num_chunks + 1):
    #         upload_chunk(num_chunks - 1, chunk_string, compressed_chunks, s3)
    #     else:
    #         read_chunk(i, chunk_string, read_chunks, s3)
    #         compress_chunk(i - 1, read_chunks, compressed_chunks)
    #         upload_chunk(i - 2, chunk_string, compressed_chunks, s3)


    # # **** UPLOAD FILE TO BUCKET ****
    # s3 = boto3.resource("s3")
    # os.chdir("/tmp")
    # f = open("test.txt","w")
    # f.write("hello this is a test")
    # f.close()
    # s3.meta.client.upload_file("/tmp/test.txt", "succinct-datasets", "test.txt")

    # # **** COMPRESS BY INPUT ****
    # # Get file content from S3 and save as "input"
    # s3 = boto3.client("s3")
    # obj = s3.get_object(Bucket='succinct-datasets', Key=event['key1'])
    # input = obj['Body'].read().decode('utf-8')

    # # Compress the input using file module
    # q = file.File(0, input, 32, 32, 128, 0, 1)
    # content = q.GetContent().tobytes()

    # # Upload content back onto S3 in .succinct file
    # s3.put_object(Body=content, Bucket='succinct-datasets', Key=event['key1'] + ".succinct")

    # # **** COMPRESS BY FILE ****
    # s3 = boto3.resource("s3")
    # os.chdir("/tmp")
    # s3.Bucket('succinct-datasets').download_file(event['key1'], event['key1'])
    # q = file.File(event['key1'], 32, 32, 128, 0, 1)


    # **** CALCULATE COMPRESSION RATIO ****
    # s3 = boto3.resource("s3")
    # bucket = s3.Bucket('succinct-datasets')
    # orignal_sizes = 0
    # compressed_sizes = 0
    # for i in range (0, num_chunks):
    #     orignal_sizes += boto3.resource('s3').Bucket('succinct-datasets').Object(event['key1'] + "-chunk-" + str(i) + ".succinct").content_length
    #     compressed_sizes += boto3.resource('s3').Bucket('succinct-datasets').Object(event['key1'] + "-chunk-compressed-" + str(i) + ".succinct").content_length

    # print(orignal_sizes)
    # print(compressed_sizes)
    # print(compressed_sizes/orignal_sizes)

    
    # # **** REMOVE OBJECTS FROM S3 ****
    # s3 = boto3.resource("s3")
    # for i in range (0, 1500):
    #     obj = s3.Object("succinct-datasets", event['key1'] + "-chunk-compressed-" + str(i) +  ".succinct")
    #     obj.delete()
        #obj = s3.Object("succinct-datasets", event['key1'] + "-chunk-" + str(i) +  ".succinct")
        #obj.delete()
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