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