import file
import sys
import getopt

# Try catch block for non integer argument checking
def RepresentsInt(s):
    try: 
        int(s)
        return True
    except ValueError:
        return False

# Default values
sa_sampling_rate = 32
isa_sampling_rate = 32
sampling_scheme = 0
npa_sampling_rate = 128
npa_encoding_scheme = 1
type = "file"
inputpath = ""

# Get user input to either load from memory or compress a new file
option = input("Usage: [load/compress] [file]\n")
option = option.split()
if (len(option) != 2):
    print("Usage: [load/compress] [file]\n")
    sys.exit(2)
else:
    inputpath = option[1]
    if (option[0] == "load"):
        # Load file from memory
        print("loading ", inputpath, " from file")
        q = file.File(inputpath)
    elif (option[0] == "compress"):
        # Compress the file
        print("Please enter the sampling rates")
        option = input("Usage: [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-n npa_sampling_rate] [-r npa_encoding_scheme] [-t input_type]\n")
        # Loop through arguments to get sampling rates using getopt
        try:
            optlist, args = getopt.getopt(option, 's:i:x:n:r:t:')
        except getopt.GetoptError as err:
            print("Get opt error")
            sys.exit(2)
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
        q = file.File(inputpath, sa_sampling_rate, 
            isa_sampling_rate, npa_sampling_rate, 
            sampling_scheme, npa_encoding_scheme)
    else:
        print("Usage: [load/compress] [file]\n")
        sys.exit(2)

print("Command must be one of: search [query], count [query], extract [offset] [length]")

# Parse through line by line
while (True):
    line = input("succinct> ")
    line = line.split(" ", 1)
    if (line[0] == "search"):
        if (len(line) != 2):
            print("Could not parse command: ")
            continue
        else:
            print(q.Search(line[1].strip()))
    elif(line[0] == "count"):
        if (len(line) != 2):
            print("Could not parse command: ")
            continue
        else:
            print(q.Count(line[1].strip()))
    elif(line[0] == "extract"):
        if (len(line) == 1):
            print("Could not parse command: ")
            continue
        line = line[1].split()
        offset = line[0].strip()
        length = line[1].strip()
        if (len(line) != 2 or RepresentsInt(offset) == False or RepresentsInt(length) == False):
            print("Could not parse command: ")
        else:
            print(q.Extract(int(line[0].strip()), int(line[1].strip())))
    elif(line[0] == "exit"):
        break
    else:
        print("Unsupported command")
        print("Command must be one of: search [query], count [query], extract [offset] [length]")