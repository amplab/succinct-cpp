# Declare variables
column_12 = []
output_chunks = []
num_records = 100000

# Open file
f = open("sample.txt", "r", encoding = "ISO-8859-1")

# Parse input
for line in f:
    last_item = (line.split())[-1]
    column_12.append(last_item)

# Separate column_12 list into chunks of size num_records
for i in range(0, len(column_12), num_records):
    output_chunks.append(column_12[i:i+num_records])

# Output these chunks into their own files
for i in range(0, len(output_chunks)):
    output_file = open("sample.txt-chunk-" + str(i) + ".succinct", "w")
    for j in range(0, len(output_chunks[i])):
        output_file.write(str(output_chunks[i][j]) + "\n")