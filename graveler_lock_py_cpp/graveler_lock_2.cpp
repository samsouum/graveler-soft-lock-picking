// A better random number generator can lead to significan improvements.
// Simply using the Mersenne Twister results in a speed up by almost
// a factor of 10. This shows in particular that the random number generation
// is the bottleneck of the script.

// Performance on 1M: ~0.4sec
// Performance on 100M: ~40sec

#include <iostream>
#include <random>
#include <chrono>

int main() {
    int n = 1e8;
    int k = 231;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 rng(std::random_device{}()); // Mersenne Twister RNG
    std::uniform_int_distribution<int> dist(0, 3);

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int value = 0;
        for (int j = 0; j < k; ++j) {
            value += (dist(rng) == 0);
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