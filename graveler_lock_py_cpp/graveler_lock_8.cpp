// The xorshift is a good and extremely fast method to generate
// random numbers in only a few bit operations. Despite its
// simplicity it still passes many statistical test such as the
// ones needed to generate "fair" results in this kind of simulation.
// We're now at the point where it is difficult to make serious
// improvements without taking hardware into consideration.

// Performance on 100M: ~2.2sec
// Performance on 1B: ~22sec

#include <iostream>
#include <chrono>

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

int main() {
    int n = 1e9;
    //int k = 231; // 231 = 64 * 4 - 25

    Xorshift64 gen(42); // seed for the random number generator

    auto start_time = std::chrono::high_resolution_clock::now();

    int max_value = 0;
    for (int i = 0; i < n; ++i) {
        int value = popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & gen.next());
        value += popcnt64(gen.next() & (gen.next() >> 25));
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