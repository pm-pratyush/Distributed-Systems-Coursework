
# Read the input from the file 'input.txt'
file = open('input.txt', 'r')
N = int(file.readline())
matrix = []

for i in range(N):
    matrix.append(list(map(int, file.readline().split())))
file.close()

# Floyd Warshall Algorithm
for k in range(N):
    for i in range(N):
        for j in range(N):
            if matrix[i][k] != -1 and matrix[k][j] != -1:
                if matrix[i][j] == -1:
                    matrix[i][j] = matrix[i][k] + matrix[k][j]
                else:
                    matrix[i][j] = min(matrix[i][j], matrix[i][k] + matrix[k][j])

for (i, row) in enumerate(matrix):
    for (j, element) in enumerate(row):
        if element != 0:
            print(i, j, element)

# Take input from output.txt
file = open('output.txt', 'r')
output = []
for i in range(N):
    output.append(list(map(int, file.readline().split())))
file.close()

# Compare the output
flag = True
for i in range(N):
    for j in range(N):
        if matrix[i][j] != output[i][j]:
            print(matrix[i][j], output[i][j])
            flag = False
            break

if flag:
    print('Correct')
else:
    print('Incorrect')