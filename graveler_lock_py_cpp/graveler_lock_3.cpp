// Instead of throwing away all the randomness created through hard work
// this algorithm makes use of the first 22 bits leading to 11 random
// dice rolls. A dice roll consists of two consecutive bits in a random number.
// This leads to an performance boost by a factor of 2 (not
// as one might expect 11, since an additional loop and calculations are needed)
// Thus the random number generation is no longer the bottleneck but
// rather the counting function.

// Performance on 1M: ~0.18sec
// Performance on 100M: ~18sec

#include <iostream>
#include <random>
#include <chrono>

int main() {
    int n = 1e8;
    //int k = 231; // 231 = 11 * 21
    int l = 11;
    int m = 21;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(std::random_device{}());

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int value = 0;
        for (int j = 0; j < m; ++j) {
            int rand_num = gen();
            for (int j = 0; j < l; ++j) {
                value += (rand_num % 4 == 0);
                rand_num /= 4;
            }
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