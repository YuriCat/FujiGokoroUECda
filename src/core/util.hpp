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

template <typename T> inline T popLsb(T v) {
    return v & (v - T(1));
}

/*template <typename T, size_t N = sizeof(T) * 8, typename RT = T>
class MiniBitSetCore {
    using this_t = MiniBitSetCore<T, N, RT>;
    static_assert(sizeof(T) * 8 >= N, "");
    static constexpr T mask(const size_t p) { return T(1) << p; }        
    static constexpr T mask_lower(const size_t p) { return mask(p) - T(1); }
    static constexpr T mask_range(const size_t p0, const size_t p1) {
        return mask_lower(p1) & ~mask_lower(p0);
    }
    static constexpr T mask_full() { return T(-1); }
    public:
    using data_type = T;
    void reset() { data_ = T(0); }
    void clear() { reset(); }
    void remove_all() { reset(); }
    void force_remove_all() { reset(); }
    void fill() { data_ = mask_full(); }
    void flip() { data_ = ~data_; }
    void reset(size_t p) {
        assert(0 <= p && p < N);
        data_ &= ~mask(p);
    }
    bool remove(size_t p) {
        bool contain = get(p);
        reset(p);
        return contain;
    }
    void set(size_t p) {
        assert(0 <= p && p < N);
        data_ |= mask(p);
    }
    bool insert(size_t p) {
        bool contain = get(p);
        set(p);
        return !contain;
    }
    void flip(size_t p) {
        assert(0 <= p && p < N);
        data_ ^= mask(p);
    }
    void set_value(size_t p, bool v) {
        assert(0 <= p && p < N);
        data_ |= v << p;
    }
    void assign(size_t p, bool v) {
        assert(0 <= p && p < N);
        data_ = (data_ & ~mask(p)) | (v << p);
    }
    T test(size_t p) const {
        assert(0 <= p && p < N);
        return data_ & mask(p);
    }
    bool get(size_t p) const {
        assert(0 <= p && p < N);
        return (data_ >> p) & T(1);
    }
    void force_merge(const this_t& bs) { data_ |= bs.data_; }
    void merge(const this_t& bs) { force_merge(bs); }
    void force_remove_all(const this_t& bs) { data_ &= ~bs.data_; }
    void remove_all(const this_t& bs) { force_remove_all(bs); }
    size_t count() const { return popcnt<T>(data_); }
    size_t size() const { return count(); }
    size_t front() const { return bsf<T>(data_); }
    size_t back() const { return bsr<T>(data_); }
    size_t at(size_t cnt) const {
        size_t i = 0;
        for (size_t p : *this) {
        if (i == cnt) return p;
        i++;
        }
        return -1;
    }
    void pop_front() { data_ = pop_lsb<T>(data_); }
    bool any() const { return data_ != 0; }
    bool empty() const { return !any(); }
    bool contains(size_t p) const { return test(p); }
    bool holds(const this_t& bs) const {
        return !(~data_ & bs.data_);
    }
    bool is_exclusive(const this_t& bs) const {
        return !(data_ & bs.data_);
    }
    bool operator ==(const this_t& bs) const {
        return data() == bs.data();
    }
    bool operator !=(const this_t& bs) const {
        return !(*this == bs);
    }
    this_t& operator =(T src) {
        data_ = src;
        return *this;
    }
    this_t& operator =(const this_t& src) {
        data_ = src.data();
        return *this;
    }
    class const_iterator : public std::iterator<std::input_iterator_tag, size_t> {
        friend MiniBitSetCore<T, N, RT>;
    public:
        size_t operator *() const {
        return bsf<T>(temp_data_);
        }
        bool operator !=(const const_iterator& itr) const {
        return pclass_ != itr.pclass_ || temp_data_ != itr.temp_data_;
        }
        const_iterator& operator ++() {
        temp_data_ = pop_lsb(temp_data_); // ok even if already 0
        return *this;
        }
    protected:
        explicit const_iterator(const MiniBitSetCore<T, N, RT> *pclass): pclass_(pclass), temp_data_(pclass->data()) {}
        explicit const_iterator(const MiniBitSetCore<T, N, RT> *pclass, T data): pclass_(pclass), temp_data_(data) {}
        const MiniBitSetCore<T, N, RT> *const pclass_;
        T temp_data_;
    };
    const_iterator begin() const { return const_iterator(this); }
    const_iterator end() const { return const_iterator(this, T(0)); }
    T data() const { return data_; }

    MiniBitSetCore(): data_() {}
    MiniBitSetCore(const T& data): data_(data) {}
    MiniBitSetCore(const MiniBitSetCore<T, N, RT>& bs): data_(bs.data()) {}
    protected:
    RT data_;
};

template <typename T, size_t N = sizeof(T) * 8, typename RT = T>
class MiniBitSet : public MiniBitSetCore<T, N, RT> {
public:
// zero clear in default constructor
MiniBitSet(): MiniBitSetCore<T, N, RT>(0) {} 
};*/