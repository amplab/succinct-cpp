import pyquery_file
import sys
import getopt

def RepresentsInt(s):
    try: 
        int(s)
        return True
    except ValueError:
        return False

#Argument size error checking
argc = len(sys.argv)
if (argc < 2 or argc > 12):
    print("Invalid number of arguments \n")
    sys.exit(2)

#Loop through arguments to change default values and get input path using getopt
try:
    optlist, args = getopt.getopt(sys.argv[1:], 'm:')
except getopt.GetoptError as err:
    print("Get opt error\n")
    sys.exit(2)

#Default values
mode = 0
filename = ""

#Modify default values
for o, a in optlist:
    if o == "-m":
        mode = int(a)
    else:
        printf("Invalid Option")

#Process filename
if (len(args) != 1):
    #Should have 1 argument left for filename
    print("File path not found\n")
    sys.exit(2)
else:
    #Set the last unparsed element to the filename
    filename = str(args[0])

#Create the pyquery_file struct and run command given on next input
q = pyquery_file.QueryFile(filename, mode)
# q.ProcessCommands()

#parse through line by line
while (True):
    line = input("succinct> ")
    line = line.split(" ", 1)
    #Search command
    if (line[0] == "search"):
        if (len(line) != 2):
            print("Could not parse command: ")
        else:
            q.search(line[1].strip())
    elif(line[0] == "count"):
        if (len(line) != 2):
            print("Could not parse command: "])
        else:
            q.count(line[1].strip())
    elif(line[0] == "extract"):
        if (len(line) == 1):
            print("Could not parse command: ")
        line = line[1].split(" ", 1)
        offset = line[0].strip()
        length = line[1].strip()
        if (len(line) != 2 or RepresentsInt(offset) == False or RepresentsInt(length) == False):
            print("Could not parse command: ")
        else:
            q.extract(int(line[0].strip()), int(line[1].strip()))
    elif(line[0] == "exit"):
        break
    else:
        print("Unsupported command")
        print("Command must be one of: search [query], count [query], extract [offset] [length]")