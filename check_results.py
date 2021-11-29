THRESHOLD = 0.005
design = 2 # 1 = SW only, 2 = Inner Accel
if design == 1:
    file_results = open('Software_Only/software_only_log.txt')
elif design == 2:
    file_results = open('Inner_Accel/inner_accel_log.txt')

# Figure out 'size' and 'loops'
while True:
    line = file_results.readline()

    if line[0:6] == "matrix":
        line = line.replace(", ", " = ")
        line = line.split(" = ")
        size = int(line[1])
        loops = int(line[3])
        break

# Get correct 'cholesky_memory' file
file_true = open('memory_files/cholesky_memory/cholesky_mem_text_' + str(size) + 'x' + str(size) + '.txt')
# Read it all and split into individual values
trueChol = file_true.readline().split(",")

# Get line containing results
i = 0
testChol = []
while i < loops:
    line = file_results.readline()

    if line[0] != 'L' and line != '\n' and line[0] != "[":
        line = line.split(' ')
        line.remove('\n')
        testChol += line
        i += 1

# Convert all 'str' to 'float' and format to common sig figs
for i in range(size*size*loops):
    trueChol[i] = round(float(trueChol[i]), 3)
    testChol[i] = round(float(testChol[i]), 3)

# Check if results are correct
isCorrect = True
print("Incorrect values: ")
for i in range(size*size*loops):
    if abs(trueChol[i] - testChol[i]) > THRESHOLD:
        print("i = " + str(i) + "\tTrue = " + str(trueChol[i]) + "\tResult = " + str(testChol[i]))