# Write a program to generate test cases for Flyodd warshall algorithm
# Line 1: N
# Line 2: N*N matrix, space separated with diagonal elements as 0, if no edge then -1, else all non-negative integers

# Write the output to a file 'input.txt'

import random

file = open('input.txt', 'w')

N = 100
file.write(str(N) + '\n')

for i in range(N):
    for j in range(N):
        if i == j:
            file.write('0 ')
        else:
            file.write(str(random.randint(-1, 1000)) + ' ')
    file.write('\n')
file.close()
