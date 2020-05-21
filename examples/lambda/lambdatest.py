import file
import kvstore
import semistructuredstore

# compress file
def call_compress (event, context):
   # q = file.File(inputpath, 32, 32, 128, 0, 1)
    print("success")

def call_query (event, context):
    print("success")

print(call_compress(None, None))