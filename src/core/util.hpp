#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#if !defined(NDEBUG)
#define UNREACHABLE assert(0)
#elif defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define UNREACHABLE __assume(0)
#elif defined(__INTEL_COMPILER)
#define UNREACHABLE __assume(0)
#elif defined(__GNUC__) && (4 < __GNUC__ || (__GNUC__ == 4 && 4 < __GNUC_MINOR__))
#define UNREACHABLE __builtin_unreachable()
#else
#define UNREACHABLE assert(0)
#endif

// 条件x、命令等y
#ifdef NDEBUG
#define ASSERT(X, Y)
#define FASSERT(f, o)
#define FEQUALS(f0, f1, o)
#define ASSERT_EQ(k0, k1)
#else
#define ASSERT(X, Y)  if(!(X)){ Y; assert(0); }
// 浮動小数点がまともな値を取っているかどうかのアサーション
#define FASSERT(f, o) if(!(!std::isinf(f) && !std::isnan(f))){ cerr << (f) << endl; {o}\
assert(!std::isinf(f) && !std::isnan(f)); assert(0); };
// 浮動小数点が「ほぼ」同じ値を取っているかのチェック && FASSERT
#define FEQUALS(f0, f1, o) { if(!(!std::isinf(f0) && !std::isnan(f0))){ cerr << (f0) << endl; {o}\
assert(!std::isinf(f0) && !std::isnan(f0)); assert(0); };\
if(!(!std::isinf(f1) && !std::isnan(f1))){ cerr << (f1) << endl; {o}};\
assert(!std::isinf(f1) && !std::isnan(f1)); assert(0); };\
if(!(abs((f0) - (f1)) <= 0.00001)){ cerr << (f0) << " <-> " << (f1) << endl; {o}\
assert(abs((f0) - (f1)) <= 0.00001); assert(0); }
#define ASSERT_EQ(k0, k1) ASSERT((k0) == (k1), cerr << (k0) << " <-> " << (k1) << endl;);
#endif // NDEBUG

// 標準ライブラリ使用
using std::cout;
using std::cerr;
using std::endl;
using std::max;
using std::min;
using std::size_t;

template <typename T>
constexpr T cmax(const T& a, const T& b) { return a < b ? b : a; }
template <typename T>
constexpr T cmin(const T& a, const T& b) { return a > b ? b : a; }

// 出力
#ifdef DEBUG
#define DOUT std::cout
#define DERR std::cerr
#else
#define DERR 0 && std::cerr
#define DOUT 0 && std::cout
#endif

#ifndef MINIMUM
#define COUT cout
#define CERR cerr
#else
#define CERR 0 && cerr
#define COUT 0 && cout
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
static uint64_t pickNBits64(uint64_t arg, int N0, int N1, dice64_t *const pdice) {
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

template <class dice64_t>
static void dist2_64(uint64_t *goal0, uint64_t *goal1,
                     uint64_t arg, int N0, int N1, dice64_t *const dice) {
    assert((int)popcnt64(arg) == N0 + N1);
    uint64_t tmp = pickNBits64(arg, N0, N1, dice);
    *goal0 |= tmp;
    *goal1 |= (arg - tmp);
}
template <int N, typename T, class dice64_t>
static void dist64(uint64_t *const dst, uint64_t arg, const T *argNum, dice64_t *const dice) {
    if (N <= 1) dst[0] = arg;
    else if (N == 2) dist2_64(dst, dst + 1, arg, argNum[0], argNum[1], dice);
    else {
        const int NH = N / 2;
        int num[2] = {0};
        for (int i = 0; i < NH; i++) num[0] += argNum[i];
        for (int i = NH; i < N; i++) num[1] += argNum[i];
        assert(num[0] + num[1] == (int)popcnt64(arg));
        uint64_t half[2] = {0};
        dist64<2>(half, arg, num, dice);
        dist64<NH>(dst, half[0], argNum, dice);
        dist64<N - NH>(dst + NH, half[1], argNum + NH, dice);
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

double dFactorial(int n) {
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
constexpr int ipow(int m, int n) { return n <= 0 ? 1 : (m * ipow(m, n - 1)); }
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

struct ExponentialDistribution {
  double lambda;

  ExponentialDistribution(double l): lambda(l) {}

  double random(double urand) const { return -std::log(urand) / lambda; }
  template<class dice_t>
  double random(dice_t *const pdice) const { return random(pdice->random()); }

  double relative_dens(double x) const { return std::exp(-lambda * x); }
  double dens(double x) const { return lambda * relative_dens(x); }
  double dist(double x) const { return 1 - std::exp(-lambda * x); }
  double mean() const { return 1 / lambda; }
  double med() const { return std::log(2) / lambda; }
  double var() const { return 1 / (lambda * lambda); }
  double std() const { return 1 / lambda; }
  double ent() const { return 1 - std::log(lambda); }

  static double mod() { return 0; }
  static double skew() { return 2; }
  static double kur() { return 6; }
};

struct GammaDistribution {
  double k, theta;

  GammaDistribution(): k(), theta() {}
  GammaDistribution(double ak, double atheta = 1): k(ak), theta(atheta) {};

  GammaDistribution& set(double ak, double atheta = 1) {
    k = ak; theta = atheta;
    return *this;
  }

  template <class dice_t>
  double rand(dice_t *const pdice) const {
    double x, y, z;
    double u, v, w, b, c, e;
    if (k < 1) {
      ExponentialDistribution ex(1);
      e = ex.random(pdice);
      do {
        x = std::pow(pdice->drand(), 1 / k);
        y = std::pow(pdice->drand(), 1 / (1 - k));
      } while (x + y > 1);
      return (e * x / (x + y)) * theta;
    } else {
      b = k - 1;
      c = 3 * k - 0.75;
      while (true) {
        u = pdice->drand();
        v = pdice->drand();
        w = u * (1 - u);
        y = std::sqrt(c / w) * (u - 0.5);
        x = b + y;
        if (x >= 0) {
          z = 64 * w * w * w * v * v;
          if (z <= 1 - (2 * y * y) / x) {
            return x * theta;
          } else {
            if (std::log(z) < 2 * (b * std::log(x / b) - y)) {
              return x * theta;
            }
          }
        }
      }
      return x * theta;
    }
  }

  double ralative_dens(double x) const { return std::pow(x, k - 1) * std::exp(-x / theta); }
  double dens(double x) const { return ralative_dens(x) / tgamma(k) / std::pow(theta, k); }
  double mean() const { return k * theta; }
  double mod() const { return (k - 1) * theta; }
  double var() const { return k * theta * theta; }
  double std() const { return std::sqrt(k) * theta; }
};

struct BetaDistribution{
    double a, b;
    
    template<class dice_t>
    double rand(dice_t *const pdice)const{
        double r1 = GammaDistribution(a).rand(pdice);
        double r2 = GammaDistribution(b).rand(pdice);
        return r1 / (r1 + r2);
    }
    constexpr double size() const { return a + b; }
    constexpr double mean() const { return a / (a + b); }
    constexpr double rate() const { return a / b; }
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
    
    BetaDistribution& add(const BetaDistribution& arg)noexcept{
        a += arg.a;
        b += arg.b;
        return *this;
    }
    BetaDistribution& subtr(const BetaDistribution& arg)noexcept{
        a -= arg.a;
        b -= arg.b;
        return *this;
    }
    
    BetaDistribution& operator+=(const BetaDistribution& rhs)noexcept{
        a += rhs.a;
        b += rhs.b;
        return *this;
    }
    BetaDistribution& operator-=(const BetaDistribution& rhs)noexcept{
        a -= rhs.a;
        b -= rhs.b;
        return *this;
    }
    BetaDistribution& operator*=(const double m)noexcept{
        a *= m;
        b *= m;
        return *this;
    }
    BetaDistribution& operator/=(const double d){
        (*this) *= 1 / d;
        return *this;
    }
    BetaDistribution& mul(const double m)noexcept{
        a *= m;
        b *= m;
        return *this;
    }
    
    BetaDistribution& rev()noexcept{
        std::swap(a, b);
        return *this;
    }
    
    BetaDistribution& set(const double aa, const double ab)noexcept{
        a = aa;
        b = ab;
        return *this;
    }
    BetaDistribution& set_by_mean(double m, double size)noexcept{
        set(m * size, (1 - m) * size);
        return *this;
    }
    BetaDistribution& set_by_rate(double r, double size)noexcept{
        set_by_mean(r / (1 + r), size);
        return *this;
    }
    
    BetaDistribution& resize(double h){
        // サイズをhにする
        double s = size();
        
        assert(s);
        
        double h_s = h / s;
        a *= h_s;
        b *= h_s;
        return *this;
    }
    
    bool exam()const noexcept{
        if(a < 0 || b < 0){ return false; }
        if(a == 0 && b == 0){ return false; }
        return true;
    }
    
    std::string toString()const{
        std::ostringstream oss;
        oss << "Be(" << a << ", " << b << ")";
        return oss.str();
    }
    
    constexpr BetaDistribution():
    a(), b(){}
    explicit constexpr BetaDistribution(const double aa, const double ab):
    a(aa), b(ab){}
};

inline BetaDistribution operator+(const BetaDistribution& lhs, const BetaDistribution& rhs)noexcept{
    return BetaDistribution(lhs.a + rhs.a, lhs.b + rhs.b);
}
inline BetaDistribution operator-(const BetaDistribution& lhs, const BetaDistribution& rhs)noexcept{
    return BetaDistribution(lhs.a - rhs.a, lhs.b - rhs.b);
}
inline BetaDistribution operator*(const BetaDistribution& lhs, const double m)noexcept{
    return BetaDistribution(lhs.a * m, lhs.b * m);
}
inline BetaDistribution operator*(const double m, const BetaDistribution& rhs)noexcept{
    return BetaDistribution(rhs.a * m, rhs.b * m);
}

static std::ostream& operator<<(std::ostream& out, const BetaDistribution& b){
    out << b.toString();
    return out;
}

struct NormalDistribution {
    double mu, sigma;
    
    template<class dice_t>
    double rand(dice_t *const pdice)const{
        // Box-Muller
        double r1 = pdice->drand();
        double r2 = pdice->drand();
        double z1 = std::sqrt(-2.0 * std::log(r1)) * std::cos(2.0 * M_PI * r2);
        return z1 * sigma + mu;
    }
    template<class dice_t>
    void rand(double *const pa, double *const pb, dice_t *const pdice)const{
        // 2つ同時に発生させる
        double r1 = pdice->drand();
        double r2 = pdice->drand();
        
        double z1 = std::sqrt(-2.0 * std::log(r1)) * std::cos(2.0 * M_PI * r2);
        double z2 = std::sqrt(-2.0 * std::log(r1)) * std::sin(2.0 * M_PI * r2);
        *pa = z1 * sigma + mu;
        *pb = z2 * sigma + mu;
    }
    
    double relative_dens(double x)const{
        return std::exp(-(x - mu) * (x - mu) / (2 * sigma * sigma));
    }
    double dens(double x)const{
        return relative_dens(x) / sigma * (1 / std::sqrt(2 * M_PI));
    }
    double dist(double x)const{
        return (1 + std::erf((x - mu) / sigma * (1 / std::sqrt(2)))) / 2;
    }
    
    double ent()const{
        return sigma * std::sqrt(2 * M_PI * std::exp(1));
    }
    constexpr double mean()const noexcept{ return mu; }
    constexpr double var()const noexcept{ return sigma * sigma; }
    constexpr double std()const noexcept{ return sigma; }
    constexpr double med()const noexcept{ return mu; }
    constexpr double mod()const noexcept{ return mu; }
    
    NormalDistribution& operator*=(const double m) {
        sigma *= m;
        return *this;
    }
    NormalDistribution& operator/=(const double m) {
        sigma /= m;
        return *this;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "N(" << mu << ", " << sigma << ")";
        return oss.str();
    }
    
    constexpr NormalDistribution(): mu(),sigma() {}
    constexpr NormalDistribution(double argMu, double argSigma)
    :mu(argMu), sigma(argSigma){}
    
    NormalDistribution& set(double argMu, double argSigma) {
        mu = argMu;
        sigma = argSigma;
        return *this;
    }
};

static std::ostream& operator<<(std::ostream& out, const NormalDistribution& n){
    out << n.toString();
    return out;
}

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
    
    void init() {
        std::memset(page_, 0, sizeof(page_));
    }
    void clear() {}
    TwoValueBook() { init(); }
    
    int read(uint64_t key) {
        const page_t& fpage = page_[KeyToIndex(key)];
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

template <typename T = int>
class SpinLock {
public:
    void lock() {
        while (true) {
            while (data_);
            T tmp = 1;
            if (data_.exchange(tmp, std::memory_order_acquire) == 0) return;
        }
    }
    bool try_lock() {
        T tmp = 1;
        if (data_.exchange(tmp, std::memory_order_acquire) == 0) return true;
        return false;
    }
    void unlock() { data_ = 0; }
    SpinLock() { unlock(); }
private:
    std::atomic<T> data_; 
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

    double prob(int i) const {
        return score_[i] / sum_;
    }
    int select(double urand) const {
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
    base_t(ascore, asize) {
        for (int i = 0; i < asize; i++) {
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
    base_t(ascore, asize) {
        for (int i = 0; i < asize; i++) {
            double es = std::exp(base_t::score_[i] / atemp);
            base_t::score_[i] = es;
        }
        for (int i = 0; i < asize; i++) {
            base_t::score_[i] = std::max(base_t::score_[i] - athreshold, 1e-8);
            base_t::sum_ += base_t::score_[i];
        }
    }
};

template <typename T>
struct BiasedSoftmaxSelector : public StochasticSelector<T> {
    using base_t = StochasticSelector<T>;
    BiasedSoftmaxSelector(T *const ascore, int asize,
                          double atemp, double acoef, double arate):
    base_t(ascore, asize) {
        T max_score = -std::numeric_limits<T>::max();
        for (int i = 0; i < asize; i++) {
            max_score = std::max(max_score, base_t::score_[i]);
        }
        // minus bias by the difference from best score
        for (int i = 0; i < asize; i++) {
            base_t::score_[i] -= acoef * std::pow(max_score - base_t::score_[i], arate);
        }
        for (int i = 0; i < asize; i++) {
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
    base_t(ascore, asize) {
        T max_score = -std::numeric_limits<T>::max();
        for (int i = 0; i < asize; i++) {
            max_score = std::max(max_score, base_t::score_[i]);
        }
        // minus bias by the difference from best score
        for (int i = 0; i < asize; i++) {
            base_t::score_[i] -= acoef * std::exp(max_score - base_t::score_[i] / aetemp);
        }
        for (int i = 0; i < asize; i++) {
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

static std::string toupper(const std::string& str) {
    std::string s;
    for (char c : str) s.push_back(std::toupper(c));
    return s;
}
static std::string tolower(const std::string& str) {
    std::string s;
    for (char c : str) s.push_back(std::tolower(c));
    return s;
}
static bool isSuffix(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.substr(s.size() - suffix.size()) == suffix;
}
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

static std::vector<std::string> getFilePathVector(const std::string& ipath, const std::string& suffix = "",
                                                  bool recursive = false) {
    // make file path vector by suffix
    std::vector<std::string> file_names;
    DIR *pdir;
    dirent *pentry;

    pdir = opendir(ipath.c_str());
    if (pdir == nullptr) return file_names;
    do {
        pentry = readdir(pdir);
        if (pentry != nullptr) {
            const std::string name = std::string(pentry->d_name);
            const std::string full_path = ipath + name;
            struct stat st;
            stat(full_path.c_str(), &st);
            if ((st.st_mode & S_IFMT) == S_IFDIR) { // is directory
                if (recursive && name != "." && name != "..") {
                    std::vector<std::string> tfile_names = getFilePathVector(full_path + "/", suffix, true);
                    file_names.insert(file_names.end(), tfile_names.begin(), tfile_names.end()); // add vector
                }
            } else if (suffix.size() == 0 || isSuffix(name, suffix)) {
                file_names.emplace_back(full_path);
            }
        }
    } while (pentry != nullptr);
    return file_names;
}

static std::vector<std::string> getFilePathVectorRecursively(const std::string& ipath, const std::string& suffix = "") {
    // make file path vector by suffix recursively
    return getFilePathVector(ipath, suffix, true);
}