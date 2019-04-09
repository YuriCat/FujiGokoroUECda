#pragma once

#include <iostream>
#include <array>
#include <map>
#include <iterator>
#include "util.hpp"

namespace UECda {
    
    // 大富豪における最も基本的な型の実装
    
    /**************************オーダー**************************/
    
    inline constexpr int flipOrder(int ord) { return 1 - ord; }
    
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

    inline bool isEightSeqRank(int rank, int qty) {
        return rank <= RANK_8 && RANK_8 < rank + qty;
    }

    // 出力
    static const std::string rankChar  = "-3456789TJQKA2+:";
    static const std::string rankCharM = "-3456789tjqka2+:";
    
    struct OutRank {
        int r;
        constexpr OutRank(const int& arg): r(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutRank& arg) {
        out << rankChar[arg.r];
        return out;
    }
    
    struct OutRankM {
        int r;
        constexpr OutRankM(const int& arg) :r(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutRankM& arg) {
        out << rankCharM[arg.r];
        return out;
    }

    struct RankRange { // 連続ランク
        int r0, r1;
        constexpr RankRange(const int& arg0, const int& arg1) :r0(arg0), r1(arg1) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const RankRange& arg) {
        for (int r = arg.r0; r <= arg.r1; r++) {
            out << rankChar[r];
        }
        return out;
    }
    
    struct RankRangeM { // 連続ランク
        int r0, r1;
        constexpr RankRangeM(const int& arg0, const int& arg1) :r0(arg0), r1(arg1) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const RankRangeM& arg) {
        for (int r = arg.r0; r <= arg.r1; r++) {
            out << rankCharM[r];
        }
        return out;
    }
    
    inline int CharToRank(char c) {
        int r = rankChar.find(c);
        if (r == std::string::npos) return RANK_NONE;
        return r;
    }
    inline int CharToRankM(char c) {
        int r = rankCharM.find(c);
        if (r == std::string::npos) return RANK_NONE;
        return r;
    }
    
    /**************************スート番号**************************/

    // 単スート
    enum {
        SUITNUM_C, SUITNUM_D, SUITNUM_H, SUITNUM_S,
        SUITNUM_X,
        SUITNUM_MIN = SUITNUM_C,
        SUITNUM_MAX = SUITNUM_S,
        SUITNUM_NONE = -1
    };
    
    constexpr int N_SUITS = 4;
    
    // 出力
    const std::string suitNumChar  = "CDHSX";
    const std::string suitNumCharM = "cdhsx";
        
    struct OutSuitNum {
        int sn;
        constexpr OutSuitNum(const int& arg) :sn(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutSuitNum& arg) {
        out << suitNumChar[arg.sn];
        return out;
    }
    
    struct OutSuitNumM {
        int sn;
        constexpr OutSuitNumM(const int& arg) :sn(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutSuitNumM& arg) {
        out << suitNumCharM[arg.sn];
        return out;
    }
    
    inline int CharToSuitNum(char c) {
        int sn = suitNumChar.find(c);
        if (sn == std::string::npos) { return SUITNUM_NONE; }
        return sn;
    }
    inline int CharToSuitNumM(char c) {
        int sn = suitNumCharM.find(c);
        if (sn == std::string::npos) { return SUITNUM_NONE; }
        return sn;
    }
    
    /**************************スート**************************/
    
    using Suits = unsigned int;
    
    // 単スート
    enum {
        SUIT_NULL = 0,
        SUIT_C = 1, SUIT_D = 2, SUIT_H = 4, SUIT_S = 8,
        SUIT_X = 16,
        SUIT_MIN = SUIT_C,
        SUIT_MAX = SUIT_S
    };
    
    // スート集合 (スートの和集合)
    enum {
        SUITS_NULL, SUITS_C,   SUITS_D,   SUITS_CD,
        SUITS_H,    SUITS_CH,  SUITS_DH,  SUITS_CDH,
        SUITS_S,    SUITS_CS,  SUITS_DS,  SUITS_CDS,
        SUITS_HS,   SUITS_CHS, SUITS_DHS, SUITS_CDHS,
        SUITS_CDHSX = SUITS_CDHS | SUIT_X, // クインタプル
        SUITS_ALL = SUITS_CDHS,
    };
    
    inline int countSuits(uint32_t s) { return popcnt(s); }
    
    // スートインデックス
    // スートビットから、その種類の役の中で何番目のスートパターンとされているかを得る
    constexpr int suitsIdx[18] = {
        -1, 0, 1, 0, 2, 1, 3, 0, 3, 2, 4, 1, 5, 2, 3, 0, 0, 5
    };
    
    inline int SuitToSuitNum(unsigned int suit) { return bsf32(suit); }
    
    // 単スート番号からスート集合への変換
    inline constexpr unsigned SuitNumToSuits(int sn0) { return (1U << sn0); }
    // 複スートの場合も
    inline int SuitsToSuitNum(int suit) { return suitsIdx[suit]; }
    
    // 出力
    struct OutSuits {
        uint32_t s;
        constexpr OutSuits(const uint32_t& arg) :s(arg) {}
    };
    static std::ostream& operator <<(std::ostream& out, const OutSuits& arg) { // 出力の時だけ第５のスートは16として対応している
        for (int sn = 0; sn < N_SUITS + 1; sn++) {
            if (arg.s & SuitNumToSuits(sn)) {
                out << suitNumChar[sn];
            }
        }
        return out;
    }

    struct OutSuitsM {
        uint32_t s;
        constexpr OutSuitsM(const uint32_t& arg) :s(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutSuitsM& arg) { // 出力の時だけ第5のスートは16として対応している
        for (int sn = 0; sn < N_SUITS + 1; sn++) {
            if (arg.s & SuitNumToSuits(sn)) {
                out << suitNumCharM[sn];
            }
        }
        return out;
    }
    
    // (スート, スート)のパターン
    uint8_t suitsSuitsIndexTable[16][16];
    uint8_t twoSuitsIndexTable[16][16];
    BitArray64<4, 16> suitSuitsIndexTable[16];
    
    // (スート, スート, スート)のパターン
    uint16_t suitsSuitsSuitsIndexTable[16][16][16];
    uint16_t suitSuitsSuitsIndexTable[16][16][16];
    
    constexpr int N_PATTERNS_SUIT_SUITS = 8;
    constexpr int N_PATTERNS_2SUITS = 22;
    constexpr int N_PATTERNS_SUITS_SUITS = 35;
    constexpr int N_PATTERNS_SUITS_SUITS_SUITS = 330;
    constexpr int N_PATTERNS_SUIT_SUITS_SUITS = 80;
    
    inline int getSuitSuitsIndex(uint32_t s0, uint32_t s1) {
        return suitSuitsIndexTable[s0][s1];
    }
    inline int getSuitsSuitsIndex(uint32_t s0, uint32_t s1) {
        return suitsSuitsIndexTable[s0][s1];
    }
    inline int get2SuitsIndex(uint32_t s0, uint32_t s1) {
        return twoSuitsIndexTable[s0][s1];
    }
    inline int getSuitsSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2) {
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    inline int getSuitSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2) {
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    
    inline void initSuits() {
        
        // (suits suits) pattern index (exchangable) 0 ~ 21
        int twoSuitsCountIndex[5][5][5] = {0};
        int cnt = 0;
        for (int c0 = 0; c0 <= 4; c0++) {
            for (int c1 = 0; c1 <= c0; c1++) {
                for (int c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); ++c01) {
                    DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c01 << endl;
                    twoSuitsCountIndex[c0][c1][c01] = cnt;
                    ++cnt;
                }
            }
        }
        ASSERT(cnt == N_PATTERNS_2SUITS, cerr << cnt << " <-> " << N_PATTERNS_2SUITS << endl;);
        
        // (suits, suits) pattern index 0 ~ 34
        int suitsSuitsCountIndex[5][5][5] = {0};
        cnt = 0;
        for (int c0 = 0; c0 <= 4; c0++) {
            for (int c1 = 0; c1 <= 4; c1++) {
                for (size_t c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); ++c01) {
                    DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c01 << endl;
                    suitsSuitsCountIndex[c0][c1][c01] = cnt++;
                }
            }
        }
        ASSERT(cnt == N_PATTERNS_SUITS_SUITS,
               cerr << cnt << " <-> " << N_PATTERNS_SUITS_SUITS << endl;);
        
        for (uint32_t s0 = 0; s0 < 16; s0++) {
            for (uint32_t s1 = 0; s1 < 16; s1++) {
                const uint32_t s01 = s0 & s1;
                const uint32_t c0 = popcnt(s0), c1 = popcnt(s1);
                const uint32_t cmin = min(c0, c1), cmax = max(c0, c1);
                const uint32_t c01 = popcnt(s01);

                suitsSuitsIndexTable[s0][s1]
                = suitsSuitsCountIndex[c0][c1][c01];
                
                twoSuitsIndexTable[s0][s1]
                = twoSuitsCountIndex[cmax][cmin][c01];
            }
        }
        
        // (suit, suits) pattern index 0 ~ 7
        int suitSuitsCountIndex[5][2] = {0};
        cnt = 0;
        for (int c1 = 0; c1 <= 4; ++c1) {
            for (int c01 = max(0, 1 + c1 - 4); c01 <= min(c1, 1); ++c01) {
                assert(c01 == 0 || c01 == 1);
                DERR << "pattern " << cnt << " = " << c1 << ", " << c01 << endl;
                suitSuitsCountIndex[c1][c01] = cnt;
                ++cnt;
            }
        }
        ASSERT(cnt == N_PATTERNS_SUIT_SUITS, cerr << cnt << " <-> " << N_PATTERNS_SUIT_SUITS << endl;);
        
        for (int sn0 = 0; sn0 < 4; ++sn0) {
            for (uint32_t s1 = 0; s1 < 16; ++s1) {
                const uint32_t s0 = SuitNumToSuits(sn0);
                const uint32_t s01 = s0 & s1;
                const uint32_t c1 = popcnt(s1);
                const uint32_t c01 = popcnt(s01);
                
                suitSuitsIndexTable[s0].assign(s1, suitSuitsCountIndex[c1][c01]);
            }
        }
        
        // (suits, suits, suits) pattern index
        //int suitsSuitsSuitsCountIndex[5][5][5][5][5][5][5] = {0};
        std::map<std::array<uint32_t, ipow(2, 3) - 1>, int> sssMap;
        for (uint32_t s0 = 0; s0 < 16; ++s0) {
            for (uint32_t s1 = 0; s1 < 16; ++s1) {
                for (uint32_t s2 = 0; s2 < 16; ++s2) {
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c0 = popcnt(s0), c1 = popcnt(s1), c2 = popcnt(s2);
                    const uint32_t c01 = popcnt(s01), c02 = popcnt(s02), c12 = popcnt(s12);
                    const uint32_t c012 = popcnt(s012);
                    std::array<uint32_t, ipow(2, 3) - 1> pattern = {c0, c1, c2, c01, c02, c12, c012};
                    int cnt;
                    if (sssMap.count(pattern) == 0) {
                        cnt = sssMap.size();
                        sssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << ", " << c012 << endl;
                    } else {
                        cnt = sssMap[pattern];
                    }
                    //suitsSuitsSuitsCountIndex[c0][c1][c2][c01][c02][c12][c012] = cnt;
                    suitsSuitsSuitsIndexTable[s0][s1][s2] = cnt;
                }
            }
        }
        ASSERT(sssMap.size() == N_PATTERNS_SUITS_SUITS_SUITS,
               cerr << sssMap.size() << " <-> " << N_PATTERNS_SUITS_SUITS_SUITS << endl;);
        
        //int suitSuitsSuitsCountIndex[5][5][5][5][5] = {0};
        std::map<std::array<uint32_t, ipow(2, 3) - 3>, int> s1ssMap;
        for (uint32_t s0 = 1; s0 < 16; s0 <<= 1) {
            for (uint32_t s1 = 0; s1 < 16; ++s1) {
                for (uint32_t s2 = 0; s2 < 16; ++s2) {
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c1 = popcnt(s1), c2 = popcnt(s2);
                    const uint32_t c01 = popcnt(s01), c02 = popcnt(s02), c12 = popcnt(s12);
                    std::array<uint32_t, ipow(2, 3) - 3> pattern = {c1, c2, c01, c02, c12};
                    int cnt;
                    if (s1ssMap.count(pattern) == 0) {
                        cnt = s1ssMap.size();
                        s1ssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << endl;
                    } else {
                        cnt = s1ssMap[pattern];
                    }
                    //suitSuitsSuitsCountIndex[c1][c2][c01][c02][c12] = cnt;
                    suitSuitsSuitsIndexTable[s0][s1][s2] = cnt;
                }
            }
        }
    }
    
    struct SuitsInitializer {
        SuitsInitializer() {
            initSuits();
        }
    };
    
    SuitsInitializer suitsInitializer;

    /**************************カード整数**************************/
    
    // U3456789TJQKA2O、CDHSの順番で0-59　ジョーカーは60
    
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
    
    // 出力用クラス
    struct OutIntCard {
        IntCard ic;
        constexpr OutIntCard(const IntCard& arg) :ic(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutIntCard& arg) {
        if (arg.ic == INTCARD_JOKER) {
            out << "JO";
        } else {
            out << OutSuitNum(IntCardToSuitNum(arg.ic)) << OutRank(IntCardToRank(arg.ic));
        }
        return out;
    }

    inline IntCard StringToIntCard(const std::string& str) {
        if (str.size() != 2) return INTCARD_NONE;
        if (str == "JO") return INTCARD_JOKER;
        int sn = CharToSuitNum(str[0]);
        int r = CharToRank(str[1]);
        if (r == RANK_NONE) return INTCARD_NONE;
        if (sn == SUITNUM_NONE) return INTCARD_NONE;
        return RankSuitNumToIntCard(r, sn);
    }

    struct OutIntCardM {
        IntCard ic;
        constexpr OutIntCardM(const IntCard& arg): ic(arg) {}
    };
    inline std::ostream& operator <<(std::ostream& out, const OutIntCardM& arg) {
        if (arg.ic == INTCARD_JOKER) {
            out << "jo";
        } else {
            out << OutSuitNumM(IntCardToSuitNum(arg.ic)) << OutRankM(IntCardToRank(arg.ic));
        }
        return out;
    }
    inline IntCard StringToIntCardM(const std::string& str) {
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

    using BitCards = uint64_t;
    
    // 基本定数
    constexpr BitCards CARDS_HORIZON = 1ULL;
    constexpr BitCards CARDS_HORIZONRANK = 15ULL; // 基準の最低ランクのカード全て
    constexpr BitCards CARDS_HORIZONSUIT = 0x0111111111111111; // 基準の最小スートのカード全て
    
    // IntCard型との互換
    constexpr BitCards IntCardToCards(IntCard ic) {
        return BitCards(CARDS_HORIZON << ic);
    }
    
    // 定数
    constexpr BitCards CARDS_MIN = CARDS_HORIZON << INTCARD_MIN;
    constexpr BitCards CARDS_MAX = CARDS_HORIZON << INTCARD_MAX;

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

    constexpr BitCards CARDS_JOKER_RANK = 0xF000000000000000;
    
    // あるランクのカード全て
    constexpr BitCards RankToCards(int r) {
        return CARDS_HORIZONRANK << (r << 2);
    }
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
    
    // スートの指定からカード集合を生成する
    constexpr BitCards SuitsToCards(uint32_t s) {
        return CARDS_HORIZONSUIT * s; // あるスートのカード全て
    }
    // ランクとスートの指定からカード集合を生成する
    // スートは集合として用いる事が出来る
    constexpr BitCards RankSuitsToCards(int r, uint32_t s) {
        return (BitCards)s << (r << 2);
    }
    
    // Cards型基本演算
    
    // 追加
    constexpr BitCards addCards(BitCards c0, BitCards c1) { return c0 | c1; }

    constexpr BitCards addIntCard(BitCards c, IntCard ic) { return addCards(c, IntCardToCards(ic)); }
    constexpr BitCards addJOKER(BitCards c) { return addCards(c, CARDS_JOKER); }
    
    // 限定
    constexpr BitCards andCards(BitCards c0, BitCards c1) { return c0 & c1; }
    
    // 削除（オーバーを引き起こさない）
    // maskは指定以外とのandをとることとしている。
    // 指定部分とのandはand処理で行う
    constexpr BitCards maskCards(BitCards c0, BitCards c1) { return c0 & ~c1; }
    constexpr BitCards maskJOKER(BitCards c) { return maskCards(c, CARDS_JOKER); }
    
    // カード減算
    // 現在では整数としての引き算処理
    constexpr BitCards subtrCards(BitCards c0, BitCards c1) { return c0 - c1; }
    constexpr BitCards subtrJOKER(BitCards c) { return subtrCards(c, CARDS_JOKER); }
    
    // 要素数
    constexpr uint32_t countFewCards(BitCards c) { return popcnt64CE(c); } // 要素が比較的少ない時の速度優先
    inline uint32_t countCards(BitCards c) { return popcnt64(c); } // 基本のカウント処理
    constexpr BitCards any2Cards(BitCards c) { return c & (c - 1ULL); }
    
    // 排他性
    constexpr bool isExclusiveCards(BitCards c0, BitCards c1) { return !(c0 & c1); }
    
    // 包含関係
    constexpr BitCards containsCard(BitCards c0, BitCards c1) { return andCards(c0, c1); } // 単体に対してはandでok
    constexpr BitCards containsIntCard(BitCards c, IntCard ic) { return containsCard(c, IntCardToCards(ic)); }
    constexpr BitCards containsJOKER(BitCards c) { return andCards(c, CARDS_JOKER); }
    constexpr BitCards containsS3(BitCards c) { return andCards(c, CARDS_S3); }
    constexpr BitCards containsD3(BitCards c) { return andCards(c, CARDS_D3); }
    constexpr BitCards contains8(BitCards c) { return andCards(c, CARDS_8); }
    constexpr bool holdsCards(BitCards c0, BitCards c1) { return !(~c0 & c1); }
    
    // 空判定
    constexpr BitCards anyCards(BitCards c) { return c; }
    
    // validation
    constexpr bool examCards(BitCards c) { return holdsCards(CARDS_ALL, c); }
    constexpr bool examImaginaryCards(BitCards c) { return holdsCards(CARDS_IMG_ALL, c); }

    // Cards型特殊演算
    
    // 特定順序の要素を選ぶ（元のデータは変えない）
    inline BitCards pickLow(const BitCards c, int n) {
        assert(n > 0 && (int)countCards(c) >= n);
        return lowestNBits(c, n);
    }
    inline BitCards pickHigh(const BitCards c, int n) {
        assert(n > 0 && (int)countCards(c) >= n);
        return highestNBits(c, n);
    }

    // IntCard型で1つ取り出し
    inline IntCard pickIntCardLow(const BitCards c) {
        return (IntCard)bsf64(c);
    }
    inline IntCard pickIntCardHigh(const BitCards c) {
        return (IntCard)bsr64(c);
    }
    // 基準cより高い、低い(同じは含まず)もの
    inline BitCards pickHigher(BitCards c) {
        return allHigherBits(c);
    }
    inline BitCards pickLower(BitCards c) {
        return allLowerBits(c);
    }
    inline BitCards pickHigher(BitCards c0, BitCards c1) {
        return c0 & allHigherBits(c1);
    }
    inline BitCards pickLower(BitCards c0, BitCards c1) {
        return c0 & allLowerBits(c1);
    }

    // ランク重合
    // ランクを１つずつ下げてandを取るのみ(ジョーカーとかその辺のことは何も考えない)
    // 階段判定に役立つ
    template <int N>
    constexpr BitCards polymRanks(BitCards c) {
        static_assert(N >= 0, "");
        return ((c >> ((N - 1) << 2)) & polymRanks<N - 1>(c));
    }
    template <> constexpr BitCards polymRanks<0>(BitCards c) { return CARDS_NULL; }
    template <> constexpr BitCards polymRanks<1>(BitCards c) { return c; }
    inline BitCards polymRanks(BitCards c, int n) { // 重合数が変数の場合
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
    template <> inline BitCards extractRanks<4>(BitCards c) {
        BitCards r = c | (c << 4);
        return r | (r << 8);
    }
    template <> inline BitCards extractRanks<5>(BitCards c) {
        BitCards r = c | (c << 4);
        return r | (c << 8) | (r << 12);
    }
    inline BitCards extractRanks(BitCards c, int n) { // 展開数が変数の場合
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
                if (e) { f |= e | (e >> 4); }
                r = f;
            } break;
            case 5: {
                BitCards d = c & (c >> 4);
                BitCards g = d | (d >> 12); // 2 + 2パターン
                BitCards e = d & (c >> 8); // 3連
                if (e) {
                    g |= (e & (c >> 16)) | (c & (e >> 8)); // 1 + 3パターン
                    BitCards f = e & (c >> 12);
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
            case 2: res = subtrCards(RankRangeToCards(RANK_MIN, RANK_MAX), RankToCards(rank)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    inline BitCards ORQToSCValidZone(int ord, int rank, int qty) { // ランク限定のみ
        BitCards res;
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
    inline bool isValidGroupRank(int mvRank, int order, int bdRank) {
        if (order == 0) return mvRank > bdRank;
        else return mvRank < bdRank;
    }
    inline bool isValidSeqRank(int mvRank, int order, int bdRank, int qty) {
        if (order == 0) return mvRank >= bdRank + qty;
        else return mvRank <= bdRank - qty;
    }

    // 枚数オンリーによる許容包括
    inline BitCards seqExistableZone(int qty) {
        return RankRangeToCards(RANK_IMG_MIN, RANK_IMG_MAX + 1 - qty);
    }

    /**************************カード集合表現(クラス版)**************************/

    struct CardsAsSet {
        // ビット単位で1つずつ取り出す用
        BitCards c_;
        constexpr CardsAsSet(BitCards c): c_(c) {}

        BitCards lowest() const { assert(c_); return c_ & -c_; }
        BitCards popLowest() {
            assert(c_);
            BitCards l = lowest();
            c_ -= l;
            return l;
        }
    
        class const_iterator : public std::iterator<std::input_iterator_tag, BitCards> {
            friend CardsAsSet;
        public:
            BitCards operator *() const {
                // 下1ビットを取り出す
                return c_ & -c_;
            }
            bool operator !=(const const_iterator& itr) const {
                return pclass_ != itr.pclass_ || c_ != itr.c_;
            }
            const_iterator& operator ++() {
                c_ = popLsb<BitCards>(c_);
                return *this;
            }
        protected:
            explicit const_iterator(const CardsAsSet *pclass): pclass_(pclass), c_(pclass->c_) {}
            explicit const_iterator(const CardsAsSet *pclass, BitCards c): pclass_(pclass), c_(c) {}
            const CardsAsSet *const pclass_;
            BitCards c_;
        };

        const_iterator begin() const { return const_iterator(this); }
        const_iterator end() const { return const_iterator(this, 0); }
    };
    
    union Cards {
        BitCards c_;
        struct {
            unsigned long long plain_: 60;
            signed int joker_: 4;
        } bf_;

        // 定数
        constexpr Cards(): c_() {}
        constexpr Cards(BitCards c): c_(c) {}
        constexpr Cards(const Cards& c): c_(c.c_) {}
        constexpr Cards(IntCard ic) = delete; // 混乱を避ける

        Cards(const std::string& str) {
            clear();
            std::vector<std::string> v = split(str, ' ');
            for (const std::string& s : v) {
                IntCard ic = StringToIntCardM(s);
                // 大文字形式かもしれないので試す
                if (ic == INTCARD_NONE) ic = StringToIntCard(s);
                if (ic != INTCARD_NONE) insert(ic);
            }
        }
        Cards(const char *cstr): Cards(std::string(cstr)) {}
     
        // 生のBitCards型への変換
        constexpr operator BitCards() const { return c_; }

        constexpr bool empty() const { return !anyCards(c_); }
        constexpr bool any() const { return anyCards(c_); }
        constexpr bool any2() const { return any2Cards(c_); }
        
        constexpr bool anyJOKER() const { return UECda::containsJOKER(c_); }
        constexpr bool contains(IntCard ic) const { return UECda::containsIntCard(c_, ic); }

        constexpr int joker() const { return countJOKER(); }
        constexpr Cards plain() const { return UECda::maskJOKER(c_); }

        int count() const { return countCards(c_); }
        constexpr int countInCompileTime() const { return countFewCards(c_); }
        constexpr int countJOKER() const { return UECda::containsJOKER(c_) ? 1 : 0; }
        int countPlain() const { return countCards(plain()); }

        constexpr bool holds(BitCards c) const { return holdsCards(c_, c); }
        constexpr bool isExclusive(BitCards c) const { return isExclusiveCards(c_, c); }

        // 指定されたランクのスート集合を得る
        constexpr unsigned int operator[] (int r) const { return (c_ >> (r * 4)) & 15; }
        
        Cards& operator |=(BitCards c) { c_ |= c; return *this; }
        Cards& operator &=(BitCards c) { c_ &= c; return *this; }
        Cards& operator ^=(BitCards c) { c_ ^= c; return *this; }
        Cards& operator +=(BitCards c) { c_ += c; return *this; }
        Cards& operator -=(BitCards c) { c_ -= c; return *this; }
        Cards& operator <<=(int i) { c_ <<= i; return *this; }
        Cards& operator >>=(int i) { c_ >>= i; return *this; }
        Cards& operator <<=(unsigned int i) { c_ <<= i; return *this; }
        Cards& operator >>=(unsigned int i) { c_ >>= i; return *this; }

        Cards& clear() { c_ = 0; return *this; }
        Cards& fill() { c_ = CARDS_ALL; return *this; }

        Cards& merge(BitCards c) { return (*this) |= c; }
        Cards& mask(BitCards c) { return (*this) &= ~c; }
        Cards& maskJOKER() { return mask(CARDS_JOKER_RANK); }

        Cards& insert(IntCard ic) { return (*this) |= IntCardToCards(ic); }
        Cards& insertJOKER() { return (*this) |= CARDS_JOKER; }

        Cards& remove(IntCard ic) { return mask(IntCardToCards(ic)); }
        Cards& forceRemove(IntCard ic) {
            assert(contains(ic));
            return (*this) -= IntCardToCards(ic);
        }
        Cards& forceRemoveAll(BitCards c) {
            assert(holds(c));
            return (*this) -= c;
        }
        
        Cards& inv(BitCards c) { return (*this) ^= c; }
        Cards& inv() { return inv(CARDS_ALL); }
        
        // pick, pop
        IntCard lowest() const {
            assert(any());
            return IntCard(bsf64(c_));
        }
        IntCard highest() const {
            assert(any());
            return IntCard(bsr64(c_));
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

        class const_iterator : public std::iterator<std::input_iterator_tag, IntCard> {
            friend Cards;
        public:
            IntCard operator *() const {
                return IntCard(bsf<BitCards>(c_));
            }
            bool operator !=(const const_iterator& itr) const {
                return pclass_ != itr.pclass_ || c_ != itr.c_;
            }
            const_iterator& operator ++() {
                // 下1ビットのみ消す
                c_ &= c_ - 1;
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

        constexpr CardsAsSet divide() const { return CardsAsSet(c_); }

        std::string toString(bool lower = false) const {
            std::ostringstream oss;
            oss << "{";
            int cnt = 0;
            for (IntCard ic : *this) {
                if (cnt++ > 0) oss << " "; 
                if (lower) oss << OutIntCardM(ic);
                else oss << OutIntCard(ic);
            }
            oss << " }";
            return oss.str();
        }
        std::string toLowerString() const {
            return toString(true);
        }
    };

    inline std::ostream& operator <<(std::ostream& out, const Cards& c) {
        out << c.toString();
        return out;
    }

    struct CardArray : public BitArray64<4, 16> {
        constexpr CardArray(): BitArray64<4, 16>() {}
        constexpr CardArray(BitCards c): BitArray64<4, 16>(c) {}
        constexpr CardArray(const BitArray64<4, 16>& a): BitArray64<4, 16>(a) {}
        constexpr CardArray(const Cards& c): BitArray64<4, 16>(c.c_) {}
        operator BitCards() const { return BitCards(data()); }
    };

    struct OutCardTables {
        std::vector<Cards> cv;
        OutCardTables(const std::vector<Cards>& c): cv(c) {}
    };
    
    static std::ostream& operator <<(std::ostream& out, const OutCardTables& arg) {
        // テーブル形式で見やすく
        // ２つを横並びで表示
        for (int i = 0; i < (int)arg.cv.size(); i++) {
            out << " ";
            for (int r = RANK_3; r <= RANK_2; r++) out << " " << OutRank(r);
            out << " X    ";
        }
        out << endl;
        for (int sn = 0; sn < N_SUITS; sn++) {
            for (Cards c : arg.cv) {
                out << OutSuitNum(sn) << " ";
                for (int r = RANK_3; r <= RANK_2; r++) {
                    IntCard ic = RankSuitNumToIntCard(r, sn);
                    if (c.contains(ic)) out << "O ";
                    else out << ". ";
                }
                if (sn == 0) {
                    if (containsJOKER(c)) out << "O ";
                    else out << ". ";
                    out << "   ";
                } else {
                    out << "     ";
                }
            }
            out << endl;
        }
        return out;
    }

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
    constexpr BitCards QtyToPQR(uint32_t q) { return PQR_1 << (q - 1); }

    // パラレル演算関数
    inline CardArray CardsToQR(BitCards c) {
        // 枚数が各4ビットに入る
        BitCards a = (c & PQR_13) + ((c >> 1) & PQR_13);
        return (a & PQR_12) + ((a >> 2) & PQR_12);
    }

    // ランク中に丁度 n ビットあれば PQR_1 の位置にビットが立つ
    inline BitCards CardsToFR(BitCards c) {
        BitCards a = c & (c >> 1);
        return a & (a >> 2) & PQR_1;
    }
    inline BitCards CardsTo3R(BitCards c) {
        BitCards ab_cd = c & (c >> 1);
        BitCards axb_cxd = c ^ (c >> 1);
        return ((ab_cd & (axb_cxd >> 2)) | ((ab_cd >> 2) & axb_cxd)) & PQR_1;
    }
    inline BitCards CardsTo2R(BitCards c) {
        BitCards qr = CardsToQR(c);
        return (qr >> 1) & ~qr & PQR_1;
    }
    inline BitCards CardsTo1R(BitCards c) {
        return CardsTo3R(~c);
    }
    inline BitCards CardsTo0R(BitCards c) {
        return CardsToFR(~c);
    }
    inline BitCards CardsToNR(BitCards c, int q) {
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
    inline BitCards CardsToER(BitCards c) {
        // ランク中に1ビットでもあればPQR_1の位置にビットが立つ
        BitCards a = c | (c >> 1);
        return (a | (a >> 2)) & PQR_1;
    }
    inline BitCards CardsToPQR(BitCards arg) {
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
    inline BitCards QRToPQR(const CardArray qr) {
        // qr -> pqr 変換
        const BitCards iqr = ~qr;
        const BitCards qr_l1 = (qr << 1);
        const BitCards r = (PQR_1 & qr & (iqr >> 1)) | (PQR_2 & qr & (iqr << 1)) | ((qr & qr_l1) << 1) | (qr_l1 & PQR_4);
        return r;
    }
    inline BitCards PQRToSC(BitCards pqr) {
        // pqr -> sc はビットを埋めていくだけ
        BitCards r = pqr;
        r |= (r & PQR_234) >> 1;
        r |= (r & PQR_34) >> 2;
        return r;
    }
    inline void PQRToND(BitCards pqr, uint32_t jk, Cards *const nd) {
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

    // 役の作成可能性判定
    inline BitCards plainGroupCards(BitCards c, int q) {
        BitCards ret;
        switch (q) {
            case 0: ret = CARDS_ALL; break; // 0枚のグループは必ずできるとする
            case 1: ret = maskJOKER(c); break;
            case 2: ret = CardsToQR(c) & PQR_234; break;
            case 3: ret = (CardsToQR(c) + PQR_1) & PQR_34; break;
            case 4: ret = CardsToFR(c); break;
            default: ret = CARDS_NULL; break;
        }
        return ret;
    }
    inline BitCards groupCards(BitCards c, int q) {
        return plainGroupCards(c, containsJOKER(c) ? (q - 1) : q);
    }

    // スート圧縮判定
    // あるカード集合（スート限定がなされていてもよい）の中にn枚グループが作成可能かの判定
    // ただしジョーカーの分は最初から引いておく
    // 高速な処理には場合分けが必要か
    inline bool canMakeGroup(BitCards c, int n) {
        if (c) {
            if (n <= 1) return true;
            c = CardsToQR(c);
            if (c & PQR_234) { // 2枚以上
                if (n == 2) return true;
                if (c & PQR_34) { // 4枚
                    if (n <= 4) return true;
                } else {
                    if (((c & PQR_2) >> 1) & c) { // 3枚
                        if (n == 3) return true;
                    }
                }
            }
        }
        return false;
    }
    inline void initCards() {
        // カード集合関係の初期化
        
        // nd計算に使うテーブル
        for (int r = 0; r < 16; r++) {
            // オーダー通常
            ORQ_NDTable[0][r][0] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1;
            ORQ_NDTable[0][r][1] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_12;
            ORQ_NDTable[0][r][2] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_123;
            ORQ_NDTable[0][r][3] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            for (int q = 4; q < 8; q++) {
                ORQ_NDTable[0][r][q] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            }
            // オーダー逆転
            ORQ_NDTable[1][r][0] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1;
            ORQ_NDTable[1][r][1] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_12;
            ORQ_NDTable[1][r][2] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_123;
            ORQ_NDTable[1][r][3] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            for (int q = 4; q < 8; q++) {
                ORQ_NDTable[1][r][q] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            }
            // 複数ジョーカーには未対応
        }
    }

    struct CardsInitializer {
        CardsInitializer() {
            initCards();
        }
    };

    CardsInitializer cardsInitializer;

    /**************************着手表現**************************/

    // 最小のMove構造
    union Move16 {
        uint16_t m_;
        struct {
            unsigned s       : 4;
            signed   r       : 4;
            unsigned jks     : 4;
            signed   jkr     : 4;
        };
    };

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
        
        void clear()                      { Move tmp = {0}; (*this) = tmp; }
        void setPASS()                    { clear(); t = 0; }
        void setSingleJOKER()             { clear(); q = 1; t = 1; jks = SUITS_ALL; } // シングルジョーカーのランクは未定義
        void setS3()                      { setSingle(INTCARD_S3); } // スペ3切りの場合のみ

        void setSingle()                  { t = 1; }
        void setGroup()                   { t = 2; }
        void setSeq()                     { t = 3; }
        void setQty(uint32_t qty)         { q = qty; }
        void setRank(uint32_t rank)       { r = rank; }
        void setSuits(uint32_t suits)     { s = suits; }
        void setJokerRank(uint32_t jr)    { jkr = jr; }
        void setJokerSuits(uint32_t js)   { jks = js; }
        void setSpecialJokerSuits()       { jks = SUITS_ALL; }

        // タイプを指定してまとめて処理
        void setSingle(int rank, int suits) {
            clear();
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        void setGroup(int qty, int rank, int suits) {
            clear();
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setSeq(int qty, int rank, int suits) {
            clear();
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        // IntCard型からシングル着手をセットする
        void setSingle(IntCard ic) {
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // True or False
        constexpr bool isPASS() const { return  t == 0; }
        constexpr uint32_t isSeq() const { return  t == 3; }
        constexpr uint32_t isGroup() const { return  t == 2; }
        constexpr uint32_t isSingle() const { return  t == 1; }
        constexpr uint32_t isSingleOrGroup() const { return  t == 1 ||  t == 2; }
        constexpr bool isQuintuple() const {
            return isGroup() && q == 5;
        }
        constexpr uint32_t containsJOKER() const { return jks || jkr; }
        
        constexpr bool isSingleJOKER() const { return isSingle() && jks == SUITS_ALL; }
        constexpr bool isS3() const { return !isSeq() && rank() == RANK_3 && suits() == SUITS_S; }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s) const {
            // rank と スートが一致するか
            return rank() == r && suits() == s;
        }

        // 情報を得る
        constexpr uint32_t suits()      const { return s; }
        constexpr int qty()             const { return q; }
        constexpr int rank()            const { return r; }
        constexpr int jokerRank()       const { return jkr; }
        constexpr uint32_t jokerSuits() const { return jks; }
        constexpr uint32_t type()       const { return t; }

        int typeNum() const {
            uint32_t q = qty();
            if (isSeq()) {
                if (q >= 6) return 8;
                return 2 + q;
            } else {
                if (q >= 5) return 8;
                return q;
            }
        }
        
        Cards cards() const { // 構成するカード集合を得る
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            int r = rank();
            uint32_t s = suits();
            if (!isSeq()) {
                Cards c = CARDS_NULL;
                uint32_t jks = jokerSuits();
                if (jks) {
                    c |= CARDS_JOKER;
                    if (jks != SUITS_CDHS) s -= jks; // クインタプル対策
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
            Cards c = RankSuitsToCards(rank(), suits());
            if (isSeq()) c = extractRanks(c, qty());
            return c;
        }
        
        template <int QTY = 256>
        Cards charaPQR() const {
            static_assert((QTY == 256 || (1 <= QTY && QTY <= 4)), "Move::charaPQR\n");
            // 性質カードのPQRを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            // パスとシングルジョーカーも関係ないし、
            // 階段にも今の所対応していない(意味が無さそう)
            if (QTY == 0) {
                return CARDS_NULL;
            } else if (QTY == 1) {
                return CARDS_HORIZON << (rank() << 2);
            } else if (QTY != 256) {
                constexpr int sft = (QTY - 1) >= 0 ? ((QTY - 1) < 32 ? (QTY - 1) : 31) : 0; // warningに引っかからないように...
                if (1 <= QTY && QTY <= 4) {
                    return Cards(1U << sft) << (rank() << 2);
                } else {
                    return CARDS_NULL;
                }
            } else {
                return Cards(1U << (qty() - 1)) << (rank() << 2);
            }
        }

        constexpr bool domInevitably() const {
            if (isSeq()) return rank() <= RANK_8 && RANK_8 < rank() + qty();
            else return rank() == RANK_8;
        }
        constexpr bool isRev() const {
            if (isSeq()) return qty() >= 5;
            else return qty() >= 4;
        }
        constexpr bool isBack() const {
            return false;
        }
        constexpr uint32_t changesPrmState() const {
            return isRev();
        }
        bool exam() const {
            // 変な値でないかチェック
            int q = qty();
            int r = rank();
            uint32_t s = suits();
            if (!isPASS()) {
                if (q < 0) return false;
                if (isSeq()) {
                    if (q < 3) return false;
                    if (countSuits(s) != 1) return false;
                    if (isEightSeqRank(r, 3)) {
                        if (!domInevitably()) return false;
                    } else {
                        if (domInevitably()) return false;
                    }
                } else if (isSingle()) {
                    if (q != 1) return false;
                    if (countSuits(s) != 1) return false;
                } else {
                    if (!isQuintuple()) {
                        if (q != countSuits(s)) return false;
                    }
                }
            }
            return true;
        }

        bool operator ==(const Move& m) const {
            return toInt() == m.toInt();
        }
    };

    const Move MOVE_NULL = {0};
    const Move MOVE_PASS = {0};
    const Move MOVE_NONE = {15, 15};

    struct MeldChar : public Move {
        MeldChar(Move m): Move(m) {}
    };

    std::ostream& operator <<(std::ostream& out, const MeldChar& m) { // MeldChar出力
        if (m.isPASS()) {
            out << "PASS";
        } else if (m.isSingleJOKER()) {
            out << "JOKER";
        } else {
            // スート
            if (m.isQuintuple()) { // クインタ特別
                out << OutSuits(SUITS_CDHSX);
            } else {
                out << OutSuits(m.suits());
            }
            out << "-";
            
            // ランク
            int r = m.rank();
            if (m.isSeq()) {
                uint32_t q = m.qty();
                out << RankRange(r, r + q - 1);
            } else {
                out << OutRank(r);
            }
        }
        return out;
    }
    
    static std::ostream& operator <<(std::ostream& out, const Move& m) { // Move出力
        out << MeldChar(m) << m.cards();
        return out;
    }
    
    class LogMove : public Move {
        // ログ出力用
    public:
        LogMove(const Move& arg) : Move(arg) {}
    };
    
    static std::ostream& operator <<(std::ostream& out, const LogMove& m) { // LogMove出力
        if (m.isPASS()) {
            out << "p";
        } else if (m.isSingleJOKER()) {
            out << "jk";
        } else {
            int r = m.rank();
            if (m.isSeq()) {
                out << OutSuitsM(m.suits());
                out << "-";
                int q = m.qty();
                out << RankRangeM(r, r + q - 1);
                // ジョーカー
                if (m.containsJOKER()) {
                    out << "(" << OutRankM(m.jokerRank()) << ")";
                }
            } else {
                if (m.isQuintuple()) { // クインタ特別
                    out << OutSuitsM(SUITS_CDHSX);
                } else {
                    out << OutSuitsM(m.suits());
                }
                out << "-";
                out << OutRankM(r);
                if (m.containsJOKER()) {
                    out << "(" << OutSuitsM(m.jokerSuits()) << ")";
                }
            }
        }
        return out;
    }
        
    inline Move CardsToMove(const Cards chara, const Cards used) {
        // 性質 chara 構成札 used のカードから着手への変換
        Move m = MOVE_NULL;
        DERR << "pointer &m = " << (uint64_t)(&m) << endl << m << endl;
        if (chara == CARDS_NULL) return MOVE_PASS;
        if (chara == CARDS_JOKER) {
            m.setSingleJOKER();
            return m;
        }
        IntCard ic = chara.lowest();
        int r = IntCardToRank(ic);
        uint32_t s = chara[r];
        uint32_t ps = used[r]; // ジョーカーなしのスート
        int q = countCards(chara);
        if (!polymRanks<2>(chara)) { // グループ系
            if (q == 1) {
                m.setSingle(r, s);
            } else {
                m.setGroup(q, r, s);
                uint32_t js = s - ps;
                if (js) m.setJokerSuits(js);
            }
        } else { // 階段系
            m.setSeq(q, r, s);
            if (containsJOKER(used)) {
                IntCard jic = Cards(subtrCards(chara, used.plain())).lowest();
                uint32_t jr = IntCardToRank(jic);
                m.setJokerRank(jr);
                m.setJokerSuits(s);
            }
        }
        DERR << "pointer &m = " << (uint64_t)(&m) << endl;
        DERR << "chara " << chara << " used " << used << " -> " << MeldChar(m) << endl;
        return m;
    }
    
    inline Move StringToMoveM(const std::string& str) {
        // 入力文字列からMove型への変更
        Move mv = MOVE_NULL;
        bool jk = false; // joker used
        uint32_t s = SUITS_NULL;
        int rank = RANK_NONE;
        uint32_t ns = 0; // num of suits
        uint32_t nr = 0; // num of ranks
        size_t i = 0;
        
        // special
        if (str == "p") return MOVE_PASS;
        if (str == "jk") {
            mv.setSingleJOKER();
            return mv;
        }
        // suits
        for (; i < str.size(); i++) {
            char c = str[i];
            if (c == '-') { i++; break; }
            int sn = CharToSuitNumM(c);
            if (sn == SUITNUM_NONE) {
                CERR << "illegal suit number" << endl;
                return MOVE_NONE;
            }
            if (sn != SUITNUM_X) {
                s |= SuitNumToSuits(sn);
            } else { // クインタプル
                jk = true;
            }
            ns++;
        }
        // rank
        for (; i < str.size(); i++) {
            char c = str[i];
            if (c == '(') { jk = true; i++; break; }
            int r = CharToRankM(c);
            if (r == RANK_NONE) {
                CERR << "illegal rank" << endl;
                return MOVE_NONE;
            }
            if (rank == RANK_NONE) {
                rank = r;
            }
            ++nr;
        }
        // invalid
        if (s == SUITS_NULL) { CERR << "null suits" << endl; return MOVE_NONE; }
        if (!ns) { CERR << "zero suits" << endl; return MOVE_NONE; }
        if (rank == RANK_NONE) { CERR << "null lowest-rank" << endl; return MOVE_NONE; }
        if (!nr) { CERR << "zero ranks" << endl; return MOVE_NONE; }
        // seq or group?
        if (nr > 1) { // seq
            mv.setSeq(nr, rank, s);
        } else if (ns == 1) { // single
            mv.setSingle(rank, s);
        } else { // group
            mv.setGroup(ns, rank, s);
        }
        // joker
        if (jk) {
            if (!mv.isSeq()) {
                uint32_t jks = SUITS_NULL;
                for (; i < str.size(); i++) {
                    char c = str[i];
                    if (c == ')') break;
                    int sn = CharToSuitNumM(c);
                    if (sn == SUITNUM_NONE) {
                        CERR << "illegal joker-suit" << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jks |= SuitNumToSuits(sn);
                }
                if (jks == SUITS_NULL) jks = SUITS_CDHS; // クインタのときはスート指定なくてもよい
                mv.setJokerSuits(jks);
            } else {
                int jkr = RANK_NONE;
                for (; i < str.size(); i++) {
                    char c = str[i];
                    if (c == ')') break;
                    int r = CharToRankM(c);
                    if (r == RANK_NONE) {
                        CERR << "illegal joker-rank " << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jkr = r;
                }
                mv.setJokerRank(jkr);
                mv.setJokerSuits(mv.suits());
            }
        }
        return mv;
    }
    
    // 生成関数
    Move IntCardToSingleMove(IntCard ic) {
        // IntCard型から対応するシングル役に変換
        // シングルジョーカー、ジョーカーS3は非対応
        Move m = MOVE_NULL;
        m.setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        return m;
    }
    
    template <class move_buf_t>
    int searchMove(const move_buf_t *const buf, const int moves, const move_buf_t& move) {
        // 同じ着手の探索
        for (int m = 0; m < moves; m++) {
            if (buf[m]== move) return m;
        }
        return -1;
    }
    
    template <class move_buf_t, typename callback_t>
    int searchMove(const move_buf_t *const buf, const int moves, const callback_t& callback) {
        // callback を条件とする着手の探索
        for (int m = 0; m < moves; m++) {
            if (callback(buf[m])) return m;
        }
        return -1;
    }

    /**************************場表現**************************/
    
    // 各プレーヤーの情報を持たない場表現
    // 32ビット着手表現と同一の系列で扱えるようにする
    // ジョーカー情報などを残すかは難しいが、現在はほとんどの情報を残したまま
    
    struct Board : public Move {
        
        void init() { clear(); }
        Move move() const { return Move(*this); }
        
        // set, fix
        
        void setTmpOrder(uint32_t ord) { Move::o = ord; }
        void setPrmOrder(uint32_t ord) { Move::po = ord; }
        
        void fixTmpOrder(uint32_t ord) { setTmpOrder(ord); }
        void fixPrmOrder(uint32_t ord) { setPrmOrder(ord); }
        
        void flipTmpOrder() { Move::o ^= 1; }
        void flipPrmOrder() { Move::po ^= 1; }
        
        void resetDom() { Move::invalid = 0; }
        
        // 2体情報をメンバ関数で返す関数
        // 半マスク化みたいな感じ
        constexpr bool domInevitably() const { return invalid; }
        constexpr bool domConditionally(Move m) const { return isSingleJOKER() && m.isS3(); }
        
        constexpr bool locksSuits(Move m) const { return !isNull() && suits() == m.suits(); }
        constexpr bool locksRank(Move m) const { return false; } // ルールにない
        
        constexpr uint32_t afterPrmOrder(Move m) const {
            return prmOrder() ^ bool(m.isRev());
        }
        constexpr uint32_t afterTmpOrder(Move m) const {
            return order() ^ bool(m.isRev()) ^ bool(m.isBack());
        }
        constexpr bool afterSuitsLocked(Move m) const {
            return suitsLocked() || locksSuits(m);
        }
        
        // get
        constexpr int prmOrder()   const { return Move::po; }
        constexpr int order()      const { return Move::o; }
        
        // true or false
        constexpr bool isNull() const { return type() == 0; }
        constexpr bool suitsLocked() const { return Move::sl; }
        constexpr bool rankLocked() const { return Move::rl; }
        
        constexpr bool isRev() const { return po; }
        
        bool isInvalid() const {
            return Move::invalid;
        }
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
            fixPrmOrder(ord);
            fixTmpOrder(ord);
        }
        void lockSuits() { Move::sl = 1; }
        void procPASS() {} //何もしない

        void fixMeld (Move m) {
            Move::s = m.s;
            Move::r = m.r;
            Move::jks = m.jks;
            Move::jkr = m.jkr;
            Move::q = m.q;
            Move::t = m.t;
        }
        
        void proc(Move m) { // プレーヤー等は関係なく局面のみ進める
            if (m.isPASS()) procPASS();
            else {
                if (m.domInevitably() || domConditionally(m)) { // 無条件完全支配
                    if (m.isRev()) flipPrmOrder();
                    flush();
                } else {
                    procOrder(m);
                    if (locksSuits(m)) lockSuits();
                    fixMeld(m);
                }
            }
        }
        
        void procAndFlush(Move m) {
            // 局面を更新し、強引に場を流す
            if (m.isRev()) flipPrmOrder();
            flush();
        }
        
        void procExceptFlush(Move m) {
            // 局面を更新するが場を流さない
            procOrder(m);
            // スートロック
            if (locksSuits(m)) lockSuits();
            fixMeld(m);
            if (m.domInevitably() || domConditionally(m)) {
                invalid = 1;
            }
        }
    };
    
    Board OrderToNullBoard(int o) {
        Board b;
        b.init();
        b.o = o; b.po = o;
        return b;
    }
    
    static std::ostream& operator <<(std::ostream& out, const Board& b) { // Board出力
        if (b.isNull()) out << "NULL";
        else out << b.move();
        // オーダー...一時オーダーのみ
        out << "  Order : ";
        if (b.order() == 0) out << "NORMAL";
        else out << "REVERSED";
        out << "  Suits : ";
        if (b.suitsLocked()) out << "LOCKED";
        else out << "FREE";
        return out;
    }
    
    inline bool isSubjectivelyValid(Board b, Move mv, const Cards& c, const uint32_t q) {
        // 不完全情報の上での合法性判定
        // c はそのプレーヤーが所持可能なカード
        // q はそのプレーヤーの手札枚数（公開されている情報）
        if (mv.isPASS()) return true;
        // 枚数オーバー
        if (mv.qty() > q) return false;
        // 持っていないはずの札を使った場合
        if (!holdsCards(c, mv.cards())) return false;
        if (!b.isNull()) {
            if (b.type() != mv.type()) return false; // 型違い
            if (b.isSeq()) {
                if (!isValidSeqRank(mv.rank(), b.order(), b.rank(), mv.qty())) {
                    return false;
                }
                if (b.suitsLocked()) {
                    if (b.suits() != mv.suits()) return false;
                }
            } else {
                if (b.isSingleJOKER()) {
                    return mv.isS3();
                }
                if (mv.isSingleJOKER()) {
                    if (!b.isSingle()) return false;
                } else {
                    if (!isValidGroupRank(mv.rank(), b.order(), b.rank())) {
                        return false;
                    }
                    if (b.suitsLocked()) {
                        if (b.suits() != mv.suits()) return false;
                    }
                }
            }
        }
        return true;
    }
}