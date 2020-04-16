import pycompress
import sys
import getopt

#Loop through arguments to change default values and get input path using getopt
try:
    optlist, args = getopt.getopt(sys.argv[1:], 's:i:x:n:r:t:')
except getopt.GetoptError as err:
        sys.exit(2)

#Default values
sa_sampling_rate = 32
isa_sampling_rate = 32
sampling_scheme = 0
npa_sampling_rate = 128
npa_encoding_scheme = 1
type = "file"
inputpath = "test.txt"

#Modify default values
for o, a in optlist:
    if o == "-s":
        sa_sampling_rate = int(a)
        #print("sampling rate is ", sa_sampling_rate, "\n")
    elif o == "-i":
        isa_sampling_rate = int(a)
    elif o == "-x":
        sampling_scheme = int(a)
    elif o == "-n":
        npa_sampling_rate = int(a)
    elif o == "-r":
        npa_encoding_scheme = int(a)
    elif o == "-t":
        type = a
    else:
        assert False, "unhandled option"


#Process input path
if (len(args) != 1):
    #Should have 1 argument left for input path
    sys.exit(2)
else:
    #Set the last unparsed element to the input path
    inputpath = str(args[0])

#Create the file struct and compress
file = pycompress.File(inputpath, sa_sampling_rate, 
    isa_sampling_rate, npa_sampling_rate, 
    sampling_scheme, npa_encoding_scheme)

#Compress file or shard depending on type
if (type == "file"):
    #Compress file
    file.compressFile()
elif (type == "kv"):
    #Compress shard
    file.compressShard()
else:
    #Error
    print("Invalid type\n")
