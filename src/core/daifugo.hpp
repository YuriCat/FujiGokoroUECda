#pragma once

#include <iostream>
#include <array>
#include <iterator>
#include "../base/util.hpp"

// 大富豪における最も基本的な型の実装

/**************************オーダー**************************/

constexpr int flipOrder(int ord) { return 1 - ord; }

/**************************ランク**************************/

// Uが3の1つ下、Oが2の1つ上
enum {
    RANK_U,
    RANK_3, RANK_4, RANK_5, RANK_6,
    RANK_7, RANK_8, RANK_9, RANK_T,
    RANK_J, RANK_Q, RANK_K, RANK_A,
    RANK_2,
    RANK_O,
    RANK_JOKER,
    // 実際に使用される通常カードのランク限度
    RANK_MIN = RANK_3,
    RANK_MAX = RANK_2,
    // 階段作成の際の仮想上のランク限度
    RANK_IMG_MIN = RANK_U,
    RANK_IMG_MAX = RANK_O,

    RANK_NONE = -1
};

struct RankRange { // 連続ランク
    int r0, r1;
    constexpr RankRange(int arg0, int arg1): r0(arg0), r1(arg1) {}
};

extern const std::string rankChar;
extern std::ostream& operator <<(std::ostream& ost, const RankRange& arg);
extern int CharToRank(char c);

/**************************スート番号**************************/

// 単スート
enum { SUITNUM_NONE = -1 };

constexpr int N_SUITS = 4;

extern const std::string suitNumChar;
extern int CharToSuitNum(char c);

/**************************スート**************************/

// スート集合 (スートの和集合)
enum {
    SUITS_NULL, SUITS_C,   SUITS_D,   SUITS_CD,
    SUITS_H,    SUITS_CH,  SUITS_DH,  SUITS_CDH,
    SUITS_S,    SUITS_CS,  SUITS_DS,  SUITS_CDS,
    SUITS_HS,   SUITS_CHS, SUITS_DHS, SUITS_CDHS,
    SUITS_X,
    SUITS_CDHSX = SUITS_CDHS | SUITS_X, // クインタプル
    SUITS_ALL = SUITS_CDHS,
};

inline int countSuits(unsigned s) { return popcnt(s); }

// スートインデックス
// スートビットから、その種類の役の中で何番目のスートパターンとされているかを得る
constexpr int suitsIdx[18] = {
    -1, 0, 1, 0, 2, 1, 3, 0, 3, 2, 4, 1, 5, 2, 3, 0, 0, 5
};

inline int SuitToSuitNum(unsigned int suit) { return bsf(suit); }

// 単スート番号からスート集合への変換
constexpr unsigned SuitNumToSuits(int sn0) { return 1U << sn0; }
constexpr int SuitsToSuitNum(int suit) { return suitsIdx[suit]; } // 複スートの場合もOK

// 出力
struct OutSuits {
    unsigned s;
    constexpr OutSuits(unsigned arg): s(arg) {}
};

extern std::ostream& operator <<(std::ostream& ost, const OutSuits& arg);

// (スート, スート)のパターン
extern uint8_t sSIndex[16][16];
extern uint8_t S2Index[16][16];
extern uint8_t SSIndex[16][16];

constexpr int N_PATTERNS_SUIT_SUITS = 8;
constexpr int N_PATTERNS_2SUITS = 22;
constexpr int N_PATTERNS_SUITS_SUITS = 35;

extern void initSuits();

/**************************カード整数**************************/

// U3456789TJQKA2O、CDHSの順番で0-59 ジョーカーは60

// 定数
enum IntCard : int {
    INTCARD_CU, INTCARD_DU, INTCARD_HU, INTCARD_SU,
    INTCARD_C3, INTCARD_D3, INTCARD_H3, INTCARD_S3,
    INTCARD_C4, INTCARD_D4, INTCARD_H4, INTCARD_S4,
    INTCARD_C5, INTCARD_D5, INTCARD_H5, INTCARD_S5,
    INTCARD_C6, INTCARD_D6, INTCARD_H6, INTCARD_S6,
    INTCARD_C7, INTCARD_D7, INTCARD_H7, INTCARD_S7,
    INTCARD_C8, INTCARD_D8, INTCARD_H8, INTCARD_S8,
    INTCARD_C9, INTCARD_D9, INTCARD_H9, INTCARD_S9,
    INTCARD_CT, INTCARD_DT, INTCARD_HT, INTCARD_ST,
    INTCARD_CJ, INTCARD_DJ, INTCARD_HJ, INTCARD_SJ,
    INTCARD_CQ, INTCARD_DQ, INTCARD_HQ, INTCARD_SQ,
    INTCARD_CK, INTCARD_DK, INTCARD_HK, INTCARD_SK,
    INTCARD_CA, INTCARD_DA, INTCARD_HA, INTCARD_SA,
    INTCARD_C2, INTCARD_D2, INTCARD_H2, INTCARD_S2,
    INTCARD_CO, INTCARD_DO, INTCARD_HO, INTCARD_SO,
    INTCARD_JOKER,
    // IntCardの最大はS2ではなくJOKERと定義されている(最大ランクの定義と違う)ので注意
    INTCARD_MIN = INTCARD_C3,
    INTCARD_MAX = INTCARD_JOKER,

    INTCARD_PLAIN_MIN = INTCARD_C3,
    INTCARD_PLAIN_MAX = INTCARD_S2,

    INTCARD_IMG_MIN = INTCARD_CU,
    INTCARD_IMG_MAX = INTCARD_JOKER,

    INTCARD_NONE = -1
};

// 総カード数（ゲーム上では存在しないUやOのカードも定義されているため、ゲームの定義ではなくこちらを使う）
constexpr int N_CARDS = 53;
constexpr int N_JOKERS = 1;
constexpr int N_IMG_CARDS = 61;

constexpr bool examIntCard(IntCard ic) {
    return (INTCARD_C3 <= ic && ic <= INTCARD_S2) || ic == INTCARD_JOKER;
}
constexpr bool examImaginaryIntCard(IntCard ic) {
    return INTCARD_CU <= ic && ic <= INTCARD_JOKER;
}
inline IntCard RankSuitsToIntCard(int r, unsigned int s) {
    return IntCard((r << 2) + SuitToSuitNum(s));
}
constexpr IntCard RankSuitNumToIntCard(int r, int sn) {
    return IntCard((r << 2) + sn);
}
constexpr int IntCardToRank(IntCard ic) { return int(ic) >> 2; }
constexpr int IntCardToSuitNum(IntCard ic) { return int(ic) & 3; }
constexpr unsigned int IntCardToSuits(IntCard ic) { return SuitNumToSuits(IntCardToSuitNum(ic)); }

extern std::ostream& operator <<(std::ostream& ost, const IntCard& ic);
extern IntCard StringToIntCard(const std::string& str);

/**************************カード集合**************************/

// 下位からIntCard番目にビットをたてる

using BitCards = uint64_t;

// 基本定数
constexpr BitCards CARDS_HORIZON = 1ULL;
constexpr BitCards CARDS_HORIZONRANK = 15ULL; // 基準の最低ランクのカード全て
constexpr BitCards CARDS_HORIZONSUIT = 0x0111111111111111; // 基準の最小スートのカード全て

// IntCard型との互換
constexpr BitCards IntCardToCards(IntCard ic) { return BitCards(CARDS_HORIZON << ic); }

// 定数
constexpr BitCards CARDS_MIN = CARDS_HORIZON << INTCARD_MIN;
constexpr BitCards CARDS_MAX = CARDS_HORIZON << INTCARD_MAX;

constexpr BitCards CARDS_NULL = 0ULL;
constexpr BitCards CARDS_PLAIN_ALL = 0x00FFFFFFFFFFFFF0;
constexpr BitCards CARDS_IMG_PLAIN_ALL = 0x0FFFFFFFFFFFFFFF;

constexpr BitCards CARDS_D3 = IntCardToCards(INTCARD_D3);
constexpr BitCards CARDS_S3 = IntCardToCards(INTCARD_S3);
constexpr BitCards CARDS_JOKER = IntCardToCards(INTCARD_JOKER);

constexpr BitCards CARDS_ALL = CARDS_PLAIN_ALL + CARDS_JOKER;
constexpr BitCards CARDS_IMG_ALL = CARDS_IMG_PLAIN_ALL + CARDS_JOKER;

// 各ランクのカード全体
constexpr BitCards CARDS_U  = 0x000000000000000F;
constexpr BitCards CARDS_3  = 0x00000000000000F0;
constexpr BitCards CARDS_4  = 0x0000000000000F00;
constexpr BitCards CARDS_5  = 0x000000000000F000;
constexpr BitCards CARDS_6  = 0x00000000000F0000;
constexpr BitCards CARDS_7  = 0x0000000000F00000;
constexpr BitCards CARDS_8  = 0x000000000F000000;
constexpr BitCards CARDS_9  = 0x00000000F0000000;
constexpr BitCards CARDS_T  = 0x0000000F00000000;
constexpr BitCards CARDS_J  = 0x000000F000000000;
constexpr BitCards CARDS_Q  = 0x00000F0000000000;
constexpr BitCards CARDS_K  = 0x0000F00000000000;
constexpr BitCards CARDS_A  = 0x000F000000000000;
constexpr BitCards CARDS_2  = 0x00F0000000000000;
constexpr BitCards CARDS_O  = 0x0F00000000000000;

constexpr BitCards CARDS_JOKER_RANK = 0xF000000000000000;

// あるランクのカード全て
constexpr BitCards RankToCards(int r) { return CARDS_HORIZONRANK << (r << 2); }

// ランク間（両端含む）のカード全て
constexpr BitCards RankRangeToCards(int r0, int r1) {
    return ~((CARDS_HORIZON << (r0 << 2)) - 1ULL)
           & ((CARDS_HORIZON << ((r1 + 1) << 2)) - 1ULL);
}
// スート
// 各スートのカードにはジョーカーは含めない
// UやOは含めるので注意
constexpr BitCards CARDS_C    = 0x0111111111111111;
constexpr BitCards CARDS_D    = 0x0222222222222222;
constexpr BitCards CARDS_CD   = 0x0333333333333333;
constexpr BitCards CARDS_H    = 0x0444444444444444;
constexpr BitCards CARDS_CH   = 0x0555555555555555;
constexpr BitCards CARDS_DH   = 0x0666666666666666;
constexpr BitCards CARDS_CDH  = 0x0777777777777777;
constexpr BitCards CARDS_S    = 0x0888888888888888;
constexpr BitCards CARDS_CS   = 0x0999999999999999;
constexpr BitCards CARDS_DS   = 0x0AAAAAAAAAAAAAAA;
constexpr BitCards CARDS_CDS  = 0x0BBBBBBBBBBBBBBB;
constexpr BitCards CARDS_HS   = 0x0CCCCCCCCCCCCCCC;
constexpr BitCards CARDS_CHS  = 0x0DDDDDDDDDDDDDDD;
constexpr BitCards CARDS_DHS  = 0x0EEEEEEEEEEEEEEE;
constexpr BitCards CARDS_CDHS = 0x0FFFFFFFFFFFFFFF;

// あるスート集合のカード全て
constexpr BitCards SuitsToCards(unsigned s) { return CARDS_HORIZONSUIT * s; }

// ランクとスート集合の指定からのカード集合の生成
constexpr BitCards RankSuitsToCards(int r, unsigned s) { return BitCards(s) << (r << 2); }

// 削除（オーバーを引き起こさない）
// maskは指定以外とのandをとることとしている。
// 指定部分とのandはand処理で行う
constexpr BitCards maskCards(BitCards c0, BitCards c1) { return c0 & ~c1; }
constexpr BitCards maskJOKER(BitCards c) { return maskCards(c, CARDS_JOKER_RANK); }

// 要素数
inline unsigned countCards(BitCards c) { return popcnt(c); }
constexpr BitCards any2Cards(BitCards c) { return c & (c - 1ULL); }

// 排他性
constexpr bool isExclusiveCards(BitCards c0, BitCards c1) { return isExclusiveBits(c0, c1); }

// 包含関係
constexpr BitCards containsJOKER(BitCards c) { return c & CARDS_JOKER_RANK; }
constexpr BitCards containsS3(BitCards c) { return c & CARDS_S3; }
constexpr BitCards containsD3(BitCards c) { return c & CARDS_D3; }
constexpr bool holdsCards(BitCards c0, BitCards c1) { return holdsBits(c0, c1); }

// 空判定
constexpr BitCards anyCards(BitCards c) { return c; }

// validation
constexpr bool examPlainCards(BitCards c) { return holdsCards(CARDS_PLAIN_ALL, c); }
constexpr bool examImaginaryPlainCards(BitCards c) { return holdsCards(CARDS_IMG_PLAIN_ALL, c); }

// 特定順序の要素を選ぶ（元のデータは変えない）
inline BitCards pickLow(const BitCards c, int n) { return lowestNBits(c, n); }
inline BitCards pickHigh(const BitCards c, int n) { return highestNBits(c, n); }

// IntCard型で1つ取り出し
inline IntCard pickIntCardLow(const BitCards c) { return (IntCard)bsf(c); }
inline IntCard pickIntCardHigh(const BitCards c) { return (IntCard)bsr(c); }

// 基準cより高い、低い(同じは含まず)もの
inline BitCards pickHigher(BitCards c) { return allHigherBits(c); }
inline BitCards pickLower(BitCards c) { return allLowerBits(c); }

// ランク重合
// ランクを１つずつ下げてandを取るのみ(ジョーカーとかその辺のことは何も考えない)
// 階段判定に役立つ
template <int N>
constexpr BitCards polymRanks(BitCards c) {
    static_assert(N >= 0, "");
    return (c >> ((N - 1) << 2)) & polymRanks<N - 1>(c);
}
template <> constexpr BitCards polymRanks<0>(BitCards c) { return -1; }
inline BitCards polymRanks(BitCards c, int n) { // 重合数が変数の場合
    assert(n > 0);
    while (--n) c = polymRanks<2>(c);
    return c;
}
constexpr BitCards polymJump(BitCards c) { return c & (c >> 8); }

// ランク展開
// ランクを１つずつ上げてorをとるのが基本だが、
// 4以上の場合は倍々で増やしていった方が少ない命令で済む
template <int N>
inline BitCards extractRanks(BitCards c) {
    static_assert(N >= 0, "");
    return (c << ((N - 1) << 2)) | extractRanks<N - 1>(c);
}
template <> constexpr BitCards extractRanks<0>(BitCards c) { return CARDS_NULL; }
inline BitCards extractRanks(BitCards c, int n) { // 展開数が変数の場合
    assert(n > 0);
    while (--n) c = extractRanks<2>(c);
    return c;
}

inline BitCards polymRanksWithJOKER(BitCards c, int qty) {
    BitCards r;
    switch (qty) {
        case 0: r = CARDS_NULL; break;
        case 1: r = CARDS_ALL; break;
        case 2: r = c; break;
        case 3: {
            BitCards d = c & (c >> 4);
            r = (d | (d >> 4)) | (c & (c >> 8));
        } break;
        case 4: {
            BitCards d = c & (c >> 4);
            BitCards f = (d & (c >> 12)) | (c & (d >> 8)); // 1 + 2パターン
            BitCards e = d & (c >> 8); // 3連パターン
            if (e) f |= e | (e >> 4);
            r = f;
        } break;
        case 5: {
            BitCards d = c & (c >> 4);
            BitCards g = d | (d >> 12); // 2 + 2パターン
            BitCards e = d & (c >> 8); // 3連
            if (e) {
                g |= (e & (c >> 16)) | (c & (e >> 8)); // 1 + 3パターン
                BitCards f = e & (c >> 12);
                if (f) g |= (f | (f >> 4)); // 4連パターン
            }
            r = g;
        } break;
        default: r = CARDS_NULL; break;
    }
    return r;
}
inline BitCards polymRanks(BitCards plain, int jk, int qty) {
    if (jk == 0) return polymRanks(plain, qty);
    else return polymRanksWithJOKER(plain, qty);
}

// 役の作成可能性
// あくまで作成可能性。展開可能な型ではなく有無判定の速度を優先したもの
inline BitCards canMakePlainSeq(BitCards c, int qty) {
    assert(!containsJOKER(c));
    return polymRanks(c, qty);
}
inline BitCards canMakeJokerSeq(BitCards c, int joker, int qty) {
    assert(!containsJOKER(c));
    if (joker == 0) return 0;
    BitCards res;
    switch (qty) {
        case 0: res = CARDS_NULL; break;
        case 1: res = CARDS_NULL; break;
        case 2: res = anyCards(c); break;
        case 3: res = (c & (c >> 4)) | (c & (c >> 8)); break;
        case 4: {
            BitCards c12 = c & (c >> 4), c3 = c >> 8, c4 = c >> 12;
            res = (c12 & c3) | (c12 & c4) | (c & c3 & c4);
        } break;
        case 5: {
            BitCards c12 = c & (c >> 4), c3 = c >> 8, c4 = c >> 12, c5 = c >> 16, c45 = c4 & c5;
            res = (c12 & c3 & c4) | (c12 & c3 & c5) | (c12 & c45) | (c & c3 & c45);
        } break;
        default: res = CARDS_NULL; break;
    }
    return res;
}
inline BitCards canMakeSeq(BitCards c, int joker, int qty) {
    assert(!containsJOKER(c));
    if (joker == 0) return canMakePlainSeq(c, qty);
    else return canMakeJokerSeq(c, joker, qty);
}

// 主に支配性判定用の許容ゾーン計算
// 支配性の判定のため、合法着手がカード集合表現のどこに存在しうるかを計算
inline BitCards ORToGValidZone(int ord, int rank) { // ランク限定のみ
    BitCards res;
    switch (ord) {
        case 0: res = RankRangeToCards(rank + 1, RANK_MAX); break;
        case 1: res = RankRangeToCards(RANK_MIN, rank - 1); break;
        case 2: res = RankRangeToCards(RANK_MIN, RANK_MAX) - RankToCards(rank); break;
        default: assert(0); res = CARDS_NULL; break;
    }
    return res;
}

inline BitCards ORQToSCValidZone(int ord, int rank, int qty) { // ランク限定のみ
    BitCards res;
    switch (ord) {
        case 0: res = RankRangeToCards(rank + qty, RANK_MAX); break;
        case 1: res = RankRangeToCards(RANK_MIN, rank - 1); break;
        case 2: res = RankRangeToCards(RANK_MIN, rank - 1)
                      | RankRangeToCards(rank + qty, RANK_MAX); break;
        default: assert(0); res = CARDS_NULL; break;
    }
    return res;
}

// 許容包含
// あるランクやスートを指定して、そのランクが許容ゾーンに入るか判定する
// MINやMAXとの比較は変な値が入らない限りする必要がないので省略している
inline bool isValidGroupRank(int moveRank, int order, int boardRank) {
    if (order == 0) return moveRank > boardRank;
    else return moveRank < boardRank;
}
inline bool isValidSeqRank(int moveRank, int order, int boardRank, int qty) {
    if (order == 0) return moveRank >= boardRank + qty;
    else return moveRank <= boardRank - qty;
}

/**************************カード集合表現(クラス版)**************************/

union Cards {
    BitCards c_;
    struct {
        unsigned long long plain_: 60;
        signed int joker_: 4;
    };

    // 定数
    constexpr Cards(): c_() {}
    constexpr Cards(BitCards c): c_(c) {}
    constexpr Cards(const Cards& c): c_(c.c_) {}
    constexpr Cards(BitCards plain, int joker): plain_(plain), joker_(joker) {}
    constexpr Cards(IntCard ic) = delete; // 混乱を避ける

    Cards(const std::string& str);
    Cards(const char *cstr): Cards(std::string(cstr)) {}

    // 生のBitCards型への変換
    constexpr operator BitCards() const { return c_; }

    constexpr bool empty() const { return !anyCards(c_); }
    constexpr bool any() const { return anyCards(c_); }
    constexpr bool any2() const { return any2Cards(c_); }

    constexpr bool contains(IntCard ic) const { return c_ & IntCardToCards(ic); }

    constexpr unsigned joker() const { return joker_; }
    constexpr Cards plain() const { return plain_; }

    unsigned count() const { return joker_ + countPlain(); }
    unsigned countPlain() const { return countCards(plain_); }

    constexpr bool holds(Cards c) const {
        return joker_ >= c.joker_ && holdsCards(plain_, c.plain_);
    }
    constexpr bool isExclusive(Cards c) const {
        return joker_ + c.joker_ <= N_JOKERS && isExclusiveCards(plain_, c.plain_);
    }

    Cards masked(Cards c) const {
        return Cards(plain_ & ~c.plain_, std::max(0, joker_ - c.joker_));
    }
    Cards high(int n) const {
        if (joker() >= n) return Cards(0, n);
        else return Cards(pickHigh(plain_, n - joker_), joker_);
    }
    Cards common(Cards c) const {
        return Cards(plain_ & c.plain_, std::min(joker_, c.joker_));
    }

    // 指定されたランクのスート集合を得る
    constexpr unsigned int operator[] (int r) const { return (c_ >> (r * 4)) & 15; }

    Cards& operator |=(BitCards c) { c_ |= c; return *this; }
    Cards& operator &=(BitCards c) { c_ &= c; return *this; }
    Cards& operator ^=(BitCards c) { c_ ^= c; return *this; }
    Cards& operator +=(BitCards c) { c_ += c; return *this; }
    Cards& operator -=(BitCards c) { c_ -= c; return *this; }
    Cards& operator <<=(int i) { c_ <<= i; return *this; }
    Cards& operator >>=(int i) { c_ >>= i; return *this; }
    Cards& operator <<=(unsigned i) { c_ <<= i; return *this; }
    Cards& operator >>=(unsigned i) { c_ >>= i; return *this; }

    Cards& clear() { c_ = 0; return *this; }
    Cards& fill() { c_ = CARDS_ALL; return *this; }

    Cards& merge(BitCards c) { return *this += c; }
    Cards& mask(BitCards c) { return *this &= ~c; }
    Cards& maskJOKER() { return mask(CARDS_JOKER_RANK); }

    Cards& insert(IntCard ic) { return *this |= IntCardToCards(ic); }
    Cards& insertJOKER() { return *this |= CARDS_JOKER; }

    Cards& remove(IntCard ic) { return mask(IntCardToCards(ic)); }
    Cards& forceRemove(IntCard ic) {
        assert(contains(ic));
        return *this -= IntCardToCards(ic);
    }
    Cards& forceRemoveAll(BitCards c) {
        assert(holds(c));
        return *this -= c;
    }

    // pick, pop
    IntCard lowest() const {
        assert(any());
        return IntCard(bsf(c_));
    }
    IntCard highest() const {
        assert(any());
        return IntCard(bsr(c_));
    }
    IntCard popLowest() {
        assert(any());
        IntCard ic = lowest();
        c_ = popLsb(c_);
        return ic;
    }
    IntCard popHighest() {
        assert(any());
        IntCard ic = highest();
        remove(ic);
        return ic;
    }
    Cards lowestCard() const {
        assert(any());
        return lsb(c_);
    }
    Cards popLowestCard() {
        assert(any());
        BitCards l = lowestCard();
        c_ -= l;
        return l;
    }
    Cards exceptLowest() const { return popLsb(c_); }

    class const_iterator {
        friend Cards;
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = IntCard;
        using pointer           = IntCard*;
        using reference         = IntCard&;
        using iterator_category = std::input_iterator_tag;
        IntCard operator *() const {
            return IntCard(bsf<BitCards>(c_));
        }
        bool operator !=(const const_iterator& itr) const {
            return pclass_ != itr.pclass_ || c_ != itr.c_;
        }
        const_iterator& operator ++() {
            c_ &= c_ - 1; // 下1ビットのみ消す
            return *this;
        }
    protected:
        explicit const_iterator(const Cards *pclass): pclass_(pclass), c_(pclass->c_) {}
        explicit const_iterator(const Cards *pclass, BitCards c): pclass_(pclass), c_(c) {}
        const Cards *const pclass_;
        BitCards c_;
    };

    const_iterator begin() const { return const_iterator(this); }
    const_iterator end() const { return const_iterator(this, 0); }

    bool exam() const {
        return joker_ <= N_JOKERS && examPlainCards(plain());
    }

    std::string toString() const;
};

extern std::ostream& operator <<(std::ostream& ost, const Cards& c);

struct CardArray : public BitArray64<4, 16> {
    constexpr CardArray(): BitArray64<4, 16>() {}
    constexpr CardArray(BitCards c): BitArray64<4, 16>(c) {}
    constexpr CardArray(const Cards& c): BitArray64<4, 16>(c.c_) {}
    operator BitCards() const { return BitCards(data()); }
};

inline BitCards canMakeSeq(Cards c, int qty) {
    int joker = c.joker();
    if (joker == 0) return canMakePlainSeq(c, qty);
    else return canMakeJokerSeq(c.plain(), joker, qty);
}

/**************************カード集合表現の変形型**************************/

// 諸々の演算のため、ビット集合として表現されたカード集合を同じく64ビット整数である別の表現系に変換する
// 演算の際に、64ビット全体にかかる演算か存在するランクの部分のみにかかる演算かは問題になるが、
// 現在はいずれにせよ、ジョーカーは絡めないことにしている

// 無支配ゾーン
extern BitCards ORQ_NDTable[2][16][4]; // (order, rank, qty - 1)

// 現在用意されている型は以下

// ER ランク存在型
// FR ランク全存在型
// MER ランク存在マスク型
// QR ランク枚数型
// PQR ランク枚数位置型
// SC スート圧縮型
// ND 無支配型(ジョーカーのビットは関係ないが、存在は加味)

// PQR定数
constexpr BitCards PQR_1    = CARDS_IMG_PLAIN_ALL & 0x1111111111111111;
constexpr BitCards PQR_2    = CARDS_IMG_PLAIN_ALL & 0x2222222222222222;
constexpr BitCards PQR_3    = CARDS_IMG_PLAIN_ALL & 0x4444444444444444;
constexpr BitCards PQR_4    = CARDS_IMG_PLAIN_ALL & 0x8888888888888888;
constexpr BitCards PQR_12   = CARDS_IMG_PLAIN_ALL & 0x3333333333333333;
constexpr BitCards PQR_13   = CARDS_IMG_PLAIN_ALL & 0x5555555555555555;
constexpr BitCards PQR_14   = CARDS_IMG_PLAIN_ALL & 0x9999999999999999;
constexpr BitCards PQR_23   = CARDS_IMG_PLAIN_ALL & 0x6666666666666666;
constexpr BitCards PQR_24   = CARDS_IMG_PLAIN_ALL & 0xaaaaaaaaaaaaaaaa;
constexpr BitCards PQR_34   = CARDS_IMG_PLAIN_ALL & 0xcccccccccccccccc;
constexpr BitCards PQR_123  = CARDS_IMG_PLAIN_ALL & 0x7777777777777777;
constexpr BitCards PQR_124  = CARDS_IMG_PLAIN_ALL & 0xbbbbbbbbbbbbbbbb;
constexpr BitCards PQR_134  = CARDS_IMG_PLAIN_ALL & 0xdddddddddddddddd;
constexpr BitCards PQR_234  = CARDS_IMG_PLAIN_ALL & 0xeeeeeeeeeeeeeeee;
constexpr BitCards PQR_1234 = CARDS_IMG_PLAIN_ALL & 0xffffffffffffffff;

// 定義通りの関数
constexpr BitCards QtyToPQR(unsigned q) { return PQR_1 << (q - 1); }

// パラレル演算関数
inline CardArray CardsToQR(BitCards c) {
    // 枚数が各4ビットに入る
    BitCards a = (c & PQR_13) + ((c >> 1) & PQR_13);
    return (a & PQR_12) + ((a >> 2) & PQR_12);
}

// ランク中に丁度 n ビットあれば PQR_1 の位置にビットが立つ
constexpr BitCards QRToFR(BitCards qr) { return (qr >> 2) & PQR_1; }
constexpr BitCards QRTo3R(BitCards qr) { return (qr >> 1) & qr; }
constexpr BitCards QRTo2R(BitCards qr) { return (qr >> 1) & ~qr & PQR_1; }
constexpr BitCards QRTo1R(BitCards qr) { return ~(qr >> 1) & qr & PQR_1; }
constexpr BitCards QRTo0R(BitCards qr) { return ~(qr | (qr >> 1) | (qr >> 2)) & PQR_1; }
inline BitCards QRToNR(BitCards qr, int q) {
    BitCards nr;
    switch (q) {
        case 0: nr = QRTo0R(qr); break;
        case 1: nr = QRTo1R(qr); break;
        case 2: nr = QRTo2R(qr); break;
        case 3: nr = QRTo3R(qr); break;
        case 4: nr = QRToFR(qr); break;
        default: assert(0); nr = CARDS_NULL; break;
    }
    return nr;
}

inline BitCards CardsToFR(BitCards c) {
    BitCards a = c & (c >> 1);
    return a & (a >> 2) & PQR_1;
}
inline BitCards CardsTo3R(BitCards c) {
    BitCards a = c & (c >> 1);
    BitCards b = c ^ (c >> 1);
    return ((a & (b >> 2)) | ((a >> 2) & b)) & PQR_1;
}
inline BitCards CardsTo2R(BitCards c) { return QRTo2R(CardsToQR(c)); }
inline BitCards CardsTo1R(BitCards c) {
    BitCards a = ~(c | (c >> 1));
    BitCards b = c ^ (c >> 1);
    return ((a & (b >> 2)) | ((a >> 2) & b)) & PQR_1;
}
inline BitCards CardsTo0R(BitCards c) {
    BitCards a = ~(c | (c >> 1));
    return a & (a >> 2) & PQR_1;
}
inline BitCards CardsToNR(BitCards c, int q) {
    BitCards nr;
    switch (q) {
        case 0: nr = CardsTo0R(c); break;
        case 1: nr = CardsTo1R(c); break;
        case 2: nr = CardsTo2R(c); break;
        case 3: nr = CardsTo3R(c); break;
        case 4: nr = CardsToFR(c); break;
        default: assert(0); nr = CARDS_NULL; break;
    }
    return nr;
}

// ランク中にnビット以上あればPQR_1の位置にビットが立つ
inline BitCards CardsToER(BitCards c) {
    BitCards a = c | (c >> 1);
    return (a | (a >> 2)) & PQR_1;
}
inline BitCards CardsToE2R(BitCards c) {
    BitCards a = c & (c >> 1);
    BitCards b = c | (c >> 1);
    return (a | (a >> 2) | (b & (b >> 2))) & PQR_1;
}
inline BitCards CardsToE3R(BitCards c) {
    BitCards a = c & (c >> 1);
    BitCards b = c | (c >> 1);
    return ((a & (b >> 2)) | ((a >> 2) & b)) & PQR_1;
}
inline BitCards CardsToENR(BitCards c, int q) {
    BitCards enr;
    switch (q) {
        case 0: enr = -1; break;
        case 1: enr = CardsToER(c); break;
        case 2: enr = CardsToE2R(c); break;
        case 3: enr = CardsToE3R(c); break;
        case 4: enr = CardsToFR(c); break;
        default: assert(0); enr = CARDS_NULL; break;
    }
    return enr;
}

constexpr BitCards QRToPQR(BitCards qr) {
    // qr -> pqr 変換
    return qr + (qr & PQR_3) + (qr & (qr >> 1));
}
inline BitCards CardsToPQR(BitCards c) {
    // ランクごとの枚数を示す位置にビットが立つようにする
    return QRToPQR(CardsToQR(c));
}
inline BitCards PQRToSC(BitCards pqr) {
    // pqr -> sc はビットを埋めていくだけ
    BitCards r = pqr;
    r |= (r & PQR_234) >> 1;
    r |= (r & PQR_34) >> 2;
    return r;
}
inline void PQRToND(BitCards pqr, unsigned jk, Cards *const nd) {
    // pqr -> nd[2] 変換
    // ジョーカーの枚数の情報も必要
    assert(jk == 0 || jk == 1); // 0or1枚
    assert(nd != nullptr);

    nd[0] = nd[1] = CARDS_NULL;

    // pqrの1ランクシフトが無支配限度
    // 以降、tmpのビットの示す位置は実際にカードがあるランクよりずれている事に注意
    BitCards tmp0 = pqr >> 4;
    BitCards tmp1 = pqr << 4;
    while (tmp0) { // 無支配ゾーンがまだ広いはず
        IntCard ic = pickIntCardHigh(tmp0);
        int r = IntCardToRank(ic);
        int sn = IntCardToSuitNum(ic);
        nd[0] |= ORQ_NDTable[0][r][sn]; // このゾーンに対して返せることが確定
        tmp0 &= ~nd[0]; // もう関係なくなった部分は外す
    }
    while (tmp1) { // 無支配ゾーンがまだ広いはず
        IntCard ic = pickIntCardLow(tmp1);
        int r = IntCardToRank(ic);
        int sn = IntCardToSuitNum(ic);
        nd[1] |= ORQ_NDTable[1][r][sn]; // このゾーンに対して返せることが確定
        tmp1 &= ~nd[1]; // もう関係なくなった部分は外す
    }

    // ジョーカーがある場合は1枚分ずらして、全てのシングルを加える
    // 逆転オーダーの場合は+の4枚にフラグがある可能性があるのでマスクする
    if (jk) {
        nd[0] <<= 1;
        nd[0] |= PQR_1;
        nd[1] &= PQR_123;
        nd[1] <<= 1;
        nd[1] |= PQR_1;
    }
}

// あるプレーンカード集合の中にn枚グループが作成可能かの判定
inline bool canMakeGroup(BitCards c, int n) {
    if (c) {
        if (n <= 1) return true;
        c = CardsToQR(c);
        if (c & PQR_234) { // 2枚以上
            if (n == 2) return true;
            if (c & PQR_34) { // 4枚
                if (n <= 4) return true;
            } else {
                if (c & (c >> 1)) { // 3枚
                    if (n == 3) return true;
                }
            }
        }
    }
    return false;
}

// 一枚一枚に乱数をあてたゾブリストハッシュ
// 線形のため合成や進行が楽
constexpr uint64_t cardsHashKeyTable[64] = {
    // インデックスがIntCard番号に対応
    0x15cc5ec4cae423e2, 0xa1373ceae861f22a, 0x7b60ee1280de0951, 0x970b602e9f0a831a,
    0x9c2d0e84fa38fd7b, 0xf8e8f5de24c6613c, 0x59e1e0ec5c2dcf0f, 0xee5236f6cc5ecd7c,
    0x955cdae1107b0a6f, 0x664c969fef782110, 0x131d24cfbc6cc542, 0x4747206ff1446e2c,
    0x02ea232067f62eec, 0xb53c73b144873900, 0x62623a5213bdae74, 0x655c7b3f43d2ea77,
    0x4e4f49ed97504cd0, 0x62e37cdd7416e4d1, 0x8d82596514f50486, 0x85eeb4e5f361ad26,
    0xca376df878e7b568, 0x012caaf9c1c68d82, 0x9ae28611b76ac1d6, 0xe8b42904d7ac4688,
    0x50ebe782f7343538, 0xf2876e2b5a0d5da5, 0xf308e93cd29a1fb5, 0x3e58ae2a9e1fb64a,
    0x143a9b63f5128d58, 0xd7e31ea845745bf5, 0xcc59315a5031ae64, 0x77591890cfbe493a,
    0xea239dd1932bfc0b, 0xbb4a9b581dc50a58, 0xd7640b6cb72a9798, 0x537b3fcac53dcefc,
    0xa52fb140c73cc931, 0xd123cf73f9aab466, 0x6eed725d80ead216, 0x151b7aa1f03f0532,
    0xfba74ec660ed2e46, 0x8aa22769ccf87343, 0x1896000f642b41ac, 0x97b0de139c5b487c,
    0x20a10996d700d1b3, 0x76e8529b3f3d425e, 0xf48b294add39ea07, 0x1abdb74a2202e8ea,
    0xff502998b9aed7e7, 0x6629aa61eb40d7e0, 0x87e72aef918d27b7, 0xf1b25fdda49f70bd,
    0xb10abbc401dd1e03, 0xb9ddbad67b370949, 0xefa07417c6906e38, 0x1616cec390c9db9f,
    0xb048124c6ef48ff5, 0x65978b47dbc1debb, 0x925e60277ee19bbf, 0xed776c6b664087e8,
    0x29bf249af2b02a7b, 0xc64ed74ce9ea7c77, 0xc05774752bed93f3, 0x5fc31db82af16d07,
};

constexpr uint64_t IntCardToHashKey(IntCard ic) {
    return cardsHashKeyTable[ic];
}
constexpr uint64_t addCardKey(uint64_t a, uint64_t b) {
    return a + b;
}
constexpr uint64_t subCardKey(uint64_t a, uint64_t b) {
    return a - b;
}
inline uint64_t CardsToHashKey(Cards c) {
    uint64_t key = 0ULL;
    for (IntCard ic : c) key = addCardKey(key, IntCardToHashKey(ic));
    return key;
}

extern uint64_t HASH_CARDS_ALL;

inline uint64_t CardsCardsToHashKey(Cards c0, Cards c1) {
    return cross64(CardsToHashKey(c0), CardsToHashKey(c1));
}
constexpr uint64_t knitCardsCardsHashKey(uint64_t key0, uint64_t key1) {
    return cross64(key0, key1);
}

extern void initCards();

/**************************着手表現**************************/

struct Move {
    unsigned s       : 4;
    unsigned r       : 4;
    unsigned jks     : 4;
    unsigned jkr     : 4;
    unsigned q       : 4;
    unsigned t       : 2;
    unsigned         : 1;
    unsigned invalid : 1;
    unsigned o       : 1;
    unsigned po      : 1;
    unsigned sl      : 1;
    unsigned rl      : 1;
    unsigned reverse : 1;
    unsigned         : 3;
    unsigned flags   :32;

    uint32_t toInt() const {
        return uint32_t(*reinterpret_cast<const uint64_t*>(this));
    }
    bool operator ==(const Move& m) const {
        return toInt() == m.toInt();
    }
    static Move fromInt(uint32_t a) {
        Move m;
        *reinterpret_cast<uint64_t*>(&m) = uint64_t(a);
        return m;
    }

    void clear()                      { *this = Move({0}); }
    void setPASS()                    { clear(); t = 0; }
    void setSingleJOKER()             { clear(); q = 1; t = 1; jks = SUITS_ALL; } // シングルジョーカーのランクは未定義
    void setS3()                      { setSingle(INTCARD_S3); } // スペ3切りの場合のみ
    void setJokerRank(unsigned jr)    { jkr = jr; }
    void setJokerSuits(unsigned js)   { jks = js; }
    void setSpecialJokerSuits()       { jks = SUITS_ALL; }

    // タイプを指定してまとめて処理
    void setGroup(int qty, int rank, int suits) {
        clear(); t = 1; q = qty; r = rank; s = suits;
    }
    void setSeq(int qty, int rank, int suits) {
        clear(); t = 2; q = qty; r = rank; s = suits;
    }
    // IntCard型からシングル着手をセットする
    void setSingle(IntCard ic) {
        setGroup(1, IntCardToRank(ic), IntCardToSuits(ic));
    }

    // True or False
    bool isPASS() const { return t == 0; }
    bool isGroup() const { return t == 1; }
    bool isSeq() const { return t == 2; }
    bool isSingle() const { return qty() == 1; }
    bool containsJOKER() const { return jks || jkr; }
    bool isSingleJOKER() const { return isSingle() && jks == SUITS_ALL; }
    bool isS3() const { return isSingle() && rank() == RANK_3 && suits() == SUITS_S; }

    // 情報を得る
    unsigned suits()      const { return s; }
    int qty()             const { return q; }
    int rank()            const { return r; }
    int jokerRank()       const { return jkr; }
    unsigned jokerSuits() const { return jks; }
    int type()            const { return t; }

    // ランク全体
    RankRange ranks() const {
        return RankRange(rank(), rank() + (isSeq() ? (qty() - 1) : 0));
    }

    // 特別スート関係
    bool isExtendedJokerGroup() const {
        return isGroup() && jokerSuits() == 15;
    }
    unsigned extendedJokerSuits() const {
        return jokerSuits() | (isExtendedJokerGroup() ? SUITS_X : 0);
    }
    unsigned extendedSuits() const {
        return suits() | (isExtendedJokerGroup() ? SUITS_X : 0);
    }

    Cards cards() const { // 構成するカード集合を得る
        if (isPASS()) return CARDS_NULL;
        if (isSingleJOKER()) return CARDS_JOKER;
        int r = rank();
        unsigned s = suits();
        if (!isSeq()) {
            Cards c = CARDS_NULL;
            unsigned jks = jokerSuits();
            if (jks) {
                c |= CARDS_JOKER;
                if (jks != SUITS_ALL) s -= jks; // クインタプル対策
            }
            return c | RankSuitsToCards(r, s);
        } else {
            Cards c = RankSuitsToCards(r, s);
            c = extractRanks(c, qty());
            if (containsJOKER()) {
                c -= RankSuitsToCards(jokerRank(), s);
                c |= CARDS_JOKER;
            }
            return c;
        }
    }

    Cards charaCards() const {
        // 性質カードを返す
        // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
        if (isPASS()) return CARDS_NULL;
        if (isSingleJOKER()) return CARDS_JOKER;
        BitCards c = RankSuitsToCards(rank(), suits());
        if (isSeq()) c = extractRanks(c, qty());
        return c;
    }

    Cards charaPQR() const {
        // 性質カードのPQRを返す (グループ限定)
        return BitCards(1U << (qty() - 1)) << (rank() << 2);
    }

    bool domInevitably() const;
    bool isRev() const;
    bool isBack() const;
    bool exam() const;
};

const Move MOVE_NULL = {0};
const Move MOVE_PASS = {0};
const Move MOVE_NONE = {15, 15};

struct MeldChar : public Move {
    MeldChar(Move m): Move(m) {}
};

extern std::ostream& operator <<(std::ostream& ost, const MeldChar& m);
extern std::ostream& operator <<(std::ostream& ost, const Move& m);
extern std::string toRecordString(Move m);
extern Move CardsToMove(const Cards chara, const Cards used);
extern Move StringToMoveM(const std::string& str);

template <class move_buf_t>
int searchMove(const move_buf_t *const buf, const int numMoves, const Move& move) {
    // 同じ着手の探索
    for (int i = 0; i < numMoves; i++) {
        if (buf[i] == move) return i;
    }
    return -1;
}

/**************************場表現**************************/

// 各プレーヤーの情報を持たない場表現
// 着手表現と同一の系列

struct Board : public Move {
    void init() { clear(); }
    Move move() const { return Move(*this); }

    void setTmpOrder(int ord) { Move::o = ord; }
    void setPrmOrder(int ord) { Move::po = ord; }

    void flipTmpOrder() { Move::o ^= 1; }
    void flipPrmOrder() { Move::po ^= 1; }

    // 場 x 提出役 の効果
    bool domConditionally(Move m) const;

    bool locksSuits(Move m) const;
    bool locksRank(Move m) const;

    int nextPrmOrder(Move m) const {
        return prmOrder() ^ bool(m.isRev());
    }
    int nextOrder(Move m) const {
        return order() ^ bool(m.isRev()) ^ bool(m.isBack());
    }

    // get
    int prmOrder()   const { return Move::po; }
    int order()      const { return Move::o; }

    // true or false
    bool isNull() const { return type() == 0; }
    bool suitsLocked() const { return Move::sl; }
    bool rankLocked() const { return Move::rl; }
    bool isRev() const { return Move::po; }

    bool isInvalid() const { return Move::invalid; }

    // 進行
    void procOrder(Move m) {
        if (m.isRev()) {
            flipTmpOrder();
            flipPrmOrder();
        }
        if (m.isBack()) flipTmpOrder();
    }
    void flush() {
        int ord = Move::po;
        init();
        setPrmOrder(ord);
        setTmpOrder(ord);
    }
    void lockSuits() { Move::sl = 1; }

    void setMeld (Move m) {
        Move::s = m.s;
        Move::r = m.r;
        Move::jks = m.jks;
        Move::jkr = m.jkr;
        Move::q = m.q;
        Move::t = m.t;
    }

    void proc(Move m) { // プレーヤー等は関係なく局面のみ進める
        if (m.isPASS()) return;
        if (m.domInevitably() || domConditionally(m)) { // 無条件完全支配
            if (m.isRev()) flipPrmOrder();
            flush();
        } else {
            procOrder(m);
            if (locksSuits(m)) lockSuits();
            setMeld(m);
        }
    }

    void procAndFlush(Move m) {
        // 局面を更新し、強引に場を流す
        if (m.isRev()) flipPrmOrder();
        flush();
    }
};

inline Board OrderToNullBoard(int o) {
    Board b;
    b.init();
    b.o = o; b.po = o;
    return b;
}

extern std::ostream& operator <<(std::ostream& ost, const Board& b);
extern bool isSubjectivelyValid(Board b, Move mv, const Cards& c, const int q);

// L2局面ハッシュ値
// 空場
// 空場のときは、場の変数はオーダー関連だけである事が多いのでそのまま
inline uint64_t NullBoardToHashKey(Board b) {
    return b.order();
}
inline uint64_t BoardToHashKey(Board b) {
    return b.toInt();
}

// 完全情報局面のシンプルな後退ハッシュ値
// 先手、後手の順番でカード集合ハッシュ値をクロスして場のハッシュ値を線形加算
inline uint64_t L2NullFieldToHashKey(Cards c0, Cards c1, Board b) {
    return CardsCardsToHashKey(c0, c1) ^ NullBoardToHashKey(b);
}

// すでにハッシュ値が部分的に計算されている場合
inline uint64_t knitL2NullFieldHashKey(uint64_t ckey0, uint64_t ckey1, uint64_t boardKey) {
    return knitCardsCardsHashKey(ckey0, ckey1) ^ boardKey;
}

/**************************着手決定のための基本追加場情報**************************/

// 試合結果の宣言情報

constexpr int LCT_FINAL    = 16;
constexpr int LCT_PW       = 17;
constexpr int LCT_MPMATE   = 18;
constexpr int LCT_L2MATE   = 19;
constexpr int LCT_MPGIVEUP = 20;
constexpr int LCT_L2GIVEUP = 21;

// 場の一時状況に対するする宣言情報
constexpr int LCT_SELFFOLLOW = 22;
constexpr int LCT_UNRIVALED = 23;
constexpr int LCT_LASTAWAKE = 24;
constexpr int LCT_FLUSHLEAD = 25;
constexpr int LCT_NPDOM = 26;
constexpr int LCT_PDOM = 27;
constexpr int LCT_DOMOTHERS = 28;
constexpr int LCT_DOMME = 29;

constexpr int LCT_CHECKED = 31;

struct FieldAddInfo {
    // 着手決定のためにこの程度は調べておきたい場情報
    // 着手ごとの情報と被る場合もあるけれども、検索が面倒な場合はこちらに記録しておく

    uint32_t data;

    // set
    void setFinal() {    set(LCT_FINAL, LCT_PW, LCT_MPMATE); }
    void setPW() {       set(LCT_PW, LCT_MPMATE); }
    void setMPMate() {   set(LCT_MPMATE); }
    void setL2Mate() {   set(LCT_L2MATE); }
    void setMPGiveUp() { set(LCT_MPGIVEUP); }
    void setL2GiveUp() { set(LCT_L2GIVEUP); }

    void setSelfFollow() { set(LCT_SELFFOLLOW, LCT_UNRIVALED, LCT_LASTAWAKE, LCT_FLUSHLEAD, LCT_NPDOM, LCT_PDOM, LCT_DOMOTHERS); }
    void setUnrivaled() { set(LCT_UNRIVALED, LCT_FLUSHLEAD, LCT_NPDOM, LCT_PDOM, LCT_DOMOTHERS); }
    void setLastAwake() { set(LCT_LASTAWAKE, LCT_NPDOM, LCT_DOMOTHERS); }
    void setFlushLead() { set(LCT_FLUSHLEAD); }
    void setNPDom() { set(LCT_NPDOM); }
    void setPassDom() { set(LCT_PDOM); }
    void setBDO() { set(LCT_DOMOTHERS); }
    void setBDM() { set(LCT_DOMME); }
    void setBDALL() { set(LCT_DOMOTHERS, LCT_DOMME); }
    void setNoChance() { setBDM(); }

    void setMinNumCards(uint32_t n) { assert(n < 16U); data = (data & 0xFFFFFFF0U) | n; }
    void setMaxNumCards(uint32_t n) { assert(n < 16U); data = (data & 0xFFFFFF0FU) | (n << 4); }
    void setMinNumCardsAwake(uint32_t n) { assert(n < 16U); data = (data & 0xFFFFF0FFU) | (n << 8); }
    void setMaxNumCardsAwake(uint32_t n) { assert(n < 16U); data = (data & 0xFFFF0FFFU) | (n << 12); }

    // get
    // 一時情報
    bool isFinal() const {    return test(LCT_FINAL); }
    bool isPW() const {       return test(LCT_PW); }
    bool isMPMate() const {   return test(LCT_MPMATE); }
    bool isMPGiveUp() const { return test(LCT_MPGIVEUP); }
    bool isL2Mate() const {   return test(LCT_L2MATE); }
    bool isL2GiveUp() const { return test(LCT_L2GIVEUP); }
    bool isMate() const {     return test(LCT_FINAL, LCT_PW, LCT_MPMATE, LCT_L2MATE); }
    bool isGiveUp() const {   return test(LCT_MPGIVEUP, LCT_L2GIVEUP); }

    bool isSelfFollow() const { return test(LCT_SELFFOLLOW); }
    bool isUnrivaled() const { return test(LCT_UNRIVALED); }
    bool isLastAwake() const { return test(LCT_LASTAWAKE); }
    bool isFlushLead() const { return test(LCT_FLUSHLEAD); }
    bool isNonPassDom() const { return test(LCT_NPDOM); }
    bool isPassDom() const { return test(LCT_PDOM); }
    bool isBDO() const { return test(LCT_DOMOTHERS); }
    bool isBDM() const { return test(LCT_DOMME); }
    bool isBDALL() const { return holds(LCT_DOMOTHERS, LCT_DOMME); }
    bool isNoChance() const { return isBDM(); }

    uint32_t minNumCards() const { return data & 15U; }
    uint32_t maxNumCards() const { return (data >> 4) & 15U; }
    uint32_t minNumCardsAwake() const { return (data >> 8) & 15U; }
    uint32_t maxNumCardsAwake() const { return (data >> 12) & 15U; }

    void init() {
        // カード枚数については、無設定の場合はmaxが15、minの場合は0になるようにする
        data = 0x0000F0F0U;
    }
    void procTmpInfo() {
        data &= 0x0000FFFFU;
    }

    constexpr FieldAddInfo(): data() {}
    constexpr FieldAddInfo(const FieldAddInfo& arg): data(arg.data) {}

    void set(size_t i) { data |= 1U << i; }
    template <class... args_t>
    void set(size_t i0, args_t... args) { set(i0); set(args...); }
    bool holds(size_t i0, size_t i1) const {
        uint32_t dst = (1U << i0) | (1U << i1);
        return !(~data & dst);
    }
    bool test(size_t i) const { return data & (1U << i); }
    template <class... args_t>
    bool test(size_t i0, args_t... args) const { return test(i0) || test(args...); }
};

static std::ostream& operator <<(std::ostream& out, const FieldAddInfo& i) { // 出力
    out << "Field :";
    if (i.isFinal()) out << " -FIN";
    else if (i.isPW()) out << " -PW";
    else if (i.isMPMate()) out << " -MPMATE";

    if (i.isL2Mate()) out << " -L2MATE";

    if (i.isMPGiveUp()) out << " -MPGIVEUP";
    if (i.isL2GiveUp()) out << " -L2GIVEUP";

    if (i.isSelfFollow()) out << " -SFOL";
    if (i.isUnrivaled()) out << " -UNRIV";
    if (i.isLastAwake()) out << " -LA";
    if (i.isFlushLead()) out << " -FLEAD";
    if (i.isNonPassDom()) out << " -NPD";
    if (i.isPassDom()) out << " -PD";

    if (i.isBDALL()) out << " -BDALL";
    else {
        if (i.isBDO()) out << " -BDO";
        if (i.isBDM()) out << " -BDM";
    }
    return out;
}

inline void flushFieldAddInfo(const FieldAddInfo& fieldInfo, FieldAddInfo *const pnext) {
    pnext->init();
    pnext->setMinNumCardsAwake(fieldInfo.minNumCards());
    pnext->setMaxNumCardsAwake(fieldInfo.maxNumCards());
    pnext->setMinNumCards(fieldInfo.minNumCards());
    pnext->setMaxNumCards(fieldInfo.maxNumCards());
    pnext->setFlushLead();
}
inline void procUnrivaled(const FieldAddInfo& fieldInfo, FieldAddInfo *const pnext) {
    *pnext = fieldInfo;
    pnext->procTmpInfo();
    pnext->setUnrivaled();
}

/**************************着手情報＋追加情報**************************/

// 基本的にはこれを使う

struct MoveInfo : public Move {

    constexpr MoveInfo(): Move() {}
    constexpr MoveInfo(const Move& m): Move(m) {}
    constexpr MoveInfo(const MoveInfo& m): Move(m) {}

    void set(size_t i) { Move::flags |= 1U << i; }
    template <class... args_t>
    void set(size_t i0, args_t... args) { set(i0); set(args...); }
    bool holds(size_t i0, size_t i1) const {
        uint32_t dst = (1U << i0) | (1U << i1);
        return !(~Move::flags & dst);
    }
    bool test(size_t i) const { return Move::flags & (1U << i); }
    template <class... args_t>
    bool test(size_t i0, args_t... args) const { return test(i0) || test(args...); }

    void setFinal() {    set(LCT_FINAL, LCT_PW, LCT_MPMATE); }
    void setPW() {       set(LCT_PW, LCT_MPMATE); }
    void setMPMate() {   set(LCT_MPMATE); }
    void setL2Mate() {   set(LCT_L2MATE); }
    void setMPGiveUp() { set(LCT_MPGIVEUP); }
    void setL2GiveUp() { set(LCT_L2GIVEUP); }

    // 当座支配
    void setDO() { set(LCT_DOMOTHERS); }
    void setDM() { set(LCT_DOMME); }
    void setDALL() { set(LCT_DOMOTHERS, LCT_DOMME); }
    void setDomOthers() { setDO(); }
    void setDomMe() { setDM(); }
    void setDomAll() { setDALL(); }
    void setChecked() { set(LCT_CHECKED); }

    // get
    bool isFinal() const {    return test(LCT_FINAL); }
    bool isPW() const {       return test(LCT_PW); }
    bool isMPMate() const {   return test(LCT_MPMATE); }
    bool isMPGiveUp() const { return test(LCT_MPGIVEUP); }
    bool isL2Mate() const {   return test(LCT_L2MATE); }
    bool isL2GiveUp() const { return test(LCT_L2GIVEUP); }
    bool isMate() const {     return test(LCT_FINAL, LCT_PW, LCT_MPMATE, LCT_L2MATE); }
    bool isGiveUp() const {   return test(LCT_MPGIVEUP, LCT_L2GIVEUP); }

    bool dominatesOthers() const { return test(LCT_DOMOTHERS); }
    bool dominatesMe() const { return test(LCT_DOMME); }
    bool dominatesAll() const { return holds(LCT_DOMME, LCT_DOMOTHERS); }

    bool isChecked() const { return test(LCT_CHECKED); }
};

static std::string toInfoString(const MoveInfo& i, const Board b) { // 出力
    std::ostringstream oss;
    // 勝敗
    if (i.isFinal()) oss << " -FIN";
    else if (i.isPW()) oss << " -PW";
    else if (i.isMPMate()) oss << " -MPMATE";

    if (i.isL2Mate()) oss << " -L2MATE";

    if (i.isMPGiveUp()) oss << " -MPGIVEUP";
    if (i.isL2GiveUp()) oss << " -L2GIVEUP";

    // 後場
    if (b.nextOrder(i) != 0) oss << " -TREV";
    if (b.locksSuits(i)) oss << " -SLOCK";

    // 当座支配
    if (i.dominatesAll()) oss<< " -DALL";
    else {
        if (i.dominatesOthers()) oss << " -DO";
        if (i.dominatesMe()) oss << " -DM";
    }
    return oss.str();
}

/**************************初期化**************************/

struct DaifugoInitializer {
    DaifugoInitializer() {
        initSuits();
        initCards();
        if (Move({1, 1, 1, 1, 3, 2}).toInt() != 2298129) exit(1);
    }
};