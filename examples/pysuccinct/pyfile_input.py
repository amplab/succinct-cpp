import file
import sys
import getopt
import os

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

        # # Get input size
        # FILE *f = fopen(filename.c_str(), "r")
        # fseek(f, 0, SEEK_END)
        # uint64_t fsize = ftell(f)
        # fseek(f, 0, SEEK_SET)

        # # Read input from file
        # auto *data = (uint8_t *) s_allocator.s_malloc(fsize + 1)
        # fread(data, fsize, 1, f)
        # fclose(f)
        # data[fsize] = 1
        # uint8_t *, size_t, uint32_t, uint32_t, int32_t, uint32_t, int, int, uint32_t>

        # context_len = 3 or sampling_range = 1024

        f = open(inputpath, "r")
        input = f.read()

        q = file.File(0, input, sa_sampling_rate, 
            isa_sampling_rate, npa_sampling_rate,
            sampling_scheme, npa_encoding_scheme)

        content = (q.GetContent().tobytes())
        # print(str(content,'ISO-8859-1'))
        text_file = open(inputpath + ".succinct", "wb")
        text_file.write(content)

    else:
        print("Usage: [load/compress] [file]\n")
        sys.exit(2)

