//
// Created by sebastian on 12/2/20.
//

#ifndef INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H
#define INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H

static uint64_t s[2];

static inline uint64_t randLCG() {
    //LCG values from numerical recipes
    s[0] = 1664525 * s[0] + 1013904223;
    return s[0];
}

static inline uint64_t xorshift() {
    s[0] ^= (s[0] << 13);
    s[0] ^= (s[0] >> 17);
    s[0] ^= (s[0] << 5);
    return s[0];
}

static inline uint64_t xoroshiro128plus() {
    uint64_t s0 = s[0];
    uint64_t s1 = s[1];
    uint64_t result = s0 + s1;
    s1 ^= s0;
    s[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14);
    s[1] = (s1 << 36) | (s1 >> 28);
    return result;
}

static inline uint64_t xorshift64star() {
    uint64_t x = s[0];
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    s[0] = x;
    return x * UINT64_C(0x2545f4914f6cdd1d);
}

static inline uint64_t xorshift128plus() {
    uint64_t x = s[0];
    uint64_t y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s[1] + y;
}
#endif //INC_02_GETTING_AN_IMAGE_ON_THE_SCREEN_PRNGS_H
