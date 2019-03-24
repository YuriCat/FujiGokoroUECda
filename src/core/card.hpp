#pragma once

#include <iostream>
#include <array>
#include <map>

namespace UECda {

    /**************************カード整数**************************/
    
    // U3456789TJQKA2O、CDHSの順番で0-59　ジョーカーは60
    
    // 定数
    using IntCard = int;
    
    enum {
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
    constexpr int N_IMG_CARDS = 61;
    
    constexpr bool examIntCard(IntCard ic) {
        return (INTCARD_C3 <= ic && ic <= INTCARD_S2) || ic == INTCARD_JOKER;
    }
    constexpr bool examImaginaryIntCard(IntCard ic) {
        return INTCARD_CU <= ic && ic <= INTCARD_JOKER;
    }
    
    template <typename rank_t, typename suit_t>
    constexpr IntCard RankSuitsToIntCard(rank_t r, suit_t s) {
        return (r << 2) + SuitToSuitNum(s);
    }
    
    template <typename r4x_t, typename suit_t>
    constexpr IntCard Rank4xSuitsToIntCard(r4x_t r4x, suit_t s) {
        return r4x + SuitToSuitNum(s);
    }
    
    template <typename rank_t>
    constexpr IntCard RankSuitNumToIntCard(rank_t r, int sn) {
        return (r << 2) + sn;
    }
    
    constexpr int IntCardToRank(IntCard ic) { return ic >> 2; }
    constexpr int IntCardToRank4x(IntCard ic) { return ic & ~3U; }
    constexpr int IntCardToSuitNum(IntCard ic) { return ic & 3U; }
    constexpr int IntCardToSuits(IntCard ic) { return SuitNumToSuits(IntCardToSuitNum(ic)); }
    
    // 出力用クラス
    struct OutIntCard {
        IntCard ic;
        constexpr OutIntCard(const IntCard& arg) :ic(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutIntCard& arg) {
        if (arg.ic == INTCARD_JOKER) {
            out << "JO";
        } else {
            out << OutSuitNum(IntCardToSuitNum(arg.ic)) << OutRank(IntCardToRank(arg.ic));
        }
        return out;
    }

    struct OutIntCardM {
        IntCard ic;
        constexpr OutIntCardM(const IntCard& arg) :ic(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutIntCardM& arg) {
        if (arg.ic == INTCARD_JOKER) {
            out << "jo";
        } else {
            out << OutSuitNumM(IntCardToSuitNum(arg.ic)) << OutRankM(IntCardToRank(arg.ic));
        }
        return out;
    }
    
    IntCard StringToIntCardM(const std::string& str) {
        if (str.size() != 2) return INTCARD_NONE;
        if (str == "jo") return INTCARD_JOKER;
        int sn = CharToSuitNumM(str[0]);
        int r = CharToRankM(str[1]);
        if (r == RANK_NONE) return INTCARD_NONE;
        if (sn == SUITNUM_NONE) return INTCARD_NONE;
        return RankSuitNumToIntCard(r, sn);
    }
    
    /**************************カード集合**************************/
    
    // 下位からIntCard番目にビットをたてる
    // 強い方からpopするのを楽にする(SSEを使わなくてよい)ため、昔は逆順だったが
    // ぱおーん氏の作との互換性と簡単のため同一の順(向きが同じだけだが)とした
    
    // 関数名のJOKERは目立つように大文字とした
    
    using BitCard = uint64_t; // 1枚だけの時用
    using BitCards = uint64_t;
    using Cards = uint64_t;
    
    // 基本定数
    constexpr BitCards CARDS_HORIZON = 1ULL;
    constexpr BitCards CARDS_HORIZONRANK = 15ULL; // 基準の最低ランクのカード全て
    constexpr BitCards CARDS_HORIZONSUIT = 0x0111111111111111; // 基準の最小スートのカード全て
    
    // IntCard型との互換
    constexpr BitCards IntCardToCards(IntCard ic) {
        return BitCards(CARDS_HORIZON << ic);
    }
    
    IntCard CardsToLowestIntCard(BitCards c) { return bsf64(c); } // 一番低いもの
    IntCard CardsToHighestIntCard(BitCards c) { return bsr64(c); } // 一番高いもの
    
    // 定数
    constexpr BitCards CARDS_IMG_MIN = CARDS_HORIZON << INTCARD_IMG_MIN;
    constexpr BitCards CARDS_IMG_MAX = CARDS_HORIZON << INTCARD_IMG_MAX;
    
    constexpr BitCards CARDS_MIN = CARDS_HORIZON << INTCARD_MIN;
    constexpr BitCards CARDS_MAX = CARDS_HORIZON << INTCARD_MAX;
    
    constexpr BitCards CARDS_IMG_MINRANK = CARDS_HORIZONRANK << RANK_IMG_MIN;
    constexpr BitCards CARDS_IMG_MAXRANK = CARDS_HORIZONRANK << RANK_IMG_MAX;
    
    constexpr BitCards CARDS_MINRANK = CARDS_HORIZONRANK << RANK_MIN;
    constexpr BitCards CARDS_MAXRANK = CARDS_HORIZONRANK << RANK_MAX;
    
    constexpr BitCards CARDS_NULL = 0ULL;
    constexpr BitCards CARDS_ALL = 0x10FFFFFFFFFFFFF0; // このゲームに登場する全て
    constexpr BitCards CARDS_IMG_ALL = 0x1FFFFFFFFFFFFFFF; // 存在を定義しているもの全て
    
    constexpr BitCards CARDS_D3 = IntCardToCards(INTCARD_D3);
    constexpr BitCards CARDS_S3 = IntCardToCards(INTCARD_S3);
    constexpr BitCards CARDS_JOKER = IntCardToCards(INTCARD_JOKER);
    
    constexpr BitCards CARDS_ALL_PLAIN = CARDS_ALL - CARDS_JOKER;
    constexpr BitCards CARDS_IMG_ALL_PLAIN = CARDS_IMG_ALL - CARDS_JOKER;
    
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
    constexpr BitCards CARDS_OO = 0xF000000000000000;
    
    // ランクの指定からカード集合を生成する
    constexpr Cards RankToCards(uint32_t r) {
        // あるランクのカード全て
        return CARDS_HORIZONRANK << (r << 2);
    }
    constexpr Cards RankRangeToCards(uint32_t r0, uint32_t r1) {
        // ランク間（両端含む）のカード全て
        // r0 <= r1 でない場合は CARDS_NULL
        return ~((CARDS_HORIZON << (r0 << 2)) - 1ULL)
        & ((CARDS_HORIZON << ((r1 + 1) << 2)) - 1ULL);
    }
    
    constexpr Cards Rank4xToCards(uint32_t r4x) {
        // あるランク4xのカード全て
        return CARDS_HORIZONRANK << r4x;
    }
    
    constexpr Cards RankRange4xToCards(uint32_t r4x_0, uint32_t r4x_1) {
        // ランク4x間（両端含む）のカード全て
        // r4x_0 <= r4x_1でない場合はNULL
        return ~((CARDS_HORIZON << r4x_0) - 1ULL)
        & ((CARDS_HORIZON << (r4x_1 + 4)) - 1ULL);
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
    
    // スートの指定からカード集合を生成する
    constexpr BitCards SuitsToCards(uint32_t s) {
        return CARDS_HORIZONSUIT * s; // あるスートのカード全て
    }
    
    // ランク x スート
    
    // ランクとスートの指定からカード集合を生成する
    // スートは集合として用いる事が出来る
    constexpr BitCards RankSuitsToCards(int r, uint32_t s) {
        return (BitCards)s << (r << 2);
    }
    constexpr BitCards Rank4xSuitsToCards(int r4x, uint32_t s) {
        return (BitCards)s << r4x;
    }
    
    // ランク両端とスートの指定
    BitCards RRSToCards(uint32_t r0, uint32_t r1, uint32_t suits) {
        return RankRangeToCards(r0, r1) & SuitsToCards(suits);
    }
    
    BitCards RR4SToCards(uint32_t r4x_0, uint32_t r4x_1, uint32_t suits) {
        return RankRange4xToCards(r4x_0, r4x_1) & SuitsToCards(suits);
    }
    
    constexpr uint32_t CardsRankToSuits(BitCards c, int r) {
        return uint32_t(c >> (r << 2)) & 15U;
    }
    constexpr uint32_t CardsRank4xToSuits(BitCards c, int r4x) {
        return uint32_t(c >> r4x) & 15U;
    }
    
    // 関係ないビットを除外したり、実在するカードのみに限定したり
    constexpr BitCards disarmCards(BitCards c) { return c & CARDS_ALL; }
    void disarmCards(BitCards *const c) { (*c) &= CARDS_ALL; }
    
    // Cards型基本演算
    
    // 追加
    constexpr BitCards addCards(BitCards c0, BitCards c1) { return c0 | c1; }
    template <typename ... args_t>
    constexpr BitCards addCards(BitCards c0, BitCards c1, args_t ... others) {
        return c0 | addCards(c1, others...);
    }

    constexpr BitCards addIntCard(BitCards c, IntCard ic) { return addCards(c, IntCardToCards(ic)); }
    constexpr BitCards addJOKER(BitCards c) { return addCards(c, CARDS_JOKER); }
    
    void addCards(BitCards *const c0, BitCards c1) { (*c0) |= c1; }
    void addJOKER(BitCards *const cptr) { (*cptr) |= CARDS_JOKER; }
    
    void addIntCard(BitCards *const pc, IntCard ic) { addCards(pc, IntCardToCards(ic)); }
    
    // 限定
    constexpr BitCards commonCards(BitCards c0, BitCards c1) { return c0 & c1; }
    constexpr BitCards andCards(BitCards c0, BitCards c1) { return c0 & c1; }
    void andCards(BitCards *const cptr, BitCards c) { (*cptr) &= c; }
    
    // 削除（オーバーを引き起こさない）
    // maskは指定以外とのandをとることとしている。
    // 指定部分とのandはand処理で行う
    constexpr BitCards maskCards(BitCards c0, BitCards c1) { return c0 & ~c1; }
    void maskCards(BitCards *const c0, BitCards c1) { (*c0) &= ~c1; }
    constexpr BitCards maskJOKER(Cards c) { return maskCards(c, CARDS_JOKER); }
    void maskJOKER(BitCards *const cptr) { maskCards(cptr, CARDS_JOKER); }
    
    // 状態逆転（追加や削除のため用いた場合はオーバー処理で不具合となる危険性あり）
    constexpr BitCards invCards(BitCards c0, BitCards c1) { return c0 ^ c1; }
    void invCards(BitCards *const c0, BitCards c1) { (*c0) ^= c1; }
    constexpr BitCards invJOKER(BitCards c0) { return invCards(c0, CARDS_JOKER); }
    void invJOKER(BitCards *const cptr) { invCards(cptr, CARDS_JOKER); }
    
    // カード減算
    // 全ての（安定でなくても良い）カード減算処理から最も高速なものとして定義したいところだが、
    // 現在では整数としての引き算処理。
    constexpr BitCards subtrCards(BitCards c0, BitCards c1) { return c0 - c1; }
    void subtrCards(Cards *const cptr, BitCards c) { (*cptr) -= c; }
    constexpr BitCards subtrJOKER(BitCards c) { return subtrCards(c, CARDS_JOKER); }
    void subtrJOKER(BitCards *const cptr) { subtrCards(cptr, CARDS_JOKER); }
    
    // 要素数
    constexpr uint32_t countFewCards(BitCards c) { return countFewBits64(c); } // 要素が比較的少ない時の速度優先
    uint32_t countManyCards(BitCards c) { return countBits64(c); } // 要素が比較的多い時の速度優先
    uint32_t countCards(BitCards c) { return countBits64(c); } // 基本のカウント処理
    constexpr BitCards any2Cards(BitCards c) { return c & (c - 1ULL); }
    
    // Cards型基本判定
    
    // 要素の部分一致
    constexpr BitCards hasSameCards(BitCards c0, BitCards c1) { return commonCards(c0, c1); }
    constexpr bool isExclusiveCards(BitCards c0, BitCards c1) { return !(c0 & c1); }
    
    // 包括関係
    constexpr BitCards containsCard(BitCards c0, BitCard c1) { return andCards(c0, c1); } // 単体に対してはandでok
    constexpr BitCards containsIntCard(BitCards c, IntCard ic) { return containsCard(c, IntCardToCards(ic)); }
    constexpr BitCards containsJOKER(BitCards c) { return andCards(c, CARDS_JOKER); }
    constexpr BitCards containsS3(BitCards c) { return andCards(c, CARDS_S3); }
    constexpr BitCards containsD3(BitCards c) { return andCards(c, CARDS_D3); }
    constexpr BitCards contains8(BitCards c) { return andCards(c, CARDS_8); }

    constexpr bool holdsCards(BitCards c0, BitCards c1) { return holdsBits(c0, c1); }
    
    // 空判定
    constexpr BitCards anyCards(BitCards c) { return c; }
    
    // validation
    constexpr bool examCards(BitCards c) { return holdsCards(CARDS_ALL, c); }
    constexpr bool examImaginaryCards(BitCards c) { return holdsCards(CARDS_IMG_ALL, c); }
    
    // Cards型生成
    BitCards StringToCards(const std::string& str) {
        BitCards c = CARDS_NULL;
        std::vector<std::string> v = split(str, ' ');
        for (const auto& s : v) {
            IntCard ic = StringToIntCardM(s);
            if (ic != INTCARD_NONE) {
                addIntCard(&c, ic);
            }
        }
        return c;
    }
    // Cards型特殊演算
    
    // 特定順序の要素を選ぶ（元のデータは変えない）
    inline Cards pickLow(const Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        return lowestNBits(c, n);
    }
    template <int N = 1>
    inline Cards pickLow(const Cards c) {
        assert(N > 0);
        assert((int)countCards(c) >= N);
        return pickLow(c, N);
    }
    template <> inline constexpr Cards pickLow<1>(const Cards c) {
        return lowestBit(c);
    }
    template <> inline Cards pickLow<2>(const Cards c) {
        Cards res = lowestBit(c);
        return res | lowestBit(c - res);
    }
    
    inline Cards pickHigh(const Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        return highestNBits(c, n);
    }
    template <int N = 1>
    inline Cards pickHigh(const Cards c) {
        assert(N > 0);
        assert((int)countCards(c) >= N);
        return pickHigh(c, N);
    }
    template <> inline Cards pickHigh<1>(const Cards c) {
        return highestBit(c);
    }
    template <> inline Cards pickHigh<2>(const Cards c) {
        Cards r = highestBit(c);
        return r | highestBit(c - r);
    }
    
    // 特定順序の要素の取り出し(元のデータから引く)
    inline Cards popHigh(Cards *const c) {
        assert(*c);
        Cards r = 1ULL << bsr64(*c);
        (*c) -= r;
        return r;
    }
    
    inline Cards popHigh2(Cards *const c) {
        assert(countCards(*c) >= 2U);
        Cards r1 = 1ULL << bsr64(*c);
        (*c) -= r1;
        Cards r2 = 1ULL << bsr64(*c);
        (*c) -= r2;
        return r1 | r2;
    }
    
    inline Cards popLow(Cards *const c) {
        assert(anyCards(*c));
        Cards r = (*c) & (-(*c));
        subtrCards(c, r);
        return r;
    }
    
    // Cards pop
    
    // IntCard型で1つ取り出し
    inline IntCard pickIntCardLow(const Cards c) {
        return (IntCard)bsf64(c);
    }
    
    inline IntCard pickIntCardHigh(const Cards c) {
        ASSERT(anyCards(c),);
        return (IntCard)bsr64(c);
    }
    
    inline IntCard popIntCardLow(Cards *const c) {
        IntCard ic = pickIntCardLow(*c);
        (*c) &= ((*c) - 1ULL);
        return ic;
    }
    
    inline IntCard popIntCardHigh(Cards *const c) {
        IntCard ic = pickIntCardHigh(*c);
        subtrCards(c, IntCardToCards(ic));
        return ic;
    }
    
    // 適当に（ランダムではない）１つ取り出し
    // 順序がどうでも良い取り出し操作の中で最速な操作で実装したい
    inline Cards pop(Cards *const c) {
        Cards r = (*c) & (-(*c));
        (*c) -= r;
        return r;
    }
    
    inline constexpr Cards pick(const Cards c) {
        return c & (-c);
    }
    
    inline IntCard pickIntCard(const Cards c) {
        return pickIntCardLow(c);
    }
    
    inline IntCard popIntCard(Cards *const c) {
        return popIntCardLow(c);
    }
    
    // 完全ランダム取り出し
    // ビット分割関数(bitPartition.hpp)を使う
    template <int N = 1, class dice64_t>
    inline Cards popRand(Cards *const c, dice64_t *const dice) {
        static_assert(N >= 0, " popRand N < 0 ");
        Cards res;
        switch (N) {
            case 0: res = CARDS_NULL; break;
            case 1: res = pop1Bit64(c, dice); break;
                //case 2:res=pickNBits64(c,2,countCards(c)-2,dice);break;
            default: UNREACHABLE; break;
        }
        return res;
    }
    
    template <int N = 1, class dice64_t>
    inline Cards pickRand(const Cards c, dice64_t *const dice) {
        static_assert(N >= 0, " pickRand N < 0 ");
        Cards res;
        switch (N) {
            case 0: res = CARDS_NULL; break;
            case 1: res = pick1Bit64(c, dice); break;
            case 2: res = pickNBits64(c, 2, countCards(c) - 2, dice); break;
            default: UNREACHABLE; break;
        }
        return res;
    }
    
    // n番目(nは1から)に高い,低いもの
    inline Cards pickNthHigh(Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return NthHighestBit(c, n);
    }
    
    inline Cards pickNthLow(Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return NthLowestBit(c, n);
    }
    
    // n番目(nは1から)に高い,低いもの以外
    inline Cards maskNthHigh(Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return subtrCards(c, pickNthHigh(c, n));
    }
    
    inline Cards maskNthLow(Cards c, int n) {
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return subtrCards(c, pickNthLow(c, n));
    }
    
    // 基準c1より高い、低い(同じは含まず)もの
    
    // 単体。変なカードも入るが...
    inline Cards pickHigher(Cards c1) {
        return allHigherBits(c1);
    }
    
    inline Cards pickLower(Cards c1) {
        return allLowerBits(c1);
    }
    
    inline Cards pickHigher(Cards c0, Cards c1) {
        return c0 & allHigherBits(c1);
    }
    
    inline Cards pickLower(Cards c0, Cards c1) {
        return c0 & allLowerBits(c1);
    }
    
    // 基準c1より高い、低い(同じは含まず)もの以外
    inline Cards maskHigher(Cards c0, Cards c1) {
        return c0 & ~allHigherBits(c1);
    }
    
    inline Cards maskLower(Cards c0, Cards c1) {
        return c0 & ~allLowerBits(c1);
    }
    
    // 基準としてランクを用いる場合
    inline constexpr BitCards higherMask(Rank r) {
        return ~((1ULL << ((r + 1) * 4)) - 1ULL);
    }
    inline constexpr BitCards lowerMask(Rank r) {
        return (1ULL << (r * 4)) - 1ULL;
    }
    inline constexpr BitCards strongerMask(Rank r, Order ord) {
        return ord == ORDER_NORMAL ? higherMask(r) : lowerMask(r);
    }
    inline constexpr BitCards weakerMask(Rank r, Order ord) {
        return ord == ORDER_NORMAL ? lowerMask(r) : higherMask(r);
    }

    // ランク重合
    // ランクを１つずつ下げてandを取るのみ(ジョーカーとかその辺のことは何も考えない)
    // 階段判定に役立つ
    template <int N = 3>
    inline Cards polymRanks(const Cards c) {
        static_assert(N >= 0, "polymRanks<>()");
        return ((c >> ((N - 1) << 2)) & polymRanks<N - 1>(c));
    }
        
    template <> inline constexpr Cards polymRanks<1>(const Cards c) { return c; }
    template <> inline constexpr Cards polymRanks<0>(const Cards c) { return CARDS_NULL; }
        
    inline Cards polymRanks(Cards c, uint32_t num) { // 重合数が変数の場合
        assert(num > 0);
        --num;
        while (num) {
            c = polymRanks<2>(c);
            --num;
        }
        return c;
     }
        
    inline Cards polymRanks_PR(Cards c, uint32_t arg, uint32_t dst) {
        // 既にargランク重合が成された状態から、より高次のdstランク重合結果を得る
        assert(arg > 0);
        assert(dst > 0);
        assert(dst - arg + 1U > 0);
        return polymRanks(c, dst - arg + 1U);
    }
        
    inline constexpr Cards polymJump(const Cards c) { // 1ランクとばし
        return c & (c >> 8);
    }
        
    // ランク展開
    // ランクを１つずつ上げてorをとるのが基本だが、
    // 4以上の場合は倍々で増やしていった方が少ない命令で済む
    template <int N = 3>
    inline Cards extractRanks(const Cards c) {
        return ((c << ((N - 1) << 2)) | extractRanks<N - 1>(c));
    }
        
    template <> inline constexpr Cards extractRanks<0>(const Cards c) { return CARDS_NULL; }
    template <> inline constexpr Cards extractRanks<1>(const Cards c) { return c; }
    template <> inline constexpr Cards extractRanks<2>(const Cards c) { return c | (c << 4); }
    template <> inline constexpr Cards extractRanks<3>(const Cards c) { return c | (c << 4) | (c << 8); }
    template <> inline Cards extractRanks<4>(const Cards c) {
        Cards r = c | (c << 4);
        return r | (r << 8);
    }
    template <> inline Cards extractRanks<5>(const Cards c) {
        Cards r = c | (c << 4);
        return r | (c << 8) | (r << 12);
    }
        
    Cards extractRanks(Cards c, uint32_t num) { // 展開数が変数の場合
        assert(num > 0);
        for (int n = num - 1U; n; --n) {
            c = extractRanks<2>(c);
        }
        return c;
    }
    
    Cards polymRanksWithJOKER(const Cards c, int qty) {
        Cards r;
        switch (qty) {
            case 0: r = CARDS_NULL; break;
            case 1: r = CARDS_ALL; break;
            case 2: r = c; break;
            case 3: {
                Cards d = c & (c >> 4);
                r = (d | (d >> 4)) | (c & (c >> 8));
            } break;
            case 4: {
                
                Cards d = c & (c >> 4);
                Cards f = (d & (c >> 12)) | (c & (d >> 8)); // 1 + 2パターン
                Cards e = d & (c >> 8); // 3連パターン
                if (e) { f |= e | (e >> 4); }
                r = f;
            } break;
            case 5: {
                Cards d = c & (c >> 4);
                Cards g = d | (d >> 12); // 2 + 2パターン
                Cards e = d & (c >> 8); // 3連
                if (e) {
                    g |= (e & (c >> 16)) | (c & (e >> 8)); // 1 + 3パターン
                    Cards f = e & (c >> 12);
                    if (f) {
                        g |= (f | (f >> 4)); // 4連パターン
                    }
                }
                r = g;
            } break;
            default: r = CARDS_NULL; break;
        }
        return r;
    }
    
    // 役の作成可能性
    // あくまで作成可能性。展開可能な型ではなく有無判定の速度を優先したもの
    // 先にスート圧縮版、ランク重合版を計算していた場合はそっちを使った方が速いはず
    // ジョーカーがあるか(JK)は重要な情報なので、テンプレートで分岐可能とする(通常は_BOTH)
    // 特に階段においてn枚階段判定は出されている状態でm枚判定を出すような状況にも対応が必要
    
    template <int JK = _BOTH> Cards canMakeNJ2Seq(Cards c) { return JK; }
    template <int JK = _BOTH> Cards canMakeJ2Seq(Cards c);
    template <int JK = _BOTH> Cards canMake2Seq(Cards c);
    
    template <int JK = _BOTH> Cards canMakeNJ3Seq(Cards c);
    template <int JK = _BOTH> Cards canMakeJ3Seq(Cards c);
    template <int JK = _BOTH> Cards canMake3Seq(Cards c);
    
    template <int JK = _BOTH> Cards canMakeNJ4Seq(Cards c);
    template <int JK = _BOTH> Cards canMakeJ4Seq(Cards c);
    template <int JK = _BOTH> Cards canMake4Seq(Cards c);
    
    template <int JK = _BOTH> Cards canMakeNJ5Seq(Cards c);
    template <int JK = _BOTH> Cards canMakeJ5Seq(Cards c);
    template <int JK = _BOTH> Cards canMake5Seq(Cards c);
    
    template <int JK = _BOTH> Cards canMakeNJ6Seq(Cards c);
    template <int JK = _BOTH> Cards canMakeJ6Seq(Cards c);
    template <int JK = _BOTH> Cards canMake6Seq(Cards c);
    
    template <int JK = _BOTH>
    Cards canMakePlainSeq(Cards c, int qty) {
        Cards res;
        switch (qty) {
            case 0: res = CARDS_NULL; break;
            case 1: res = maskJOKER(c); break;
            case 2: res = canMakeNJ2Seq<JK>(c); break;
            case 3: res = canMakeNJ3Seq<JK>(c); break;
            case 4: res = canMakeNJ4Seq<JK>(c); break;
            case 5: res = canMakeNJ5Seq<JK>(c); break;
            default: res = CARDS_NULL; break;
        }
        return res;
    }
    template <int JK = _BOTH>
    Cards canMakeJokerSeq(Cards c, int qty) {
        Cards res;
        switch (qty) {
            case 0: res = CARDS_NULL; break;
            case 1: UNREACHABLE; res = c & CARDS_JOKER; break;
            case 2: res = canMakeJ2Seq<JK>(c); break;
            case 3: res = canMakeJ3Seq<JK>(c); break;
            case 4: res = canMakeJ4Seq<JK>(c); break;
            case 5: res = canMakeJ5Seq<JK>(c); break;
            default: res = CARDS_NULL; break;
        }
        return res;
    }
    template <int JK = _BOTH>
    Cards canMakeSeq(Cards c, int qty) {
        Cards res;
        switch (qty) {
            case 0: res = CARDS_NULL; break;
            case 1: res = c; break;
            case 2: res = canMake2Seq<JK>(c); break;
            case 3: res = canMake3Seq<JK>(c); break;
            case 4: res = canMake4Seq<JK>(c); break;
            case 5: res = canMake5Seq<JK>(c); break;
            default: res = CARDS_NULL; break;
        }
        return res;
    }
    
    template <int QTY = 3, int JK = _BOTH>
    Cards canMakeNJSeq(Cards c) { return canMakeNJSeq<JK>(c, QTY); }
    template <int QTY = 3, int JK = _BOTH>
    Cards canMakeJSeq(Cards c) { return canMakeJSeq<JK>(c, QTY); }
    template <int QTY = 3, int JK = _BOTH>
    Cards canMakeSeq(Cards c) { return canMakeSeq<JK>(c, QTY); }
    
    // 2seq
    template <> Cards canMakeNJ2Seq<0>(Cards c) { return c & (c >> 4); }
    template <> Cards canMakeNJ2Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return canMakeNJ2Seq<0>(nj);
    }
    template <> Cards canMakeNJ2Seq<2>(Cards c) { return canMakeNJ2Seq<1>(c); }
    
    template <> Cards canMakeJ2Seq<0>(Cards c) { return CARDS_NULL; }
    template <> Cards canMakeJ2Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return anyCards(nj);
    }
    template <> Cards canMakeJ2Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMakeJ2Seq<1>(c);
        } else {
            return canMakeJ2Seq<0>(c);
        }
    }
    
    template <> Cards canMake2Seq<0>(Cards c) { return canMakeNJ2Seq<0>(c); }
    template <> Cards canMake2Seq<1>(Cards c) { return canMakeJ2Seq<1>(c); }
    template <> Cards canMake2Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMake2Seq<1>(c);
        } else {
            return canMakeNJ2Seq<0>(c);
        }
    }
    
    // 3seq
    template <> Cards canMakeNJ3Seq<0>(Cards c) { return c&(c >> 4)&(c >> 8); }
    template <> Cards canMakeNJ3Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return canMakeNJ3Seq<0>(nj);
    }
    template <> Cards canMakeNJ3Seq<2>(Cards c) { return canMakeNJ3Seq<1>(c); }
    
    template <> Cards canMakeJ3Seq<0>(Cards c) { return CARDS_NULL; }
    template <> Cards canMakeJ3Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return (nj & (nj >> 4)) | (nj & (nj >> 8));
    }
    template <> Cards canMakeJ3Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMakeJ3Seq<1>(c);
        } else {
            return canMakeJ3Seq<0>(c);
        }
    }
    
    template <> Cards canMake3Seq<0>(Cards c) { return canMakeNJ3Seq<0>(c); }
    template <> Cards canMake3Seq<1>(Cards c) { return canMakeJ3Seq<1>(c); }
    template <> Cards canMake3Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMake3Seq<1>(c);
        } else {
            return canMakeNJ3Seq<0>(c);
        }
    }
    
    // 4seq
    template <> Cards canMakeNJ4Seq<0>(Cards c) { return c&(c >> 4)&(c >> 8)&(c >> 12); }
    template <> Cards canMakeNJ4Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return canMakeNJ4Seq<0>(nj);
    }
    template <> Cards canMakeNJ4Seq<2>(Cards c) { return canMakeNJ4Seq<1>(c); }
    
    template <> Cards canMakeJ4Seq<0>(Cards c) { return CARDS_NULL; }
    template <> Cards canMakeJ4Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        Cards c12 = nj & (nj >> 4), c3 = nj >> 8, c4 = nj >> 12;
        return (c12 & c3) | (c12 & c4) | (nj & c3 & c4);
    }
    template <> Cards canMakeJ4Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMakeJ4Seq<1>(c);
        }
        else{
            return canMakeJ4Seq<0>(c);
        }
    }
    
    template <> Cards canMake4Seq<0>(Cards c) { return canMakeNJ4Seq<0>(c); }
    template <> Cards canMake4Seq<1>(Cards c) { return canMakeJ4Seq<1>(c); }
    template <> Cards canMake4Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMake4Seq<1>(c);
        }
        else{
            return canMakeNJ4Seq<0>(c);
        }
    }
    
    // 5seq
    template <> Cards canMakeNJ5Seq<0>(Cards c) { return c & (c >> 4) & (c >> 8) & (c >> 12) & (c >> 16); }
    template <> Cards canMakeNJ5Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return canMakeNJ5Seq<0>(nj);
    }
    template <> Cards canMakeNJ5Seq<2>(Cards c) { return canMakeNJ5Seq<1>(c); }
    
    template <> Cards canMakeJ5Seq<0>(Cards c) { return CARDS_NULL; }
    template <> Cards canMakeJ5Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        Cards c12 = nj & (nj >> 4), c3 = nj >> 8, c4 = nj >> 12, c5 = nj >> 16, c45 = c4 & c5;
        return (c12 & c3 & c4) | (c12 & c3 & c5) | (c12 & c45) | (nj & c3 & c45);
    }
    template <> Cards canMakeJ5Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMakeJ5Seq<1>(c);
        } else {
            return canMakeJ5Seq<0>(c);
        }
    }
    
    template <> Cards canMake5Seq<0>(Cards c) { return canMakeNJ5Seq<0>(c); }
    template <> Cards canMake5Seq<1>(Cards c) { return canMakeJ5Seq<1>(c); }
    template <> Cards canMake5Seq<2>(Cards c) {
        if (containsJOKER(c)) {
            return canMake5Seq<1>(c);
        } else {
            return canMakeNJ5Seq<0>(c);
        }
    }
    
    // 6seq
    template <> Cards canMakeNJ6Seq<0>(Cards c) { return c & (c >> 4) & (c >> 8) & (c >> 12) & (c >> 16) & (c >> 20); }
    template <> Cards canMakeNJ6Seq<1>(Cards c) {
        Cards nj = maskJOKER(c);
        return canMakeNJ6Seq<0>(nj);
    }
    template <> Cards canMakeNJ6Seq<2>(Cards c) { return canMakeNJ6Seq<1>(c); }
    
    //uint32_t removeAllGroups(Cards c)
   
    // 主に支配性判定用の許容ゾーン計算
    // 支配性の判定のため、合法着手がカード集合表現のどこに存在しうるかを計算
    
    // グループ版（ジョーカーを除けばシングルも一緒）
    inline Cards ORSToGValidZone(int ord, uint32_t rank, uint32_t suits) {
        Cards res;
        switch (ord) {
            case 0: res = RRSToCards(rank + 1, RANK_MAX, suits); break;
            case 1: res = RRSToCards(RANK_MIN, rank - 1, suits); break;
            case 2: res = subtrCards(RRSToCards(RANK_MIN, RANK_MAX, suits), RankSuitsToCards(rank, suits)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    inline Cards ORToGValidZone(int ord, uint32_t rank) { // ランク限定のみ
        Cards res;
        switch (ord) {
            case 0: res = RankRangeToCards(rank + 1, RANK_MAX); break;
            case 1: res = RankRangeToCards(RANK_MIN, rank - 1); break;
            case 2: res = subtrCards(RankRangeToCards(RANK_MIN, RANK_MAX), RankToCards(rank)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    // 階段版
    inline Cards ORSQToSCValidZone(int ord, uint32_t rank, uint32_t suits, int qty) {
        Cards res;
        switch (ord) {
            case 0: res = RRSToCards(rank + qty, RANK_MAX, suits); break;
            case 1: res = RRSToCards(RANK_MIN, rank - 1, suits); break;
            case 2: res = addCards(RRSToCards(RANK_MIN, rank - 1, suits),
                                   RRSToCards(rank + qty, RANK_MAX, suits)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    inline Cards ORQToSCValidZone(int ord, uint32_t rank, int qty) { // ランク限定のみ
        Cards res;
        switch (ord) {
            case 0: res = RankRangeToCards(rank + qty, RANK_MAX); break;
            case 1: res = RankRangeToCards(RANK_MIN, rank - 1); break;
            case 2: res = addCards(RankRangeToCards(RANK_MIN, rank - 1),
                                   RankRangeToCards(rank + qty, RANK_MAX)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
 
    // 許容包括
    // あるランクやスートを指定して、そのランクが許容ゾーンに入るか判定する
    // MINやMAXとの比較は変な値が入らない限りする必要がないので省略している
    bool isValidGroupRank(int mvRank, int order, int bdRank) {
        if (order == ORDER_NORMAL) {
            return mvRank > bdRank;
        } else {
            return mvRank < bdRank;
        }
    }
    
    bool isValidSeqRank(int mvRank, int order, int bdRank, int qty) {
        if (order == ORDER_NORMAL) {
            return mvRank >= (bdRank + qty);
        } else {
            return mvRank <= (bdRank - qty);
        }
    }
    
    // 枚数オンリーによる許容包括
    Cards seqExistableZone(uint32_t qty) {
        return RankRangeToCards(RANK_IMG_MIN, RANK_IMG_MAX + 1 - qty);
    }
    
    // 出力用クラス
    struct OutCards {
        BitCards c;
        constexpr OutCards(const BitCards& arg) :c(arg) {}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCards& arg) {
        assert(holdsCards(CARDS_IMG_ALL, arg.c));
        out << "{";
        BitCards tmp = arg.c;
        while (anyCards(tmp)) {
            out << " " << OutIntCard(popIntCardLow(&tmp));
        }
        out << " }";
        return out;
    }
    
    struct OutCardsM {
        Cards c;
        constexpr OutCardsM(const Cards& arg) :c(arg) {}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCardsM& arg) {
        assert(holdsCards(CARDS_IMG_ALL, arg.c));
        out << "{";
        Cards tmp = arg.c;
        while (anyCards(tmp)) {
            out << " " << OutIntCardM(popIntCardLow(&tmp));
        }
        out << " }";
        return out;
    }
    
    // テーブル形式で出力
    struct OutCardTable {
        BitCards c;
        OutCardTable(const BitCards& ac) : c(ac) {}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCardTable& arg) {
        // テーブル形式で見やすく
        BitCards c = arg.c;
        out << " ";
        for (int r = RANK_3; r <= RANK_2; ++r) {
            out << " " << OutRank(r);
        }
        out << " " << "X" << endl;
        for (int sn = 0; sn < 4; ++sn) {
            out << OutSuitNum(sn) << " ";
            for (int r = RANK_3; r <= RANK_2; ++r) {
                if (containsCard(c, RankSuitsToCards(r, SuitNumToSuits(sn)))) {
                    out << "O ";
                } else {
                    out << ". ";
                }
            }
            if (sn == 0) {
                if (containsJOKER(c)) {
                    out << "O ";
                } else {
                    out << ". ";
                }
            }
            out << endl;
        }
        return out;
    }
    
    struct Out2CardTables {
        BitCards c0, c1;
        Out2CardTables(const BitCards& ac0, const BitCards& ac1) : c0(ac0), c1(ac1) {}
    };
    
    std::ostream& operator <<(std::ostream& out, const Out2CardTables& arg) {
        // テーブル形式で見やすく
        // ２つを横並びで表示
        BitCards c[2] = { arg.c0, arg.c1 };
        for (int i = 0; i < 2; ++i) {
            out << " ";
            for (int r = RANK_3; r <= RANK_2; ++r) {
                out << " " << OutRank(r);
            }
            out << " " << "X";
            out << "    ";
        }
        out << endl;
        for (int sn = 0; sn < N_SUITS; ++sn) {
            for (int i = 0; i < 2; ++i) {
                out << OutSuitNum(sn) << " ";
                for (int r = RANK_3; r <= RANK_2; ++r) {
                    if (containsCard(c[i], RankSuitsToCards(r, SuitNumToSuits(sn)))) {
                        out << "O ";
                    } else {
                        out << ". ";
                    }
                }
                if (sn == 0) {
                    if (containsJOKER(c[i])) {
                        out << "O ";
                    } else {
                        out << ". ";
                    }
                    out << "   ";
                } else {
                    out << "     ";
                }
            }
            out << endl;
        }
        return out;
    }

    // イテレーション
    template <typename callback_t>
    void iterateIntCard(BitCards c, const callback_t& callback) {
        while (anyCards(c)) {
            IntCard ic = popIntCard(&c);
            callback(ic);
        }
    }
    template <typename callback_t>
    void iterateCard(BitCards c, const callback_t& callback) {
        while (anyCards(c)) {
            Cards tc = pop(&c);
            callback(tc);
        }
    }
    
    template <class cards_buf_t, typename callback_t>
    int searchCards(const cards_buf_t *const buf, const int n, const callback_t& callback) {
        for (int m = 0; m < n; ++m) {
            if (callback(buf[m])) { return m; }
        }
        return -1;
    }
    
    /**************************カード集合表現の変形型**************************/
    
    // 諸々の演算のため、ビット集合として表現されたカード集合を同じく64ビット整数である別の表現系に変換する
    // 演算の際に、64ビット全体にかかる演算か存在するランクの部分のみにかかる演算かは問題になるが、
    // 現在はいずれにせよ、ジョーカーは絡めないことにしている

    // 無支配ゾーン
    BitCards ORQ_NDTable[2][16][8]; // (order, rank, qty - 1)
    
    // 現在用意されている型は以下
    
    // ER ランク存在型
    // FR ランク全存在型
    // MER ランク存在マスク型
    // QR ランク枚数型
    // PQR ランク枚数位置型
    // SC スート圧縮型
    // ND 無支配型(ジョーカーのビットは関係ないが、存在は加味)
    
    // PQR定数
    constexpr BitCards PQR_NULL = 0ULL;
    constexpr BitCards PQR_1    = CARDS_IMG_ALL_PLAIN & 0x1111111111111111;
    constexpr BitCards PQR_2    = CARDS_IMG_ALL_PLAIN & 0x2222222222222222;
    constexpr BitCards PQR_3    = CARDS_IMG_ALL_PLAIN & 0x4444444444444444;
    constexpr BitCards PQR_4    = CARDS_IMG_ALL_PLAIN & 0x8888888888888888;
    constexpr BitCards PQR_12   = CARDS_IMG_ALL_PLAIN & 0x3333333333333333;
    constexpr BitCards PQR_13   = CARDS_IMG_ALL_PLAIN & 0x5555555555555555;
    constexpr BitCards PQR_14   = CARDS_IMG_ALL_PLAIN & 0x9999999999999999;
    constexpr BitCards PQR_23   = CARDS_IMG_ALL_PLAIN & 0x6666666666666666;
    constexpr BitCards PQR_24   = CARDS_IMG_ALL_PLAIN & 0xaaaaaaaaaaaaaaaa;
    constexpr BitCards PQR_34   = CARDS_IMG_ALL_PLAIN & 0xcccccccccccccccc;
    constexpr BitCards PQR_123  = CARDS_IMG_ALL_PLAIN & 0x7777777777777777;
    constexpr BitCards PQR_124  = CARDS_IMG_ALL_PLAIN & 0xbbbbbbbbbbbbbbbb;
    constexpr BitCards PQR_134  = CARDS_IMG_ALL_PLAIN & 0xdddddddddddddddd;
    constexpr BitCards PQR_234  = CARDS_IMG_ALL_PLAIN & 0xeeeeeeeeeeeeeeee;
    constexpr BitCards PQR_1234 = CARDS_IMG_ALL_PLAIN & 0xffffffffffffffff;
    
    // 定義通りの関数
    inline Cards QtyToPQR(uint32_t q) {
        return PQR_1 << (q - 1);
    }
    
    BitArray64<4, 16> CardsToQR_slow(BitCards c) {
        BitArray64<4, 16> ret = 0;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            ret.set(r, countCards(RankToCards(r) & c));
        }
        return ret.data();
    }
    BitArray64<4, 16> CardsToENR_slow(BitCards c, int n) {
        BitArray64<4, 16> ret = 0;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            if (countCards(c & RankToCards(r)) >= n) {
                ret.set(r, 1);
            }
        }
        return ret.data();
    }
    BitArray64<4, 16> CardsToNR_slow(BitCards c, int n) {
        BitArray64<4, 16> ret = 0;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            if (countCards(c & RankToCards(r)) == n) {
                ret.set(r, 1);
            }
        }
        return ret.data();
    }
    BitCards QRToPQR_slow(BitArray64<4, 16> qr) {
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            if (arr[r]) {
                ret.set(r, 1 << (arr[r] - 1));
            }
        }
        return ret.data();
    }
    BitCards QRToSC_slow(BitArray64<4, 16> qr) {
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            ret.set(r, (1 << arr[r]) - 1);
        }
        return ret.data();
    }
    BitCards PQRToSC_slow(BitArray64<4, 16> qr) {
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for (int r = RANK_U; r <= RANK_O; ++r) {
            if (arr[r]) {
                uint32_t q = bsf(arr[r]) + 1;
                ret.set(r, (1 << q) - 1);
            }
        }
        return ret.data();
    }
    
    // パラレル演算関数
    inline BitArray64<4, 16> CardsToQR(const BitCards c) {
        // 枚数が各4ビットに入る
        BitCards a = (c & PQR_13) + ((c >> 1) & PQR_13);
        return (a & PQR_12) + ((a >> 2) & PQR_12);
    }
    inline BitCards CardsToFR(const BitCards c) {
        // ランク中の4ビット全てあればPQR_1の位置にビットが立つ
        BitCards a = c & (c >> 1);
        return a & (a >> 2) & PQR_1;
    }
    inline BitCards CardsTo3R(const BitCards c) {
        // ランク中に丁度3ビットあればPQR_1の位置にビットが立つ
        BitCards ab_cd = c & (c >> 1);
        BitCards axb_cxd = c ^ (c >> 1);
        return ((ab_cd & (axb_cxd >> 2)) | ((ab_cd >> 2) & axb_cxd)) & PQR_1;
    }
    inline BitCards CardsTo2R(const BitCards c) {
        // ランク中に丁度2ビットあればPQR_1の位置にビットが立つ
        BitCards qr = CardsToQR(c);
        return (qr >> 1) & ~qr & PQR_1;
    }
    inline BitCards CardsTo1R(const BitCards c) {
        // ランク中に丁度1ビットあればPQR_1の位置にビットが立つ
        return CardsTo3R(~c);
    }
    inline BitCards CardsTo0R(const BitCards c) {
        // ランク中に1ビットもなければPQR_1の位置にビットが立つ
        return CardsToFR(~c);
    }
        
    inline BitCards CardsToNR(const BitCards c, int q) {
        Cards nr;
        switch (q) {
            case 0: nr = CardsTo0R(c); break;
            case 1: nr = CardsTo1R(c); break;
            case 2: nr = CardsTo2R(c); break;
            case 3: nr = CardsTo3R(c); break;
            case 4: nr = CardsToFR(c); break;
            default: UNREACHABLE; nr = CARDS_NULL; break;
        }
        return nr;
    }
        
    inline BitCards CardsToER(const BitCards c) {
        // ランク中に1ビットでもあればPQR_1の位置にビットが立つ
        BitCards a = c | (c >> 1);
        return (a | (a >> 2)) & PQR_1;
    }
    
    inline BitCards CardsToPQR(const BitCards arg) {
        // ランクごとの枚数を示す位置にビットが立つようにする
        // 2ビットごとの枚数を計算
        BitCards a = (arg & PQR_13) + ((arg >> 1) & PQR_13);
        // 4ビットあったところを4に配置
        BitCards r = a & (a << 2) & PQR_4;
        // 3ビットあったところを3に配置
        BitCards r3 = (a << 2) & (a >> 1) & PQR_3;
        r3 |= a & (a << 1) & PQR_3;
        
        // 残りは足すだけ。ただし3,4ビットがすでにあったところにはビットを置かない。
        BitCards r12 = ((a & PQR_12) + ((a >> 2) & PQR_12)) & PQR_12;
        if (r3) {
            r |= r3;
            r |= ~((r3 >> 1) | (r3 >> 2)) & r12;
        } else {
            r |= r12;
        }
        return r;
    }
    
    inline BitCards QRToPQR(const BitArray64<4, 16> qr) {
        // qr -> pqr 変換
        const Cards iqr = ~qr;
        const Cards qr_l1 = (qr << 1);
        const Cards r = (PQR_1 & qr & (iqr >> 1)) | (PQR_2 & qr & (iqr << 1)) | ((qr & qr_l1) << 1) | (qr_l1 & PQR_4);
        return r;
    }
    
    inline BitCards PQRToSC(const BitCards pqr) {
        // pqr -> sc はビットを埋めていくだけ
        Cards r = pqr;
        r |= (r & PQR_234) >> 1;
        r |= (r & PQR_34) >> 2;
        return r;
    }
    
    inline void PQRToND(const BitCards pqr, uint32_t jk, Cards *const nd) {
        // pqr -> nd[2] 変換
        // ジョーカーの枚数の情報も必要
        assert(jk == 0 || jk == 1); // 0or1枚
        assert(nd != nullptr);
        
        nd[0] = nd[1] = CARDS_NULL;

        // pqrの1ランクシフトが無支配限度
        // 以降、tmpのビットの示す位置は実際にカードがあるランクよりずれている事に注意
        Cards tmp0 = pqr >> 4;
        Cards tmp1 = pqr << 4;
        while (tmp0) { // 無支配ゾーンがまだ広いはず
            IntCard ic = pickIntCardHigh(tmp0);
            int r = IntCardToRank(ic);
            int sn = IntCardToSuitNum(ic);
            nd[0] |= ORQ_NDTable[0][r][sn]; // このゾーンに対して返せることが確定
            tmp0 &= (~nd[0]); // もう関係なくなった部分は外す
        }
        while (tmp1) { // 無支配ゾーンがまだ広いはず
            IntCard ic = pickIntCardLow(tmp1);
            int r = IntCardToRank(ic);
            int sn = IntCardToSuitNum(ic);
            nd[1] |= ORQ_NDTable[1][r][sn]; // このゾーンに対して返せることが確定
            tmp1 &= (~nd[1]); // もう関係なくなった部分は外す
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

    // 役の作成可能性判定
    Cards canMakePlainGroup(Cards c, int q) {
        Cards ret;
        switch (q) {
            case 0: ret = CARDS_ALL; break; // 0枚のグループは必ずできるとする(canMakeGroupでジョーカーありの場合のため)
            case 1: ret = maskJOKER(c); break;
            case 2: ret = CardsToQR(c) & PQR_234; break;
            case 3: ret = (CardsToQR(c) + PQR_1) & PQR_34; break;
            case 4: ret = CardsToFR(c); break;
            default: ret = CARDS_NULL; break;
        }
        return ret;
    }
    Cards canMakeGroup(Cards c, int q) {
        return canMakePlainGroup(c, containsJOKER(c) ? (q - 1) : q);
    }
    
    // スート圧縮判定
    // あるカード集合（スート限定がなされていてもよい）の中にn枚グループが作成可能かの判定
    // ただしジョーカーの分は最初から引いておく
    // 高速な処理には場合分けが必要か
    inline bool judgeSuitComp(Cards c, int n) {
        if (c) {
            if (n == 1) {
                return true;
            } else {
                c = CardsToQR(c);
                if (c & PQR_234) { // 2枚以上
                    if (n == 2) {
                        return true;
                    } else {
                        if (c & PQR_34) { // 4枚
                            if (n <= 4) { return true; }
                        } else {
                            if (((c & PQR_2) >> 1) & c) { // 3枚
                                if (n == 3) { return true; }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    void initCards() {
        // カード集合関係の初期化
        
        // nd計算に使うテーブル
        for (int r = 0; r < 16; ++r) {
            // オーダー通常
            ORQ_NDTable[0][r][0] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1;
            ORQ_NDTable[0][r][1] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_12;
            ORQ_NDTable[0][r][2] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_123;
            ORQ_NDTable[0][r][3] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            for (int q = 4; q < 8; ++q) {
                ORQ_NDTable[0][r][q] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            }
            // オーダー逆転
            ORQ_NDTable[1][r][0] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1;
            ORQ_NDTable[1][r][1] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_12;
            ORQ_NDTable[1][r][2] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_123;
            ORQ_NDTable[1][r][3] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            for (int q = 4; q < 8; ++q) {
                ORQ_NDTable[1][r][q] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            }
            //複数ジョーカーには未対応
        }
    }
    
    struct CardsInitializer {
        CardsInitializer() {
            initCards();
        }
    };
    
    CardsInitializer cardsInitializer;
    
    /**************************カード集合表現(クラス版)**************************/
    
    /*struct Cards : public BitSet64 {
        
        // 定数
        constexpr Cards() : BitSet64() {}
        constexpr Cards(const BitCards ac) : BitSet64(ac) {}
        constexpr Cards(const Cards& ac) : BitSet64(ac.cards()) {}
        
        Cards& fill() {
            set(CARDS_ALL);
        }
     
        // 生のBitCards型への変換
        BitCards cards() const { return static_cast<BitCards>(data()); }
        BitCards& cards() { return data(); }
        
        bool anyJOKER() const { return anyJOKER(cards()); }
        bool anyPlain() const { return anyPlain(cards()); }
        bool any() const { return anyCards(cards()); }
        bool any2() const { return any2Cards(cards()); }
        
        bool containsJOKER() const{ return containsJOKER(cards()); }
        bool containsS3() const{ return containsS3(cards()); }
        bool containsD3() const{ return containsD3(cards()); }
        
        uint32_t countPlain() const {
             return countCards(maskJOKER(cards()));
        }
        
        Cards& set(BitCards ac) {
            (*this) = ac;
        }
        Cards& set(Cards ac) {
            (*this) = ac.cards();
        }
        Cards& add(BitCards ac) {
            addCards(&cards(), ac);
            return *this;
        }
        Cards& addJOKER() {
            addJOKER(&cards());
            return *this;
        }
        Cards& addIntCard(IntCard ic) {
            addIntCard(&cards(), ic);
            return *this;
        }
        Cards& sub(BitCards ac) {
            subtrCards(&cards(), ac);
        }
        
        Cards& inv(Cards argc) {
            invCards(&cards, argc);
        }
        Cards& inv() {
            invCards(&cards, CARDS_ALL);
        }
        Cards& mask(BitCards ac) {
            maskCards(&cards(), ac);
            return *this;
        }
        Cards& maskJOKER() {
            UECda::maskJOKER(&cards());
            return *this;
        }

        // 包括性
        bool holds(const BitCards ac) const {
            return holdsCards(cards, ac);
        }
        
        
        // pick, pop
        IntCard pickIntCardLow() const {
            assert(any());
            return UECda::pickIntCardLow(cards());
        }
        IntCard pickIntCardHigh() const {
            assert(any());
            return UECda::pickIntCardHigh(cards());
        }
        IntCard popIntCardLow() {
            assert(any());
            return UECda::popIntCardLow(&cards());
        }
        IntCard popIntCardHigh() {
            assert(any());
            return UECda::popIntCardHigh(&cards());
        }
        Cards pickLow() const{
            assert(any());
            pickL
        }
        Cards pickHigh() const{
            assert(any());
        }
        Cards pickHigh() const{
            assert(any());
        }
        Cards pickLow() const{
            assert(any());
        }
        
        // 複数pick, pop
        template <int N = 1>Cards pickLow(
        ) const{
            assert(count() >= N);
        }
        
        template <int N = 1,int JK = 1>
        void pickHigh(Cards96<cards_t,NJK> *const dst) const {
            assert(count() >= N);
            if (JK && jk >= (uint32_t)N) {
                dst->cards=CARDS_NULL;
                dst->jk = (uint32_t)N;
            } else {
                dst->cards=::pickHigh(cards,(uint32_t)N-jk);
                dst->jk=jk;
            }
        }
        
        template <int N = 1,int JK = 1> void popLow(Cards96<cards_t,NJK> *const dst) {
            assert( count() >= N );
            uint32_t nex=countPlain();
            if ( (!JK) || nex >= (uint32_t)N ) {
                Cards tmp=::pickLow<N>(cards);
                dst->cards=tmp;
                dst->jk=0;
                subtr(tmp);
            } else {
                dst->cards=cards;
                dst->jk=(uint32_t)N - nex;
                cards=CARDS_NULL;
                jk-=(uint32_t)N - nex;
            }
        }
        
        template <int N=1,int JK=1> void popHigh(Cards96<cards_t,NJK> *const dst) {
            assert( count() >= N );
            if ( JK && jk >= (uint32_t)N ) {
                dst->cards=CARDS_NULL;
                dst->jk=(uint32_t)N;
                jk-=(uint32_t)N;
            } else {
                Cards tmp=::pickHigh(cards,(uint32_t)N-jk);
                dst->cards=tmp;
                dst->jk=jk;
                subtr(tmp);
                jk=0;
            }
        }
        
        template <int N=1,int JK=1,class dice_t> void popRand(Cards96<cards_t,NJK> *const dst,int n,dice_t *const pdice) {
            assert( count() >= n );
            if ( !JK ) {
                Cards tmp=pickNBits64(cards,n,countPlain()-n,pdice);
                subtr(tmp);
                dst->cards=tmp;
                dst->jk=0;
            } else {
                dst->clear();
                Cards tmp=cards;
                if ( jk ) {
                    addCards(&tmp,CARDS_JOKER);
                    if ( jk==2 ) {
                        addCards(&tmp,CARDS_JOKER<<1);
                    }
                }
                Cards ret=pickNBits64(tmp,n,count()-n,pdice);
                dst->cards=pickCDHS(ret);
                dst->jk=countCards(ret-pickCDHS(ret));
                subtr(*dst);
            }
        }
        
        template <int N=1,int JK=1,class dice_t> void pickRand(Cards96<cards_t,NJK> *const dst,int n,dice_t *const pdice) const{
            assert( count() >= n );
            if ( !JK ) {
                Cards tmp=pickNBits64(cards,n,countPlain()-n,pdice);
                dst->cards=tmp;
                dst->jk=0;
            } else {
                dst->clear();
                Cards tmp=cards;
                if ( jk ) {
                    addCards(&tmp,CARDS_JOKER);
                    if ( jk==2 ) {
                        addCards(&tmp,CARDS_JOKER<<1);
                    }
                }
                Cards ret=pickNBits64(tmp,n,count()-n,pdice);
                dst->cards=pickCDHS(ret);
                dst->jk=countCards(ret-pickCDHS(ret));
            }
        }
        
        //階段の作成可能性
        template <int N=MIN_SEQ_QTY,int JK_QTY=-1>Cards canMakeSeq() {
            //持っているジョーカーの枚数と異なる枚数でJK_QTYを指定されてもよいことにする
            switch ( JK_QTY ) {
                case 0: return polymRanks<N>(cards);break;
                case 1: return polymRanksWithJOKER<N,1>(cards);break;
                case 2: return polymRanksWithJOKER<N,2>(cards);break;
                default:
                    assert( jk<=N_JOKERS );
                    switch ( jk ) {
                        case 0: return canMakeSeq<N,0>();break;
                        case 1: return canMakeSeq<N,1>();break;
                        case 2: return canMakeSeq<N,2>();break;
                        default: assert(0);break;
                    }
                    break;
            }
        }
        
        //static関数
     
    };*/
    /**************************役集合表現**************************/
    
    // 着手の逐次生成用の軽い役集合表現
    // タイプによってサイズが違う
    
    //階段用
    /*
     template <int CON_JOKER>struct SeqMelds{
     
     Cards melds;
     
     Move popLow() {
     
     }
     
     };
     */
}