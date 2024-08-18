// Bitwise operations are also useful in counting the number of
// 1 rolls. This is basically the popcnt function. This leads to
// another improvement of a factor ~2

// Performance on 1M: ~0.06sec
// Performance on 100M: ~6sec

#include <iostream>
#include <random>
#include <chrono>


int main() {
    int n = 1e8;
    //int k = 231; // 231 = 11 * 21
    int m = 21;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(std::random_device{}());

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int value = 0;
        for (int j = 0; j < m; ++j) {
            int rand_num = gen();
            rand_num = rand_num & 0x155555 & ((rand_num & 0x2aaaaa) >> 1);
            rand_num = (rand_num & 0x5555) + (rand_num >> 16);
            rand_num = (rand_num & 0x3333) + ((rand_num >> 2) & 0x3333);
            rand_num = (rand_num & 0x0F0F) + ((rand_num >> 4) & 0x0F0F);
            value += (rand_num & 0x00FF) + ((rand_num >> 8) & 0x00FF);
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