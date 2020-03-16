//
// Created by sebastian on 12/2/20.
//

#ifndef INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H
#define INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H



static volatile uint64_t prngState[2];

static inline uint64_t randLCG() {
    //LCG values from numerical recipes
    prngState[0] = 1664525 * prngState[0] + 1013904223;
    return prngState[0];
}

static inline uint64_t xorshift() {
    prngState[0] ^= (prngState[0] << 13);
    prngState[0] ^= (prngState[0] >> 17);
    prngState[0] ^= (prngState[0] << 5);
    return prngState[0];
}

static inline uint64_t xoroshiro128plus() {
    uint64_t s0 = prngState[0];
    uint64_t s1 = prngState[1];
    uint64_t result = s0 + s1;
    s1 ^= s0;
    prngState[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14);
    prngState[1] = (s1 << 36) | (s1 >> 28);
    return result;
}

static inline uint64_t xorshift64star() {
    uint64_t x = prngState[0];
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    prngState[0] = x;
    return x * UINT64_C(0x2545f4914f6cdd1d);
}

static inline uint64_t xorshift128plus() {
    uint64_t x = prngState[0];
    uint64_t y = prngState[1];
    prngState[0] = y;
    x ^= x << 23;
    prngState[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return prngState[1] + y;
}



#endif //INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H
