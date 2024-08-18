// This version finally implements multithreading. This is very
// Hardware dependant, which is why I've waited this long to
// include it. On my 12 threads the performance increase is close
// to a factor of 10.

// Performance on 100M: ~0.29sec
// Performance on 1B: ~2.9sec

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

class Xorshift64 {
public:
    Xorshift64(uint64_t seed) : state(seed) {}
    
    uint64_t next() {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }
    
private:
    uint64_t state;
};

int popcnt64(uint64_t x) {
    x = (x & 0x5555555555555555) + ((x >> 1) & 0x5555555555555555);
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    x = (x & 0x0F0F0F0F0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F0F0F0F0F);
    x = (x & 0x00FF00FF00FF00FF) + ((x >> 8) & 0x00FF00FF00FF00FF);
    x = (x & 0x0000FFFF0000FFFF) + ((x >> 16) & 0x0000FFFF0000FFFF);
    x = (x & 0x00000000FFFFFFFF) + ((x >> 32) & 0x00000000FFFFFFFF);

    return x;
}

void thread_action(int n, std::atomic<int>& max_value, int seed){
    Xorshift64 gen(seed); // seed for the random number generator
    int local_max = 0;
    for (int i = 0; i < n; ++i) {
        int value = popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & (gen.next() >> 25));
        if (value > local_max) {
            local_max = value;
        }
    }
    if (local_max > max_value){
        max_value = local_max;
    }
}

int main() {
    int n = 1e9;
    //int k = 231; // 231 = 64 * 4 - 25

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::atomic<int> max_value(0);
    int chunk_size = n / num_threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_action, chunk_size, std::ref(max_value), 42 + i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    std::cout << "Highest Ones Roll: " << max_value << std::endl;
    std::cout << "Number of Roll Sessions: " << n << std::endl;
    std::cout << "On " << num_threads << " Threads";
    std::cout << "Total Elapsed Time: " << total_time.count() * 1e-3 << "s" << std::endl;
    
    return 0;
}