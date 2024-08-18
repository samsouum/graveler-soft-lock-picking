# Optimized version using bitwise operations and multithreadding
# On my 12 threads this is a disappointing improvement of a
# factor ~20 from the original. That's 1600 times slower than the
# more or less identical implementation in c++.

# Performance on 1M: 4.8sec

import concurrent.futures
import time
import os

class Xorshift64:
    def __init__(self, seed):
        self.state = seed

    def next(self):
        x = self.state
        x ^= (x << 13) & 0xFFFFFFFFFFFFFFFF
        x ^= (x >> 7) & 0xFFFFFFFFFFFFFFFF
        x ^= (x << 17) & 0xFFFFFFFFFFFFFFFF
        self.state = x
        return x

def popcnt64(x):
    x = (x & 0x5555555555555555) + ((x >> 1) & 0x5555555555555555)
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333)
    x = (x & 0x0F0F0F0F0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F0F0F0F0F)
    x = (x & 0x00FF00FF00FF00FF) + ((x >> 8) & 0x00FF00FF00FF00FF)
    x = (x & 0x0000FFFF0000FFFF) + ((x >> 16) & 0x0000FFFF0000FFFF)
    x = (x & 0x00000000FFFFFFFF) + ((x >> 32) & 0x00000000FFFFFFFF)
    return x

def thread_action(n, seed):
    gen = Xorshift64(seed)
    local_max = 0
    for _ in range(n):
        value = popcnt64(gen.next() & gen.next())
        value += popcnt64(gen.next() & gen.next())
        value += popcnt64(gen.next() & gen.next())
        value += popcnt64(gen.next() & (gen.next() >> 25))
        if value > local_max:
            local_max = value
    return local_max

if __name__ == "__main__":
    n = int(1e6)
    num_threads = os.cpu_count()
    chunk_size = n // num_threads

    start_time = time.time()

    with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = [executor.submit(thread_action, chunk_size, 42 + i) for i in range(num_threads)]
        max_value = max(f.result() for f in concurrent.futures.as_completed(futures))

    total_time = time.time() - start_time

    print(f"Highest Ones Roll: {max_value}")
    print(f"Number of Roll Sessions: {n}")
    print(f"On {num_threads} Threads")
    print(f"Total Elapsed Time: {total_time:.3f}s")
