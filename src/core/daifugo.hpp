#pragma once

#include <iostream>
#include <array>
#include <map>

namespace UECda {
    
    // 大富豪における最も基本的な型
    // オーダー int Order
    // ランク int or uint32_t Rank
    // スート uint32_t Suits
    // 整数カード型 int IntCard
    // カード集合型 uint64_t BitCards, Cards
    // 単着手情報型 struct(uint32_t) Move
    // 着手型と対応した場型 struct(uint32_t) Board
    
    /**************************オーダー定数**************************/
    
    // この定数を使わずに 0, 1, 2 表記する事も多いので注意
    
    using Order = int;
    
    enum {
        ORDER_NORMAL = 0, ORDER_REVERSED = 1,
        ORDER_BOTH = 2 // 永続的性質演算で使う
    };
    
    constexpr int flipOrder(Order ord) { return ORDER_NORMAL + ORDER_REVERSED - ord; }
    
    /**************************ランク**************************/
    
    // Uが3の1つ下、Oが2の1つ上
    // rank -> cards 計算等が絡む場合にはRankではなくRank4xを使う方が数命令減らせるかもしれない
    
    using Rank = int;
    
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
    
    constexpr bool isEightSeqRank(int rank, int qty) { // 階段が８切りかどうか
        return (rank <= RANK_8) && (RANK_8 < rank + qty);
    }
    
    constexpr inline int flipRank(int r) {
        // オーダー逆転時の仮ランクを使用する場合
        return RANK_3 + RANK_2 - r;
    }
    
    // 出力
    const std::string rankChar  = "-3456789TJQKA2+";
    const std::string rankCharM = "-3456789tjqka2+";
    
    struct OutRank {
        int r;
        constexpr OutRank(const int& arg) :r(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutRank& arg) {
        out << rankChar[arg.r];
        return out;
    }
    
    struct OutRankM {
        int r;
        constexpr OutRankM(const int& arg) :r(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutRankM& arg) {
        out << rankCharM[arg.r];
        return out;
    }

    struct RankRange { // 連続ランク
        int r0, r1;
        constexpr RankRange(const int& arg0, const int& arg1) :r0(arg0), r1(arg1) {}
    };
    std::ostream& operator <<(std::ostream& out, const RankRange& arg) {
        for (int r = arg.r0; r <= arg.r1; ++r) {
            out << rankChar[r];
        }
        return out;
    }
    
    struct RankRangeM { // 連続ランク
        int r0, r1;
        constexpr RankRangeM(const int& arg0, const int& arg1) :r0(arg0), r1(arg1) {}
    };
    std::ostream& operator<<(std::ostream& out, const RankRangeM& arg) {
        for (int r = arg.r0; r <= arg.r1; ++r) {
            out << rankCharM[r];
        }
        return out;
    }
    
    int CharToRank(char c) {
        int r = rankChar.find(c);
        if (r == std::string::npos) { return RANK_NONE; }
        return r;
    }
    int CharToRankM(char c) {
        int r = rankCharM.find(c);
        if (r == std::string::npos) { return RANK_NONE; }
        return r;
    }
    
    /**************************ランク4倍型**************************/
    
    // ランクの4倍
    using Rank4x = int;
    
    enum {
        RANK4X_U = RANK_U * 4,
        RANK4X_3 = RANK_3 * 4, RANK4X_4 = RANK_4 * 4,
        RANK4X_5 = RANK_5 * 4, RANK4X_6 = RANK_6 * 4,
        RANK4X_7 = RANK_7 * 4, RANK4X_8 = RANK_8 * 4,
        RANK4X_9 = RANK_9 * 4, RANK4X_T = RANK_T * 4,
        RANK4X_J = RANK_J * 4, RANK4X_Q = RANK_Q * 4,
        RANK4X_K = RANK_K * 4, RANK4X_A = RANK_A * 4,
        RANK4X_2 = RANK_2 * 4,
        RANK4X_O = RANK_O * 4,
        // 実際に使用される通常カードのランク限度
        RANK4X_MIN = RANK_MIN * 4,
        RANK4X_MAX = RANK_MAX * 4,
        // 階段作成の際の仮想上のランク限度
        RANK4X_IMG_MIN = RANK_IMG_MIN * 4,
        RANK4X_IMG_MAX = RANK_IMG_MAX * 4,
        
        RANK4X_NONE = -1
    };
    
    constexpr int RankToRank4x(int r) { return r << 2; }
    constexpr int Rank4xToRank(int r4x) { return r4x >> 2; }
    
    /**************************スート番号**************************/
    
    using SuitNum = int;
    
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
    std::ostream& operator <<(std::ostream& out, const OutSuitNum& arg) {
        out << suitNumChar[arg.sn];
        return out;
    }
    
    struct OutSuitNumM {
        int sn;
        constexpr OutSuitNumM(const int& arg) :sn(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuitNumM& arg) {
        out << suitNumCharM[arg.sn];
        return out;
    }
    
    int CharToSuitNum(char c) {
        int sn = suitNumChar.find(c);
        if (sn == std::string::npos) { return SUITNUM_NONE; }
        return sn;
    }
    int CharToSuitNumM(char c) {
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
    
    int countSuits(uint32_t s) {
        return countBits(s);
    }
    
    // スートインデックス
    // スートビットから、その種類の役の中で何番目のスートパターンとされているかを得る
    constexpr int suitsIdx[18] = {
        -1, 0, 1, 0, 2, 1, 3, 0, 3, 2, 4, 1, 5, 2, 3, 0, 0, 5
    };
    
    int SuitToSuitNum(uint32_t suit) {
        return bsf32(suit);
    }
    
    // 単スート番号からスート集合への変換
    constexpr uint32_t SuitNumToSuits(int sn0) {
        return (1U << sn0);
    }
    int SuitsToSuitNum(int suit) {
        // 複スートの場合も
        return suitsIdx[suit];
    }
    
    // 出力
    struct OutSuits {
        uint32_t s;
        constexpr OutSuits(const uint32_t& arg) :s(arg) {}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuits& arg) { // 出力の時だけ第５のスートは16として対応している
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
    std::ostream& operator <<(std::ostream& out, const OutSuitsM& arg) { // 出力の時だけ第5のスートは16として対応している
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
    
    int getSuitSuitsIndex(uint32_t s0, uint32_t s1) {
        return suitSuitsIndexTable[s0][s1];
    }
    int getSuitsSuitsIndex(uint32_t s0, uint32_t s1) {
        return suitsSuitsIndexTable[s0][s1];
    }
    int get2SuitsIndex(uint32_t s0, uint32_t s1) {
        return twoSuitsIndexTable[s0][s1];
    }
    int getSuitsSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2) {
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    int getSuitSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2) {
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    
    void initSuits() {
        
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
                const uint32_t c0 = countBits(s0), c1 = countBits(s1);
                const uint32_t cmin = min(c0, c1), cmax = max(c0, c1);
                const uint32_t c01 = countBits(s01);

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
                const uint32_t c1 = countBits(s1);
                const uint32_t c01 = countBits(s01);
                
                suitSuitsIndexTable[s0].assign(s1, suitSuitsCountIndex[c1][c01]);
            }
        }
        
        // (suits, suits, suits) pattern index
        int suitsSuitsSuitsCountIndex[5][5][5][5][5][5][5] = {0};
        std::map<std::array<uint32_t, ipow(2, 3) - 1>, int> sssMap;
        for (uint32_t s0 = 0; s0 < 16; ++s0) {
            for (uint32_t s1 = 0; s1 < 16; ++s1) {
                for (uint32_t s2 = 0; s2 < 16; ++s2) {
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c0 = countBits(s0), c1 = countBits(s1), c2 = countBits(s2);
                    const uint32_t c01 = countBits(s01), c02 = countBits(s02), c12 = countBits(s12);
                    const uint32_t c012 = countBits(s012);
                    std::array<uint32_t, ipow(2, 3) - 1> pattern = {c0, c1, c2, c01, c02, c12, c012};
                    int cnt;
                    if (sssMap.count(pattern) == 0) {
                        cnt = sssMap.size();
                        sssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << ", " << c012 << endl;
                    } else {
                        cnt = sssMap[pattern];
                    }
                    suitsSuitsSuitsCountIndex[c0][c1][c2][c01][c02][c12][c012] = cnt;
                    suitsSuitsSuitsIndexTable[s0][s1][s2] = cnt;
                }
            }
        }
        ASSERT(sssMap.size() == N_PATTERNS_SUITS_SUITS_SUITS,
               cerr << sssMap.size() << " <-> " << N_PATTERNS_SUITS_SUITS_SUITS << endl;);
        
        int suitSuitsSuitsCountIndex[5][5][5][5][5]= {0};
        std::map<std::array<uint32_t, ipow(2, 3) - 3>, int> s1ssMap;
        for (uint32_t s0 = 1; s0 < 16; s0 <<= 1) {
            for (uint32_t s1 = 0; s1 < 16; ++s1) {
                for (uint32_t s2 = 0; s2 < 16; ++s2) {
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c1 = countBits(s1), c2 = countBits(s2);
                    const uint32_t c01 = countBits(s01), c02 = countBits(s02), c12 = countBits(s12);
                    std::array<uint32_t, ipow(2, 3) - 3> pattern = {c1, c2, c01, c02, c12};
                    int cnt;
                    if (s1ssMap.count(pattern) == 0) {
                        cnt = s1ssMap.size();
                        s1ssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << endl;
                    } else {
                        cnt = s1ssMap[pattern];
                    }
                    suitSuitsSuitsCountIndex[c1][c2][c01][c02][c12] = cnt;
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
}

// あまり模範的ではないが、ここで他の基本的な型もインクルードしておく
#include "card.hpp"
#include "move.hpp"
#include "board.hpp"