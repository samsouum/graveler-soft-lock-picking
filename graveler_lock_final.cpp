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

The final version does the calculation in 0.52sec. This is an improvement
of over 130 million percent compared to the 8 days!

Can the code be improved even further?
    In terms of single thread speed the code is pretty much at the limit
    of what is doable in c++ when staying with ordinary instructions
    and a random number generation that passes the necessary statistical
    tests. With multi-threading there is still a lot to gain though. 
    With clever code that could run on many GPU threads there are still
    multiple orders of magnitude to gain in terms of speed. However as
    I am just a mathematician, with enough coding experience to 
    be dangerous, I don't have the knowledge and experience to write
    GPU instructions and leave it up to others to do so. It is noteworthy
    for anyone trying this to keep the number of RNG calls per thread
    in mind. If the value gets low the xorshift algorithm will no longer
    pass the needed statistical tests and may cause mathematically
    unprobable results. In this case I suggest using the almost as
    performant PCG algorithm
https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/.
    Good luck!

For anyone running the code on their device:
    - Remember to use the O3 or Ofast optimization flags.
    - If there is a problem with the __m256i data type your CPU
      probably doesn't support AVX's. In this case you can try to
      run the slightly less efficient version 9 which should work
      on every device.

The github repository contains all improvements including what has been
changed between versions.
https://github.com/samsouum/graveler-soft-lock-picking/tree/main
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
