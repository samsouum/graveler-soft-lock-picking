/*
The purpose of this code is to simulate 1 billion simulations of 231
D4 throws and count the occurrences of the result 1.

The Idea is due to ShoddyCasts video
https://www.youtube.com/watch?v=M8C8dHQE2Ro whose simulation took
8 days to compute. They then challenged the viewers to improve
upon their code.

Having a little programming experience I took on this challenge with
a few ideas on improvements. I'm honestly surprised by how much I was
able to improve upon the initial code. Here is how it works:

A single D4 throw can be considerd as the result of the generation
of two random bits, where the result is consider a 1 if and only if
both bits are a 1. This can be checked quickly through the and
operation. The random numbers are generated through the well documented
xorshift algorithm (see https://en.wikipedia.org/wiki/Xorshift, 
https://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/).
The way the algorithm is used here (that millions of times per thread)
it passes all the necessary statistical tests to yield a "fair" result
in such a simulation. The script generates 2 256-bit numbers, making
use of AVXs as the SIMD bit-operations are extremely efficient in
modern CPUs. The numbers are combined with an and operation and a mask
to get rid of 25 bits yielding the 231 simulations of a D4. The function
popcnt counts the number of positive results using the algorithm described
in the article https://arxiv.org/pdf/1611.07612. The 1 billion simulations
are distributed on the threads available (on my ordinary laptop 12) and
local maxima are combined in the final result.

The final version does the calculation in 0.69sec. This is an improvement
of 100 million percent compared to the 8 days!

The github repository contains all improvements including what has been
changed between versions.
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <immintrin.h>
#include <stdint.h>

class Xorshift256 {
public:
    Xorshift256(__m256i seed) : state(seed) {}

    __m256i next() {
        state = _mm256_xor_si256(state, _mm256_slli_epi64(state, 13));
        state = _mm256_xor_si256(state, _mm256_srli_epi64(state, 7));
        state = _mm256_xor_si256(state, _mm256_slli_epi64(state, 17));

        return state;
    }

private:
    __m256i state;
};

__m256i inline clear_bottom_25_bits(__m256i v) {
    return _mm256_and_si256(v, _mm256_set_epi64x(0xFFFFFFFFFFE00000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL));
}

__m256i popcnt_epi64(__m256i v) {
    __m256i lookup = _mm256_setr_epi8 (0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 , 1 , 2 ,
    2 , 3 , 2 , 3 , 3 , 4 , 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
    1 , 2 , 2 , 3 , 2 , 3 , 3 , 4) ;
    __m256i low_mask = _mm256_set1_epi8(0x0f);
    __m256i lo = _mm256_and_si256(v,low_mask ) ;
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(v, 4), low_mask);
    __m256i popcnt1 = _mm256_shuffle_epi8 (lookup, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8 (lookup, hi);
    __m256i total = _mm256_add_epi8 (popcnt1, popcnt2);
    return _mm256_sad_epu8 (total, _mm256_setzero_si256());
}

int inline popcnt256(__m256i v) {
    __m256i popcnt = popcnt_epi64(v);
    uint64_t result[4];
    _mm256_storeu_si256((__m256i*)result, popcnt);
    return result[0] + result[1] + result[2] + result[3];
}

void thread_action(long long n, std::atomic<int>& max_value, __m256i seed){
    Xorshift256 gen(seed); // seed for the random number generator
    int local_max = 0;
    for (long long i = 0; i < n; ++i) {
        int value = popcnt256(clear_bottom_25_bits(_mm256_and_si256(gen.next(), gen.next())));
        //local_max -= (local_max - value) & ((local_max - value) >> 31);
        local_max = (value > local_max) ? value : local_max;
    }
    if (local_max > max_value){
        max_value = local_max;
    }
}

int main() {
    long long n = 1e9;
    //int k = 231; // 231 = 64 * 4 - 25

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::atomic<int> max_value(0);
    long long chunk_size = n / num_threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_action, chunk_size, std::ref(max_value), _mm256_setr_epi32(0,42 + 4 * i,0,43 + 4 * i,0,44 + 4 * i,0,45 + 4 * i));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    std::cout << "Highest Ones Roll: " << max_value << std::endl;
    std::cout << "Number of Roll Sessions: " << n << std::endl;
    std::cout << "On " << num_threads << " Threads" << std::endl;
    std::cout << "Total Elapsed Time: " << total_time.count() * 1e-3 << "s" << std::endl;
    
    return 0;
}