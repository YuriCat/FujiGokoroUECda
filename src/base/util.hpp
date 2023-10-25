#pragma once

#include <sys/time.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// 条件x、命令等y
#ifdef NDEBUG
#define ASSERT(X, Y)
#define FASSERT(f, o)
#define FEQUALS(f0, f1, o)
#else
#define ASSERT(X, Y)  if(!(X)){ Y; assert(0); }
// 浮動小数点がまともな値を取っているかどうかのアサーション
#define FASSERT(f, o) if(!(!std::isinf(f) && !std::isnan(f))){ std::cerr << (f) << std::endl; {o}\
assert(!std::isinf(f) && !std::isnan(f)); assert(0); };
// 浮動小数点が「ほぼ」同じ値を取っているかのチェック && FASSERT
#define FEQUALS(f0, f1, o) { if(!(!std::isinf(f0) && !std::isnan(f0))){ std::cerr << (f0) << std::endl; {o}\
assert(!std::isinf(f0) && !std::isnan(f0)); assert(0); };\
if(!(!std::isinf(f1) && !std::isnan(f1))){ std::cerr << (f1) << std::endl; {o}};\
assert(!std::isinf(f1) && !std::isnan(f1)); assert(0); };\
if(!(abs((f0) - (f1)) <= 0.00001)){ std::cerr << (f0) << " <-> " << (f1) << std::endl; {o}\
assert(abs((f0) - (f1)) <= 0.00001); assert(0); }
#endif // NDEBUG

// 標準ライブラリ使用
using std::size_t;

// 出力
#ifdef DEBUG
#define DERR std::cerr
#else
#define DERR 0 && std::cerr
#endif

#ifndef MINIMUM
#define CERR std::cerr
#else
#define CERR 0 && std::cerr
#endif

static double internal_clock_sec() {
    return double(clock()) / CLOCKS_PER_SEC;
}

static double clock_sec() {
    // clock() is insuffient in multi thread procedure
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

static uint64_t cputime() {
    unsigned int ax, dx;
    asm volatile("rdtsc\nmovl %%eax,%0\nmovl %%edx,%1":"=g"(ax),"=g"(dx): :"eax","edx");
    return ((unsigned long long)(dx) << 32) + (unsigned long long)(ax);
}

class Clock {
private:
    uint64_t c_start;
public:
    void start() { c_start = cputime(); }
    uint64_t stop() const { return cputime() - c_start; }
    uint64_t restart() { // 結果を返し、0から再スタート
        uint64_t tmp = cputime();
        uint64_t diff = tmp - c_start;
        c_start = tmp;
        return diff;
    }
    constexpr Clock(): c_start() {}
};

class ClockMicS { // microsec単位
public:
    void start() { t_ = clock_sec(); }
    double stop() const { return (clock_sec() - t_) * 1000000; }
    double restart() { // 結果を返し、0から再スタート
        double t = clock_sec();
        double diff = t - t_;
        t_ = t;
        return diff * 1000000;
    }
    ClockMicS() {}
    ClockMicS(int m) { start(); }
private:
    double t_;
};

template <typename T> constexpr bool holdsBits(T a, T b) { return !(~a & b); }
template <typename T> constexpr bool isExclusiveBits(T a, T b) { return !(a & b); }

template <typename T> inline int popcnt(T a) { return std::popcount(a); };
template <typename T> inline int bsf(T v) { return std::countr_zero(v); }
template <typename T> inline int bsr(T v) { return sizeof(T) * 8 - 1 - std::countl_zero(v); }

template <typename T> constexpr T lsb(T v) { return v & -v; }
template <typename T> constexpr T popLsb(T v) { return v & (v - T(1)); }
template <typename T> inline T rsb(T v) { return std::bit_floor(v); }

// 最下位/上位ビットより下位/上位のビット全て
template <typename T> constexpr T allLowerBits(T v) { return ~v & (v - T(1)); }
template <typename T> inline T allHigherBits(T a) { return ~(rsb(a) - T(1)) << 1; }

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

template <typename T, int W, int MAX_SIZE = sizeof(T) * 8>
constexpr T fill_bits_impl(T a, int n) {
    return n <= 0 ? a :
      (W <= 0 ? T(0) :
      (W >= MAX_SIZE ? a :
      ((fill_bits_impl<T, W, MAX_SIZE>(a, n - 1) << W) | a)));
}

template <typename T, int W, int MAX_SIZE = sizeof(T) * 8>
constexpr T fill_bits(T a) {
    return W <= 0 ? T(0) :
      (W >= MAX_SIZE ? a :
      (((fill_bits_impl<T, W, MAX_SIZE>(a, (MAX_SIZE - 1) / W)) << W) | a));
}

constexpr uint64_t cross64(uint64_t a, uint64_t b) {
    return (a & 0x5555555555555555) | (b & 0xAAAAAAAAAAAAAAAA);
}
template <size_t N, size_t M>
constexpr uint64_t cross64Impl(uint64_t *a) {
    return M == 0 ? 0
           : (fill_bits<uint64_t, N>(1ULL << (N - M)) & *a) | cross64Impl<N, (M > 0 ? (M - 1) : 0)>(a + 1);
}
template <size_t N>
constexpr uint64_t cross64(uint64_t *a) {
    return cross64Impl<N, N>(a);
}

template <class dice64_t>
inline uint64_t pick1Bit64(uint64_t arg, dice64_t& dice) {
    uint64_t tmp;
    while (1) {
        tmp = arg & dice();
        if (tmp) {
            if (tmp & (tmp - 1ULL)) arg = tmp; // 2つ以上
            else break;
        }
    }
    return tmp;
}

template <class dice64_t>
static uint64_t pickNBits64(uint64_t arg, int N0, int N1, dice64_t& dice) {
    // argからランダムにN0ビット抽出する
    // 最初はN0 + N1ビットある必要あり
    assert(popcnt(arg) == N0 + N1);

    uint64_t res = 0;

    if (N0 < N1) {
        if (N0 == 0) return res;
        if (N0 == 1) return pick1Bit64(arg, dice);
    } else {
        if (N1 == 0) return arg;
        if (N1 == 1) return arg - pick1Bit64(arg, dice);
    }
    while (1) {
        uint64_t dist = arg & dice();
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

template <class dice64_t>
static void dist2_64(uint64_t *goal0, uint64_t *goal1,
                     uint64_t arg, int N0, int N1, dice64_t& dice) {
    assert(popcnt(arg) == N0 + N1);
    uint64_t tmp = pickNBits64(arg, N0, N1, dice);
    *goal0 |= tmp;
    *goal1 |= arg - tmp;
}

template <int N, typename T, class dice64_t>
static void dist64(uint64_t *const dst, uint64_t arg, const T *argNum, dice64_t& dice) {
    if (N <= 1) dst[0] = arg;
    else if (N == 2) dist2_64(dst, dst + 1, arg, argNum[0], argNum[1], dice);
    else {
        constexpr int NH = N / 2;
        int num[2] = {0};
        for (int i = 0; i < NH; i++) num[0] += argNum[i];
        for (int i = NH; i < N; i++) num[1] += argNum[i];
        assert(num[0] + num[1] == popcnt(arg));
        uint64_t half[2] = {0};
        dist64<2>(half, arg, num, dice);
        dist64<NH>(dst, half[0], argNum, dice);
        dist64<N - NH>(dst + NH, half[1], argNum + NH, dice);
    }
}

inline uint64_t splitmix64(uint64_t& x) {
    x += 0x9E3779B97F4A7C15ULL;
    uint64_t t = x;
    t = (t ^ (t >> 30)) * 0xBF58476D1CE4E5B9ULL;
    t = (t ^ (t >> 27)) * 0x94D049BB133111EBULL;
    return t ^ (t >> 31);
}

class XorShift64 {
private:
    uint64_t x, y;
public:
    using result_type = uint64_t;
    uint64_t operator ()() {
        uint64_t r = std::rotl(x + y, 17) + x;
        uint64_t z = y ^ x;
        x = std::rotl(x, 49) ^ z ^ (z << 21);
        y = std::rotl(z, 28);
        return r;
    }
    double random() {
        return operator ()() / double(0xFFFFFFFFFFFFFFFFULL);
    }
    void seed(uint64_t s) {
        x = splitmix64(s);
        y = splitmix64(s);
    }
    static constexpr uint64_t min() { return 0ULL; }
    static constexpr uint64_t max() { return 0xFFFFFFFFFFFFFFFFULL; }

    constexpr XorShift64(): x(), y() {}
    XorShift64(uint64_t s): x(), y() { seed(s); }
};

static double dFactorial(int n) {
    double ans = 1;
    while (n > 1) ans *= n--;
    return ans;
}
static double dPermutation(int n, int r) {
    double ans = 1;
    while (r--) ans *= n - r;
    return ans;
}
static double dCombination(int n, int r) {
    if (n >= r) return dPermutation(n, r) / dFactorial(r);
    else return 0;
}

constexpr int ipow(int m, int n) {
    return n <= 0 ? 1 : (m * ipow(m, n - 1));
}
static double sigmoid(double x, double alpha = 1) {
    return 1 / (1 + std::exp(-x / alpha));
}
static double logit(double s, double alpha = 1) {
    return -std::log((1.0 / s) - 1.0) * alpha;
}
static double beta(double x, double y){ // ベータ関数
    return tgamma(x) * tgamma(y) / tgamma(x + y);
}
static double log_beta(double x, double y) {
    return lgamma(x) + lgamma(y) - lgamma(x + y);
}

struct BetaDistribution {
    double a, b;

    constexpr double size() const { return a + b; }
    constexpr double mean() const { return a / (a + b); }
    constexpr double med() const {
        return (a - 1 / 3.0) / (a + b - 2 / 3.0); // a, b > 1 での近似値
    }
    constexpr double mod() const {
        return (a - 1) / (a + b - 2); // a, b > 1 のとき
    }
    double var() const {
        return (a * b) / ((a + b) * (a + b) * (a + b + 1));
    }
    double std() const {
        return sqrt(a * b / (a + b + 1)) / (a + b);
    }
    double relative_dens(double x) const {
        return pow(x, a - 1) * pow(1 - x, b - 1);
    }
    double dens(double x) const {
        return relative_dens(x) / beta(a, b);
    }
    double log_relative_dens(double x) const {
        return (a - 1) * log(x) + (b - 1) * log(1 - x);
    }
    double log_dens(double x) const {
        return log_relative_dens(x) - log_beta(a, b);
    }

    BetaDistribution& operator+=(const BetaDistribution& rhs) {
        a += rhs.a;
        b += rhs.b;
        return *this;
    }
    BetaDistribution& operator-=(const BetaDistribution& rhs) {
        a -= rhs.a;
        b -= rhs.b;
        return *this;
    }
    BetaDistribution& operator*=(const double m) {
        a *= m;
        b *= m;
        return *this;
    }
    BetaDistribution& operator/=(const double d) {
        *this *= 1 / d;
        return *this;
    }

    BetaDistribution& set(const double aa, const double ab) {
        a = aa;
        b = ab;
        return *this;
    }
    BetaDistribution& set_by_mean(double m, double size) {
        set(m * size, (1 - m) * size);
        return *this;
    }
    BetaDistribution reversed() const {
        return BetaDistribution(b, a);
    }

    bool exam() const;
    std::string toString() const;

    constexpr BetaDistribution(): a(), b() {}
    explicit constexpr BetaDistribution(double aa, double ab): a(aa), b(ab) {}
};

inline BetaDistribution operator+(const BetaDistribution& lhs, const BetaDistribution& rhs) {
    return BetaDistribution(lhs.a + rhs.a, lhs.b + rhs.b);
}
inline BetaDistribution operator-(const BetaDistribution& lhs, const BetaDistribution& rhs) {
    return BetaDistribution(lhs.a - rhs.a, lhs.b - rhs.b);
}
inline BetaDistribution operator*(const BetaDistribution& lhs, const double m) {
    return BetaDistribution(lhs.a * m, lhs.b * m);
}

extern std::ostream& operator<<(std::ostream& out, const BetaDistribution& b);

template <typename T, size_t B, size_t N>
class MiniBitArray {
    static_assert(sizeof(T) * 8 >= B * N, "");
    static constexpr T MASK_BASE = (T(1) << B) - T(1);
    static constexpr size_t place(const size_t p) { return p * B; }
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

template <size_t B, size_t N = 32 / B> using BitArray32 = MiniBitArray<std::uint32_t, B, N>;
template <size_t B, size_t N = 64 / B> using BitArray64 = MiniBitArray<std::uint64_t, B, N>;

class TwoValuePage32 {
public:
    uint32_t any() const { return data_; }
    void regist(uint32_t value, uint64_t key) {
        assert(value < 4U);
        data_ = ((uint32_t)(key >> 32) & ~3U) | value;
    }
    uint32_t compareKey(uint64_t key) const {
        return (data_ ^ uint32_t(key >> 32)) & ~3U;
    }
    uint32_t value() const { return data_ & 3U; }
private:
    uint32_t data_;
};

template <size_t N>
class TwoValueBook {
    // 2(+中間1)値を保存するハッシュ表
public:
    using page_t = TwoValuePage32;

    void clear() { std::memset(page_, 0, sizeof(page_)); }
    TwoValueBook() { clear(); }

    int read(uint64_t key) const {
        page_t fpage = page_[KeyToIndex(key)];
        if (!fpage.any() || fpage.compareKey(key)) return -1;
        return fpage.value();
    }
    void regist(uint32_t value, uint64_t key) {
        page_t& fpage = page_[KeyToIndex(key)];
        fpage.regist(value, key);
    }
private:
    page_t page_[N];
    static constexpr int KeyToIndex(uint64_t key) {
        return key % N;
    }
};

class SpinLock {
public:
    void lock() {
        while (locked_.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        locked_.clear(std::memory_order_release);
    }
private:
    std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
};

template <int L, int ... shape_t>
struct TensorIndexTypeImpl {
    template <typename ... args_t>
    static constexpr int get(int i, args_t ... args) {
        return i * size() + TensorIndexTypeImpl<shape_t...>::get(args...);
    }
    static constexpr int size() {
        return L * TensorIndexTypeImpl<shape_t...>::size();
    }
};

template <int L>
struct TensorIndexTypeImpl<L> {
    template <typename ... args_t>
    static constexpr int get(int i, int j) { return i * size() + j; }
    static constexpr int size() { return L; }
};
template <int L, int ... shape_t>
struct TensorIndexType {
    template <typename ... args_t>
    static constexpr int get(args_t ... args) {
        return TensorIndexTypeImpl<shape_t...>::get(args...);
    }
    static constexpr int size() {
        return L * TensorIndexType<shape_t...>::size();
    }
};

template <int L>
struct TensorIndexType<L> {
    static constexpr int get(int i) { return i; }
    static constexpr int size() { return L; }
};

template <typename T>
struct StochasticSelector {
    T *const score_;
    const int size_;

    double sum_;

    StochasticSelector(T *const ascore, int asize):
    score_(ascore), size_(asize), sum_(0) {}

    void init_max() {
        // max selector としての初期化
        if (size_ == 0) return;
        T maxValue = *std::max_element(score_, score_ + size_);
        for (int i = 0; i < size_; i++) {
            if (score_[i] == maxValue) {
                score_[i] = 1;
                sum_ += 1;
            } else score_[i] = 0;
        }
    }

    double prob(int i) const {
        return sum_ == 0 ? 1.0 / size_ : score_[i] / sum_;
    }
    int select(double urand) const {
        if (sum_ == 0) return int(urand * size_);
        double r = urand * sum_;
        int i = 0;
        for (; i < size_ - 1; i++) {
            r -= score_[i];
            if (r <= 0) break;
        }
        return i;
    }
    double entropy() const {
        double ent = 0;
        for (int i = 0; i < size_; i++) {
            double prb = prob(i);
            if (prb > 0) ent -= prb * std::log(prb);
        }
        return ent / std::log(2);
    }
};

template <typename T>
struct SoftmaxSelector : public StochasticSelector<T> {
    using base_t = StochasticSelector<T>;

    SoftmaxSelector(T *const ascore, int asize, double atemp):
    base_t(ascore, asize) { init(atemp); }

    void init(double atemp) {
        if (atemp == 0) return base_t::init_max();
        for (int i = 0; i < base_t::size_; i++) {
            double es = std::exp(base_t::score_[i] / atemp);
            base_t::score_[i] = es;
            base_t::sum_ += es;
        }
    }
};

template <typename T>
struct ThresholdSoftmaxSelector : public StochasticSelector<T> {
    using base_t = StochasticSelector<T>;

    ThresholdSoftmaxSelector(T *const ascore, int asize, double atemp, double athreshold):
    base_t(ascore, asize) { init(atemp, athreshold); }

    void init(double atemp, double athreshold) {
        if (atemp == 0) return base_t::init_max();
        for (int i = 0; i < base_t::size_; i++) {
            double es = std::exp(base_t::score_[i] / atemp);
            base_t::score_[i] = es;
            base_t::sum_ += es;
        }
        if (base_t::sum_ == 0) return;
        T new_sum = 0;
        for (int i = 0; i < base_t::size_; i++) {
            base_t::score_[i] = std::max(base_t::score_[i] / base_t::sum_ - athreshold, T(0));
            new_sum += base_t::score_[i];
        }
        base_t::sum_ = new_sum;
    }
};

template <typename T>
struct BiasedSoftmaxSelector : public StochasticSelector<T> {
    using base_t = StochasticSelector<T>;
    BiasedSoftmaxSelector(T *const ascore, int asize,
                          double atemp, double acoef, double arate):
    base_t(ascore, asize) { init(atemp, acoef, arate); }

    void init(double atemp, double acoef, double arate) {
        if (atemp == 0) return base_t::init_max();
        if (base_t::size_ == 0) return;
        T max_score = *std::max_element(base_t::score_, base_t::score_ + base_t::size_);
        // minus bias by the difference from best score
        for (int i = 0; i < base_t::size_; i++) {
            base_t::score_[i] -= acoef * std::pow(max_score - base_t::score_[i], arate);
        }
        for (int i = 0; i < base_t::size_; i++) {
            double es = std::exp(base_t::score_[i] / atemp);
            base_t::score_[i] = es;
            base_t::sum_ += es;
        }
    }
};

template <typename T>
struct ExpBiasedSoftmaxSelector : public StochasticSelector<T> {
    using base_t = StochasticSelector<T>;
    ExpBiasedSoftmaxSelector(T *const ascore, int asize,
                             double atemp, double acoef, double aetemp):
    base_t(ascore, asize) { init(atemp, acoef, aetemp); }

    void init(double atemp, double acoef, double aetemp) {
        if (atemp == 0) return base_t::init_max();
        if (base_t::size_ == 0) return;
        T max_score = *std::max_element(base_t::score_, base_t::score_ + base_t::size_);
        // minus bias by the difference from best score
        for (int i = 0; i < base_t::size_; i++) {
            base_t::score_[i] -= acoef * std::exp((max_score - base_t::score_[i]) / aetemp);
        }
        for (int i = 0; i < base_t::size_; i++) {
            double es = std::exp(base_t::score_[i] / atemp);
            base_t::score_[i] = es;
            base_t::sum_ += es;
        }
    }
};

template <typename T, size_t B, size_t N>
MiniBitArray<T, B, N> invert(const MiniBitArray<T, B, N>& ba) {
    MiniBitArray<T, B, N> ret(0);
    for (size_t i = 0; i < N; i++) ret.set(ba[i], i);
    return ret;
}

template <class T, std::size_t N>
std::ostream& operator <<(std::ostream& ost, const std::array<T, N>& a) {
    ost << "{";
    for (int i = 0; i < (int)N - 1; i++) ost << a[i] << ", ";
    if (a.size() > 0) ost << a[N - 1];
    ost << "}";
    return ost;
}
template <std::size_t N>
std::ostream& operator <<(std::ostream& ost, const std::array<std::int8_t, N>& a) {
    ost << "{";
    for (int i = 0; i < (int)N - 1; i++) ost << (int)a[i] << ", ";
    if (a.size() > 0) ost << (int)a[N - 1];
    ost << "}";
    return ost;
}
template <std::size_t N>
std::ostream& operator <<(std::ostream& ost, const std::array<std::uint8_t, N>& a) {
    ost << "{";
    for (int i = 0; i < (int)N - 1; i++) ost << (unsigned int)a[i] << ", ";
    if (a.size() > 0) ost << (unsigned int)a[N - 1];
    ost << "}";
    return ost;
}
template <class T>
std::ostream& operator <<(std::ostream& ost, const std::vector<T>& v) {
    ost << "{";
    for (int i = 0; i < (int)v.size() - 1; i++) ost << v[i] << ", ";
    if (v.size() > 0) ost << v.back();
    ost << "}";
    return ost;
}
template <typename T, size_t B, size_t N>
static std::ostream& operator <<(std::ostream& ost, const MiniBitArray<T, B, N>& ba) {
    ost << "{";
    for (int i = 0; i < (int)N - 1; i++) ost << ba[i] << ", ";
    if (ba.size() > 0) ost << ba[N - 1];
    ost << "}";
    return ost;
}
template <std::size_t N>
std::ostream& operator <<(std::ostream& ost, const std::bitset<N>& a) {
    ost << "[";
    for (int i = 0; i < N; i++) ost << bool(a.test(i));
    ost << "]";
    return ost;
}

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

extern std::string toupper(const std::string& str);
extern std::string tolower(const std::string& str);
extern bool isSuffix(const std::string& s, const std::string& suffix);
extern std::vector<std::string> split(const std::string& s, char separator);
extern std::vector<std::string> split(const std::string& s, const std::string& separators);
extern std::vector<std::string> getFilePathVector(const std::string& ipath, const std::string& suffix = "",
                                                  bool recursive = false);
extern std::vector<std::string> getFilePathVectorRecursively(const std::string& ipath, const std::string& suffix = "");