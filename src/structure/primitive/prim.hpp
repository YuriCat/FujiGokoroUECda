/*
 prim.hpp
 Katsuki Ohto
 */
#pragma once

#include <iostream>
#include <array>
#include <map>

namespace UECda{
    
    // プリミティブな型
    
    // 各タイプに一つの構造を与える
    // 各classは暗黙にこれらの型が使用されると仮定してメンバ関数を作ってよい
    
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
    
    enum{
        ORDER_NORMAL = 0, ORDER_REVERSED = 1,
        ORDER_BOTH = 2// 永続的性質演算で使う
    };
    
    constexpr int flipOrder(Order ord)noexcept{ return ORDER_NORMAL + ORDER_REVERSED - ord; }
    
    /**************************ランク**************************/
    
    // Uが3の1つ下、Oが2の1つ上
    // rank -> cards 計算等が絡む場合にはRankではなくRank4xを使う方が数命令減らせるかもしれない
    
    using Rank = int;
    
    enum{
        RANK_U,
        RANK_3, RANK_4, RANK_5, RANK_6,
        RANK_7, RANK_8, RANK_9, RANK_T,
        RANK_J, RANK_Q, RANK_K,
        RANK_A, RANK_2,
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
    
    constexpr bool isEightSeqRank(int rank, int qty)noexcept{ // 階段が８切りかどうか
        return (rank <= RANK_8) && (RANK_8 < rank + qty);
    }
    
    constexpr inline int flipRank(int r)noexcept{
        // オーダー逆転時の仮ランクを使用する場合
        return RANK_3 + RANK_2 - r;
    }
    
    // 出力
    const std::string rankChar  = "-3456789TJQKA2+";
    const std::string rankCharM = "-3456789tjqka2+";
    
    struct OutRank{
        int r;
        constexpr OutRank(const int& arg) :r(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutRank& arg){
        out << rankChar[arg.r];
        return out;
    }
    
    struct OutRankM{
        int r;
        constexpr OutRankM(const int& arg) :r(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutRankM& arg){
        out << rankCharM[arg.r];
        return out;
    }

    struct RankRange{ // 連続ランク
        int r0, r1;
        constexpr RankRange(const int& arg0, const int& arg1) :r0(arg0), r1(arg1){}
    };
    std::ostream& operator <<(std::ostream& out, const RankRange& arg){
        for (int r = arg.r0; r <= arg.r1; ++r){
            out << rankChar[r];
        }
        return out;
    }
    
    struct RankRangeM{ // 連続ランク
        int r0, r1;
        constexpr RankRangeM(const int& arg0, const int& arg1) :r0(arg0), r1(arg1){}
    };
    std::ostream& operator<<(std::ostream& out, const RankRangeM& arg){
        for (int r = arg.r0; r <= arg.r1; ++r){
            out << rankCharM[r];
        }
        return out;
    }
    
    int CharToRank(char c)noexcept{
        int r = rankChar.find(c);
        if(r == std::string::npos){ return RANK_NONE; }
        return r;
    }
    int CharToRankM(char c)noexcept{
        int r = rankCharM.find(c);
        if(r == std::string::npos){ return RANK_NONE; }
        return r;
    }
    
    /**************************ランク4倍型**************************/
    
    // ランクの4倍
    using Rank4x = int;
    
    enum{
        RANK4X_U = RANK_U * 4,
        RANK4X_3 = RANK_3 * 4,
        RANK4X_4 = RANK_4 * 4,
        RANK4X_5 = RANK_5 * 4,
        RANK4X_6 = RANK_6 * 4,
        RANK4X_7 = RANK_7 * 4,
        RANK4X_8 = RANK_8 * 4,
        RANK4X_9 = RANK_9 * 4,
        RANK4X_T = RANK_T * 4,
        RANK4X_J = RANK_J * 4,
        RANK4X_Q = RANK_Q * 4,
        RANK4X_K = RANK_K * 4,
        RANK4X_A = RANK_A * 4,
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
    
    constexpr int RankToRank4x(int r)noexcept{ return r << 2; }
    constexpr int Rank4xToRank(int r4x)noexcept{ return r4x >> 2; }
    
    /**************************スート番号**************************/
    
    using SuitNum = int;
    
    // 単スート
    enum{
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
        
    struct OutSuitNum{
        int sn;
        constexpr OutSuitNum(const int& arg) :sn(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuitNum& arg){
        out << suitNumChar[arg.sn];
        return out;
    }
    
    struct OutSuitNumM{
        int sn;
        constexpr OutSuitNumM(const int& arg) :sn(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuitNumM& arg){
        out << suitNumCharM[arg.sn];
        return out;
    }
    
    int CharToSuitNum(char c)noexcept{
        int sn = suitNumChar.find(c);
        if(sn == std::string::npos){ return SUITNUM_NONE; }
        return sn;
    }
    int CharToSuitNumM(char c)noexcept{
        int sn = suitNumCharM.find(c);
        if(sn == std::string::npos){ return SUITNUM_NONE; }
        return sn;
    }
    
    /**************************スート**************************/
    
    using Suits = unsigned int;
    
    // 単スート
    enum{
        SUIT_NULL = 0,
        SUIT_C = 1, SUIT_D = 2, SUIT_H = 4, SUIT_S = 8,
        SUIT_X = 16,
        SUIT_MIN = SUIT_C,
        SUIT_MAX = SUIT_S
    };
    
    // スート集合 (スートの和集合)
    enum{
        SUITS_NULL, SUITS_C,   SUITS_D,   SUITS_CD,
        SUITS_H,    SUITS_CH,  SUITS_DH,  SUITS_CDH,
        SUITS_S,    SUITS_CS,  SUITS_DS,  SUITS_CDS,
        SUITS_HS,   SUITS_CHS, SUITS_DHS, SUITS_CDHS,
        SUITS_CDHSX = SUITS_CDHS | SUIT_X, // クインタプル
        SUITS_ALL = SUITS_CDHS,
    };
    
    int countSuits(uint32_t s)noexcept{
        return countBits(s);
    }
    
    // スートインデックス
    // スートビットから、その種類の役の中で何番目のスートパターンとされているかを得る
    constexpr int suitsIdx[18] = { -1, 0, 1, 0, 2, 1, 3, 0, 3, 2, 4, 1, 5, 2, 3, 0, 0, 5 };
    
    //const char suitstr[5]={'C','D','H','S','X'};
    
    int SuitToSuitNum(uint32_t suit)noexcept{
        return bsf32(suit);
    }
    
    // 単スート番号からスート集合への変換
    constexpr uint32_t SuitNumToSuits(int sn0)noexcept{
        return (1U << sn0);
    }
    int SuitsToSuitNum(int suit){
        // 複スートの場合も
        return suitsIdx[suit];
    }
    
    // 出力
    struct OutSuits{
        uint32_t s;
        constexpr OutSuits(const uint32_t& arg) :s(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuits& arg){ // 出力の時だけ第５のスートは16として対応している
        if(arg.s & SUIT_C){ out << 'C'; }
        if(arg.s & SUIT_D){ out << 'D'; }
        if(arg.s & SUIT_H){ out << 'H'; }
        if(arg.s & SUIT_S){ out << 'S'; }
        if(arg.s & SUIT_X){ out << 'X'; }
        return out;
    }

    struct OutSuitsM{
        uint32_t s;
        constexpr OutSuitsM(const uint32_t& arg) :s(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutSuitsM& arg){ // 出力の時だけ第5のスートは16として対応している
        if(arg.s & SUIT_C){ out << 'c'; }
        if(arg.s & SUIT_D){ out << 'd'; }
        if(arg.s & SUIT_H){ out << 'h'; }
        if(arg.s & SUIT_S){ out << 's'; }
        if(arg.s & SUIT_X){ out << 'x'; }
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
    
    int getSuitSuitsIndex(uint32_t s0, uint32_t s1){
        return suitSuitsIndexTable[s0][s1];
    }
    int getSuitsSuitsIndex(uint32_t s0, uint32_t s1){
        return suitsSuitsIndexTable[s0][s1];
    }
    int get2SuitsIndex(uint32_t s0, uint32_t s1){
        return twoSuitsIndexTable[s0][s1];
    }
    int getSuitsSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2){
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    int getSuitSuitsSuitsIndex(uint32_t s0, uint32_t s1, uint32_t s2){
        return suitsSuitsSuitsIndexTable[s0][s1][s2];
    }
    
    void initSuits(){
        
        // (suits suits) pattern index (exchangable) 0 ~ 21
        int twoSuitsCountIndex[5][5][5] = {0};
        int cnt = 0;
        for(int c0 = 0; c0 <= 4; ++c0){
            for(int c1 = 0; c1 <= c0; ++c1){
                for(int c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); ++c01){
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
        for(int c0 = 0; c0 <= 4; ++c0){
            for(int c1 = 0; c1 <= 4; ++c1){
                for(size_t c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); ++c01){
                    DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c01 << endl;
                    suitsSuitsCountIndex[c0][c1][c01] = cnt;
                    ++cnt;
                }
            }
        }
        ASSERT(cnt == N_PATTERNS_SUITS_SUITS,
               cerr << cnt << " <-> " << N_PATTERNS_SUITS_SUITS << endl;);
        
        for(uint32_t s0 = 0; s0 < 16; ++s0){
            for(uint32_t s1 = 0; s1 < 16; ++s1){
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
        for(int c1 = 0; c1 <= 4; ++c1){
            for(int c01 = max(0, 1 + c1 - 4); c01 <= min(c1, 1); ++c01){
                assert(c01 == 0 || c01 == 1);
                DERR << "pattern " << cnt << " = " << c1 << ", " << c01 << endl;
                suitSuitsCountIndex[c1][c01] = cnt;
                ++cnt;
            }
        }
        ASSERT(cnt == N_PATTERNS_SUIT_SUITS, cerr << cnt << " <-> " << N_PATTERNS_SUIT_SUITS << endl;);
        
        for(int sn0 = 0; sn0 < 4; ++sn0){
            for(uint32_t s1 = 0; s1 < 16; ++s1){
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
        for(uint32_t s0 = 0; s0 < 16; ++s0){
            for(uint32_t s1 = 0; s1 < 16; ++s1){
                for(uint32_t s2 = 0; s2 < 16; ++s2){
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c0 = countBits(s0), c1 = countBits(s1), c2 = countBits(s2);
                    const uint32_t c01 = countBits(s01), c02 = countBits(s02), c12 = countBits(s12);
                    const uint32_t c012 = countBits(s012);
                    std::array<uint32_t, ipow(2, 3) - 1> pattern = {c0, c1, c2, c01, c02, c12, c012};
                    int cnt;
                    if(sssMap.count(pattern) == 0){
                        cnt = sssMap.size();
                        sssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << ", " << c012 << endl;
                    }else{
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
        for(uint32_t s0 = 1; s0 < 16; s0 <<= 1){
            for(uint32_t s1 = 0; s1 < 16; ++s1){
                for(uint32_t s2 = 0; s2 < 16; ++s2){
                    const uint32_t s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                    const uint32_t s012 = s0 & s1 & s2;
                    const uint32_t c1 = countBits(s1), c2 = countBits(s2);
                    const uint32_t c01 = countBits(s01), c02 = countBits(s02), c12 = countBits(s12);
                    std::array<uint32_t, ipow(2, 3) - 3> pattern = {c1, c2, c01, c02, c12};
                    int cnt;
                    if(s1ssMap.count(pattern) == 0){
                        cnt = s1ssMap.size();
                        s1ssMap[pattern] = cnt;
                        DERR << "pattern " << cnt << " = " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << endl;
                    }else{
                        cnt = s1ssMap[pattern];
                    }
                    suitSuitsSuitsCountIndex[c1][c2][c01][c02][c12] = cnt;
                    suitSuitsSuitsIndexTable[s0][s1][s2] = cnt;
                }
            }
        }
    }
    
    struct SuitsInitializer{
        SuitsInitializer(){
            initSuits();
        }
    };
    
    SuitsInitializer suitsInitializer;

    /**************************カード整数**************************/
    
    // U3456789TJQKA2O、CDHSの順番で0-59　ジョーカーは60
    
    // 定数
    using IntCard = int;
    
    enum{
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
    
    constexpr bool examIntCard(IntCard ic)noexcept{
        return (INTCARD_C3 <= ic && ic <= INTCARD_S2) || (ic == INTCARD_JOKER);
    }
    constexpr bool examImaginaryIntCard(IntCard ic)noexcept{
        return INTCARD_CU <= ic && ic <= INTCARD_JOKER;
    }
    
    template<typename rank_t, typename suit_t>
    constexpr IntCard RankSuitsToIntCard(rank_t r, suit_t s)noexcept{
        return (r << 2) + SuitToSuitNum(s);
    }
    
    template<typename r4x_t, typename suit_t>
    constexpr IntCard Rank4xSuitsToIntCard(r4x_t r4x, suit_t s)noexcept{
        return r4x + SuitToSuitNum(s);
    }
    
    template<typename rank_t>
    constexpr IntCard RankSuitNumToIntCard(rank_t r, int sn)noexcept{
        return (r << 2) + sn;
    }
    
    constexpr int IntCardToRank(IntCard ic)noexcept{ return ic >> 2; }
    constexpr int IntCardToRank4x(IntCard ic)noexcept{ return ic & ~3U; }
    constexpr int IntCardToSuitNum(IntCard ic)noexcept{ return ic & 3U; }
    constexpr int IntCardToSuits(IntCard ic)noexcept{ return SuitNumToSuits(IntCardToSuitNum(ic)); }
    
    // 出力用クラス
    struct OutIntCard{
        IntCard ic;
        constexpr OutIntCard(const IntCard& arg) :ic(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutIntCard& arg){
        if(arg.ic == INTCARD_JOKER){
            out << "JO";
        }else{
            out << OutSuitNum(IntCardToSuitNum(arg.ic)) << OutRank(IntCardToRank(arg.ic));
        }
        return out;
    }

    struct OutIntCardM{
        IntCard ic;
        constexpr OutIntCardM(const IntCard& arg) :ic(arg){}
    };
    std::ostream& operator <<(std::ostream& out, const OutIntCardM& arg){
        if(arg.ic == INTCARD_JOKER){
            out << "jo";
        }else{
            out << OutSuitNumM(IntCardToSuitNum(arg.ic)) << OutRankM(IntCardToRank(arg.ic));
        }
        return out;
    }
    
    IntCard StringToIntCardM(const std::string& str){
        if(str.size() != 2){ return INTCARD_NONE; }
        if(str == "jo"){ return INTCARD_JOKER; }
        int sn = CharToSuitNumM(str[0]);
        int r = CharToRankM(str[1]);
        if(r == RANK_NONE){ return INTCARD_NONE; }
        if(sn == SUITNUM_NONE){ return INTCARD_NONE; }
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
    constexpr BitCards IntCardToCards(IntCard ic)noexcept{
        return BitCards(CARDS_HORIZON << ic);
    }
    
    IntCard CardsToLowestIntCard(BitCards c)noexcept{ return bsf64(c); } // 一番低いもの
    IntCard CardsToHighestIntCard(BitCards c)noexcept{ return bsr64(c); } // 一番高いもの
    
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
    constexpr Cards RankToCards(uint32_t r)noexcept{
        // あるランクのカード全て
        return CARDS_HORIZONRANK << (r << 2);
    }
    constexpr Cards RankRangeToCards(uint32_t r0, uint32_t r1)noexcept{
        // ランク間（両端含む）のカード全て
        // r0 <= r1 でない場合は CARDS_NULL
        return ~((CARDS_HORIZON << (r0 << 2)) - 1ULL)
        & ((CARDS_HORIZON << ((r1 + 1) << 2)) - 1ULL);
    }
    
    constexpr Cards Rank4xToCards(uint32_t r4x)noexcept{
        // あるランク4xのカード全て
        return CARDS_HORIZONRANK << r4x;
    }
    
    constexpr Cards RankRange4xToCards(uint32_t r4x_0, uint32_t r4x_1)noexcept{
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
    constexpr BitCards SuitsToCards(uint32_t s)noexcept{
        return CARDS_HORIZONSUIT * s; // あるスートのカード全て
    }
    
    // ランク x スート
    
    // ランクとスートの指定からカード集合を生成する
    // スートは集合として用いる事が出来る
    constexpr BitCards RankSuitsToCards(int r, uint32_t s)noexcept{
        return (BitCards)s << (r << 2);
    }
    constexpr BitCards Rank4xSuitsToCards(int r4x, uint32_t s)noexcept{
        return (BitCards)s << r4x;
    }
    
    // ランク両端とスートの指定
    BitCards RRSToCards(uint32_t r0, uint32_t r1, uint32_t suits)noexcept{
        return RankRangeToCards(r0, r1) & SuitsToCards(suits);
    }
    
    BitCards RR4SToCards(uint32_t r4x_0, uint32_t r4x_1, uint32_t suits)noexcept{
        return RankRange4xToCards(r4x_0, r4x_1) & SuitsToCards(suits);
    }
    
    constexpr uint32_t CardsRankToSuits(BitCards c, int r)noexcept{
        return uint32_t(c >> (r << 2)) & 15U;
    }
    constexpr uint32_t CardsRank4xToSuits(BitCards c, int r4x)noexcept{
        return uint32_t(c >> r4x) & 15U;
    }
    
    // 関係ないビットを除外したり、実在するカードのみに限定したり
    constexpr BitCards disarmCards(BitCards c)noexcept{ return c & CARDS_ALL; }
    void disarmCards(BitCards *const c){ (*c) &= CARDS_ALL; }
    
    // Cards型基本演算
    
    // 追加
    constexpr BitCards addCards(BitCards c0, BitCards c1)noexcept{ return c0 | c1; }
    template<typename ... args_t>
    constexpr BitCards addCards(BitCards c0, BitCards c1, args_t ... others)noexcept{
        return c0 | addCards(c1, others...);
    }

    constexpr BitCards addIntCard(BitCards c, IntCard ic)noexcept{ return addCards(c, IntCardToCards(ic)); }
    constexpr BitCards addJOKER(BitCards c)noexcept{ return addCards(c, CARDS_JOKER); }
    
    void addCards(BitCards *const c0, BitCards c1){ (*c0) |= c1; }
    void addJOKER(BitCards *const cptr){ (*cptr) |= CARDS_JOKER; }
    
    void addIntCard(BitCards *const pc, IntCard ic){ addCards(pc, IntCardToCards(ic)); }
    
    // 限定
    constexpr BitCards commonCards(BitCards c0, BitCards c1)noexcept{ return c0 & c1; }
    constexpr BitCards andCards(BitCards c0, BitCards c1)noexcept{ return c0 & c1; }
    void andCards(BitCards *const cptr, BitCards c){ (*cptr) &= c; }
    
    // 削除（オーバーを引き起こさない）
    // maskは指定以外とのandをとることとしている。
    // 指定部分とのandはand処理で行う
    constexpr BitCards maskCards(BitCards c0, BitCards c1)noexcept{ return c0 & ~c1; }
    void maskCards(BitCards *const c0, BitCards c1){ (*c0) &= ~c1; }
    constexpr BitCards maskJOKER(Cards c)noexcept{ return maskCards(c, CARDS_JOKER); }
    void maskJOKER(BitCards *const cptr){ maskCards(cptr, CARDS_JOKER); }
    
    // 状態逆転（追加や削除のため用いた場合はオーバー処理で不具合となる危険性あり）
    constexpr BitCards invCards(BitCards c0, BitCards c1)noexcept{ return c0 ^ c1; }
    void invCards(BitCards *const c0, BitCards c1){ (*c0) ^= c1; }
    constexpr BitCards invJOKER(BitCards c0)noexcept{ return invCards(c0, CARDS_JOKER); }
    void invJOKER(BitCards *const cptr){ invCards(cptr, CARDS_JOKER); }
    
    // カード減算
    // 全ての（安定でなくても良い）カード減算処理から最も高速なものとして定義したいところだが、
    // 現在では整数としての引き算処理。
    constexpr BitCards subtrCards(BitCards c0, BitCards c1)noexcept{ return c0 - c1; }
    void subtrCards(Cards *const cptr, BitCards c){ (*cptr) -= c; }
    constexpr BitCards subtrJOKER(BitCards c)noexcept{ return subtrCards(c, CARDS_JOKER); }
    void subtrJOKER(BitCards *const cptr){ subtrCards(cptr, CARDS_JOKER); }
    
    // 要素数
    constexpr uint32_t countFewCards(BitCards c)noexcept{ return countFewBits64(c); } // 要素が比較的少ない時の速度優先
    uint32_t countManyCards(BitCards c)noexcept{ return countBits64(c); } // 要素が比較的多い時の速度優先
    uint32_t countCards(BitCards c)noexcept{ return countBits64(c); } // 基本のカウント処理
    constexpr BitCards any2Cards(BitCards c)noexcept{ return c & (c - 1ULL); }
    
    // Cards型基本判定
    
    // 要素の部分一致
    constexpr BitCards hasSameCards(BitCards c0, BitCards c1)noexcept{ return commonCards(c0, c1); }
    constexpr bool isExclusiveCards(BitCards c0, BitCards c1)noexcept{ return !(c0 & c1); }
    
    // 包括関係
    constexpr BitCards containsCard(BitCards c0, BitCard c1)noexcept{ return andCards(c0, c1); } // 単体に対してはandでok
    constexpr BitCards containsIntCard(BitCards c, IntCard ic)noexcept{ return containsCard(c, IntCardToCards(ic)); }
    constexpr BitCards containsJOKER(BitCards c)noexcept{ return andCards(c, CARDS_JOKER); }
    constexpr BitCards containsS3(BitCards c)noexcept{ return andCards(c, CARDS_S3); }
    constexpr BitCards containsD3(BitCards c)noexcept{ return andCards(c, CARDS_D3); }
    constexpr BitCards contains8(BitCards c)noexcept{ return andCards(c, CARDS_8); }

    constexpr bool holdsCards(BitCards c0, BitCards c1)noexcept{ return holdsBits(c0, c1); }
    
    // 空判定
    constexpr BitCards anyCards(BitCards c)noexcept{ return c; }
    
    // validation
    constexpr bool examCards(BitCards c)noexcept{ return holdsCards(CARDS_ALL, c); }
    constexpr bool examImaginaryCards(BitCards c)noexcept{ return holdsCards(CARDS_IMG_ALL, c); }
    
    // Cards型生成
    BitCards StringToCards(const std::string& str){
        BitCards c = CARDS_NULL;
        std::vector<std::string> v = split(str, ' ');
        for(const auto& s : v){
            IntCard ic = StringToIntCardM(s);
            if(ic != INTCARD_NONE){
                addIntCard(&c, ic);
            }
        }
        return c;
    }
    // Cards型特殊演算
    
    // 特定順序の要素を選ぶ（元のデータは変えない）
    inline Cards pickLow(const Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        return lowestNBits(c, n);
    }
    template<int N = 1>
    inline Cards pickLow(const Cards c)noexcept{
        assert(N > 0);
        assert((int)countCards(c) >= N);
        return pickLow(c, N);
    }
    template<>inline constexpr Cards pickLow<1>(const Cards c)noexcept{
        return lowestBit(c);
    }
    template<>inline Cards pickLow<2>(const Cards c)noexcept{
        Cards res = lowestBit(c);
        return res | lowestBit(c - res);
    }
    
    inline Cards pickHigh(const Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        return highestNBits(c, n);
    }
    template<int N = 1>
    inline Cards pickHigh(const Cards c)noexcept{
        assert(N > 0);
        assert((int)countCards(c) >= N);
        return pickHigh(c, N);
    }
    template<>inline Cards pickHigh<1>(const Cards c)noexcept{
        return highestBit(c);
    }
    template<>inline Cards pickHigh<2>(const Cards c)noexcept{
        Cards r = highestBit(c);
        return r | highestBit(c - r);
    }
    
    // 特定順序の要素の取り出し(元のデータから引く)
    inline Cards popHigh(Cards *const c)noexcept{
        assert(*c);
        Cards r = 1ULL << bsr64(*c);
        (*c) -= r;
        return r;
    }
    
    inline Cards popHigh2(Cards *const c)noexcept{
        assert(countCards(*c) >= 2U);
        Cards r1 = 1ULL << bsr64(*c);
        (*c) -= r1;
        Cards r2 = 1ULL << bsr64(*c);
        (*c) -= r2;
        return r1 | r2;
    }
    
    inline Cards popLow(Cards *const c)noexcept{
        assert(anyCards(*c));
        Cards r = (*c)&(-(*c));
        subtrCards(c, r);
        return r;
    }
    
    // Cards pop
    
    // IntCard型で1つ取り出し
    inline IntCard pickIntCardLow(const Cards c)noexcept{
        return (IntCard)bsf64(c);
    }
    
    inline IntCard pickIntCardHigh(const Cards c)noexcept{
        ASSERT(anyCards(c),);
        return (IntCard)bsr64(c);
    }
    
    inline IntCard popIntCardLow(Cards *const c)noexcept{
        IntCard ic = pickIntCardLow(*c);
        (*c) &= ((*c) - 1ULL);
        return ic;
    }
    
    inline IntCard popIntCardHigh(Cards *const c)noexcept{
        IntCard ic = pickIntCardHigh(*c);
        subtrCards(c, IntCardToCards(ic));
        return ic;
    }
    
    // 適当に（ランダムではない）１つ取り出し
    // 順序がどうでも良い取り出し操作の中で最速な操作で実装したい
    inline Cards pop(Cards *const c)noexcept{
        Cards r = (*c) & (-(*c));
        (*c) -= r;
        return r;
    }
    
    inline constexpr Cards pick(const Cards c)noexcept{
        return c & (-c);
    }
    
    inline IntCard pickIntCard(const Cards c)noexcept{
        return pickIntCardLow(c);
    }
    
    inline IntCard popIntCard(Cards *const c)noexcept{
        return popIntCardLow(c);
    }
    
    // 完全ランダム取り出し
    // ビット分割関数(bitPartition.hpp)を使う
    template<int N = 1, class dice64_t>
    inline Cards popRand(Cards *const c, dice64_t *const dice){
        static_assert(N >= 0, " popRand N < 0 ");
        Cards res;
        switch (N){
            case 0: res = CARDS_NULL; break;
            case 1: res = pop1Bit64(c, dice); break;
                //case 2:res=pickNBits64(c,2,countCards(c)-2,dice);break;
            default: UNREACHABLE; break;
        }
        return res;
    }
    
    template<int N = 1, class dice64_t>
    inline Cards pickRand(const Cards c, dice64_t *const dice){
        static_assert(N >= 0, " pickRand N < 0 ");
        Cards res;
        switch (N){
            case 0: res = CARDS_NULL; break;
            case 1: res = pick1Bit64(c, dice); break;
            case 2: res = pickNBits64(c, 2, countCards(c) - 2, dice); break;
            default: UNREACHABLE; break;
        }
        return res;
    }
    
    // n番目(nは1から)に高い,低いもの
    inline Cards pickNthHigh(Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return NthHighestBit(c, n);
    }
    
    inline Cards pickNthLow(Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return NthLowestBit(c, n);
    }
    
    // n番目(nは1から)に高い,低いもの以外
    inline Cards maskNthHigh(Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return subtrCards(c, pickNthHigh(c, n));
    }
    
    inline Cards maskNthLow(Cards c, int n)noexcept{
        assert(n > 0);
        assert((int)countCards(c) >= n);
        
        return subtrCards(c, pickNthLow(c, n));
    }
    
    // 基準c1より高い、低い(同じは含まず)もの
    
    // 単体。変なカードも入るが...
    inline Cards pickHigher(Cards c1)noexcept{
        return allHigherBits(c1);
    }
    
    inline Cards pickLower(Cards c1)noexcept{
        return allLowerBits(c1);
    }
    
    inline Cards pickHigher(Cards c0, Cards c1)noexcept{
        return c0 & allHigherBits(c1);
    }
    
    inline Cards pickLower(Cards c0, Cards c1)noexcept{
        return c0 & allLowerBits(c1);
    }
    
    // 基準c1より高い、低い(同じは含まず)もの以外
    inline Cards maskHigher(Cards c0, Cards c1)noexcept{
        return c0 & ~allHigherBits(c1);
    }
    
    inline Cards maskLower(Cards c0, Cards c1)noexcept{
        return c0 & ~allLowerBits(c1);
    }
    
    // 基準としてランクを用いる場合
    inline constexpr BitCards higherMask(Rank r)noexcept{
        return ~((1ULL << ((r + 1) * 4)) - 1ULL);
    }
    inline constexpr BitCards lowerMask(Rank r)noexcept{
        return (1ULL << (r * 4)) - 1ULL;
    }
    inline constexpr BitCards strongerMask(Rank r, Order ord)noexcept{
        return ord == ORDER_NORMAL ? higherMask(r) : lowerMask(r);
    }
    inline constexpr BitCards weakerMask(Rank r, Order ord)noexcept{
        return ord == ORDER_NORMAL ? lowerMask(r) : higherMask(r);
    }

    // ランク重合
    // ランクを１つずつ下げてandを取るのみ(ジョーカーとかその辺のことは何も考えない)
    // 階段判定に役立つ
    template<int N = 3>
    inline Cards polymRanks(const Cards c){
        static_assert(N >= 0, "polymRanks<>()");
        return ((c >> ((N - 1) << 2)) & polymRanks<N - 1>(c));
    }
        
    template<>inline constexpr Cards polymRanks<1>(const Cards c){ return c; }
    template<>inline constexpr Cards polymRanks<0>(const Cards c){ return CARDS_NULL; }
        
    inline Cards polymRanks(Cards c, uint32_t num){ // 重合数が変数の場合
        assert(num > 0);
        --num;
        while (num){
            c = polymRanks<2>(c);
            --num;
        }
        return c;
     }
        
    inline Cards polymRanks_PR(Cards c, uint32_t arg, uint32_t dst){
        // 既にargランク重合が成された状態から、より高次のdstランク重合結果を得る
        assert(arg > 0);
        assert(dst > 0);
        assert(dst - arg + 1U > 0);
        return polymRanks(c, dst - arg + 1U);
    }
        
    inline constexpr Cards polymJump(const Cards c)noexcept{ // 1ランクとばし
        return c & (c >> 8);
    }
        
    // ランク展開
    // ランクを１つずつ上げてorをとるのが基本だが、
    // 4以上の場合は倍々で増やしていった方が少ない命令で済む
    template<int N = 3>
    inline Cards extractRanks(const Cards c)noexcept{
        return ((c << ((N - 1) << 2)) | extractRanks<N - 1>(c));
    }
        
    template<>inline constexpr Cards extractRanks<0>(const Cards c)noexcept{ return CARDS_NULL; }
    template<>inline constexpr Cards extractRanks<1>(const Cards c)noexcept{ return c; }
    template<>inline constexpr Cards extractRanks<2>(const Cards c)noexcept{ return c | (c << 4); }
    template<>inline constexpr Cards extractRanks<3>(const Cards c)noexcept{ return c | (c << 4) | (c << 8); }
    template<>inline Cards extractRanks<4>(const Cards c)noexcept{
        Cards r = c | (c << 4);
        return r | (r << 8);
    }
    template<>inline Cards extractRanks<5>(const Cards c)noexcept{
        Cards r = c | (c << 4);
        return r | (c << 8) | (r << 12);
    }
        
    Cards extractRanks(Cards c, uint32_t num)noexcept{ // 展開数が変数の場合
        assert(num > 0);
        for (int n = num - 1U; n; --n){
            c = extractRanks<2>(c);
        }
        return c;
    }
    
    Cards polymRanksWithJOKER(const Cards c, int qty)noexcept{
        Cards r;
        switch(qty){
            case 0: r = CARDS_NULL; break;
            case 1: r = CARDS_ALL; break;
            case 2: r = c; break;
            case 3:{
                Cards d = c & (c >> 4);
                r = (d | (d >> 4)) | (c & (c >> 8));
            }break;
            case 4:{
                
                Cards d = c & (c >> 4);
                Cards f = (d & (c >> 12)) | (c & (d >> 8)); // 1 + 2パターン
                Cards e = d & (c >> 8); // 3連パターン
                if(e){ f |= e | (e >> 4); }
                r = f;
            }break;
            case 5:{
                Cards d = c & (c >> 4);
                Cards g = d | (d >> 12); // 2 + 2パターン
                Cards e = d & (c >> 8); // 3連
                if(e){
                    g |= (e & (c >> 16)) | (c & (e >> 8)); // 1 + 3パターン
                    Cards f = e & (c >> 12);
                    if(f){
                        g |= (f | (f >> 4)); // 4連パターン
                    }
                }
                r = g;
            }break;
            default: r = CARDS_NULL; break;
        }
        return r;
    }
    
    // 役の作成可能性
    // あくまで作成可能性。展開可能な型ではなく有無判定の速度を優先したもの
    // 先にスート圧縮版、ランク重合版を計算していた場合はそっちを使った方が速いはず
    // ジョーカーがあるか(JK)は重要な情報なので、テンプレートで分岐可能とする(通常は_BOTH)
    // 特に階段においてn枚階段判定は出されている状態でm枚判定を出すような状況にも対応が必要
    
    template<int JK = _BOTH>Cards canMakeNJ2Seq(Cards c){ return JK; }
    template<int JK = _BOTH>Cards canMakeJ2Seq(Cards c);
    template<int JK = _BOTH>Cards canMake2Seq(Cards c);
    
    template<int JK = _BOTH>Cards canMakeNJ3Seq(Cards c);
    template<int JK = _BOTH>Cards canMakeJ3Seq(Cards c);
    template<int JK = _BOTH>Cards canMake3Seq(Cards c);
    
    template<int JK = _BOTH>Cards canMakeNJ4Seq(Cards c);
    template<int JK = _BOTH>Cards canMakeJ4Seq(Cards c);
    template<int JK = _BOTH>Cards canMake4Seq(Cards c);
    
    template<int JK = _BOTH>Cards canMakeNJ5Seq(Cards c);
    template<int JK = _BOTH>Cards canMakeJ5Seq(Cards c);
    template<int JK = _BOTH>Cards canMake5Seq(Cards c);
    
    template<int JK = _BOTH>Cards canMakeNJ6Seq(Cards c);
    template<int JK = _BOTH>Cards canMakeJ6Seq(Cards c);
    template<int JK = _BOTH>Cards canMake6Seq(Cards c);
    
    template<int JK = _BOTH>
    Cards canMakePlainSeq(Cards c, int qty){
        Cards res;
        switch(qty){
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
    template<int JK = _BOTH>
    Cards canMakeJokerSeq(Cards c, int qty){
        Cards res;
        switch(qty){
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
    template<int JK = _BOTH>
    Cards canMakeSeq(Cards c, int qty){
        Cards res;
        switch (qty){
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
    
    template<int QTY = 3, int JK = _BOTH>
    Cards canMakeNJSeq(Cards c){ return canMakeNJSeq<JK>(c, QTY); }
    template<int QTY = 3, int JK = _BOTH>
    Cards canMakeJSeq(Cards c){ return canMakeJSeq<JK>(c, QTY); }
    template<int QTY = 3, int JK = _BOTH>
    Cards canMakeSeq(Cards c){ return canMakeSeq<JK>(c, QTY); }
    
    // 2seq
    template<>Cards canMakeNJ2Seq<0>(Cards c){ return c & (c >> 4); }
    template<>Cards canMakeNJ2Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return canMakeNJ2Seq<0>(nj);
    }
    template<>Cards canMakeNJ2Seq<2>(Cards c){ return canMakeNJ2Seq<1>(c); }
    
    template<>Cards canMakeJ2Seq<0>(Cards c){ return CARDS_NULL; }
    template<>Cards canMakeJ2Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return anyCards(nj);
    }
    template<>Cards canMakeJ2Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMakeJ2Seq<1>(c);
        }else{
            return canMakeJ2Seq<0>(c);
        }
    }
    
    template<>Cards canMake2Seq<0>(Cards c){ return canMakeNJ2Seq<0>(c); }
    template<>Cards canMake2Seq<1>(Cards c){ return canMakeJ2Seq<1>(c); }
    template<>Cards canMake2Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMake2Seq<1>(c);
        }else{
            return canMakeNJ2Seq<0>(c);
        }
    }
    
    // 3seq
    template<>Cards canMakeNJ3Seq<0>(Cards c){ return c&(c >> 4)&(c >> 8); }
    template<>Cards canMakeNJ3Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return canMakeNJ3Seq<0>(nj);
    }
    template<>Cards canMakeNJ3Seq<2>(Cards c){ return canMakeNJ3Seq<1>(c); }
    
    template<>Cards canMakeJ3Seq<0>(Cards c){ return CARDS_NULL; }
    template<>Cards canMakeJ3Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return (nj & (nj >> 4)) | (nj & (nj >> 8));
    }
    template<>Cards canMakeJ3Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMakeJ3Seq<1>(c);
        }else{
            return canMakeJ3Seq<0>(c);
        }
    }
    
    template<>Cards canMake3Seq<0>(Cards c){ return canMakeNJ3Seq<0>(c); }
    template<>Cards canMake3Seq<1>(Cards c){ return canMakeJ3Seq<1>(c); }
    template<>Cards canMake3Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMake3Seq<1>(c);
        }else{
            return canMakeNJ3Seq<0>(c);
        }
    }
    
    // 4seq
    template<>Cards canMakeNJ4Seq<0>(Cards c){ return c&(c >> 4)&(c >> 8)&(c >> 12); }
    template<>Cards canMakeNJ4Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return canMakeNJ4Seq<0>(nj);
    }
    template<>Cards canMakeNJ4Seq<2>(Cards c){ return canMakeNJ4Seq<1>(c); }
    
    template<>Cards canMakeJ4Seq<0>(Cards c){ return CARDS_NULL; }
    template<>Cards canMakeJ4Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        Cards c12 = nj & (nj >> 4), c3 = nj >> 8, c4 = nj >> 12;
        return (c12 & c3) | (c12 & c4) | (nj & c3 & c4);
    }
    template<>Cards canMakeJ4Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMakeJ4Seq<1>(c);
        }
        else{
            return canMakeJ4Seq<0>(c);
        }
    }
    
    template<>Cards canMake4Seq<0>(Cards c){ return canMakeNJ4Seq<0>(c); }
    template<>Cards canMake4Seq<1>(Cards c){ return canMakeJ4Seq<1>(c); }
    template<>Cards canMake4Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMake4Seq<1>(c);
        }
        else{
            return canMakeNJ4Seq<0>(c);
        }
    }
    
    // 5seq
    template<>Cards canMakeNJ5Seq<0>(Cards c){ return c & (c >> 4) & (c >> 8) & (c >> 12) & (c >> 16); }
    template<>Cards canMakeNJ5Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return canMakeNJ5Seq<0>(nj);
    }
    template<>Cards canMakeNJ5Seq<2>(Cards c){ return canMakeNJ5Seq<1>(c); }
    
    template<>Cards canMakeJ5Seq<0>(Cards c){ return CARDS_NULL; }
    template<>Cards canMakeJ5Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        Cards c12 = nj & (nj >> 4), c3 = nj >> 8, c4 = nj >> 12, c5 = nj >> 16, c45 = c4 & c5;
        return (c12 & c3 & c4) | (c12 & c3 & c5) | (c12 & c45) | (nj & c3 & c45);
    }
    template<>Cards canMakeJ5Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMakeJ5Seq<1>(c);
        }else{
            return canMakeJ5Seq<0>(c);
        }
    }
    
    template<>Cards canMake5Seq<0>(Cards c){ return canMakeNJ5Seq<0>(c); }
    template<>Cards canMake5Seq<1>(Cards c){ return canMakeJ5Seq<1>(c); }
    template<>Cards canMake5Seq<2>(Cards c){
        if(containsJOKER(c)){
            return canMake5Seq<1>(c);
        }else{
            return canMakeNJ5Seq<0>(c);
        }
    }
    
    // 6seq
    template<>Cards canMakeNJ6Seq<0>(Cards c){ return c & (c >> 4) & (c >> 8) & (c >> 12) & (c >> 16) & (c >> 20); }
    template<>Cards canMakeNJ6Seq<1>(Cards c){
        Cards nj = maskJOKER(c);
        return canMakeNJ6Seq<0>(nj);
    }
    template<>Cards canMakeNJ6Seq<2>(Cards c){ return canMakeNJ6Seq<1>(c); }
    
    //uint32_t removeAllGroups(Cards c)
   
    // 主に支配性判定用の許容ゾーン計算
    // 支配性の判定のため、合法着手がカード集合表現のどこに存在しうるかを計算
    
    // グループ版（ジョーカーを除けばシングルも一緒）
    inline Cards ORSToGValidZone(int ord, uint32_t rank, uint32_t suits)noexcept{
        Cards res;
        switch(ord){
            case 0: res = RRSToCards(rank + 1, RANK_MAX, suits); break;
            case 1: res = RRSToCards(RANK_MIN, rank - 1, suits); break;
            case 2: res = subtrCards(RRSToCards(RANK_MIN, RANK_MAX, suits), RankSuitsToCards(rank, suits)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    inline Cards ORToGValidZone(int ord, uint32_t rank)noexcept{ // ランク限定のみ
        Cards res;
        switch(ord){
            case 0: res = RankRangeToCards(rank + 1, RANK_MAX); break;
            case 1: res = RankRangeToCards(RANK_MIN, rank - 1); break;
            case 2: res = subtrCards(RankRangeToCards(RANK_MIN, RANK_MAX), RankToCards(rank)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    // 階段版
    inline Cards ORSQToSCValidZone(int ord, uint32_t rank, uint32_t suits, int qty)noexcept{
        Cards res;
        switch(ord){
            case 0: res = RRSToCards(rank + qty, RANK_MAX, suits); break;
            case 1: res = RRSToCards(RANK_MIN, rank - 1, suits); break;
            case 2: res = addCards(RRSToCards(RANK_MIN, rank - 1, suits),
                                   RRSToCards(rank + qty, RANK_MAX, suits)); break;
            default: UNREACHABLE; res = CARDS_NULL; break;
        }
        return res;
    }
    
    inline Cards ORQToSCValidZone(int ord, uint32_t rank, int qty)noexcept{ // ランク限定のみ
        Cards res;
        switch(ord){
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
    bool isValidGroupRank(int mvRank, int order, int bdRank)noexcept{
        if(order == ORDER_NORMAL){
            return mvRank > bdRank;
        }else{
            return mvRank < bdRank;
        }
    }
    
    bool isValidSeqRank(int mvRank, int order, int bdRank, int qty)noexcept{
        if(order == ORDER_NORMAL){
            return mvRank >= (bdRank + qty);
        }else{
            return mvRank <= (bdRank - qty);
        }
    }
    
    // 枚数オンリーによる許容包括
    Cards seqExistableZone(uint32_t qty){
        return RankRangeToCards(RANK_IMG_MIN, RANK_IMG_MAX + 1 - qty);
    }
    
    // 出力用クラス
    struct OutCards{
        BitCards c;
        constexpr OutCards(const BitCards& arg) :c(arg){}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCards& arg){
        assert(holdsCards(CARDS_IMG_ALL, arg.c));
        out << "{";
        BitCards tmp = arg.c;
        while(anyCards(tmp)){
            out << " " << OutIntCard(popIntCardLow(&tmp));
        }
        out << " }";
        return out;
    }
    
    struct OutCardsM{
        Cards c;
        constexpr OutCardsM(const Cards& arg) :c(arg){}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCardsM& arg){
        assert(holdsCards(CARDS_IMG_ALL, arg.c));
        out << "{";
        Cards tmp = arg.c;
        while(anyCards(tmp)){
            out << " " << OutIntCardM(popIntCardLow(&tmp));
        }
        out << " }";
        return out;
    }
    
    // テーブル形式で出力
    struct OutCardTable{
        BitCards c;
        OutCardTable(const BitCards& ac) : c(ac){}
    };
    
    std::ostream& operator <<(std::ostream& out, const OutCardTable& arg){
        // テーブル形式で見やすく
        BitCards c = arg.c;
        out << " ";
        for (int r = RANK_3; r <= RANK_2; ++r){
            out << " " << OutRank(r);
        }
        out << " " << "X" << endl;
        for(int sn = 0; sn < 4; ++sn){
            out << OutSuitNum(sn) << " ";
            for (int r = RANK_3; r <= RANK_2; ++r){
                if(containsCard(c, RankSuitsToCards(r, SuitNumToSuits(sn)))){
                    out << "O ";
                }else{
                    out << ". ";
                }
            }
            if(sn == 0){
                if(containsJOKER(c)){
                    out << "O ";
                }else{
                    out << ". ";
                }
            }
            out << endl;
        }
        return out;
    }
    
    struct Out2CardTables{
        BitCards c0, c1;
        Out2CardTables(const BitCards& ac0, const BitCards& ac1) : c0(ac0), c1(ac1){}
    };
    
    std::ostream& operator <<(std::ostream& out, const Out2CardTables& arg){
        // テーブル形式で見やすく
        // ２つを横並びで表示
        BitCards c[2] = { arg.c0, arg.c1 };
        for(int i = 0; i < 2; ++i){
            out << " ";
            for (int r = RANK_3; r <= RANK_2; ++r){
                out << " " << OutRank(r);
            }
            out << " " << "X";
            out << "    ";
        }
        out << endl;
        for(int sn = 0; sn < N_SUITS; ++sn){
            for(int i = 0; i < 2; ++i){
                out << OutSuitNum(sn) << " ";
                for (int r = RANK_3; r <= RANK_2; ++r){
                    if(containsCard(c[i], RankSuitsToCards(r, SuitNumToSuits(sn)))){
                        out << "O ";
                    }else{
                        out << ". ";
                    }
                }
                if(sn == 0){
                    if(containsJOKER(c[i])){
                        out << "O ";
                    }else{
                        out << ". ";
                    }
                    out << "   ";
                }else{
                    out << "     ";
                }
            }
            out << endl;
        }
        return out;
    }

    // イテレーション
    template<typename callback_t>
    void iterateIntCard(BitCards c, const callback_t& callback){
        while(anyCards(c)){
            IntCard ic = popIntCard(&c);
            callback(ic);
        }
    }
    template<typename callback_t>
    void iterateCard(BitCards c, const callback_t& callback){
        while(anyCards(c)){
            Cards tc = pop(&c);
            callback(tc);
        }
    }
    
    template<class cards_buf_t, typename callback_t>
    int searchCards(const cards_buf_t *const buf, const int n, const callback_t& callback)noexcept{
        for(int m = 0; m < n; ++m){
            if(callback(buf[m])){ return m; }
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
    inline Cards QtyToPQR(uint32_t q){
        return PQR_1 << (q - 1);
    }
    
    BitArray64<4, 16> CardsToQR_slow(BitCards c)noexcept{
        BitArray64<4, 16> ret = 0;
        for(int r = RANK_U; r <= RANK_O; ++r){
            ret.set(r, countCards(RankToCards(r) & c));
        }
        return ret.data();
    }
    BitArray64<4, 16> CardsToENR_slow(BitCards c, int n)noexcept{
        BitArray64<4, 16> ret = 0;
        for(int r = RANK_U; r <= RANK_O; ++r){
            if(countCards(c & RankToCards(r)) >= n){
                ret.set(r, 1);
            }
        }
        return ret.data();
    }
    BitArray64<4, 16> CardsToNR_slow(BitCards c, int n)noexcept{
        BitArray64<4, 16> ret = 0;
        for(int r = RANK_U; r <= RANK_O; ++r){
            if(countCards(c & RankToCards(r)) == n){
                ret.set(r, 1);
            }
        }
        return ret.data();
    }
    BitCards QRToPQR_slow(BitArray64<4, 16> qr)noexcept{
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for(int r = RANK_U; r <= RANK_O; ++r){
            if(arr[r]){
                ret.set(r, 1 << (arr[r] - 1));
            }
        }
        return ret.data();
    }
    BitCards QRToSC_slow(BitArray64<4, 16> qr)noexcept{
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for(int r = RANK_U; r <= RANK_O; ++r){
            ret.set(r, (1 << arr[r]) - 1);
        }
        return ret.data();
    }
    BitCards PQRToSC_slow(BitArray64<4, 16> qr)noexcept{
        BitArray64<4, 16> arr = qr;
        BitArray64<4, 16> ret = CARDS_NULL;
        for(int r = RANK_U; r <= RANK_O; ++r){
            if(arr[r]){
                uint32_t q = bsf(arr[r]) + 1;
                ret.set(r, (1 << q) - 1);
            }
        }
        return ret.data();
    }
    
    // パラレル演算関数
    inline BitArray64<4, 16> CardsToQR(const BitCards c)noexcept{
        // 枚数が各4ビットに入る
        BitCards a = (c & PQR_13) + ((c >> 1) & PQR_13);
        return (a & PQR_12) + ((a >> 2) & PQR_12);
    }
    inline BitCards CardsToFR(const BitCards c)noexcept{
        // ランク中の4ビット全てあればPQR_1の位置にビットが立つ
        BitCards a = c & (c >> 1);
        return a & (a >> 2) & PQR_1;
    }
    inline BitCards CardsTo3R(const BitCards c)noexcept{
        // ランク中に丁度3ビットあればPQR_1の位置にビットが立つ
        BitCards ab_cd = c & (c >> 1);
        BitCards axb_cxd = c ^ (c >> 1);
        return ((ab_cd & (axb_cxd >> 2)) | ((ab_cd >> 2) & axb_cxd)) & PQR_1;
    }
    inline BitCards CardsTo2R(const BitCards c)noexcept{
        // ランク中に丁度2ビットあればPQR_1の位置にビットが立つ
        BitCards qr = CardsToQR(c);
        return (qr >> 1) & ~qr & PQR_1;
    }
    inline BitCards CardsTo1R(const BitCards c)noexcept{
        // ランク中に丁度1ビットあればPQR_1の位置にビットが立つ
        return CardsTo3R(~c);
    }
    inline BitCards CardsTo0R(const BitCards c)noexcept{
        // ランク中に1ビットもなければPQR_1の位置にビットが立つ
        return CardsToFR(~c);
    }
        
    inline BitCards CardsToNR(const BitCards c, int q)noexcept{
        Cards nr;
        switch(q){
            case 0: nr = CardsTo0R(c); break;
            case 1: nr = CardsTo1R(c); break;
            case 2: nr = CardsTo2R(c); break;
            case 3: nr = CardsTo3R(c); break;
            case 4: nr = CardsToFR(c); break;
            default: UNREACHABLE; nr = CARDS_NULL; break;
        }
        return nr;
    }
        
    inline BitCards CardsToER(const BitCards c)noexcept{
        // ランク中に1ビットでもあればPQR_1の位置にビットが立つ
        BitCards a = c | (c >> 1);
        return (a | (a >> 2)) & PQR_1;
    }
    
    inline BitCards CardsToPQR(const BitCards arg)noexcept{
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
        if(r3){
            r |= r3;
            r |= ~((r3 >> 1) | (r3 >> 2)) & r12;
        }else{
            r |= r12;
        }
        return r;
    }
    
    inline BitCards QRToPQR(const BitArray64<4, 16> qr)noexcept{
        // qr -> pqr 変換
        const Cards iqr = ~qr;
        const Cards qr_l1 = (qr << 1);
        const Cards r = (PQR_1 & qr & (iqr >> 1)) | (PQR_2 & qr & (iqr << 1)) | ((qr & qr_l1) << 1) | (qr_l1 & PQR_4);
        return r;
    }
    
    inline BitCards PQRToSC(const BitCards pqr)noexcept{
        // pqr -> sc はビットを埋めていくだけ
        Cards r = pqr;
        r |= (r & PQR_234) >> 1;
        r |= (r & PQR_34) >> 2;
        return r;
    }
    
    inline void PQRToND(const BitCards pqr, uint32_t jk, Cards *const nd)noexcept{
        // pqr -> nd[2] 変換
        // ジョーカーの枚数の情報も必要
        assert(jk == 0 || jk == 1); // 0or1枚
        assert(nd != nullptr);
        
        nd[0] = nd[1] = CARDS_NULL;

        // pqrの1ランクシフトが無支配限度
        // 以降、tmpのビットの示す位置は実際にカードがあるランクよりずれている事に注意
        Cards tmp0 = pqr >> 4;
        Cards tmp1 = pqr << 4;
        while(tmp0){ // 無支配ゾーンがまだ広いはず
            IntCard ic = pickIntCardHigh(tmp0);
            int r = IntCardToRank(ic);
            int sn = IntCardToSuitNum(ic);
            nd[0] |= ORQ_NDTable[0][r][sn]; // このゾーンに対して返せることが確定
            tmp0 &= (~nd[0]); // もう関係なくなった部分は外す
        }
        while(tmp1){ // 無支配ゾーンがまだ広いはず
            IntCard ic = pickIntCardLow(tmp1);
            int r = IntCardToRank(ic);
            int sn = IntCardToSuitNum(ic);
            nd[1] |= ORQ_NDTable[1][r][sn]; // このゾーンに対して返せることが確定
            tmp1 &= (~nd[1]); // もう関係なくなった部分は外す
        }
        
        // ジョーカーがある場合は1枚分ずらして、全てのシングルを加える
        // 逆転オーダーの場合は+の4枚にフラグがある可能性があるのでマスクする
        if(jk){
            nd[0] <<= 1;
            nd[0] |= PQR_1;
            nd[1] &= PQR_123;
            nd[1] <<= 1;
            nd[1] |= PQR_1;
        }
    }

    // 役の作成可能性判定
    Cards canMakePlainGroup(Cards c, int q){
        Cards ret;
        switch(q){
            case 0: ret = CARDS_ALL; break; // 0枚のグループは必ずできるとする(canMakeGroupでジョーカーありの場合のため)
            case 1: ret = maskJOKER(c); break;
            case 2: ret = CardsToQR(c) & PQR_234; break;
            case 3: ret = (CardsToQR(c) + PQR_1) & PQR_34; break;
            case 4: ret = CardsToFR(c); break;
            default: ret = CARDS_NULL; break;
        }
        return ret;
    }
    Cards canMakeGroup(Cards c, int q){
        return canMakePlainGroup(c, containsJOKER(c) ? (q - 1) : q);
    }
    
    // スート圧縮判定
    // あるカード集合（スート限定がなされていてもよい）の中にn枚グループが作成可能かの判定
    // ただしジョーカーの分は最初から引いておく
    // 高速な処理には場合分けが必要か
    inline bool judgeSuitComp(Cards c, int n){
        if(c){
            if(n == 1){
                return true;
            }else{
                c = CardsToQR(c);
                if(c & PQR_234){ // 2枚以上
                    if(n == 2){
                        return true;
                    }else{
                        if(c & PQR_34){ // 4枚
                            if(n <= 4){ return true; }
                        }else{
                            if(((c & PQR_2) >> 1) & c){ // 3枚
                                if(n == 3){ return true; }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    void initCards(){
        // カード集合関係の初期化
        
        // nd計算に使うテーブル
        for(int r = 0; r < 16; ++r){
            // オーダー通常
            ORQ_NDTable[0][r][0] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1;
            ORQ_NDTable[0][r][1] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_12;
            ORQ_NDTable[0][r][2] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_123;
            ORQ_NDTable[0][r][3] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            for(int q = 4; q < 8; ++q){
                ORQ_NDTable[0][r][q] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
            }
            // オーダー逆転
            ORQ_NDTable[1][r][0] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1;
            ORQ_NDTable[1][r][1] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_12;
            ORQ_NDTable[1][r][2] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_123;
            ORQ_NDTable[1][r][3] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            for(int q = 4; q < 8; ++q){
                ORQ_NDTable[1][r][q] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
            }
            //複数ジョーカーには未対応
        }
    }
    
    struct CardsInitializer{
        CardsInitializer(){
            initCards();
        }
    };
    
    CardsInitializer cardsInitializer;
    
    /**************************カード集合表現(クラス版)**************************/
    
    /*struct Cards : BitSet64{
        
        // 定数
        constexpr Cards() : BitSet64(){}
        constexpr Cards(const BitCards ac) : BitSet64(ac){}
        constexpr Cards(const Cards& ac) : BitSet64(ac.cards()){}
        
        Cards& fill()noexcept{
            set(CARDS_ALL);
        }
     
        // 生のBitCards型への変換
        BitCards cards()const noexcept{ return static_cast<BitCards>(data()); }
        BitCards& cards()noexcept{ return data(); }
        
        bool anyJOKER()const noexcept{ return anyJOKER(cards()); }
        bool anyPlain()const noexcept{ return anyPlain(cards()); }
        bool any()const noexcept{ return anyCards(cards()); }
        bool any2()const noexcept{ return any2Cards(cards()); }
        
        bool containsJOKER()const{ return containsJOKER(cards()); }
        bool containsS3()const{ return containsS3(cards()); }
        bool containsD3()const{ return containsD3(cards()); }
        
        uint32_t countPlain()const noexcept{
             return countCards(maskJOKER(cards()));
        }
        
        Cards& set(BitCards ac)noexcept{
            (*this) = ac;
        }
        Cards& set(Cards ac)noexcept{
            (*this) = ac.cards();
        }
        Cards& add(BitCards ac)noexcept{
            addCards(&cards(), ac);
            return *this;
        }
        Cards& addJOKER()noexcept{
            addJOKER(&cards());
            return *this;
        }
        Cards& addIntCard(IntCard ic)noexcept{
            addIntCard(&cards(), ic);
            return *this;
        }
        Cards& sub(BitCards ac)noexcept{
            subtrCards(&cards(), ac);
        }
        
        Cards& inv(Cards argc)noexcept{
            invCards(&cards, argc);
        }
        Cards& inv()noexcept{
            invCards(&cards, CARDS_ALL);
        }
        Cards& mask(BitCards ac)noexcept{
            maskCards(&cards(), ac);
            return *this;
        }
        Cards& maskJOKER()noexcept{
            UECda::maskJOKER(&cards());
            return *this;
        }

        // 包括性
        bool holds(const BitCards ac)const noexcept{
            return holdsCards(cards, ac);
        }
        
        
        // pick, pop
        IntCard pickIntCardLow()const{
            assert(any());
            return UECda::pickIntCardLow(cards());
        }
        IntCard pickIntCardHigh()const{
            assert(any());
            return UECda::pickIntCardHigh(cards());
        }
        IntCard popIntCardLow(){
            assert(any());
            return UECda::popIntCardLow(&cards());
        }
        IntCard popIntCardHigh(){
            assert(any());
            return UECda::popIntCardHigh(&cards());
        }
        Cards pickLow()const{
            assert(any());
            pickL
        }
        Cards pickHigh()const{
            assert(any());
        }
        Cards pickHigh()const{
            assert(any());
        }
        Cards pickLow()const{
            assert(any());
        }
        
        // 複数pick, pop
        template<int N = 1>Cards pickLow(
        )const{
            assert(count() >= N);
        }
        
        template<int N=1,int JK=1> void pickHigh(Cards96<cards_t,NJK> *const dst)const{
            assert( count() >= N );
            if( JK && jk >= (uint32_t)N ){
                dst->cards=CARDS_NULL;
                dst->jk=(uint32_t)N;
            }else{
                dst->cards=::pickHigh(cards,(uint32_t)N-jk);
                dst->jk=jk;
            }
        }
        
        template<int N=1,int JK=1> void popLow(Cards96<cards_t,NJK> *const dst){
            assert( count() >= N );
            uint32_t nex=countPlain();
            if( (!JK) || nex >= (uint32_t)N ){
                Cards tmp=::pickLow<N>(cards);
                dst->cards=tmp;
                dst->jk=0;
                subtr(tmp);
            }else{
                dst->cards=cards;
                dst->jk=(uint32_t)N - nex;
                cards=CARDS_NULL;
                jk-=(uint32_t)N - nex;
            }
        }
        
        template<int N=1,int JK=1> void popHigh(Cards96<cards_t,NJK> *const dst){
            assert( count() >= N );
            if( JK && jk >= (uint32_t)N ){
                dst->cards=CARDS_NULL;
                dst->jk=(uint32_t)N;
                jk-=(uint32_t)N;
            }else{
                Cards tmp=::pickHigh(cards,(uint32_t)N-jk);
                dst->cards=tmp;
                dst->jk=jk;
                subtr(tmp);
                jk=0;
            }
        }
        
        template<int N=1,int JK=1,class dice_t> void popRand(Cards96<cards_t,NJK> *const dst,int n,dice_t *const pdice){
            assert( count() >= n );
            if( !JK ){
                Cards tmp=pickNBits64(cards,n,countPlain()-n,pdice);
                subtr(tmp);
                dst->cards=tmp;
                dst->jk=0;
            }else{
                dst->clear();
                Cards tmp=cards;
                if( jk ){
                    addCards(&tmp,CARDS_JOKER);
                    if( jk==2 ){
                        addCards(&tmp,CARDS_JOKER<<1);
                    }
                }
                Cards ret=pickNBits64(tmp,n,count()-n,pdice);
                dst->cards=pickCDHS(ret);
                dst->jk=countCards(ret-pickCDHS(ret));
                subtr(*dst);
            }
        }
        
        template<int N=1,int JK=1,class dice_t> void pickRand(Cards96<cards_t,NJK> *const dst,int n,dice_t *const pdice)const{
            assert( count() >= n );
            if( !JK ){
                Cards tmp=pickNBits64(cards,n,countPlain()-n,pdice);
                dst->cards=tmp;
                dst->jk=0;
            }else{
                dst->clear();
                Cards tmp=cards;
                if( jk ){
                    addCards(&tmp,CARDS_JOKER);
                    if( jk==2 ){
                        addCards(&tmp,CARDS_JOKER<<1);
                    }
                }
                Cards ret=pickNBits64(tmp,n,count()-n,pdice);
                dst->cards=pickCDHS(ret);
                dst->jk=countCards(ret-pickCDHS(ret));
            }
        }
        
        //階段の作成可能性
        template<int N=MIN_SEQ_QTY,int JK_QTY=-1>Cards canMakeSeq(){
            //持っているジョーカーの枚数と異なる枚数でJK_QTYを指定されてもよいことにする
            switch( JK_QTY ){
                case 0: return polymRanks<N>(cards);break;
                case 1: return polymRanksWithJOKER<N,1>(cards);break;
                case 2: return polymRanksWithJOKER<N,2>(cards);break;
                default:
                    assert( jk<=N_JOKERS );
                    switch( jk ){
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
     template<int CON_JOKER>struct SeqMelds{
     
     Cards melds;
     
     Move popLow(){
     
     }
     
     };
     */
    
    
    /**************************着手表現**************************/
    
    // 32ビット着手表現
    // beersongのものと並びは違うが、情報はそれほど変わりはない
    // 支配能については過程を示す（支配して流れた後の情報ではない）
    // 場に依存しない情報のみを載せ、前計算による役生成からの合法性判断にも対応
    
    // 0 一時オーダー逆転(永続、一時双方のオーダー逆転能力を持つ場合は打ち消し合って0になる)
    // 1 永続オーダー逆転
    // 2 スートしばり
    // 3 ランクしばり
    // 4-7 サイズ
    // 8-11 ランク(小さい方)
    // 12-15 スート
    // 16 シングル
    // 17 グループ(シングル除く)
    // 18 階段
    // 19 (空き)
    // 20-23 ジョーカーが占めているランク
    // 24-27 ジョーカーが占めているスート
    // 28 条件付き完全支配(JOKER, S3双方にフラグ)
    // 29 無条件完全支配(8切り)
    
    constexpr int MOVE_LCT_TMPORD  = 0;
    constexpr int MOVE_LCT_PRMORD  = 1;
    constexpr int MOVE_LCT_QTY     = 4;
    constexpr int MOVE_LCT_RANK    = 8;
    constexpr int MOVE_LCT_SUITS   = 12;
    constexpr int MOVE_LCT_JKRANK  = 20;
    constexpr int MOVE_LCT_JKSUITS = 24;
    
    // ランク4倍型
    constexpr int MOVE_LCT_RANK4X   = MOVE_LCT_RANK - 2;
    constexpr int MOVE_LCT_JKRANK4X = MOVE_LCT_JKRANK - 2;
    
    constexpr uint32_t MOVE_NONE           = 0xFFFFFFFF; // 存在しない着手

    constexpr uint32_t MOVE_PASS           = 0U;
    constexpr uint32_t MOVE_NULL           = 0U; // ある着手について何も情報が無い状態を示す
    
    constexpr uint32_t MOVE_FLAG_TMPORD    = 0x00000001;
    constexpr uint32_t MOVE_FLAG_PRMORD    = 0x00000002;
    
    constexpr uint32_t MOVE_FLAG_SUITSLOCK = 0x00000004;
    constexpr uint32_t MOVE_FLAG_RANKLOCK  = 0x00000008;
    
    constexpr uint32_t MOVE_FLAG_QTY       = 0x000000F0;
    
    constexpr uint32_t MOVE_FLAG_RANK      = 0x00000F00;
    constexpr uint32_t MOVE_FLAG_SUITS     = 0x0000F000;
    
    constexpr uint32_t MOVE_FLAG_SINGLE    = 0x00010000;
    constexpr uint32_t MOVE_FLAG_GROUP     = 0x00020000;
    constexpr uint32_t MOVE_FLAG_SEQ       = 0x00040000;
    
    constexpr uint32_t MOVE_FLAG_JKRANK    = 0x00F00000;
    constexpr uint32_t MOVE_FLAG_JKSUITS   = 0x0F000000;
        
    constexpr uint32_t MOVE_FLAG_CONDDOM   = 0x10000000;
    constexpr uint32_t MOVE_FLAG_INEVITDOM = 0x20000000;
    
    constexpr uint32_t MOVE_FLAG_ORD       = MOVE_FLAG_TMPORD    | MOVE_FLAG_PRMORD;
    constexpr uint32_t MOVE_FLAG_LOCK      = MOVE_FLAG_SUITSLOCK | MOVE_FLAG_RANKLOCK;
    constexpr uint32_t MOVE_FLAG_FORM      = MOVE_FLAG_SINGLE    | MOVE_FLAG_GROUP     | MOVE_FLAG_SEQ;
    constexpr uint32_t MOVE_FLAG_TYPE      = MOVE_FLAG_FORM      | MOVE_FLAG_QTY;
    constexpr uint32_t MOVE_FLAG_CHARA     = MOVE_FLAG_TYPE      | MOVE_FLAG_RANK      | MOVE_FLAG_SUITS; // 下16ビットさえあれば性質が分かる
    constexpr uint32_t MOVE_FLAG_JK        = MOVE_FLAG_JKRANK    | MOVE_FLAG_JKSUITS;
    constexpr uint32_t MOVE_FLAG_MELD      = MOVE_FLAG_CHARA     | MOVE_FLAG_JK;
    constexpr uint32_t MOVE_FLAG_DOM       = MOVE_FLAG_CONDDOM   | MOVE_FLAG_INEVITDOM;
    
    constexpr uint32_t MOVE_FLAG_EFFECTS   = MOVE_FLAG_ORD | MOVE_FLAG_LOCK | MOVE_FLAG_DOM;
    
    constexpr uint32_t MOVE_SINGLEJOKER    = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM | (SUITS_ALL << MOVE_LCT_JKSUITS);
    constexpr uint32_t MOVE_S3FLUSH        = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM | (SUIT_S << MOVE_LCT_SUITS) | (RANK_3 << MOVE_LCT_RANK);
    
    template<typename data_t>
    class MoveInRegister{
    public:
        constexpr MoveInRegister() : i_(){}
        constexpr MoveInRegister(data_t arg) : i_(arg){}
        
        constexpr operator data_t()const noexcept{ return i_; }
        constexpr data_t data()const noexcept{ return i_.data(); }
        constexpr BitSetInRegister<data_t> bits()const noexcept{ return i_; }
        
        void setNULL()                   noexcept{ i_ = MOVE_NULL; }
        void setPASS()                   noexcept{ i_ = MOVE_PASS; }
        void setSingleJOKER()            noexcept{ i_ = MOVE_SINGLEJOKER; } // シングルジョーカーのランクは未定義
        void setS3Flush()                noexcept{ i_ = MOVE_S3FLUSH; } // スペ3切りの場合のみ
        void setRev()                    noexcept{ i_ ^= MOVE_FLAG_ORD; }
        void setBack()                   noexcept{ i_ ^= MOVE_FLAG_TMPORD; }
        void setEight()                  noexcept{ i_ |= MOVE_FLAG_INEVITDOM; }
        void setSingle()                 noexcept{ i_ |= MOVE_FLAG_SINGLE; }
        void setGroup()                  noexcept{ i_ |= MOVE_FLAG_GROUP; }
        void setSeq()                    noexcept{ i_ |= MOVE_FLAG_SEQ; }
        void setQty(uint32_t qty)        noexcept{ i_ |= qty     << MOVE_LCT_QTY; }
        void setRank(uint32_t r)         noexcept{ i_ |= r       << MOVE_LCT_RANK; }
        void setRank4x(uint32_t r4x)     noexcept{ i_ |= r4x     << MOVE_LCT_RANK4X; } // 4倍型
        void setSuits(uint32_t s)        noexcept{ i_ |= s       << MOVE_LCT_SUITS; }
        void setJokerRank(uint32_t r)    noexcept{ i_ |= r       << MOVE_LCT_JKRANK; }
        void setJokerRank4x(uint32_t r4x)noexcept{ i_ |= r4x     << MOVE_LCT_JKRANK4X; } // 4倍型
        void setJokerSuits(uint32_t s)   noexcept{ i_ |= s       << MOVE_LCT_JKSUITS; }
        void setSpecialJokerSuits()      noexcept{ i_ |= SUITS_ALL << MOVE_LCT_JKSUITS; }
        
        // 特殊効果
        void resetEffects()noexcept{ i_ &= ~MOVE_FLAG_EFFECTS; }
        void setEffects()noexcept{
            // 普通は合法着手生成の段階で付ける
            // 棋譜から読んだ着手にはここで付ける
            if(isSeq()){
                if(qty() >= 5){ setRev(); }
                if(isEightSeqRank(rank(), qty())){
                    setEight();
                }
            }else{
                if(rank() == RANK_3 && suits() == SUITS_S && qty() == 1){
                    i_ = MOVE_S3FLUSH;
                }else if(qty() == 1 && suits() == SUITS_NULL){
                    i_ = MOVE_SINGLEJOKER;
                }else{
                    if(qty() >= 4){ setRev(); }
                    if(rank() == RANK_8){ setEight(); }
                }
            }
        }
        
        // タイプを指定してまとめて処理
        // 特殊効果フラグはここでの分岐処理は避け、呼び出し元で対応
        template<typename rank_t, typename suits_t>
        void setSingle(rank_t rank, suits_t suits)noexcept{
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        template<typename rank4x_t, typename suits_t>
        void setSingleByRank4x(rank4x_t r4x, suits_t suits)noexcept{
            setSingle(); setQty(1); setRank4x(r4x); setSuits(suits);
        }
        template<typename qty_t, typename rank_t, typename suits_t>
        void setGroup(qty_t qty, rank_t rank, suits_t suits)noexcept{
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        template<typename qty_t, typename rank4x_t, typename suits_t>
        void setGroupByRank4x(qty_t qty, rank4x_t r4x, suits_t suits)noexcept{
            setGroup(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }
        template<typename qty_t, typename rank_t, typename suits_t>
        void setSeq(qty_t qty, rank_t rank, suits_t suits)noexcept{
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        template<typename qty_t, typename rank4x_t, typename suits_t>
        void setSeqByRank4x(qty_t qty, rank4x_t r4x, suits_t suits)noexcept{
            setSeq(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }

        // IntCard型からシングル着手をセットする(特殊効果フラグ以外)
        void setSingleByIntCard(IntCard ic)noexcept{
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // 情報を得る
        constexpr uint32_t suits()      const noexcept{ return (i_ >> MOVE_LCT_SUITS)    & 15U; }
        constexpr uint32_t qty()        const noexcept{ return (i_ >> MOVE_LCT_QTY)      & 15U; }
        constexpr uint32_t rank()       const noexcept{ return (i_ >> MOVE_LCT_RANK)     & 15U; }
        constexpr uint32_t jokerRank()  const noexcept{ return (i_ >> MOVE_LCT_JKRANK)   & 15U; }
        constexpr uint32_t jokerSuits() const noexcept{ return (i_ >> MOVE_LCT_JKSUITS)  & 15U; }
        constexpr uint32_t rank4x()     const noexcept{ return (i_ >> MOVE_LCT_RANK4X)   & (15U << 2); } // 4倍型
        constexpr uint32_t jokerRank4x()const noexcept{ return (i_ >> MOVE_LCT_JKRANK4X) & (15U << 2); } // 4倍型
        
        // 部分に着目する
        constexpr uint32_t orderPart()      const noexcept{ return i_ & MOVE_FLAG_ORD;   }
        constexpr uint32_t exceptOrderPart()const noexcept{ return i_ & ~MOVE_FLAG_ORD;  }
        constexpr uint32_t suitsPart()      const noexcept{ return i_ & MOVE_FLAG_SUITS; }
        constexpr uint32_t rankPart()       const noexcept{ return i_ & MOVE_FLAG_RANK;  }
        constexpr uint32_t qtyPart()        const noexcept{ return i_ & MOVE_FLAG_QTY;   }
        constexpr uint32_t typePart()       const noexcept{ return i_ & MOVE_FLAG_TYPE;  } // サイズ + 形式
        constexpr uint32_t jokerPart()      const noexcept{ return i_ & MOVE_FLAG_JK;    } // ジョーカー関連
        constexpr uint32_t exceptJokerPart()const noexcept{ return i_ & ~MOVE_FLAG_JK;   } // ジョーカー関連以外
        constexpr uint32_t charaPart()      const noexcept{ return i_ & MOVE_FLAG_CHARA;    } // 性質決定のための必要十分条件
        constexpr uint32_t meldPart()       const noexcept{ return i_ & MOVE_FLAG_MELD;    } // 性質決定のための必要十分条件
        
        // True or False
        constexpr bool isPASS()const noexcept{ return (uint32_t)i_ == MOVE_PASS; }
        constexpr uint32_t isSeq()const noexcept{ return i_ & MOVE_FLAG_SEQ; }
        constexpr uint32_t isGroup()const noexcept{ return i_ & MOVE_FLAG_GROUP; }
        constexpr uint32_t isSingle()const noexcept{ return i_ & MOVE_FLAG_SINGLE; }
        constexpr uint32_t isSingleOrGroup()const noexcept{ return i_ & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP); }
        constexpr bool isQuadruple()const noexcept{
            return typePart() == (MOVE_FLAG_GROUP | (4U << MOVE_LCT_QTY));
        }
        constexpr bool isQuintuple()const noexcept{
            return typePart() == (MOVE_FLAG_GROUP | (5U << MOVE_LCT_QTY));
        }
        bool isOver5Seq()const noexcept{ // 5枚以上階段
            return isSeq() && (qtyPart() > (4U << MOVE_LCT_QTY));
        }
        template<int IS_SEQ = _BOTH>
        bool isSpecialRankSeq()const noexcept{
            if((IS_SEQ == _NO) || ((IS_SEQ != _YES) && !isSeq())){ return false; }
            uint32_t r = rank(), q = qty();
            return (r < RANK_MIN) || (RANK_MAX < (r + q - 1));
        }
        
        constexpr uint32_t containsJOKER()const noexcept{ return i_ & MOVE_FLAG_JK; }
        
        constexpr bool isSingleJOKER()const noexcept{ return (((uint32_t)i_) & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        constexpr bool isS3Flush()const noexcept{ return holdsBits<data_t>(i_, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM)); }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s)const noexcept{
            // rank と スートが一致するか
            return ((uint32_t)i_ & (MOVE_FLAG_RANK | MOVE_FLAG_SUITS)) == ((r << MOVE_LCT_RANK) | (s << MOVE_LCT_SUITS));
        }

        constexpr uint32_t domInevitably()const noexcept{ return i_ & MOVE_FLAG_INEVITDOM; }
        
        constexpr uint32_t flipsPrmOrder()const noexcept{ return i_ & MOVE_FLAG_PRMORD; }
        constexpr uint32_t flipsTmpOrder()const noexcept{ return i_ & MOVE_FLAG_TMPORD; }
        
        constexpr uint32_t changesPrmState()const noexcept{ return flipsPrmOrder(); }
        
        int typeNum()const noexcept{
            uint32_t q = qty();
            if(isSeq()){
                if(q >= 6){ return 8; }
                return 2 + q;
            }else{
                if(q >= 5){ return 8; }
                return q;
            }
        }
        
        template<int IS_PASS = _BOTH>
        Cards cards()const noexcept{ // カード集合を得る
            // 本来は着手表現からの計算で得るのは止めた方が良いかもしれない
            Cards res;
            if((IS_PASS == _YES) || ((IS_PASS != _NO) && isPASS())){ return CARDS_NULL; }
            if(isSingleJOKER()){ return CARDS_JOKER; }
            
            uint32_t s = suits(), r4x = rank4x();
            
            if(!isSeq()){ // 階段でない
                res = CARDS_NULL;
                uint32_t jks = jokerSuits();
                if(jks){
                    addJOKER(&res);
                    if(jks != SUITS_CDHS){ s -= jks; } // クインタプル対策
                }
                addCards(&res, Rank4xSuitsToCards(r4x, s));
            }
            else{ // 階段
                /*
                 uint32_t qty=qty();
                 res=convRx4S_Cards(r4x,suits);
                 switch(qty){
                 case 0:break;
                 case 1:break;
                 case 2:break;
                 case 3:res=extractRanks<3>(res);break;
                 case 4:res=extractRanks<4>(res);break;
                 case 5:res=extractRanks<5>(res);break;
                 default:res=extractRanks(res,qty);break;
                 }*/
                /*uint32_t qty=qty();
                 res=convRx4S_Cards(r4x,suits);
                 res=extractRanks<3>(res);
                 qty-=3;
                 while(qty){
                 res=extractRanks<2>(res);
                 --qty;
                 }*/
                uint32_t q = qty();
                
                assert(q >= 3);
                
                res = extractRanks<3>(Rank4xSuitsToCards(r4x, s));
                if(q != 3){
                    res = extractRanks<2>(res);
                    if(q != 4){
                        res = extractRanks<2>(res);
                        q -= 5;
                        while(q){
                            res = extractRanks<2>(res);
                            q -= 1;
                        }
                    }
                }
                /*
                 uint32_t qty=qtyPart();
                 res=extractRanks<3>(convRx4S_Cards(r4x,suits));
                 if(qty!=(3<<8)){
                 res=extractRanks<2>(res);
                 if(qty!=(4<<8)){
                 res=extractRanks<2>(res);
                 qty-=(5<<8);
                 while(qty){
                 res=extractRanks<2>(res);
                 qty-=(1<<8);
                 }
                 }
                 }*/
                if(containsJOKER()){
                    subtrCards(&res, Rank4xSuitsToCards(jokerRank4x(), s));
                    addJOKER(&res);
                }
            }
            //tock();
            //cout<<m<<OutCards(res);
            
            return res;
        }
        
        template<int IS_PASS = _BOTH>
        Cards charaCards()const noexcept{
            // 性質カードを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            if((IS_PASS == _YES) || ((IS_PASS != _NO) && isPASS())){ return CARDS_NULL; }
            if(isSingleJOKER()){ return CARDS_JOKER; }
            Cards res = Rank4xSuitsToCards(rank4x(), suits());
            if(isSeq()){
                uint64_t q = qty();
                switch(q){
                    case 0: break;
                    case 1: break;
                    case 2: break;
                    case 3: res = extractRanks<3>(res); break;
                    case 4: res = extractRanks<4>(res); break;
                    case 5: res = extractRanks<5>(res); break;
                    default: res = extractRanks(res, q); break;
                }
            }
            return res;
        }
        
        template<int QTY = 256>
        Cards charaPQR()const{
            static_assert((QTY == 256 || (1 <= QTY && QTY <= 4)), "Move::charaPQR\n");
            // 性質カードのPQRを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            // パスとシングルジョーカーも関係ないし、
            // 階段にも今の所対応していない(意味が無さそう)
            if(QTY == 0){
                return CARDS_NULL;
            }else if(QTY == 1){
                return CARDS_HORIZON << rank4x();
            }else if(QTY != 256){
                constexpr int sft = (QTY - 1) >= 0 ? ((QTY - 1) < 32 ? (QTY - 1) : 31) : 0; // warningに引っかからないように...
                if(1 <= QTY && QTY <= 4){
                    return Cards(1U << sft) << rank4x();
                }else{
                    return CARDS_NULL;
                }
            }else{
                return Cards(1U << (qty() - 1)) << rank4x();
            }
        }
        
        bool exam()const noexcept{
            // 変な値でないかチェック
            // 特殊効果の付け忘れなどに注意
            int q = qty();
            int r = rank();
            uint32_t s = suits();
            if(isPASS()){
                // TODO: パスの時のチェックがあれば
            }else{
                if(q < 0){ return false; }
                if(isSeq()){
                    if(q < 3){ return false; }
                    if(countSuits(s) != 1){ return false; }
                    if(isEightSeqRank(r, 3)){
                        if(!domInevitably()){ return false; }
                    }else{
                        if(domInevitably()){ return false; }
                    }
                }else if(isSingle()){
                    if(q != 1){ return false; }
                    if(countSuits(s) != 1){ return false; }
                    if(r == RANK_8){
                        if(!domInevitably()){ return false; }
                    }
                }else{
                    if(isQuintuple()){
                    }else{
                        if(q != countSuits(s)){ return false; }
                    }
                    if(r == RANK_8){
                        if(!domInevitably()){ return false; }
                    }else{
                        if(domInevitably()){ return false; }
                    }
                }
            }
            return true;
        }
    protected:
        BitSetInRegister<data_t> i_;
    };
    
    using Move = MoveInRegister<uint32_t>;
    
    class MeldChar : public Move{
        // Moveと全く同じデータなのだが、構成するカード集合に言及する気がないときにこっちで表記
    public:
        MeldChar(const Move& arg) : Move(arg.data()){}
    };
    
    std::ostream& operator <<(std::ostream& out, const MeldChar& m){ // MeldChar出力
        if(m.isPASS()){
            out << "PASS";
        }else if(m.isSingleJOKER()){
            out << "JOKER";
        }else{
            // スート
            if(m.isQuintuple()){ // クインタ特別
                out << OutSuits(SUITS_CDHSX);
            }else{
                out << OutSuits(m.suits());
            }
            out << "-";
            
            // ランク
            int r = m.rank();
            if(m.isSeq()){
                uint32_t q = m.qty();
                out << RankRange(r, r + q - 1);
            }else{
                out << OutRank(r);
            }
        }
        return out;
    }
    
    std::ostream& operator <<(std::ostream& out, const Move& m){ // Move出力
        out << MeldChar(m) << OutCards(m.cards());
        return out;
    }
    
    class LogMove : public Move{
        // ログ出力用
    public:
        LogMove(const Move& arg) : Move(arg.data()){}
    };
    
    std::ostream& operator <<(std::ostream& out, const LogMove& m){ // LogMove出力
        if(m.isPASS()){
            out << "p";
        }else if(m.isSingleJOKER()){
            out << "jk";
        }else{
            int r = m.rank();
            if(m.isSeq()){
                
                out << OutSuitsM(m.suits());
                out << "-";
                
                uint32_t q = m.qty();
                out << RankRangeM(r, r + q - 1);
                // ジョーカー
                if(m.containsJOKER()){
                    //cerr << m.jokerRank() << " " << OutRankM(m.jokerRank()) << endl;
                    out << "(" << OutRankM(m.jokerRank()) << ")";
                }
            }else{
                if(m.isQuintuple()){ // クインタ特別
                    out << OutSuitsM(SUITS_CDHSX);
                }else{
                    out << OutSuitsM(m.suits());
                }
                out << "-";
                out << OutRankM(r);
                if(m.containsJOKER()){
                    out << "(" << OutSuitsM(m.jokerSuits()) << ")";
                }
            }
        }
        return out;
    }
        
    Move CardsToMove(const Cards chara, const Cards used){
        // 性質 chara 構成札 used のカードから着手への変換
        Move m = MOVE_NULL;
        DERR << "pointer &m = " << (uint64_t)(&m) << endl << m << endl;
        if(chara == CARDS_NULL)return MOVE_PASS;
        if(chara == CARDS_JOKER)return MOVE_SINGLEJOKER;
        IntCard ic = CardsToLowestIntCard(chara);
        int r4x = IntCardToRank4x(ic);
        uint32_t s = CardsRank4xToSuits(chara, r4x);
        uint32_t ps = CardsRank4xToSuits(used, r4x); // ジョーカーなしのスート
        int q = countCards(chara);
        if(!polymRanks<2>(chara)){ // グループ系
            if(q == 1){
                m.setSingleByRank4x(r4x, s);
            }else{
                m.setGroupByRank4x(q, r4x, s);
                uint32_t js = s - ps;
                if(js){
                    m.setJokerSuits(js);
                }
            }
        }else{ // 階段系
            m.setSeqByRank4x(q, r4x, s);
            if(containsJOKER(used)){
                IntCard jic = CardsToLowestIntCard(subtrCards(chara, maskJOKER(used)));
                uint32_t jr4x = IntCardToRank4x(jic);
                m.setJokerRank4x(jr4x);
                m.setJokerSuits(s);
            }
        }
        m.setEffects();
        DERR << "pointer &m = " << (uint64_t)(&m) << endl;
        DERR << "chara " << OutCards(chara) << " used " << OutCards(used) << " -> " << MeldChar(m) << endl;
        return m;
    }
    
    Move StringToMoveM(const std::string& str){
        // 入力文字列からMove型への変更
        Move mv = MOVE_NULL;
        bool jk = false; // joker used
        uint32_t s = SUITS_NULL;
        int rank = RANK_NONE;
        uint32_t ns = 0; // num of suits
        uint32_t nr = 0; // num of ranks
        uint32_t i = 0;
        
        // special
        if(str == "p"){ return MOVE_PASS; }
        if(str == "jk"){ return MOVE_SINGLEJOKER; }
        // suits
        for(; i < str.size(); ++i){
            char c = str[i];
            if(c == '-'){
                ++i; break;
            }
            int sn = CharToSuitNumM(c);
            if(sn == SUITNUM_NONE){
                CERR << "illegal suit number" << endl;
                return MOVE_NONE;
            }
            if(sn != SUITNUM_X){
                s |= SuitNumToSuits(sn);
            }
            ++ns;
        }
        // rank
        for(; i < str.size(); ++i){
            char c = str[i];
            if(c == '('){ jk = true; ++i; break; }
            int r = CharToRankM(c);
            if(r == RANK_NONE){
                CERR << "illegal rank" << endl;
                return MOVE_NONE;
            }
            if(rank == RANK_NONE){
                rank = r;
            }
            ++nr;
        }
        // invalid
        if(s == SUITS_NULL){ CERR << "null suits" << endl; return MOVE_NONE; }
        if(!ns){ CERR << "zero suits" << endl; return MOVE_NONE; }
        if(rank == RANK_NONE){ CERR << "null lowest-rank" << endl; return MOVE_NONE; }
        if(!nr){ CERR << "zero ranks" << endl; return MOVE_NONE; }
        // seq or group?
        if(nr > 1){ // seq
            mv.setSeq(nr, rank, s);
        }else if(ns == 1){ // single
            mv.setSingle(rank, s);
        }else{ // group
            mv.setGroup(ns, rank, s);
        }
        // joker
        if(jk){
            if(mv.isSeq()){
                int jkr = RANK_NONE;
                for(; i < str.size(); ++i){
                    char c = str[i];
                    if(c == ')'){ break; }
                    int r = CharToRankM(c);
                    if(r == RANK_NONE){
                        CERR << "illegal joker-rank " << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jkr = r;
                }
                mv.setJokerRank(jkr);
                mv.setJokerSuits(mv.suits());
            }else{
                uint32_t jks = SUITS_NULL;
                for(; i < str.size(); ++i){
                    char c = str[i];
                    if(c == ')'){ break; }
                    int sn = CharToSuitNumM(c);
                    if(sn == SUITNUM_NONE){
                        CERR << "illegal joker-suit" << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jks |= SuitNumToSuits(sn);
                }
                mv.setJokerSuits(jks);
            }
        }
        mv.setEffects();
        return mv;
    }
    
    // 生成関数
    constexpr Move IntCardToSingleMove(IntCard ic)noexcept{
        // IntCard型から対応するシングル役に変換
        // シングルジョーカー、ジョーカーS3は非対応
        return Move(MOVE_FLAG_SINGLE
                    | (1U << MOVE_LCT_QTY)
                    | (IntCardToSuits(ic) << MOVE_LCT_SUITS)
                    | (IntCardToRank(ic) << MOVE_LCT_RANK));
    }
    
    template<class move_buf_t>
    int searchMove(const move_buf_t *const buf, const int moves, const move_buf_t& move)noexcept{
        // 同じ着手の探索
        for(int m = 0; m < moves; ++m){
            if(buf[m].meldPart() == move.meldPart()){ return m; }
        }
        return -1;
    }
    
    template<class move_buf_t, typename callback_t>
    int searchMove(const move_buf_t *const buf, const int moves, const callback_t& callback)noexcept{
        // callback を条件とする着手の探索
        for(int m = 0; m < moves; ++m){
            if(callback(buf[m])){ return m; }
        }
        return -1;
    }
    
    /**************************場表現**************************/
    
    // 各プレーヤーの情報を持たない場表現
    // 32ビット着手表現と同一の系列で扱えるようにする
    // ジョーカー情報などを残すかは難しいが、現在はほとんどの情報を残したまま
    
        struct Board{
        
        uint32_t bd;
        
        constexpr Board() :bd(){}
        constexpr Board(uint32_t arg) : bd(arg){}
        constexpr Board(uint64_t arg) : bd((uint32_t)arg){}
        constexpr Board(const Board& arg) : bd(arg.bd){}
        
        constexpr operator uint32_t()const noexcept{ return bd; }
        constexpr operator uint64_t()const noexcept{ return (uint64_t)bd; }
        
        void init()noexcept{ bd = 0U; }
        
        // set, fix
        
        void setTmpOrder(const uint32_t ord)noexcept{ bd |= ord << MOVE_LCT_TMPORD; }
        void setPrmOrder(const uint32_t ord)noexcept{ bd |= ord << MOVE_LCT_PRMORD; }
        
        void fixTmpOrder(const uint32_t ord)noexcept{ bd = (bd & ~MOVE_FLAG_TMPORD) | (ord << MOVE_LCT_TMPORD); }
        void fixPrmOrder(const uint32_t ord)noexcept{ bd = (bd & ~MOVE_FLAG_PRMORD) | (ord << MOVE_LCT_PRMORD); }
        
        void flipTmpOrder()noexcept{ bd ^= MOVE_FLAG_TMPORD; }
        void flipPrmOrder()noexcept{ bd ^= MOVE_FLAG_PRMORD; }
        
        void setExceptOrder(const uint32_t info)noexcept{ bd |= info; }
        void fixExceptOrder(const uint32_t info)noexcept{ bd = (bd & MOVE_FLAG_ORD) | info; }
        
        void resetDom()noexcept{ bd &= ~MOVE_FLAG_DOM; }
        
        // 2体情報をメンバ関数で返す関数
        // 半マスク化みたいな感じ
        constexpr uint32_t domConditionally(Move m)const noexcept{ return isSingleJOKER() && m.isS3Flush(); }
        
        constexpr bool locksSuits(Move m)const noexcept{ return suitsPart() == m.suitsPart(); }
        constexpr bool locksRank(Move m)const noexcept{ return false; } // ルールにない
        
        constexpr uint32_t afterPrmOrder(Move m)const noexcept{ return ((bd ^ m) >> MOVE_LCT_PRMORD) & 1U; }
        constexpr uint32_t afterTmpOrder(Move m)const noexcept{ return ((bd ^ m) >> MOVE_LCT_TMPORD) & 1U; }
        
        constexpr uint32_t isAfterTmpOrderReversed(Move m)const noexcept{ return (bd ^ m) & (1U << MOVE_LCT_TMPORD); }
        constexpr uint32_t isAfterPrmOrderReversed(Move m)const noexcept{ return (bd ^ m) & (1U << MOVE_LCT_PRMORD); }
            
        constexpr bool afterSuitsLocked(Move m)const noexcept{
            return suitsLocked() || locksSuits(m);
        }
        
        // get
        constexpr uint32_t prmOrder()   const noexcept{ return (bd >> MOVE_LCT_PRMORD) & 1U; }
        constexpr uint32_t tmpOrder()   const noexcept{ return (bd >> MOVE_LCT_TMPORD) & 1U; }
        constexpr uint32_t suits()      const noexcept{ return (bd >> MOVE_LCT_SUITS) & 15U; }
        constexpr uint32_t qty()        const noexcept{ return (bd >> MOVE_LCT_QTY) & 15U; }
        constexpr uint32_t rank()       const noexcept{ return (bd >> MOVE_LCT_RANK) & 15U; }
        constexpr uint32_t rank4x()     const noexcept{ return (bd >> MOVE_LCT_RANK4X) & (15U << 2); } // 4倍型
        constexpr uint32_t jokerRank()  const noexcept{ return (bd >> MOVE_LCT_JKRANK) & 15U; }
        constexpr uint32_t jokerRank4x()const noexcept{ return (bd >> MOVE_LCT_JKRANK4X) & (15U << 2); } // 4倍型
        constexpr uint32_t jokerSuits() const noexcept{ return (bd >> MOVE_LCT_JKSUITS) & 15U; }
        
        // 部分に着目する
        constexpr uint32_t orderPart()     const noexcept{ return bd & MOVE_FLAG_ORD; }
        constexpr uint32_t exceptOrderPart()const noexcept{ return bd & ~MOVE_FLAG_ORD; }
        constexpr uint32_t suitsPart()     const noexcept{ return bd & MOVE_FLAG_SUITS; }
        constexpr uint32_t rankPart()      const noexcept{ return bd & MOVE_FLAG_RANK; }
        constexpr uint32_t qtyPart()       const noexcept{ return bd & MOVE_FLAG_QTY; }
        constexpr uint32_t typePart()      const noexcept{ return bd & MOVE_FLAG_TYPE; } // サイズ＋形式
        constexpr uint32_t jokerPart()     const noexcept{ return bd & MOVE_FLAG_JK; } // ジョーカー関連
        constexpr uint32_t exeptJokerPart()const noexcept{ return bd & ~MOVE_FLAG_JK; } // ジョーカー関連以外
        
        // true or false
        constexpr bool isNull()const noexcept{ return !isRF(); }
        constexpr bool isNF()const noexcept{ return !isRF(); }
        constexpr uint32_t isRF()const noexcept{ return bd & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP | MOVE_FLAG_SEQ); }
        constexpr uint32_t suitsLocked()const noexcept{ return bd & MOVE_FLAG_SUITSLOCK; }
        constexpr uint32_t rankLocked()const noexcept{ return bd & MOVE_FLAG_RANKLOCK; }
        
        constexpr uint32_t isTmpOrderRev()const noexcept{ return bd & MOVE_FLAG_TMPORD; }
        constexpr uint32_t isPrmOrderRev()const noexcept{ return bd & MOVE_FLAG_PRMORD; }
        
        constexpr uint32_t containsJOKER()const noexcept{ return bd & MOVE_FLAG_JK; }
        
        constexpr bool isSingleJOKER()const noexcept{ return (bd & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        constexpr bool isS3Flush()const noexcept{ return holdsBits(bd, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM)); }
        constexpr uint32_t domInevitably()const noexcept{ return bd & MOVE_FLAG_INEVITDOM; }
        
        constexpr uint32_t isSeq()const noexcept{ return bd & MOVE_FLAG_SEQ; }
        constexpr uint32_t isGroup()const noexcept{ return bd & MOVE_FLAG_GROUP; }
        constexpr uint32_t isSingle()const noexcept{ return bd & MOVE_FLAG_SINGLE; }
        constexpr uint32_t isSingleOrGroup()const noexcept{ return bd & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP); }
        bool isQuintuple()const noexcept{
            return typePart() == (MOVE_FLAG_GROUP | (5U << MOVE_LCT_QTY));
        }
        
        bool isOver5Seq()const noexcept{ // 5枚以上階段
            return isSeq() && (qtyPart() > (4U << MOVE_LCT_QTY));
        }
        
        template<int IS_SEQ = _BOTH>
        bool isSpecialRankSeq()const noexcept{
            if((IS_SEQ == _NO) || ((IS_SEQ != _YES) && !isSeq())){ return false; }
            uint32_t r = rank();
            uint32_t q = qty();
            return (r < RANK_MIN) || (RANK_MAX < (r + q - 1));
        }
        
        int typeNum()const noexcept{
            uint32_t q = qty();
            if(isSeq()){
                if(q >= 6){ return 8; }
                return 2 + q;
            }else{
                if(q >= 5){ return 8; }
                return q;
            }
        }
        
        // 進行
        void procOrder(Move m)noexcept{ bd ^= m.orderPart(); } // オーダーフリップのみ
        
        void flush()noexcept{
            // 一時オーダーを永続オーダーに合わせる
            // TODO: ...現ルールではやらなくてよいので未実装
            bd &= MOVE_FLAG_ORD;
        }
        
        void lockSuits()noexcept{ bd |= MOVE_FLAG_SUITSLOCK; }
        
        void procPASS()const noexcept{}//何もしない
        
        template<int IS_NF = _BOTH, int IS_PASS = _BOTH>
        void proc(Move m)noexcept{ // プレーヤー等は関係なく局面のみ進める
            if(IS_PASS == _YES || ((IS_PASS != _NO) && m.isPASS())){
                procPASS();
            }else{
                if(IS_NF == _YES){
                    procOrder(m);
                    if(m.domInevitably()){ // 無条件完全支配
                        flush();
                    }else{
                        setExceptOrder(m.exceptOrderPart()); // 一時情報入れ替え
                    }
                }else if(IS_NF == _NO){
                    procOrder(m);
                    if(m.domInevitably()){ // 無条件完全支配
                        flush();
                    }else{
                        if(domConditionally(m)){ // 条件付完全支配(Joker->S3のみ)
                            flush();
                        }else{
                            // スートロック
                            if(!suitsLocked()){
                                // スートが一緒だったらロック処理
                                if(locksSuits(m)){ lockSuits(); }
                            }
                            // 一時情報入れ替え
                            bd = (bd & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                            | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD));
                        }
                    }
                }else{ // IS_NF不明
                    procOrder(m);
                    if(m.domInevitably()){ // 無条件完全支配
                        flush();
                    }else{
                        if(isNF()){
                            setExceptOrder(m.exceptOrderPart()); // 一時情報入れ替え
                        }else{
                            if(domConditionally(m)){ // 条件付完全支配(Joker->S3のみ)
                                flush();
                            }else{
                                // スートロック
                                if(!suitsLocked()){
                                    // スートが一緒だったらロック処理
                                    if(locksSuits(m)){ lockSuits(); }
                                }
                                // 一時情報入れ替え
                                bd = (bd & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                                | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD));
                            }
                        }
                    }
                }
            }
        }
        
        template<int IS_NF = _BOTH, int IS_PASS = _BOTH>
        void procAndFlush(Move m)noexcept{
            // 局面を更新し、強引に場を流す
            if(IS_PASS == _NO || ((IS_PASS != _YES) && (!m.isPASS()))){ // パスならオーダーフリップいらず
                procOrder(m); // オーダーフリップ
            }
            if(IS_NF == _NO || ((IS_NF != _YES) && (!isNF()))){
                flush();
            }
        }
        
        void procExceptFlush(Move m)noexcept{
            // 局面を更新するが場を流さない
            procOrder(m);
            
            // スートロック
            if(!suitsLocked()){
                // スートが一緒だったらロック処理
                if(locksSuits(m)){ lockSuits(); }
            }
            
            if(domConditionally(m)){ // Joker->S3のみ
                bd = ((bd & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                      | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD)));
                // ８切りと同じように無条件支配フラグをたてておく
                bd |= MOVE_FLAG_INEVITDOM;
                bd &= ~MOVE_FLAG_CONDDOM; // 代わりに条件フラグは外す
            }
            else{
                bd = (bd & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                      | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD));
            }
        }
    };
    
    constexpr Board OrderToNullBoard(const uint32_t prmOrder)noexcept{
        return Board(prmOrder | (prmOrder << 1));
    }
    
    constexpr Move BoardToMove(const Board& bd)noexcept{
        // 場->場役へと変化させる
        // 場役へとコンバート出来ない部分は変えない
        /*if(isSeq(bd)){
         bd-=(((bd&MOVE_FLAG_QTY)-(1U<<8))<<4);
         }*/
        return Move(bd.bd);
    }
    constexpr Board MoveToBoard(const Move& mv)noexcept{
        // 場->場役へと変化させる
        return Board(mv.data());
    }
    
    std::ostream& operator <<(std::ostream& out, const Board& bd){ // Board出力
        if(bd.isNull()){
            out << "NULL";
        }else{
            Move m = BoardToMove(bd); // 場役へ変化
            out << m;
        }
        // オーダー...一時オーダーのみ
        out << "  Order : ";
        if(bd.tmpOrder() == ORDER_NORMAL){
            out << "NORMAL";
        }else{
            out << "REVERSED";
        }
        out << "  Suits : ";
        if(bd.suitsLocked()){
            out << "LOCKED";
        }else{
            out << "FREE";
        }
        return out;
    }
    
    bool isSubjectivelyValid(Board bd, Move mv, const Cards& c, const uint32_t q){
        // 不完全情報の上での合法性判定
        // c はそのプレーヤーが所持可能なカード
        // q はそのプレーヤーの手札枚数（公開されている情報）
        if(mv.isPASS()){
            return true;
        }
        // 枚数オーバー
        if(mv.qty() > q){ return false; }
        // 持っていないはずの札を使った場合
        if(!holdsCards(c, mv.cards())){ return false; }
        if(bd.isNF()){
        }else{
            if(bd.typePart() != mv.typePart()){ return false; } // 型違い
            if(bd.isSeq()){
                if(!isValidSeqRank(mv.rank(), bd.tmpOrder(), bd.rank(), mv.qty())){
                    return false;
                }
                if(bd.suitsLocked()){
                    if(bd.suits() != mv.suits()){ return false; }
                }
            }else{
                if(bd.isSingleJOKER()){
                    if(!mv.isS3Flush()){ // ジョーカー->S3でなかった
                        return false;
                    }else{
                        return true;
                    }
                }
                if(mv.isSingleJOKER()){
                    if(!bd.isSingle()){ return false; }
                }else{
                    if(!isValidGroupRank(mv.rank(), bd.tmpOrder(), bd.rank())){
                        return false;
                    }
                    if(bd.suitsLocked()){
                        if(bd.suits() != mv.suits()){ return false; }
                    }
                }
            }
        }
        return true;
    }
}