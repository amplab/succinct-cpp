import file
import boto3
import time
import os
from threading import Thread, Event
import resource
import signal
import asyncio
import aiobotocore
import sys
from promise import Promise

# from memory_profiler import profile
# from guppy import hpy

# Compress time global
compress_time = 0
read_time = 0
upload_time = 0

# PID = os.getpid()

# Reusable Thread class from: https://www.codeproject.com/Tips/1271787/Python-Reusable-Thread-Class
class ReusableThread(Thread):
    """
    This class provides code for a restartale / reusable thread

    join() will only wait for one (target)functioncall to finish
    finish() will finish the whole thread (after that, it's not restartable anymore)
        
    """

    def __init__(self, target, args):
        self._startSignal = Event()
        self._oneRunFinished = Event()
        self._finishIndicator = False
        self._callable = target
        self._callableArgs = args

        Thread.__init__(self)

    def restart(self):
        """make sure to always call join() before restarting"""
        self._startSignal.set()

    def run(self):
        """ This class will reprocess the object "processObject" forever.
        Through the change of data inside processObject and start signals
        we can reuse the thread's resources"""

        self.restart()
        while(True):
            # wait until we should process
            self._startSignal.wait()

            self._startSignal.clear()

            if(self._finishIndicator):# check, if we want to stop
                self._oneRunFinished.set()
                return

            # call the threaded function
            self._callable(*self._callableArgs)

            # notify about the run's end
            self._oneRunFinished.set()

    def join(self):
        """ This join will only wait for one single run (target functioncall) to be finished"""
        self._oneRunFinished.wait()
        self._oneRunFinished.clear()

    def finish(self):
        self._finishIndicator = True
        self.restart()
        self.join()

def do_nothing(*args):
    pass


# # **** PARALLEL EXECUTION FUNCTIONS ****
async def read_chunk(i, chunk_string, read_chunks, client, code_start):
    if ( i != 0 ):
        # Await on previous chunk if not chunk 0
        await read_chunk(i - 1, chunk_string, read_chunks, client, code_start)

    # After calling on previous chunk, run current chunk i read
    print("reading chunk " + str(i))
    global read_time
    start = time.time()

    obj = await client.get_object(Bucket='succinct-datasets', Key=chunk_string + str(i) + ".succinct")
    read_chunks[i] = await obj['Body'].read()
    del obj

    end = time.time()
    read_time += end - start

    # write chunk times to file
    read_time_file = open("read_time_file.txt","a")
    read_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")

async def compress_chunk(i, read_chunks, compressed_chunks, code_start):
    if ( i != 0 ):
        # Await on previous chunk if not chunk 0
        await compress_chunk(i - 1, read_chunks, compressed_chunks, code_start)

    # After calling on previous chunk, run current chunk i upload, and check to see if corresponding compression is complete
    # Loop that exits when read_chunks thread for chunk i is complete
    while (True):
        if (read_chunks[i] != None):
            print("compressing chunk " + str(i))
            global compress_time
            start = time.time()

            # If there is read data in the list, then we can go ahead and compress
            await asyncio.sleep(0)
            q = file.File(0, read_chunks[i], 32, 32, 128, 0, 1)
            compressed_chunks[i] = q.GetContent().tobytes()
            q.DeleteContent()
            del q
            read_chunks[i] = None

            end = time.time()
            compress_time += end - start
            break
        else:
            # otherwise wait 0.05 seconds and check read_chunks again
            print("waiting for read")
            await asyncio.sleep(0.01)
            # write chunk times to file
    compress_time_file = open("compress_time_file.txt","a")
    compress_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")

async def upload_chunk(i, chunk_string, compressed_chunks, client, code_start):
    if ( i != 0 ):
        # Await on previous chunk if not chunk 0
        await upload_chunk(i - 1, chunk_string, compressed_chunks, client, code_start)

    # After calling on previous chunk, run current chunk i upload, and check to see if corresponding compression is complete

    # Loop that exits when compressed_chunks thread for chunk i is complete
    while (True):
        if (compressed_chunks[i] != None):
            print("uploading chunk " + str(i))
            global upload_time
            start = time.time()

            # If there is compressed data in the list, then we can go ahead and upload
            await client.put_object(Body=compressed_chunks[i], Bucket='succinct-datasets', Key=chunk_string + "compressed-" + str(i) + ".succinct", StorageClass='REDUCED_REDUNDANCY')
            compressed_chunks[i] = None

            end = time.time()
            upload_time += end - start
            break
        else:
            # otherwise wait 0.05 seconds and check compressed_chunks again
            print("waiting for compress")
            await asyncio.sleep(0.01)


    # write chunk times to file
    upload_time_file = open("upload_time_file.txt","a")
    upload_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")

    # # # **** SEQUENTIAL EXECUTION FUNCTIONS ****
# def read_chunk(i, chunk_string, read_chunks, client, code_start):
#     print("reading chunk " + str(i))
#     global read_time
#     start = time.time()

#     obj = client.get_object(Bucket='succinct-datasets', Key=chunk_string + str(i) + ".succinct")
#     read_chunks[i] = obj['Body'].read()
#     del obj

#     end = time.time()
#     read_time += end - start

#     # # write chunk times to file
#     # read_time_file = open("read_time_file.txt","a")
#     # read_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")

# def compress_chunk(i, read_chunks, compressed_chunks, code_start):
#     start = time.time()

#     print("compressing chunk " + str(i))
#     global compress_time

#     # await asyncio.sleep(0)
#     q = file.File(0, read_chunks[i], 32, 32, 128, 0, 1)
#     compressed_chunks[i] = q.GetContent().tobytes()
#     q.DeleteContent()
#     del q
#     read_chunks[i] = None

#     end = time.time()
#     compress_time += end - start

#     # write chunk times to file
#     # compress_time_file = open("compress_time_file.txt","a")
#     # compress_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")


# def upload_chunk(i, chunk_string, compressed_chunks, client, code_start):
#     print("uploading chunk " + str(i))
#     global upload_time
#     start = time.time()

#     client.put_object(Body=compressed_chunks[i], Bucket='succinct-datasets', Key=chunk_string + "compressed-" + str(i) + ".succinct", StorageClass='REDUCED_REDUNDANCY')
#     compressed_chunks[i] = None

#     end = time.time()
#     upload_time += end - start

#     # write chunk times to file
#     # upload_time_file = open("upload_time_file.txt","a")
#     # upload_time_file.write("chunk " + str(i) +  " start: " + str(start - code_start) + " duration: " + str(end - start) + "\n")

# **** FUNCTION START ****
async def execute():
    # compress file
    start = time.time()
    # # Define variables
    chunk_string = "sample.txt" + "-chunk-"
    # # Depends on number of chunks file is split into
    num_chunks = 1167
    read_chunks = [None] * num_chunks
    compressed_chunks = [None] * num_chunks

    # COMPRESS WITH FILES
    # q = file.File("sample.txt-chunk-1160.succinct", 32, 32, 128, 0, 1)

    # COMPRESS IN MEM
    # f = open("sample.txt-chunk-1166.succinct","r")
    # content = f.read()
    # q = file.File(0, content, 32, 32, 128, 0, 1)


    # # **** QUERY FOR NUMBER OF CHUNK FILES ****
    # s3 = boto3.resource("s3")
    # bucket = s3.Bucket('succinct-datasets')
    # for bucket_obj in bucket.objects.all():
    #     if (chunk_string in str(bucket_obj)):
    #         num_chunks += 1
    # print("The num chunks is: " + str(num_chunks))

    # signal.signal(signal.SIGUSR1, do_nothing)

    # client = boto3.client('s3', region_name='us-east-2',
    #     aws_secret_access_key="AWS_SECRET_KEY",
    #     aws_access_key_id="AWS_ACCESS_ID")
# **** PARALLEL EXECUTION WITH AIDBOTOCORE ****

    # Event driven approach
    # launch thread(0), right before compress, launch thread 1,
        # Create a "promise" object right before compress, pass it to thread 1
        # Create another "promise" object right before upload, pass it to thread 1
    # thread(1) reads, wait for compress_promise object to be resolved before doing it's compress

    # compress operation 

    session = aiobotocore.get_session()
    async with session.create_client('s3', region_name='us-east-2',
        aws_secret_access_key="AWS_SECRET_KEY",
        aws_access_key_id="AWS_ACCESS_ID") as client:

        tasks = []
        tasks.append(read_chunk(num_chunks-1, chunk_string, read_chunks, client, start))
        tasks.append(compress_chunk(num_chunks-1, read_chunks, compressed_chunks, start))
        tasks.append(upload_chunk(num_chunks-1, chunk_string, compressed_chunks, client, start))
        await asyncio.wait(tasks)
        # await asyncio.gather(*tasks)


    #     for i in range(0, num_chunks + 2):
    #         tasks = []

    #         if (i == 0):
    #             tasks.append(read_chunk(i, chunk_string, read_chunks, client, start))
    #         elif (i == 1):
    #             tasks.append(read_chunk(i, chunk_string, read_chunks, client, start))
    #             tasks.append(compress_chunk(i - 1, read_chunks, compressed_chunks, start))
    #         elif (i == num_chunks):
    #             tasks.append(compress_chunk(i - 1, read_chunks, compressed_chunks, start))
    #             tasks.append(upload_chunk(i - 2, chunk_string, compressed_chunks, client, start))
    #         elif (i == num_chunks + 1):
    #             tasks.append(upload_chunk(i - 2, chunk_string, compressed_chunks, client, start))
    #         else:
    #             tasks.append(read_chunk(i, chunk_string, read_chunks, client, start))
    #             tasks.append(compress_chunk(i - 1, read_chunks, compressed_chunks, start))
    #             tasks.append(upload_chunk(i - 2, chunk_string, compressed_chunks, client, start))

    #         await asyncio.wait(tasks)
    # os.kill(PID, signal.SIGUSR1)


    # **** SEQUENTIAL EXECUTION ****
    # for i in range(0, num_chunks + 2):
    #     if (i == 0):
    #         read_chunk(0, chunk_string, read_chunks, client, start)
    #     elif (i == 1):
    #         read_chunk(1, chunk_string, read_chunks, client, start)
    #         compress_chunk(0, read_chunks, compressed_chunks, start)
    #     elif (i == num_chunks):
    #         compress_chunk(num_chunks - 1, read_chunks, compressed_chunks, start)
    #         upload_chunk(num_chunks - 2, chunk_string, compressed_chunks, client, start)
    #     elif (i == num_chunks + 1):
    #         upload_chunk(num_chunks - 1, chunk_string, compressed_chunks, client, start)
    #     else:
    #         read_chunk(i, chunk_string, read_chunks, client, start)
    #         compress_chunk(i - 1, read_chunks, compressed_chunks, start)
    #         upload_chunk(i - 2, chunk_string, compressed_chunks, client, start)


    # signal.signal(signal.SIGUSR1, do_nothing)



    # # **** REMOVE OBJECTS FROM S3 ****
    # s3 = boto3.resource("s3")
    # for i in range (0, 200):
    #     obj = s3.Object("succinct-datasets", event['key1'] + "-chunk-compressed-" + str(i) +  ".succinct")
    #     obj.delete()
    #     obj = s3.Object("succinct-datasets", event['key1'] + "-chunk-" + str(i) +  ".succinct")
    #     obj.delete()

    print("File compression and upload is complete")
    print("read time: " + str(read_time))
    print("compress time: " + str(compress_time))
    print("upload time: " + str(upload_time))
    print("TOTAL DURATION: " + str(time.time() - start))
    print(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)

# modify recursion limit
sys.setrecursionlimit(1500)
loop = asyncio.get_event_loop()
loop.run_until_complete(execute())
loop.close()

# # **** CALCULATE COMPRESSION RATIO ****
# s3 = boto3.resource("s3", region_name='us-east-2',
#     aws_secret_access_key="AWS_SECRET_KEY",
#     aws_access_key_id="AWS_ACCESS_ID")
# bucket = s3.Bucket('succinct-datasets')
# orignal_sizes = 0
# compressed_sizes = 0
# num_chunks = 1167
# for i in range (0, num_chunks):
#     orignal_sizes += bucket.Object("sample.txt-chunk-" + str(i) + ".succinct").content_length
#     compressed_sizes += bucket.Object("sample.txt-chunk-compressed-" + str(i) + ".succinct").content_length

# print(orignal_sizes)
# print(compressed_sizes)
# print(orignal_sizes/compressed_sizes)
                                             



