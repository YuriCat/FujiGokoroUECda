#pragma once

#include <bitset>
#include <cassert>

template <typename T>
constexpr bool holdsBits(T a, T b) {
    return ((~a) & b) == T(0);
}
template <typename T>
constexpr bool isExclusive(T a, T b) {
    return (a & b) == T(0);
}

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

inline size_t bsf32(std::uint32_t v) {
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
template <> inline size_t bsr<std::uint64_t>(std::uint64_t v) { return bsr64(v); }

template <typename T> inline T lsb(T v) { return v & -v; }
template <typename T> inline T popLsb(T v) { return v & (v - T(1)); }
template <typename T> inline T rsb(T v) { return T(1) << bsr(v); }

constexpr size_t popcnt32CE(uint32_t v) {
    return v ? (1 + popcnt32CE(popLsb(v))) : 0;
}
constexpr size_t popcnt64CE(uint64_t v) {
    return v ? (1 + popcnt64CE(popLsb(v))) : 0;
}

template <typename T>
constexpr static T allLowerBits(T v) { // 最下位ビットより下位のビット全て
    return ~v & (v - T(1));
}
template <typename T> inline T allHigherBits(T a) {
    return ~((1U << bsr32(a)) - 1U);
}
template <> inline std::uint64_t allHigherBits(std::uint64_t a) {
    return ~((1ULL << bsr64(a)) - 1ULL);
}

template <typename T>
inline T lowestNBits(T v, size_t n) {
    T ans = 0;
    for (size_t i = 0; i < n; i++) {
        ans |= lsb(v);
        v = popLsb(v);
    }
    return ans;
}
template <typename T>
inline T highestNBits(T v, int n) {
    T ans = 0;
    for (size_t i = 0; i < n; i++) {
        T b = rsb(v);
        ans |= b;
        v -= b;
    }
    return ans;
}

constexpr uint64_t cross64(uint64_t a, uint64_t b) {
    return (a & 0x5555555555555555) | (b & 0xAAAAAAAAAAAAAAAA);
}

template <typename T, int W, int MAX_SIZE = sizeof(T) * 8> T fill_bits(T a);
template <typename T, int W, int MAX_SIZE = sizeof(T) * 8> T fill_bits_impl(T a, int n);

template <typename T, int W, int MAX_SIZE>
inline T fill_bits(T a) {
    return W <= 0 ? T(0) :
      (W >= MAX_SIZE ? a :
      (((fill_bits_impl<T, W, MAX_SIZE>(a, (MAX_SIZE - 1) / W)) << W) | a));
}

template <typename T, int W, int MAX_SIZE>
inline T fill_bits_impl(T a, int n) {
    return n <= 0 ? a :
      (W <= 0 ? T(0) :
      (W >= MAX_SIZE ? a :
      ((fill_bits_impl<T, W, MAX_SIZE>(a, n - 1) << W) | a)));
}

template <class dice64_t>
inline uint64_t pick1Bit64(uint64_t arg, dice64_t *const dice) {
    uint64_t tmp;
    while (1) {
        tmp = arg & dice->rand();
        if (tmp) {
            if (tmp & (tmp - 1ULL)) arg = tmp; // 2つ以上
            else break;
        }
    }
    return tmp;
}

template <class dice64_t>
static uint64_t pickNBits64NoPdep(uint64_t arg, int N0, int N1, dice64_t *const pdice) {
    // argからランダムにN0ビット抽出する
    // 最初はN0 + N1ビットある必要あり
    assert((int)popcnt(arg) == N0 + N1);
    
    uint64_t res = 0;

    if (N0 < N1) {
        if (N0 == 0) return res;
        if (N0 == 1) return pick1Bit64(arg, pdice);
    } else {
        if (N1 == 0) return arg;
        if (N1 == 1) return arg - pick1Bit64(arg, pdice);
    }
    while (1) {
        uint64_t dist = arg & pdice->rand();
        int NDist = popcnt(dist);
        
        // まずは一致チェック
        if (NDist == N0) {
            res |= dist; break;
        }
        if (NDist == N1) {
            res |= arg - dist; break;
        }
        if (NDist < N0) {
            if (NDist < N1) { // NDistが小さく、鋭い
                if (N0 > N1) {
                    res |= dist;
                    N0 -= NDist;
                } else N1 -= NDist;
                arg -= dist;
            } else { // 鈍い
                int NRDist = N0 + N1 - NDist;
                if (NDist > NRDist) { // NDistが大きい
                    N0 -= NDist;
                    res |= dist;
                    arg -= dist;
                } else { // NRDistが大きい
                    N0 -= NRDist;
                    res |= arg - dist;
                    arg = dist;
                }
            }
        } else {
            if (NDist < N1) { // 鈍い
                int NRDist = N0 + N1 - NDist;
                if (NDist > NRDist) { // NDistが大きい
                    N1 -= NDist;
                    arg -= dist;
                } else { // NRDistが大きい
                    N1 -= NRDist;
                    arg = dist;
                }
            } else { // NDistが大きく、鋭い
                if (N0 > N1) {
                    res |= arg - dist;
                    N0 = NDist - N1;
                } else N1 = NDist - N0;
                arg = dist;
            }
        }
    }
    return res;
}

template <int N, class array_t, class dice64_t>
static void dist64(uint64_t *const dst, uint64_t arg, const array_t& argNum, dice64_t *const dice) {
    static_assert(N > 0, "");
    if (N == 1) dst[0] = arg;
    else {
        const int NH = N / 2;
        int num[2] = {0};
        for (int i = 0; i < NH; i++) num[0] += argNum[i];
        for (int i = NH; i < N; i++) num[1] += argNum[i];
        assert(num[0] + num[1] == (int)popcnt64(arg));
        uint64_t half[2] = {0};
        dist64<2>(half, arg, num, dice);
        dist64<NH>(dst, half[0], argNum, dice);
        dist64<N - NH>(dst + NH, half[1], rightShift(argNum, NH), dice);
    }
}

template <size_t N>
bool isOnlyValue(std::bitset<N> bs, size_t i) {
    bs.reset(i);
    return bs.count() == 0;
}

class XorShift64 {
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
    void srand(uint64_t s) {
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
    XorShift64(uint64_t s): x(), y(), z(), t() { srand(s); }
};

//constexpr int ipow(int m, int n) { return n <= 0 ? 1 : (m * ipow(m, n - 1)); }

template <typename T, size_t B, size_t N>
class MiniBitArray {
    static_assert(sizeof(T) * 8 >= B * N, "");
    static constexpr T MASK_BASE = (T(1) << B) - T(1);
    static constexpr size_t place(const size_t p) { return p * N; }
    static constexpr T mask(const size_t p) { return MASK_BASE << place(p); }
    static constexpr T mask_lower(const size_t p) { return (T(1) << place(p)) - T(1); }
    static constexpr T mask_range(const size_t p0, const size_t p1) {
        return mask_lower(p1) & ~mask_lower(p0);
    }
    static constexpr T mask_full() { return mask_lower(N); }
public:
    using data_type = T;
    void clear() { data_ = T(0); }
    void fill(T v) { data_ = fill_bits<T, N>(v); }
    void set(size_t p) {
        assert(0 <= p && p < N);
        data_ |= mask(p);
    }
    void assign(size_t p, T v) {
        assert(0 <= p && p < N);
        data_ = (data_ & ~mask(p)) | (v << place(p));
    }
    void set(size_t p, T v) {
        assert(0 <= p && p < N);
        data_ |= (v << place(p));
    }
    void add(size_t p, T v) {
        assert(0 <= p && p < N);
        data_ += (v << place(p));
    }
    void sub(size_t p, T v) {
        assert(0 <= p && p < N);
        data_ -= (v << place(p));
    }
    MiniBitArray& operator >>=(size_t p) {
        data_ >>= place(p);
        return *this;
    }
    constexpr T operator [](size_t p) const { return (data_ >> place(p)) & MASK_BASE; }
    constexpr T part(size_t p) const { return data_ & (MASK_BASE << place(p)); }
    constexpr bool any() const { return data_ != 0; }
    constexpr bool empty() const { return !any(); }
    constexpr bool operator ==(const MiniBitArray<T, B, N>& bs) const {
        return data_ == bs.data_;
    }
    constexpr bool operator !=(const MiniBitArray<T, B, N>& bs) const {
        return !(*this == bs);
    }
    constexpr T data() const { return data_; }
    constexpr MiniBitArray(): data_() {}
    constexpr MiniBitArray(T data): data_(data) {}
    constexpr MiniBitArray(const MiniBitArray<T, B, N>& ba): data_(ba.data_) {}
    static constexpr size_t size() { return N; }
protected:
    T data_;
};

template <typename T, size_t B, size_t N>
MiniBitArray<T, B, N> invert(const MiniBitArray<T, B, N>& ba) {
    MiniBitArray<T, B, N> ret(0);
    for (size_t i = 0; i < N; i++) ret.set(ba[i], i);
    return ret;
}
template <typename T, size_t B, size_t N>
MiniBitArray<T, B, N> rightShift(MiniBitArray<T, B, N> ba, size_t n) {
    ba >>= n;
    return ba;
}

template <typename T, size_t B, size_t N>
static std::ostream& operator <<(std::ostream& out, const MiniBitArray<T, B, N>& ba) {
    out << "{";
    size_t cnt = 0;
    for (size_t i = 0; i < ba.size(); i++) {
        if (cnt++ > 0) out << ",";
        out << uint64_t(ba[i]);
    }
    out << "}";
    return out;
}

template <size_t B, size_t N = 32 / B> using BitArray32 = MiniBitArray<std::uint32_t, B, N>;
template <size_t B, size_t N = 64 / B> using BitArray64 = MiniBitArray<std::uint64_t, B, N>;

static std::vector<std::string> split(const std::string& s,
                                      char separator) {
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    while ((q = s.find(separator, p)) != std::string::npos) {
        result.emplace_back(s, p, q - p);
        p = q + 1;
    }
    result.emplace_back(s, p);
    return result;
}
static std::vector<std::string> split(const std::string& s,
                                      const std::string& separators) {
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    
    bool found;
    do {
        found = false;
        std::string::size_type minq = s.size();
        for (char sep : separators) {
            //cerr << "separator : " << sep << endl;
            if ((q = s.find(sep, p)) != std::string::npos) {
                minq = min(minq, q);
                found = true;
            }
        }
        if (found) {
            result.emplace_back(s, p, minq - p);
            p = minq + 1;
        }
    } while(found);
    result.emplace_back(s, p);
    return result;
}
