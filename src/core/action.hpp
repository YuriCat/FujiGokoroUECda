#pragma once

// ぱおーん氏、beersong.cのコードをベースにしている部分があります

#include "move.hpp"
#include "hand.hpp"
#include "field.hpp"

namespace UECda {

    /**************************交換生成**************************/

    inline int genChange(Cards *const pc0, const Cards myCards, const int changeQty) {
        Cards *pc = pc0;
        Cards tmp = myCards;
        if (changeQty == 1) {
            for (Cards c : myCards.divide()) *(pc++) = c;
        } else if (changeQty == 2) {
            while (tmp.any()) {
                Cards c = tmp.divide().lowest();
                tmp -= c;
                for (Cards c1 : tmp.divide()) *(pc++) = c | c1;
            }
        } else {
            UNREACHABLE;
        }
        return pc - pc0;
    }
    
    /**************************フォロー着手生成**************************/
    
    // 引数の型がHandとCardsのどちらでもOKにするためのサブルーチン
    inline Cards genNRankInGenFollowGroup(const Cards& c, Cards valid, int q) {
        Cards vc = c & valid;
        return CardsToNR(vc, q);
    }
    
    template <class move_t>
    inline int genFollowSingle(move_t *mv0, const Cards myCards, const Board b) {
        Cards c = myCards;
        move_t *mv = mv0;
        if (b.isSingleJOKER()) {
            if (containsS3(c)) {
                mv->setS3Flush();
                return 1;
            }
            return 0;
        }
        if (containsJOKER(c)) {
            mv->setSingleJOKER();
            c = c.plain();
            mv++;
        }
        if (b.suitsLocked()) c &= SuitsToCards(b.suits());
        if (c) {
            int br4x = b.rank4x();
            if (b.order() == 0) { // normal order
                c &= RankRange4xToCards(br4x + 4, RANK_MAX * 4);
            } else { // reversed
                c &= RankRange4xToCards(RANK_MIN * 4, br4x - 4);
            }
            for (IntCard ic : c) {
                mv->setNULL();
                mv->setSingleByIntCard(ic);
                mv++;
            }
        }
        return mv - mv0;
    }
    
#define GEN_BASE(q, s, op) mv->setNULL();\
    mv->setGroupByRank4x(q, r4x, s);op;mv++;
    
#define GEN(q, s) GEN_BASE(q, s,)
#define GEN_J(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js))
    
    template <class move_t>
    inline int genFollowDouble(move_t *const mv0, const Cards hand, const Board b) {
        const Cards x = hand;
        const int br4x = b.rank4x();
        Cards valid;
        if (b.order() == 0) { // 通常
            valid = RankRange4xToCards(br4x + 4, RANK_MAX * 4);
        } else { // オーダー逆転中
            valid = RankRange4xToCards(RANK_MIN * 4, br4x - 4);
        }
        move_t *mv = mv0;
        if (!b.suitsLocked()) { // スートしばりなし
            Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
            // 4枚ある箇所から各スートで生成
            for (IntCard ic : c4) {
                int r4x = IntCardToRank4x(ic);
                GEN(2, SUITS_CD);
                GEN(2, SUITS_CH);
                GEN(2, SUITS_CS);
                GEN(2, SUITS_DH);
                GEN(2, SUITS_DS);
                GEN(2, SUITS_HS);
            }
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            Cards c2 = genNRankInGenFollowGroup(hand, valid, 2);
            if (!containsJOKER(hand)) {
                // 3枚だけの箇所を生成
                for (IntCard ic : c3) {
                    int r4x = IntCardToRank4x(ic);
                    uint32_t suits = CardsRank4xToSuits(x, r4x);
                    uint32_t s0 = lowestBit(suits);
                    uint32_t s1 = lowestBit(suits - s0);
                    uint32_t s2 = suits - s0 - s1;
                    
                    GEN(2, s0 | s1);
                    GEN(2, s1 | s2);
                    GEN(2, s2 | s0);
                }
                // 2枚だけの箇所を生成
                for (IntCard ic : c2) {
                    int r4x = IntCardToRank4x(ic);
                    GEN(2, CardsRank4xToSuits(x, r4x));
                }
            } else {
                // ジョーカーあり
                for (IntCard ic : c3) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t s0 = lowestBit(suits);
                    const uint32_t s1 = lowestBit(suits - s0);
                    const uint32_t s2 = suits - s0 - s1;
                    
                    // プレーン
                    GEN(2, s0 | s1);
                    GEN(2, s1 | s2);
                    GEN(2, s2 | s0);
                    
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    GEN_J(2, s0 | isuits, isuits);
                    GEN_J(2, s1 | isuits, isuits);
                    GEN_J(2, s2 | isuits, isuits);
                }
                for (IntCard ic : c2) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t s0 = lowestBit(suits);
                    const uint32_t s1 = suits - s0;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t is0 = lowestBit(isuits);
                    const uint32_t is1 = isuits - is0;
                    
                    // プレーン
                    GEN(2, suits);
                    
                    // ジョーカー使用
                    GEN_J(2, s0 | is0, is0);
                    GEN_J(2, s0 | is1, is1);
                    GEN_J(2, s1 | is0, is0);
                    GEN_J(2, s1 | is1, is1);
                }
                // 1枚だけの箇所を生成
                Cards c1 = genNRankInGenFollowGroup(hand, valid, 1);
                for (IntCard ic : c1) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t is0 = lowestBit(isuits);
                    const uint32_t is1 = lowestBit(isuits - is0);
                    const uint32_t is2 = isuits - is0 - is1;
                    // ジョーカー使用
                    GEN_J(2, suits | is0, is0);
                    GEN_J(2, suits | is1, is1);
                    GEN_J(2, suits | is2, is2);
                }
            }
        } else { // スートしばりあり
            const Cards c = x & valid;
            const uint32_t suits = b.suits();
            const Cards vc = c | SuitsToCards(SUITS_ALL - suits);
            // ランクごとに4ビット全て立っているか判定
            Cards c4 = CardsToFR(vc);
            for (IntCard ic : c4) {
                const int r4x = IntCardToRank4x(ic);
                GEN(2, suits);
            }
            if (containsJOKER(hand)) {
                // 3ビット立っている部分からジョーカーを利用して生成
                Cards c3 = CardsTo3R(vc);
                for (IntCard ic : c3) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suit = (x >> r4x) & suits;
                    GEN_J(2, suits, suits - suit);
                }
            }
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genFollowTriple(move_t *const mv0, const Cards hand, const Board b) {
        const int br4x = b.rank4x();
        const Cards x = Cards(hand);
        Cards valid;
        if (b.order() == 0) { // 通常
            valid = RankRange4xToCards(br4x + 4, RANK_MAX * 4);
        } else { // オーダー逆転中
            valid = RankRange4xToCards(RANK_MIN * 4, br4x - 4);
        }
        move_t *mv = mv0;
        
        if (!b.suitsLocked()) { // スートしばりなし
            Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
            // 4枚ある箇所から各スートで生成
            for (IntCard ic : c4) {
                int r4x = IntCardToRank4x(ic);
                for (uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1) {
                    GEN(3, SUITS_ALL - s);
                }
            }
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            if (!containsJOKER(hand)) {
                // ジョーカーなし スートロックなし
                // 3枚だけの箇所を生成
                for (IntCard ic : c3) {
                    int r4x = IntCardToRank4x(ic);
                    GEN(3, (x >> r4x) & SUITS_ALL);
                }
            } else {
                // ジョーカーあり スートロックなし
                // 3枚だけの箇所を生成
                for (IntCard ic : c3) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    // プレーン
                    GEN(3, suits);
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    for (uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1) {
                        if (s != isuits) {
                            const uint32_t is = SUITS_ALL - s;
                            GEN_J(3, is, isuits);
                        }
                    }
                }
                // 2枚だけの箇所を生成
                Cards c2 = genNRankInGenFollowGroup(hand, valid, 2);
                for (IntCard ic : c2) {
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = (x >> r4x) & SUITS_ALL;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t is0 = lowestBit(isuits);
                    const uint32_t is1 = isuits - is0;
                    // ジョーカー使用
                    GEN_J(3, suits | is0, is0);
                    GEN_J(3, suits | is1, is1);
                }
            }
        } else { // スートしばりあり
            const Cards c = Cards(hand) & valid;
            const uint32_t suits = b.suits();
            // virtual cardを加えて4枚ある箇所から生成
            const Cards vc = c | SuitsToCards(SUITS_ALL - suits);
            // ランクごとに4ビット全て立っているか判定
            Cards c4 = CardsToFR(vc);
            // ジョーカーあり スートロックあり
            // virtual cardを加えて4枚ある箇所からプレーンで生成
            for (IntCard ic : c4) {
                const int r4x = IntCardToRank4x(ic);
                GEN(3, suits);
            }
            if (containsJOKER(hand)) {
                // 3枚だけの箇所をジョーカー込みで生成
                Cards c3 = CardsTo3R(vc);
                for (IntCard ic : c3) {
                    const int r4x = IntCardToRank4x(ic);
                    GEN_J(3, suits, SUITS_ALL & ~CardsRank4xToSuits(vc, r4x));
                }
            }
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genFollowQuadruple(move_t *const mv0, const Cards hand, const Board b) {
        move_t *mv = mv0;
        const Cards x = Cards(hand);
        const int br4x = b.rank4x();
        Cards valid;
        if (b.order() == 0) { // 通常
            valid = RankRange4xToCards(br4x + 4, RANK_MAX * 4);
        } else { // オーダー逆転中
            valid = RankRange4xToCards(RANK_MIN * 4, br4x - 4);
        }
        Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
        for (IntCard ic : c4) {
            int r4x = IntCardToRank4x(ic);
            GEN(4, SUITS_CDHS);
        }
        if (containsJOKER(hand)) {
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            for (IntCard ic : c3) {
                int r4x = IntCardToRank4x(ic);
                GEN_J(4, SUITS_CDHS, ~(uint32_t)(x >> r4x) & SUITS_CDHS);
            }
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genAllSingle(move_t *const mv0, const Cards hand) {
        Cards c = Cards(hand);
        move_t *mv = mv0;
        if (containsJOKER(c)) {
            mv->setSingleJOKER();
            c = c.plain();
            mv++;
        }
        for (IntCard ic : c) {
            mv->setNULL();
            mv->setSingle(IntCardToRank(ic), IntCardToSuits(ic));
            mv++;
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genLead(move_t *const mv0, const Cards hand) {
        const Cards c = Cards(hand);
        bool jk = containsJOKER(c) ? true : false;
        move_t *mv = mv0 + genAllSingle(mv0, c); // シングルはここで生成
        Cards x;
        if (jk) {
            x = maskJOKER(c);
        } else {
            x = BitCards(CardsToQR(c)) & PQR_234;
        }
        while (x) {
            const int r4x = IntCardToRank4x(pickIntCardLow(x));
            switch (uint32_t(c >> r4x) & 15U) {
                case 0: UNREACHABLE; break;
                case 1: {
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 5, 4);
                    GEN_J(2, 9, 8);
                } break;
                case 2: {
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 10, 8);
                } break;
                case 3: {
                    GEN(2, 3);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 4);
                    GEN_J(2, 9, 8);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 10, 8);
                    GEN_J(3, 7, 4);
                    GEN_J(3, 11, 8);
                } break;
                case 4: {
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 1);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 12, 8);
                } break;
                case 5: {
                    GEN(2, 5);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 9, 8);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 7, 2);
                    GEN_J(3, 13, 8);
                } break;
                case 6: {
                    GEN(2, 6);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 10, 8);
                    GEN_J(2, 5, 1);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 7, 1);
                    GEN_J(3, 14, 8);
                } break;
                case 7: {
                    GEN(2, 3);
                    GEN(2, 5);
                    GEN(2, 6);
                    GEN(3, 7);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 9, 8);
                    GEN_J(2, 10, 8);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 11, 8);
                    GEN_J(3, 13, 8);
                    GEN_J(3, 14, 8);
                    GEN_J(4, 15, 8);
                } break;
                case 8: {
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 9, 1);
                    GEN_J(2, 10, 2);
                    GEN_J(2, 12, 4);
                } break;
                case 9: {
                    GEN(2, 9);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 5, 4);
                    GEN_J(2, 10, 2);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 11, 2);
                    GEN_J(3, 13, 4);
                } break;
                case 10: {
                    GEN(2, 10);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 9, 1);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 11, 1);
                    GEN_J(3, 14, 4);
                } break;
                case 11: {
                    GEN(2, 3);
                    GEN(2, 9);
                    GEN(2, 10);
                    GEN(3, 11);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 4);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 7, 4);
                    GEN_J(3, 13, 4);
                    GEN_J(3, 14, 4);
                    GEN_J(4, 15, 4);
                } break;
                case 12: {
                    GEN(2, 12);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 1);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 9, 1);
                    GEN_J(2, 10, 2);
                    GEN_J(3, 13, 1);
                    GEN_J(3, 14, 2);
                } break;
                case 13: {
                    GEN(2, 5);
                    GEN(2, 9);
                    GEN(2, 12);
                    GEN(3, 13);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 10, 2);
                    GEN_J(3, 7, 2);
                    GEN_J(3, 11, 2);
                    GEN_J(3, 14, 2);
                    GEN_J(4, 15, 2);
                } break;
                case 14: {
                    GEN(2, 6);
                    GEN(2, 10);
                    GEN(2, 12);
                    GEN(3, 14);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 5, 1);
                    GEN_J(2, 9, 1);
                    GEN_J(3, 7, 1);
                    GEN_J(3, 11, 1);
                    GEN_J(3, 13, 1);
                    GEN_J(4, 15, 1);
                } break;
                case 15: {
                    GEN(2, 3);
                    GEN(2, 5);
                    GEN(2, 6);
                    GEN(2, 9);
                    GEN(2, 10);
                    GEN(2, 12);
                    GEN(3, 7);
                    GEN(3, 11);
                    GEN(3, 13);
                    GEN(3, 14);
                    GEN(4, 15);
                    if (!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(5, 15, 15);
                } break;
                default: UNREACHABLE; break;
            }
            x = maskCards(x, Rank4xToCards(r4x));
        }
        mv += genAllSeq(mv, c); // 階段を生成
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genLegalLead(move_t *const mv0, const Cards hand) {
        int cnt = genLead(mv0, hand);
        (mv0 + cnt)->setNULL();
        return cnt + 1;
    }
    
#undef GEN_BASE
    
#undef GEN
#undef GEN_J
    
#define GEN_BASE(q, oc, op) {\
for (IntCard ic : (oc)) {\
int r = IntCardToRank(ic);\
uint32_t s = IntCardToSuits(ic);\
mv->setNULL(); mv->setSeq(q, r, s);\
op; mv++; }}

#define GEN(q, oc) GEN_BASE(q, oc,)
#define GEN_J(q, oc, jdr) GEN_BASE(q, oc, mv->setJokerRank(r + jdr); mv->setJokerSuits(s))
    
    template <class move_t>
    inline int genAllSeqWithJoker(move_t *const mv0, const Cards x) {
        Cards c = maskJOKER(x);
        if (!c) return 0;
        move_t *mv = mv0;
        
        // 3枚階段
        const Cards c1_1 = polymJump(c);
        const Cards c2 = polymRanks<2>(c);
        const Cards c3 = c1_1 & c2;
        // プレーンを作成
        const Cards seq3 = c3;
        GEN(3, seq3);
        // 1_1型を作成
        const Cards seq1_1 = c1_1 & ~seq3;
        GEN_J(3, seq1_1, 1);
        // 2型を両側で作成
        const Cards seq2_ = c2 & ~seq3;
        const Cards seq_2 = (c2 >> 4) & ~seq3;
        GEN_J(3, seq_2, 0);
        GEN_J(3, seq2_, 2);
        
        // 4枚階段
        const Cards c2_1 = c2 & (c1_1 >> 4);
        const Cards c1_2 = c1_1 & (c2 >> 8);
        const Cards c4 = c3 & c2_1;
        // プレーンを作成
        const Cards seq4 = c4;
        GEN(4, seq4);
        // 2_1型と1_2型を作成
        const Cards seq2_1 = c2_1 & ~seq4;
        const Cards seq1_2 = c1_2 & ~seq4;
        GEN_J(4, seq2_1, 2);
        GEN_J(4, seq1_2, 1);
        // 3型を両側で作成
        const Cards seq3_ = c3 & ~seq4;
        const Cards seq_3 = (c3 >> 4) & ~seq4;
        GEN_J(4, seq_3, 0);
        GEN_J(4, seq3_, 3);
        
        // 5枚階段
        const Cards c3_1 = c3 & (c2_1 >> 4);
        const Cards c1_3 = c1_2 & (c3 >> 8);
        const Cards c2_2 = c2_1 & (c1_2 >> 4);
        const Cards c5 = c4 & c3_1;
        // プレーンを作成
        const Cards seq5 = c5;
        GEN(5, seq5);
        // 2_2型を作成
        const Cards seq2_2 = c2_2 & ~seq5;
        GEN_J(5, seq2_2, 2);
        // 3_1型と1_3型を作成
        const Cards seq3_1 = c3_1 & ~seq5;
        const Cards seq1_3 = c1_3 & ~seq5;
        GEN_J(5, seq3_1, 3);
        GEN_J(5, seq1_3, 1);
        // 4型を両側で作成
        const Cards seq4_ = c4 & ~seq5;
        const Cards seq_4 = (c4 >> 4) & ~seq5;
        GEN_J(5, seq_4, 0);
        GEN_J(5, seq4_, 4);
        
        // 6枚階段
        if (c3) {
            const Cards c4_1 = c4 & (c3_1 >> 4);
            const Cards c1_4 = c1_3 & (c4 >> 8);
            const Cards c3_2 = c3_1 & (c2_2 >> 4);
            const Cards c2_3 = c2_2 & (c1_3 >> 4);
            const Cards c6 = c5 & c4_1;
            // プレーンを作成
            const Cards seq6 = c6;
            GEN(6, seq6);
            // 3_2型と2_3型を作成
            const Cards seq3_2 = c3_2 & ~seq6;
            const Cards seq2_3 = c2_3 & ~seq6;
            GEN_J(6, seq3_2, 3);
            GEN_J(6, seq2_3, 2);
            // 4_1型と1_4型を作成
            const Cards seq4_1 = c4_1 & ~seq6;
            const Cards seq1_4 = c1_4 & ~seq6;
            GEN_J(6, seq4_1, 4);
            GEN_J(6, seq1_4, 1);
            // 5型を両側で作成
            const Cards seq5_ = c5 & ~seq6;
            const Cards seq_5 = (c5 >> 4) & ~seq6;
            GEN_J(6, seq_5, 0);
            GEN_J(6, seq5_, 5);
        }
        
        // 7枚以上階段は未実装
        return mv - mv0;
    }
    
    template <class move_t>
    int genAllPlainSeq(move_t *const mv0, const Cards x) {
        // ジョーカーを入れない階段のみ生成
        // 生成ランク制限無し
        move_t *mv = mv0;
        Cards c = polymRanks<3>(x);
        for (int q = 3; anyCards(c); q++) {
            for (IntCard ic : c) {
                int rank = IntCardToRank(ic);
                uint32_t suit = IntCardToSuits(ic);
                mv->setNULL();
                mv->setSeq(q, rank, suit);
                mv++;
            }
            // 次の枚数を考える
            c = polymRanks<2>(c); // ランク a かつランク a + 1 の n 枚階段があればランク a の n + 1 枚階段が出来る
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genAllSeq(move_t *const mv, const Cards c) {
        if (!containsJOKER(c)) return genAllPlainSeq(mv, c);
        else return genAllSeqWithJoker(mv, c);
    }
    
    template <class move_t>
    inline int genFollowSeq(move_t *const mv, const Cards c, const Board b) {
        if (!containsJOKER(c)) return genFollowPlainSeq(mv, c, b);
        else return genFollowSeqWithJoker(mv, c, b);
    }
    
    template <class move_t>
    inline int genFollowPlainSeq(move_t *const mv0, Cards c, Board b) {
        uint32_t r = b.rank();
        uint32_t q = b.qty();
        assert(q >= 3);
        if (b.order() == 0) { // 通常
            if (r + (q << 1) > RANK_MAX + 1) return 0;
            c &= RankRangeToCards(r + q, RANK_MAX);
        } else { // オーダー逆転中
            if (r < RANK_MIN + q) return 0;
            c &= RankRangeToCards(RANK_MIN, r - 1);
        }
        c = polymRanks<3>(c);
        if (!c) return 0;
        if (q > 3) c = polymRanks(c, q - 3 + 1);
        if (!c) return 0;
        if (b.suitsLocked()) c &= SuitsToCards(b.suits());
        move_t *mv = mv0;
        for (IntCard ic : c) {
            int r = IntCardToRank(ic);
            uint32_t s = IntCardToSuits(ic);
            mv->setNULL();
            mv->setSeq(q, r, s);
            mv++;
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genFollowSeqWithJoker(move_t *const mv0, const Cards plain, const Board b) {
        const uint32_t r = b.rank();
        const uint32_t qty = b.qty();
        assert(qty >= 3);
        Cards c = plain;
        Cards rCards; // 合法カードゾーン
        Cards validSeqZone; // 合法階段ゾーン
        if (b.order() == 0) { // 通常
            if (r + (qty << 1) > RANK_MAX + 2) return 0;
            rCards = RankRangeToCards(r + qty, RANK_MAX + 1);
            validSeqZone = RankRangeToCards(r + qty, RANK_MAX + 1);
        } else { // オーダー逆転中
            if (r < qty + RANK_MIN - 1) return 0;
            rCards = RankRangeToCards(RANK_MIN - 1, r - 1);
            validSeqZone = RankRangeToCards(RANK_MIN - 1, r - qty);
        }
        c &= rCards;
        if (b.suitsLocked()) c &= SuitsToCards(b.suits());
        if (!c) return 0;
        move_t *mv = mv0;
        switch (qty) {
            case 0: UNREACHABLE; break;
            case 1: UNREACHABLE; break;
            case 2: UNREACHABLE; break;
            case 3: {
                const Cards c1_1 = polymJump(c);
                const Cards c2 = polymRanks<2>(c);
                
                const Cards c3 = c1_1 & c2;
                
                // プレーンを作成
                const Cards seq3 = c3 & validSeqZone;
                GEN(3, seq3);
                // 1_1型を作成
                const Cards seq1_1 = c1_1 & validSeqZone & ~seq3;
                GEN_J(3, seq1_1, 1);
                // 2型を両側で作成
                const Cards seq2_ = c2 & validSeqZone & ~seq3;
                const Cards seq_2 = (c2 >> 4) & validSeqZone & ~seq3;
                GEN_J(3, seq_2, 0);
                GEN_J(3, seq2_, 2);
            } break;
            case 4: {
                const Cards c1_1 = polymJump(c);
                const Cards c2 = polymRanks<2>(c);
                
                const Cards c3 = c1_1 & c2;
                const Cards c2_1 = c2 & (c1_1 >> 4);
                const Cards c1_2 = c1_1 & (c2 >> 8);
                
                const Cards c4 = c3 & c2_1;
                
                // プレーンを作成
                const Cards seq4 = c4 & validSeqZone;
                GEN(4, seq4);
                // 2_1型と1_2型を作成
                const Cards seq2_1 = c2_1 & validSeqZone & ~seq4;
                const Cards seq1_2 = c1_2 & validSeqZone & ~seq4;
                GEN_J(4, seq2_1, 2);
                GEN_J(4, seq1_2, 1);
                // 3型を両側で作成
                const Cards seq3_ = c3 & validSeqZone & ~seq4;
                const Cards seq_3 = (c3 >> 4) & validSeqZone & ~seq4;
                GEN_J(4, seq3_, 3);
                GEN_J(4, seq_3, 0);
            } break;
            case 5: {
                const Cards c1_1 = polymJump(c);
                const Cards c2 = polymRanks<2>(c);
                
                const Cards c3 = c1_1 & c2;
                
                if (!c3) break;
                
                const Cards c2_1 = c2 & (c1_1 >> 4);
                const Cards c1_2 = c1_1 & (c2 >> 8);
                
                const Cards c4 = c3 & c2_1;
                const Cards c3_1 = c3 & (c2_1 >> 4);
                const Cards c1_3 = c1_2 & (c3 >> 8);
                const Cards c2_2 = c2_1 & (c1_2 >> 4);
                
                const Cards c5 = c4 & c3_1;
                
                // プレーンを作成
                const Cards seq5 = c5 & validSeqZone;
                GEN(5, seq5);
                // 2_2型を作成
                const Cards seq2_2 = c2_2 & validSeqZone & ~seq5;
                GEN_J(5, seq2_2, 2);
                // 3_1型と1_3型を作成
                const Cards seq3_1 = c3_1 & validSeqZone & ~seq5;
                const Cards seq1_3 = c1_3 & validSeqZone & ~seq5;
                GEN_J(5, seq3_1, 3);
                GEN_J(5, seq1_3, 1);
                // 4型を両側で作成
                const Cards seq4_ = c4 & validSeqZone & ~seq5;
                const Cards seq_4 = (c4 >> 4) & validSeqZone & ~seq5;
                GEN_J(5, seq_4, 0);
                GEN_J(5, seq4_, 4);
            } break;
            default: break; // 6枚以上階段は未実装
        }
        return mv - mv0;
    }
    
#undef GEN_BASE
#undef GEN
#undef GEN_J
    
    /**************************特別着手生成**************************/
    
    // 通常の着手生成では無視する着手のうち、
    // クライアントの手としては検討するものを生成
    
    // ジョーカーを使わずに作る事が出来る着手を
    // ジョーカーによって出す着手
    // 8か最強ランクであり、
    // 本来そのランクが無くなるはずの着手についてのみ検討する
    
    template <class move_t>
    inline int genNullPass(move_t *const mv0) {
        // 空場でのパスを生成
        mv0->setNULL();
        return 1;
    }
    
#define GEN_BASE(q, s, op) {mv->setNULL();\
mv->setGroup(q, r, s); op; mv++;}

#define GEN_J(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js))
    
    template <class move_t>
    inline int genJokerGroup(move_t *const mv0, Cards c, const Cards ops, const Board b) {
        
        assert(containsJOKER(c));
        assert(containsS3(ops));
        assert(b.isGroup());
        
        move_t *mv = mv0;
        int br = b.rank();
        if (b.order() == 0) { // 通常
            c &= RankRangeToCards(br + 1, RANK_MAX);
        } else { // オーダー逆転中
            c &= RankRangeToCards(RANK_MIN , br - 1);
        }
        uint32_t s = b.suits();
        int tmpOrd = b.order();
        
        for (int r = RANK_3; r <= RANK_2; r++) {
            int q = countCards(c & RankToCards(r));
            if (q == b.qty()) {
                unsigned x = c[r];
                if (r != RANK_8) {
                    // 相手が返せるかどうかチェック
                    if (tmpOrd) {
                        if (x == 15) {
                            if (!holdsCards(RankRangeToCards(RANK_3 , r), ops)) continue;
                        } else {
                            if (!holdsCards(RankRangeToCards(r, RANK_2), ops)) continue;
                        }
                    } else {
                        if (x == 15) {
                            if (!holdsCards(RankRangeToCards(r, RANK_2), ops)) continue;
                        } else {
                            if (!holdsCards(RankRangeToCards(RANK_3, r), ops)) continue;
                        }
                    }
                }
                
                switch (x) {
                    case 0: break;
                    case 1: break;
                    case 2: break;
                    case 3: {
                        if (b.suitsLocked() && s != 3) break;
                        GEN_J(2, 3, 2);
                        GEN_J(2, 3, 1);
                    } break;
                    case 4: break;
                    case 5: {
                        if (b.suitsLocked() && s != 5) break;
                        GEN_J(2, 5, 1);
                        GEN_J(2, 5, 4);
                    } break;
                    case 6: {
                        if (b.suitsLocked() && s != 6) break;
                        GEN_J(2, 6, 2);
                        GEN_J(2, 6, 4);
                    } break;
                    case 7: {
                        if (b.suitsLocked() && s != 7) break;
                        GEN_J(3, 7, 1);
                        GEN_J(3, 7, 2);
                        GEN_J(3, 7, 4);
                    } break;
                    case 8: break;
                    case 9: {
                        if (b.suitsLocked() && s != 9) break;
                        GEN_J(2, 9, 1);
                        GEN_J(2, 9, 8);
                    } break;
                    case 10: {
                        if (b.suitsLocked() && s != 10) break;
                        GEN_J(2, 10, 2);
                        GEN_J(2, 10, 8);
                    } break;
                    case 11: {
                        if (b.suitsLocked() && s != 11) break;
                        GEN_J(3, 11, 1);
                        GEN_J(3, 11, 2);
                        GEN_J(3, 11, 8);
                    } break;
                    case 12: {
                        if (b.suitsLocked() && s != 12) break;
                        GEN_J(2, 12, 4);
                        GEN_J(2, 12, 8);
                    } break;
                    case 13: { 
                        if (b.suitsLocked() && s != 13) break;
                        GEN_J(3, 13, 1);
                        GEN_J(3, 13, 4);
                        GEN_J(3, 13, 8);
                    } break;
                    case 14: {
                        if (b.suitsLocked() && s != 14) break;
                        GEN_J(3, 14, 2);
                        GEN_J(3, 14, 4);
                        GEN_J(3, 14, 8);
                    } break;
                    case 15: {
                        if (b.suitsLocked() && s != 15) break;
                        GEN_J(4, 15, 1);
                        GEN_J(4, 15, 2);
                        GEN_J(4, 15, 4);
                        GEN_J(4, 15, 8);
                    } break;
                    default: UNREACHABLE; break;
                }
            }
        }
        return mv - mv0;
    }
#undef GEN_BASE
#undef GEN_J
    
    template <class move_t>
    inline int genGroupDebug(move_t *const mv0, Cards c, Board b) {
        move_t *mv = mv0;
        int st, ed;
        // 速度は問わず、全ての合法着手を生成
        if (b.isNull()) {
            st = RANK_MIN; ed = RANK_MAX;
        } else {
            int br = b.rank();
            if (b.order() == 0) { // 通常
                st = br + 1; ed = RANK_MAX;
            } else { // オーダー逆転中
                st = RANK_MIN; ed = br - 1;
            }
        }
        for (int r = st; r <= ed; r++) {
            for (uint32_t s = 1; s <= SUITS_ALL; s++) {
                if (b.isNull() || b.suits() == s) {
                    int q = countSuits(s);
                    if (holdsCards(c, RankSuitsToCards(r, s))) {
                        mv->setNULL();
                        mv->setGroup(r, s, countSuits(s));
                        mv++;
                    } else if (containsJOKER(c)) {
                        for (uint32_t js = SUIT_MIN; s <= SUIT_MAX; s <<= 1) {
                            if ((s & js) && holdsCards(c, RankSuitsToCards(r, s - js))) {
                                mv->setNULL();
                                mv->setGroup(r, s, countSuits(s));
                                mv->setJokerSuits(js);
                                mv++;
                            }
                        }
                    }
                }
            }
        }
        return mv - mv0;
    }
    
    template <class move_t>
    inline int genFollowExceptPASS(move_t *const mv, const Cards c, const Board b) {
        // 返り値は、生成した手の数
        int ret = 0;
        if (!b.isSeq()) {
            int ret;
            switch (b.qty()) {
                case 0: ret = 0; break;
                case 1: ret = genFollowSingle(mv, c, b); break;
                case 2: ret = genFollowDouble(mv, c, b); break;
                case 3: ret = genFollowTriple(mv, c, b); break;
                case 4: ret = genFollowQuadruple(mv, c, b); break;
                default: ret = 0; break;
            }
            return ret;
        } else return genFollowSeq(mv, c, b);
    }
    template <class move_t>
    inline int genFollow(move_t *const mv, const Cards c, const Board b) {
        // 返り値は、生成した手の数
        mv->setNULL();
        return 1 + genFollowExceptPASS(mv + 1, c, b);
    }
    template <class move_t>
    inline int genMove(move_t *mv, const Cards c, const Board b) {
        if (b.isNull()) return genLead(mv, c);
        else return genFollow(mv, c, b);
    }
    template <class move_t>
    inline int genLegal(move_t *mv, const Cards c, const Board b) {
        if (b.isNull()) return genLegalLead(mv, c);
        else return genFollow(mv, c, b);
    }
    inline std::vector<MoveInfo> genMove(const Field& field) {
        std::vector<MoveInfo> moves(N_MAX_MOVES);
        int numMoves = genMove(moves.data(), field.hand[field.turn()], field.board);
        moves.resize(numMoves);
        return moves;
    }
}