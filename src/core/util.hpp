#pragma once

inline size_t popcnt32(std::uint32_t v) {
    return __builtin_popcount(v);
}
inline size_t popcnt64(std::uint64_t v) {
    return __builtin_popcountll(v);
}
template <typename T> inline size_t popcnt(T a);
template <> inline size_t popcnt<std::uint8_t >(std::uint8_t  v) { return popcnt32(v); }
template <> inline size_t popcnt<std::uint16_t>(std::uint16_t v) { return popcnt32(v); }
template <> inline size_t popcnt<std::uint32_t>(std::uint32_t v) { return popcnt32(v); }
template <> inline size_t popcnt<std::uint64_t>(std::uint64_t v) { return popcnt64(v); }

/*inline size_t bsf32(std::uint32_t v) {
    return __builtin_ctz(v);
}
inline size_t bsf64(std::uint64_t v) {
    return __builtin_ctzll(v);
}
inline size_t bsr32(std::uint32_t v) {
    std::int32_t r;
    __asm__("bsrl %1, %0;" :"=r"(r) : "r"(v));
    return r;
}
inline size_t bsr64(std::uint64_t v) {
    std::int64_t r;
    __asm__("bsrq %1, %0;" :"=r"(r) : "r"(v));
    return r;
}

template <typename T> inline size_t bsf(T v);
template <typename T> inline size_t bsr(T v);

template <> inline size_t bsf<std::uint8_t >(std::uint8_t  v) { return bsf32(v); }
template <> inline size_t bsf<std::uint16_t>(std::uint16_t v) { return bsf32(v); }
template <> inline size_t bsf<std::uint32_t>(std::uint32_t v) { return bsf32(v); }
template <> inline size_t bsf<std::uint64_t>(std::uint64_t v) { return bsf64(v); }
template <> inline size_t bsr<std::uint8_t >(std::uint8_t  v) { return bsr32(v); }
template <> inline size_t bsr<std::uint16_t>(std::uint16_t v) { return bsr32(v); }
template <> inline size_t bsr<std::uint32_t>(std::uint32_t v) { return bsr32(v); }
template <> inline size_t bsr<std::uint64_t>(std::uint64_t v) { return bsr64(v); }*/

template <typename T>
T popLsb(T v) { return v & (v - T(1)); }

template <size_t N>
bool isOnlyValue(std::bitset<N> bs, size_t i) {
    bs.reset(i);
    return bs.count() == 0;
}

class XorShift64{
private:
    uint64_t x, y, z, t;
public:
    uint64_t rand() {
        uint64_t tmp = x ^ (x << 11);
        x = y; y = z; z = t;
        t = (t ^ (t >> 19)) ^ (tmp ^ (tmp >> 8));
        return t;
    }
    double drand() {
        return rand() / double(0xFFFFFFFFFFFFFFFFULL);
    }
    void srand(const uint64_t s) {
        // seedが0だとまずい
        if (!s) x = 0x0123456789ABCDEFULL;
        else x = (s << 32) ^ s;
        y = (x << 8) | ((x & 0xff00000000000000ULL) >> 56);
        z = (y << 8) | ((y & 0xff00000000000000ULL) >> 56);
        t = (z << 8) | ((z & 0xff00000000000000ULL) >> 56);
    }
    constexpr static uint64_t min() { return 0ULL; }
    constexpr static uint64_t max() { return 0xFFFFFFFFFFFFFFFFULL; }

    constexpr XorShift64(): x(), y(), z(), t() {}
    XorShift64(const uint64_t s): x(), y(), z(), t() { srand(s); }
};

template <typename T, size_t N>
static std::array<T, N> invert(const std::array<T, N>& a, size_t n = N) {
    std::array<T, N> r;
    for (size_t i = 0; i < n; i++) r[a[i]] = i;
    for (size_t i = n; i < N; i++) r[i] = a[i];
    return r;
}

template <typename T, size_t N>
static std::vector<T> a2v(const std::array<T, N>& a) {
    std::vector<T> v;
    for (const T& val : a) v.push_back(val);
    return v;
}

#include "../../CppCommon/src/util/string.hpp"
#include "../../CppCommon/src/util/random.hpp"
#include "../../CppCommon/src/util/bitOperation.hpp"
#include "../../CppCommon/src/util/container.hpp"
#include "../../CppCommon/src/util/arrays.h"
#include "../../CppCommon/src/util/bitSet.hpp"
#include "../../CppCommon/src/util/bitArray.hpp"
#include "../../CppCommon/src/util/atomic.hpp"
#include "../../CppCommon/src/util/bitPartition.hpp"
#include "../../CppCommon/src/util/math.hpp"
#include "../../CppCommon/src/hash/hashFunc.hpp"
#include "../../CppCommon/src/hash/hashBook.hpp"
#include "../../CppCommon/src/util/softmaxClassifier.hpp"
#include "../../CppCommon/src/util/selection.hpp"
#include "../../CppCommon/src/util/lock.hpp"
#include "../../CppCommon/src/util/io.hpp"
#include "../../CppCommon/src/util/pd.hpp"
#include "../../CppCommon/src/util/statistics.hpp"
#include "../../CppCommon/src/util/index.hpp"
#include "../../CppCommon/src/analyze/analyzer.hpp"