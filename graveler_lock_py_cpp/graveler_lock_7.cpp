// Instead of thinking of a dice roll of 2 consecutive bits we can simply
// take the bits at the same location of two random numbers. This way
// we can reduce the number of bitwise operations needed leading to a
// speed up by over 30%.

// Performance on 100M: ~3.6sec

#include <iostream>
#include <random>
#include <chrono>

int popcnt32(int x) {
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
    return (x & 0x0000FFFF) + ((x >> 16) & 0x0000FFFF);
}

int main() {
    int n = 1e8;
    //int k = 231; // 231 = 32 * 7 + 7

    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(std::random_device{}());

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int rand_num = gen();
        rand_num = rand_num & 0x1555 & ((rand_num & 0x2aaa) >> 1);
        rand_num = (rand_num & 0x55) + (rand_num >> 4);
        rand_num = (rand_num & 0x33) + (rand_num >> 2 & 0x33);
        int value = (rand_num & 0x0f) + (rand_num >> 4 & 0x0f);
        for (int i = 0; i < 7; ++i) {
            value += popcnt32(gen() & gen());
        }
        if (value > max_value) {
            max_value = value;
        }
    }

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    std::cout << "Highest Ones Roll: " << max_value << std::endl;
    std::cout << "Number of Roll Sessions: " << n << std::endl;
    std::cout << "Total Elapsed Time: " << total_time.count() * 1e-3 << "s" << std::endl;
    
    return 0;
}