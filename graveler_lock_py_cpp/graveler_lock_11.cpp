/*
I don't have cpp on here but I have thought of further improvements which I will have to test later.
Improvements:
    - The mask to remove 25 bits can be built into the bitcount function.
    - Use the following function to sum up the results of the 64 bits.
            __m128i sum1_low = _mm256_extracti128_si256(sum1, 0);
            __m128i sum1_high = _mm256_extracti128_si256(sum1, 1);
        
            // Sum the 64-bit integers within each 128-bit half
            __m128i sum2 = _mm_add_epi64(sum1_low, sum1_high);
        
            // Sum the results from the two halves
            uint64_t val[2];
            _mm_storeu_si128((__m128i*)val, sum2);
            return val[0] + val[1];
    - Different way to sum up the results: Generate 29 * 2 numbers, and them, shift one by a bit (to get
        to 231 = 29 * 8 - 1), use the popcnt up to the last line. Store this as the "result". For each
        result use the _mm256_max_epu8 function to get the maximum. Finally use the last line of popcnt
        and the last 3 lines of popcnt256.
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
    __m256i lo = _mm256_and_si256(v, low_mask);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(v, 4), low_mask);
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi);
    __m256i total = _mm256_add_epi8(popcnt1, popcnt2);
    return _mm256_sad_epu8(total, _mm256_setzero_si256());
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
