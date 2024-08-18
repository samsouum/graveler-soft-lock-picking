# This script was written mostly to demonstrate the inefficiency of
# branching in python. The reason for this is that the CPU tries to
# read ahead which it cannot do or only worse at a branch point.
# Furthermore the script makes use of the built in functions of the
# random library.
# This leads to a speed up of a factor ~3

# Performance on 1M: ~30sec

import random
import time

n = 1000000
d4 = (1, 0, 0, 0)
k=231

start_time = time.time()

max_value = 0
for _ in range(n):
    value = sum(random.choices(d4, k=k))
    max_value = max(max_value, value)

total_time = time.time() - start_time

print(f"Highest Ones Roll: {max_value}")
print(f"Number of Roll Sessions: {n}")
print(f"Total Elapsed Time: {round(total_time, 1)}s")