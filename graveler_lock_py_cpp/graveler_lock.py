# Original script with time measurement

# Performance on 1M: ~100sec

import random
import math
from itertools import repeat
import time

items = [1,2,3,4]
numbers = [0,0,0,0]
rolls = 0
maxOnes = 0

start_time = time.time()

while numbers[0] < 177 and rolls < 1000000:
    numbers = [0,0,0,0]
    for i in repeat(None, 231):
        roll = random.choice(items)
        numbers[roll-1] = numbers[roll-1] + 1
    rolls = rolls + 1
    if numbers[0] > maxOnes:
        maxOnes = numbers[0]

total_time = time.time() - start_time

print(f"Highest Ones Roll: {maxOnes}")
print(f"Number of Roll Sessions: {rolls}")
print(f"Total Elapsed Time: {round(total_time, 1)}s")