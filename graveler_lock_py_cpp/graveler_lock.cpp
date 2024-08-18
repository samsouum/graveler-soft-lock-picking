// This implementation is basically the direct translation of the slightly 
// optimized implementation in Python.
// It performs around a factor 10 better than the previous script.

// Performance on 1M: ~3sec
// Performance on 100M: ~5min

#include <iostream>
#include <chrono>

int main() {
    int n = 1e6;
    int k = 231;

    auto start_time = std::chrono::high_resolution_clock::now();

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int value = 0;
        for (int j = 0; j < k; ++j) {
            value += (rand() % 4 == 0);
        }
        if (value > max_value){
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