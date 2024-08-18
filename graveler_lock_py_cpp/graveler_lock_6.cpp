// By making use of the whole 32 bit random number instead of 
// we save over 20%.

// Performance on 100M: ~4.7sec

#include <iostream>
#include <random>
#include <chrono>

int d4count(int number){
    number = number & 0x55555555 & ((number & 0xaaaaaaaa) >> 1);
    number = (number & 0x5555) + (number >> 16);
    number = (number & 0x3333) + ((number >> 2) & 0x3333);
    number = (number & 0x0F0F) + ((number >> 4) & 0x0F0F);
    return (number & 0x00FF) + ((number >> 8) & 0x00FF);
}

int main() {
    int n = 1e8;
    //int k = 231; // 231 = 16 * 14 + 7

    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(std::random_device{}());

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int rand_num = gen();
        rand_num = rand_num & 0x1555 & ((rand_num & 0x2aaa) >> 1);
        rand_num = (rand_num & 0x55) + (rand_num >> 4);
        rand_num = (rand_num & 0x33) + (rand_num >> 2 & 0x33);
        int value = (rand_num & 0x0f) + (rand_num >> 4 & 0x0f);
        for (int i = 0; i < 14; ++i) {
            value += d4count(gen());
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