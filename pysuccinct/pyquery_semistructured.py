import pyquery_semistructured
import sys
import getopt

# Try catch block for non integer argument checking
def RepresentsInt(s):
    try: 
        int(s)
        return True
    except ValueError:
        return False

# Argument size error checking
argc = len(sys.argv)
if (argc < 2 or argc > 12):
    print("Usage: %s [-m mode] [file]")
    sys.exit(2)

# Loop through arguments to change default values and get input path using getopt
try:
    optlist, args = getopt.getopt(sys.argv[1:], 'm:')
except getopt.GetoptError as err:
    print("Get opt error")
    sys.exit(2)

# Default values
mode = 0
filename = ""

# Modify default values
for o, a in optlist:
    if o == "-m":
        mode = int(a)
    else:
        printf("Invalid Option")
        sys.exit(2)

# Process filename
if (len(args) != 1):
    print("Usage: %s [-m mode] [file]")
    sys.exit(2)
else:
    filename = str(args[0])

# Create the pyquery_file struct and run command given on next input
q = pyquery_semistructured.QuerySemistructured(filename, mode)
print("Done. Starting Succinct Shell...")
print("Command must be one of:\n\t\tsearch [attr_key] [attr_val]\n\t\tcount [attr_key] [attr_val]\n\t\tget [key] [attr_key]")

# Parse through line by line
while (True):
    line = input("succinct> ")
    line = line.split(" ")
    if (line[0] == "search"):
        if (len(line) != 3):
            print("Could not parse command: ")
        else:
            q.Search(line[1].strip(), line[2].strip())
    elif(line[0] == "count"):
        if (len(line) != 3):
            print("Could not parse command: ")
        else:
            q.Count(line[1].strip(), line[2].strip())
    elif(line[0] == "get"):
        if (len(line) == 1):
            print("Could not parse command: ")
            continue
        key = line[1].strip()
        attr_key = line[2].strip()
        if (len(line) != 3 or RepresentsInt(key) == False):
            print("Could not parse command: ")
        else:
            q.Get((int(key), attr_key))
    elif(line[0] == "exit"):
        break
    else:
        print("Unsupported command")
        print("Command must be one of:\n\t\tsearch [attr_key] [attr_val]\n\t\tcount [attr_key] [attr_val]\n\t\tget [key] [attr_key]")