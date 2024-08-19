/*
Despite having already submitted the last version I've found
another point of improvement. Just by rearranging the order
in which the operations are made I can save a few operations
per simulation. This led to an improvement of around 30% and
will be the last one I make.

// Performance on 1B: ~0.51sec
// Performance on 10B: ~5.1sec
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

__m256i inline popcnt_epi8_mask(__m256i v) {
    __m256i lookup = _mm256_setr_epi8 (0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 , 1 , 2 ,
    2 , 3 , 2 , 3 , 3 , 4 , 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
    1 , 2 , 2 , 3 , 2 , 3 , 3 , 4) ;
    __m256i low_mask = _mm256_set1_epi8(0x0f);
    __m256i lo = _mm256_and_si256(v, low_mask) ;
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(v, 4), _mm256_set1_epi64x(0x070F));
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi);
    return _mm256_add_epi8(popcnt1, popcnt2);
}

__m256i inline popcnt_epi8(__m256i v) {
    __m256i lookup = _mm256_setr_epi8 (0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 , 1 , 2 ,
    2 , 3 , 2 , 3 , 3 , 4 , 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
    1 , 2 , 2 , 3 , 2 , 3 , 3 , 4) ;
    __m256i low_mask = _mm256_set1_epi8(0x0f);
    __m256i lo = _mm256_and_si256(v,low_mask ) ;
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(v, 4), low_mask);
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi);
    return _mm256_add_epi8(popcnt1, popcnt2);
}

void thread_action(int n, std::atomic<int>& max_value, __m256i seed){
    Xorshift256 gen(seed);
    int local_max = 0;
    __m256i local_max_epi8 = _mm256_setzero_si256();
    for (int i = 0; i < n; ++i) {
        __m256i total = popcnt_epi8_mask(_mm256_and_si256(gen.next(), gen.next()));
        for (int j = 0; j < 3; ++j) {
            total = _mm256_add_epi8(popcnt_epi8(_mm256_and_si256(gen.next(), gen.next())), total);
        }
        total = _mm256_sad_epu8(total, _mm256_setzero_si256());
        local_max_epi8 = _mm256_max_epu8(local_max_epi8, total);
    }
    uint64_t result[4];
    _mm256_storeu_si256((__m256i*)result, local_max_epi8);
    for (int i = 1; i < 4; ++i){
        result[0] = (result[i] > result[0]) ? result[i] : result[0];
    }
    if (result[0] > max_value){
        max_value = result[0];
    }
}

int main() {
    long long n = 1e9;

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::atomic<int> max_value(0);
    int chunk_size = n / num_threads / 4;

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
