import pycompress
import sys
import getopt

# Argument size error checking
argc = len(sys.argv)
if (argc < 2 or argc > 12):
    print("Usage: %s [-m mode] [file]")
    sys.exit(2)

# Loop through arguments to change default values and get input path using getopt
try:
    optlist, args = getopt.getopt(sys.argv[1:], 's:i:x:n:r:t:')
except getopt.GetoptError as err:
    print("Get opt error")
    sys.exit(2)

# Default values
sa_sampling_rate = 32
isa_sampling_rate = 32
sampling_scheme = 0
npa_sampling_rate = 128
npa_encoding_scheme = 1
type = "file"
inputpath = ""

# Modify default values
for o, a in optlist:
    if o == "-s":
        sa_sampling_rate = int(a)
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
        printf("Invalid Option")
        sys.exit(2)


# Process input path
if (len(args) != 1):
    print("Usage: %s [-m mode] [file]")
    sys.exit(2)
else:
    inputpath = str(args[0])

# Create the file struct and compress
file = pycompress.File(inputpath, sa_sampling_rate, 
    isa_sampling_rate, npa_sampling_rate, 
    sampling_scheme, npa_encoding_scheme)

# Compress file or shard depending on type
if (type == "file"):
    file.CompressFile()
elif (type == "kv"):
    file.CompressShard()
else:
    print("Invalid type\n")
    sys.exit(2)
