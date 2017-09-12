/*
 moveGenerator.hpp
 Katsuki Ohto
 paoon (idea, base)
 */

#pragma once

// ぱおーん氏、beersong.cのコードをベースにしている部分があります

#include "prim.hpp"
#include "hand.hpp"

namespace UECda{
    
    /**************************フォロー着手生成**************************/
    
    // 引数の型がHandとCardsのどちらでもOKにするためのサブルーチン
    template<class hand_t>
    inline Cards genNRankInGenFollowGroup(const hand_t& hand, Cards valid, int q){
        assert(hand.exam_jk()); // ついでにチェック
        assert(hand.exam_pqr());
        Cards vpqr = hand.pqr & valid;
        return vpqr & QtyToPQR(q);
    }
    template<>
    inline Cards genNRankInGenFollowGroup(const Cards& c, Cards valid, int q){
        Cards vc = c & valid;
        return CardsToNR(vc, q);
    }
    
    template<class move_t, class hand_t, class board_t>
    inline int genFollowSingle(move_t *mv, const hand_t& hand, const board_t bd){
        Cards c = Cards(hand);
        if(bd.isSingleJOKER()){
            if(containsS3(c)){
                mv->setS3Flush();
                return 1;
            }
            return 0;
        }
        int cnt;
        if(containsJOKER(c)){
            mv->setSingleJOKER();
            subtrJOKER(&c);
            ++mv;
            cnt = 1;
        }else{
            cnt = 0;
        }
        if(bd.suitsLocked()){ // スートロック
            c &= SuitsToCards(bd.suits());
            if(!c){ return cnt; }
        }
        int br4x = bd.rank4x();
        if(!bd.isTmpOrderRev()){ // normal order
            c &= RankRange4xToCards(br4x + 4, RANK4X_MAX);
        }else{ // reversed
            c &= RankRange4xToCards(RANK4X_MIN, br4x - 4);
        }
        if(c){
            //cnt += countCards(c); // ++cntで調べた方が速い!?
            Cards c8 = c & CARDS_8;
            maskCards(&c, CARDS_8);
            while(c){
                mv->setNULL();
                mv->setSingleByIntCard(popIntCardLow(&c));
                ++mv; ++cnt;
            }
            while(c8){
                IntCard ic = popIntCardLow(&c8);
                mv->setNULL();
                mv->setSingle(RANK_8, IntCardToSuits(ic));
                mv->setEight();
                ++mv; ++cnt;
            }
        }
        return cnt;
    }
    
#define GEN_BASE(q, s, op) mv->setNULL();\
    mv->setGroupByRank4x(q, r4x, s);op;++mv;
#define GEN8_BASE(q, s, op) mv->setNULL();\
    mv->setGroupByRank4x(q, RANK4X_8, s);mv->setEight();op;++mv;
    
#define GEN(q, s) GEN_BASE(q, s,)
#define GEN_J(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js))
#define GEN_R(q, s) GEN_BASE(q, s, mv->setRev())
#define GEN_8(q, s) GEN8_BASE(q, s,)
#define GEN_JR(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js);mv->setRev())
#define GEN_J8(q, s, js) GEN8_BASE(q, s, mv->setJokerSuits(js))
#define GEN_R8(q, s) GEN8_BASE(q, s, mv->setRev())
#define GEN_JR8(q, s, js) GEN8_BASE(q, s, mv->setJokerSuits(js);mv->setRev())
    
    template<class move_t, class hand_t>
    inline int genFollowDouble(move_t *const mv0, const hand_t& hand, const Board bd){
        const Cards x = Cards(hand);
        const int br4x = bd.rank4x();
        Cards valid;
        if(!bd.isTmpOrderRev()){ // 通常
            valid = RankRange4xToCards(br4x + 4, RANK4X_MAX);
        }else{ // オーダー逆転中
            valid = RankRange4xToCards(RANK4X_MIN, br4x - 4);
        }
        move_t *mv = mv0;
        if(!bd.suitsLocked()){ // スートしばりなし
            Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
            // 4枚ある箇所から各スートで生成
            if(c4 & CARDS_8){ // 8切りだけ生成
                GEN_8(2, SUITS_CD);
                GEN_8(2, SUITS_CH);
                GEN_8(2, SUITS_CS);
                GEN_8(2, SUITS_DH);
                GEN_8(2, SUITS_DS);
                GEN_8(2, SUITS_HS);
                maskCards(&c4, CARDS_8);
            }
            while(c4){
                const IntCard ic = popIntCardLow(&c4);
                const int r4x = IntCardToRank4x(ic);
                GEN(2, SUITS_CD);
                GEN(2, SUITS_CH);
                GEN(2, SUITS_CS);
                GEN(2, SUITS_DH);
                GEN(2, SUITS_DS);
                GEN(2, SUITS_HS);
            }
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            Cards c2 = genNRankInGenFollowGroup(hand, valid, 2);
            if(!containsJOKER(hand)){
                // 3枚だけの箇所を生成
                if(c3 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = CardsRank4xToSuits(x, RANK4X_8);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = lowestBit(suits - suit0);
                    const uint32_t suit2 = suits - suit0 - suit1;
                    
                    GEN_8(2, suit0 | suit1);
                    GEN_8(2, suit1 | suit2);
                    GEN_8(2, suit2 | suit0);
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = lowestBit(suits - suit0);
                    const uint32_t suit2 = suits - suit0 - suit1;
                    
                    GEN(2, suit0 | suit1);
                    GEN(2, suit1 | suit2);
                    GEN(2, suit2 | suit0);
                }
                
                // 2枚だけの箇所を生成
                if(c2 & CARDS_8){ // 8切りだけ生成
                    GEN_8(2, CardsRank4xToSuits(x, RANK4X_8));
                    maskCards(&c2, CARDS_8);
                }
                while(c2){
                    const IntCard ic = popIntCardLow(&c2);
                    const int r4x = IntCardToRank4x(ic);
                    GEN(2, CardsRank4xToSuits(x, r4x));
                }
            }else{
                // ジョーカーあり
                if(c3 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = CardsRank4xToSuits(x, RANK4X_8);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = lowestBit(suits - suit0);
                    const uint32_t suit2 = suits - suit0 - suit1;
                    
                    // プレーン
                    GEN_8(2, suit0 | suit1);
                    GEN_8(2, suit1 | suit2);
                    GEN_8(2, suit2 | suit0);
                    
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    GEN_J8(2, suit0 | isuits, isuits);
                    GEN_J8(2, suit1 | isuits, isuits);
                    GEN_J8(2, suit2 | isuits, isuits);
                    
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = lowestBit(suits - suit0);
                    const uint32_t suit2 = suits - suit0 - suit1;
                    
                    // プレーン
                    GEN(2, suit0 | suit1);
                    GEN(2, suit1 | suit2);
                    GEN(2, suit2 | suit0);
                    
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    GEN_J(2, suit0 | isuits, isuits);
                    GEN_J(2, suit1 | isuits, isuits);
                    GEN_J(2, suit2 | isuits, isuits);
                }
                
                // 2枚だけの箇所を生成
                if(c2 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = CardsRank4xToSuits(x, RANK4X_8);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = suits - suit0;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = isuits - isuit0;
                    
                    // プレーン
                    GEN_8(2, suits);
                    
                    // ジョーカー使用
                    GEN_J8(2, suit0 | isuit0, isuit0);
                    GEN_J8(2, suit0 | isuit1, isuit1);
                    GEN_J8(2, suit1 | isuit0, isuit0);
                    GEN_J8(2, suit1 | isuit1, isuit1);
                    
                    maskCards(&c2, CARDS_8);
                }
                while(c2){
                    const IntCard ic = popIntCardLow(&c2);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t suit0 = lowestBit(suits);
                    const uint32_t suit1 = suits - suit0;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = isuits - isuit0;
                    
                    // プレーン
                    GEN(2, suits);
                    
                    // ジョーカー使用
                    GEN_J(2, suit0 | isuit0, isuit0);
                    GEN_J(2, suit0 | isuit1, isuit1);
                    GEN_J(2, suit1 | isuit0, isuit0);
                    GEN_J(2, suit1 | isuit1, isuit1);
                }
                
                // 1枚だけの箇所を生成
                Cards c1 = genNRankInGenFollowGroup(hand, valid, 1);
                if(c1 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = (x >> RANK4X_8) & SUITS_ALL;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = lowestBit(isuits - isuit0);
                    const uint32_t isuit2 = isuits - isuit0 - isuit1;
                    // ジョーカー使用
                    GEN_J8(2, suits | isuit0, isuit0);
                    GEN_J8(2, suits | isuit1, isuit1);
                    GEN_J8(2, suits | isuit2, isuit2);
                    
                    maskCards(&c1, CARDS_8);
                }
                while(c1){
                    const IntCard ic = popIntCardLow(&c1);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = lowestBit(isuits - isuit0);
                    const uint32_t isuit2 = isuits - isuit0 - isuit1;
                    // ジョーカー使用
                    GEN_J(2, suits | isuit0, isuit0);
                    GEN_J(2, suits | isuit1, isuit1);
                    GEN_J(2, suits | isuit2, isuit2);
                }
            }
        }else{ // スートしばりあり
            const Cards c = x & valid;
            const uint32_t suits = bd.suits();
            const Cards vc = c | SuitsToCards(SUITS_ALL - suits);
            // ランクごとに4ビット全て立っているか判定
            Cards c4 = CardsToFR(vc);
            if(c4 & CARDS_8){ // 8切りだけ生成
                GEN_8(2, suits);
                maskCards(&c4, CARDS_8);
            }
            while(c4){
                const IntCard ic = popIntCardLow(&c4);
                const int r4x = IntCardToRank4x(ic);
                GEN(2, suits);
            }
            if(containsJOKER(hand)){
                // 3ビット立っている部分からジョーカーを利用して生成
                Cards c3 = CardsTo3R(vc);
                if(c3 & CARDS_8){
                    const uint32_t suit = (x >> RANK4X_8) & suits;
                    GEN_J8(2, suits, suits - suit);
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suit = (x >> r4x) & suits;
                    GEN_J(2, suits, suits - suit);
                }
            }
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    inline int genFollowTriple(move_t *const mv0, const hand_t& hand, const Board bd){
        const int br4x = bd.rank4x();
        const Cards x = Cards(hand);
        Cards valid;
        if(!bd.isTmpOrderRev()){ // 通常
            valid = RankRange4xToCards(br4x + 4, RANK4X_MAX);
        }else{ // オーダー逆転中
            valid = RankRange4xToCards(RANK4X_MIN, br4x - 4);
        }
        move_t *mv = mv0;
        
        if(!bd.suitsLocked()){ // スートしばりなし
            Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
            // 4枚ある箇所から各スートで生成
            if(c4 & CARDS_8){ // 8切りだけ生成
                for(uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1){
                    GEN_8(3, SUITS_ALL - s);
                }
                maskCards(&c4, CARDS_8);
            }
            while(c4){
                const IntCard ic = popIntCardLow(&c4);
                const int r4x = IntCardToRank4x(ic);
                for(uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1){
                    GEN(3, SUITS_ALL - s);
                }
            }
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            if(!containsJOKER(hand)){
                // ジョーカーなし スートロックなし
                // 3枚だけの箇所を生成
                if(c3 & CARDS_8){ // 8切りだけ生成
                    GEN_8(3, (x >> RANK4X_8) & SUITS_ALL);
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    GEN(3, (x >> r4x) & SUITS_ALL);
                }
            }else{
                // ジョーカーあり スートロックなし
                // 3枚だけの箇所を生成
                if(c3 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = CardsRank4xToSuits(x, RANK4X_8);
                    // プレーン
                    GEN_8(3, suits);
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    for(uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1){
                        if(s != isuits){
                            const uint32_t is = SUITS_ALL - s;
                            GEN_J8(3, is, isuits);
                        }
                    }
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = CardsRank4xToSuits(x, r4x);
                    // プレーン
                    GEN(3, suits);
                    // ジョーカー使用
                    const uint32_t isuits = SUITS_ALL - suits;
                    for(uint32_t s = SUITS_C; s <= SUITS_S; s <<= 1){
                        if(s != isuits){
                            const uint32_t is = SUITS_ALL - s;
                            GEN_J(3, is, isuits);
                        }
                    }
                }
                // 2枚だけの箇所を生成
                Cards c2 = genNRankInGenFollowGroup(hand, valid, 2);
                if(c2 & CARDS_8){ // 8切りだけ生成
                    const uint32_t suits = CardsRank4xToSuits(x, RANK4X_8);
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = isuits - isuit0;
                    // ジョーカー使用
                    GEN_J8(3, suits | isuit0, isuit0);
                    GEN_J8(3, suits | isuit1, isuit1);
                    maskCards(&c2, CARDS_8);
                }
                while(c2){
                    const IntCard ic = popIntCardLow(&c2);
                    const int r4x = IntCardToRank4x(ic);
                    const uint32_t suits = (x >> r4x) & SUITS_ALL;
                    const uint32_t isuits = SUITS_ALL - suits;
                    const uint32_t isuit0 = lowestBit(isuits);
                    const uint32_t isuit1 = isuits - isuit0;
                    // ジョーカー使用
                    GEN_J(3, suits | isuit0, isuit0);
                    GEN_J(3, suits | isuit1, isuit1);
                }
            }
        }else{ // スートしばりあり
            const Cards c = Cards(hand) & valid;
            const uint32_t suits = bd.suits();
            // virtual cardを加えて4枚ある箇所から生成
            const Cards vc = c | SuitsToCards(SUITS_ALL - suits);
            // ランクごとに4ビット全て立っているか判定
            Cards c4 = CardsToFR(vc);
            // ジョーカーあり スートロックあり
            // virtual cardを加えて4枚ある箇所からプレーンで生成
            if(c4 & CARDS_8){ // 8切りだけ生成
                GEN_8(3, suits);
                maskCards(&c4, CARDS_8);
            }
            while(c4){
                const IntCard ic = popIntCardLow(&c4);
                const int r4x = IntCardToRank4x(ic);
                GEN(3, suits);
            }
            if(containsJOKER(hand)){
                // 3枚だけの箇所をジョーカー込みで生成
                Cards c3 = CardsTo3R(vc);
                if(c3 & CARDS_8){ // 8切りだけ生成
                    GEN_J8(3, suits, SUITS_ALL & ~CardsRank4xToSuits(vc, RANK4X_8));
                    maskCards(&c3, CARDS_8);
                }
                while(c3){
                    const IntCard ic = popIntCardLow(&c3);
                    const int r4x = IntCardToRank4x(ic);
                    GEN_J(3, suits, SUITS_ALL & ~CardsRank4xToSuits(vc, r4x));
                }
            }
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    int genFollowQuadruple(move_t *const mv0, const hand_t& hand, const Board bd){
        move_t *mv = mv0;
        const Cards x = Cards(hand);
        const int br4x = bd.rank4x();
        Cards valid;
        if(!bd.isTmpOrderRev()){ // 通常
            valid = RankRange4xToCards(br4x + 4, RANK4X_MAX);
        }else{ // オーダー逆転中
            valid = RankRange4xToCards(RANK4X_MIN, br4x - 4);
        }
        Cards c4 = genNRankInGenFollowGroup(hand, valid, 4);
        Cards c8 = c4 & CARDS_8;
        if(c8){ // 8切りだけ生成
            GEN_R8(4, SUITS_CDHS);
            maskCards(&c4, CARDS_8);
        }
        while(c4){
            IntCard ic = popIntCardLow(&c4);
            int r4x = IntCardToRank4x(ic);
            GEN_R(4, SUITS_CDHS);
        }
        if(containsJOKER(hand)){
            Cards c3 = genNRankInGenFollowGroup(hand, valid, 3);
            Cards c8 = c3 & CARDS_8;
            if(c8){ // 8切りだけ生成
                GEN_JR8(4, SUITS_CDHS, ~(uint32_t)(x >> RANK4X_8) & SUITS_CDHS);
                maskCards(&c3, CARDS_8);
            }
            while(c3){
                IntCard ic = popIntCardLow(&c3);
                int r4x = IntCardToRank4x(ic);
                GEN_JR(4, SUITS_CDHS, ~(uint32_t)(x >> r4x) & SUITS_CDHS);
            }
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    inline int genAllSingle(move_t *const mv0, const hand_t& hand){
        Cards c = Cards(hand);
        move_t *mv = mv0;
        if(containsJOKER(c)){
            mv->setSingleJOKER();
            subtrJOKER(&c);
            ++mv;
        }
        Cards c8 = c & CARDS_8;
        maskCards(&c, CARDS_8);
        while(c){
            IntCard ic = popIntCardLow(&c);
            mv->setNULL();
            mv->setSingle(IntCardToRank(ic), IntCardToSuits(ic));
            ++mv;
        }
        while(c8){
            IntCard ic = popIntCardLow(&c8);
            mv->setNULL();
            mv->setSingle(RANK_8, IntCardToSuits(ic));
            mv->setEight();
            ++mv;
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    int genLead(move_t *const mv0, const hand_t& hand){
        const Cards c = Cards(hand);
        bool jk = containsJOKER(c) ? true : false;
        move_t *mv = mv0 + genAllSingle(mv0, c); // シングルはここで生成
        Cards x;
        if(jk){
            x = maskJOKER(c);
        }else{
            x = CardsToQR(c) & PQR_234;
        }
        
        x = maskCards(x, CARDS_8);
        
        while(x){
            const int r4x = IntCardToRank4x(pickIntCardLow(x));
            switch(uint32_t(c >> r4x) & 15U){
                case 0: UNREACHABLE; break;
                case 1:{
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 5, 4);
                    GEN_J(2, 9, 8);
                }break;
                case 2:{
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 10, 8);
                }break;
                case 3:{
                    GEN(2, 3);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 4);
                    GEN_J(2, 9, 8);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 10, 8);
                    GEN_J(3, 7, 4);
                    GEN_J(3, 11, 8);
                }break;
                case 4:{
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 1);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 12, 8);
                }break;
                case 5:{
                    GEN(2, 5);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 9, 8);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 7, 2);
                    GEN_J(3, 13, 8);
                }break;
                case 6:{
                    GEN(2, 6);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 10, 8);
                    GEN_J(2, 5, 1);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 7, 1);
                    GEN_J(3, 14, 8);
                }break;
                case 7:{
                    GEN(2, 3);
                    GEN(2, 5);
                    GEN(2, 6);
                    GEN(3, 7);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 9, 8);
                    GEN_J(2, 10, 8);
                    GEN_J(2, 12, 8);
                    GEN_J(3, 11, 8);
                    GEN_J(3, 13, 8);
                    GEN_J(3, 14, 8);
                    GEN_JR(4, 15, 8);
                }break;
                case 8:{
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 9, 1);
                    GEN_J(2, 10, 2);
                    GEN_J(2, 12, 4);
                }break;
                case 9:{
                    GEN(2, 9);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 5, 4);
                    GEN_J(2, 10, 2);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 11, 2);
                    GEN_J(3, 13, 4);
                }break;
                case 10: {
                    GEN(2, 10);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 9, 1);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 11, 1);
                    GEN_J(3, 14, 4);
                }break;
                case 11:{
                    GEN(2, 3);
                    GEN(2, 9);
                    GEN(2, 10);
                    GEN(3, 11);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 4);
                    GEN_J(2, 6, 4);
                    GEN_J(2, 12, 4);
                    GEN_J(3, 7, 4);
                    GEN_J(3, 13, 4);
                    GEN_J(3, 14, 4);
                    GEN_JR(4, 15, 4);
                }break;
                case 12:{
                    GEN(2, 12);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 5, 1);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 9, 1);
                    GEN_J(2, 10, 2);
                    GEN_J(3, 13, 1);
                    GEN_J(3, 14, 2);
                }break;
                case 13:{
                    GEN(2, 5);
                    GEN(2, 9);
                    GEN(2, 12);
                    GEN(3, 13);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 2);
                    GEN_J(2, 6, 2);
                    GEN_J(2, 10, 2);
                    GEN_J(3, 7, 2);
                    GEN_J(3, 11, 2);
                    GEN_J(3, 14, 2);
                    GEN_JR(4, 15, 2);
                }break;
                case 14:{
                    GEN(2, 6);
                    GEN(2, 10);
                    GEN(2, 12);
                    GEN(3, 14);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_J(2, 3, 1);
                    GEN_J(2, 5, 1);
                    GEN_J(2, 9, 1);
                    GEN_J(3, 7, 1);
                    GEN_J(3, 11, 1);
                    GEN_J(3, 13, 1);
                    GEN_JR(4, 15, 1);
                }break;
                case 15:{
                    GEN(2, 3);
                    GEN(2, 5);
                    GEN(2, 6);
                    GEN(2, 9);
                    GEN(2, 10);
                    GEN(2, 12);
                    GEN(3, 7);
                    GEN(3, 11);
                    GEN(3, 13);
                    GEN(3, 14)
                    GEN_R(4, 15);
                    if(!jk) break; // ジョーカーがなければ飛ばす
                    GEN_JR(5, 15, 15);
                }break;
                default: UNREACHABLE; break;
            }
            maskCards(&x, Rank4xToCards(r4x));
        }
        
        // 8切りを別に生成
        switch(uint32_t(c >> RANK4X_8) & 15U){
            case 0: break; // ここに来ることもある
            case 1:{
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 2);
                GEN_J8(2, 5, 4);
                GEN_J8(2, 9, 8);
            }break;
            case 2:{
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 1);
                GEN_J8(2, 6, 4);
                GEN_J8(2, 10, 8);
            }break;
            case 3:{
                GEN_8(2, 3);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 5, 4);
                GEN_J8(2, 9, 8);
                GEN_J8(2, 6, 4);
                GEN_J8(2, 10, 8);
                GEN_J8(3, 7, 4);
                GEN_J8(3, 11, 8);
            }break;
            case 4:{
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 5, 1);
                GEN_J8(2, 6, 2);
                GEN_J8(2, 12, 8);
            }break;
            case 5:{
                GEN_8(2, 5);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 2);
                GEN_J8(2, 9, 8);
                GEN_J8(2, 6, 2);
                GEN_J8(2, 12, 8);
                GEN_J8(3, 7, 2);
                GEN_J8(3, 13, 8);
            }break;
            case 6:{
                GEN_8(2, 6);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 1);
                GEN_J8(2, 10, 8);
                GEN_J8(2, 5, 1);
                GEN_J8(2, 12, 8);
                GEN_J8(3, 7, 1);
                GEN_J8(3, 14, 8);
            }break;
            case 7:{
                GEN_8(2, 3);
                GEN_8(2, 5);
                GEN_8(2, 6);
                GEN_8(3, 7);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 9, 8);
                GEN_J8(2, 10, 8);
                GEN_J8(2, 12, 8);
                GEN_J8(3, 11, 8);
                GEN_J8(3, 13, 8);
                GEN_J8(3, 14, 8);
                GEN_JR8(4, 15, 8);
            }break;
            case 8:{
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 9, 1);
                GEN_J8(2, 10, 2);
                GEN_J8(2, 12, 4);
            }break;
            case 9:{
                GEN_8(2, 9);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 2);
                GEN_J8(2, 5, 4);
                GEN_J8(2, 10, 2);
                GEN_J8(2, 12, 4);
                GEN_J8(3, 11, 2);
                GEN_J8(3, 13, 4);
            }break;
            case 10:{
                GEN_8(2, 10);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 1);
                GEN_J8(2, 6, 4);
                GEN_J8(2, 9, 1);
                GEN_J8(2, 12, 4);
                GEN_J8(3, 11, 1);
                GEN_J8(3, 14, 4);
            }break;
            case 11:{
                GEN_8(2, 3);
                GEN_8(2, 9);
                GEN_8(2, 10);
                GEN_8(3, 11);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 5, 4);
                GEN_J8(2, 6, 4);
                GEN_J8(2, 12, 4);
                GEN_J8(3, 7, 4);
                GEN_J8(3, 13, 4);
                GEN_J8(3, 14, 4);
                GEN_JR8(4, 15, 4);
            }break;
            case 12:{
                GEN_8(2, 12);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 5, 1);
                GEN_J8(2, 6, 2);
                GEN_J8(2, 9, 1);
                GEN_J8(2, 10, 2);
                GEN_J8(3, 13, 1);
                GEN_J8(3, 14, 2);
            }break;
            case 13:{
                GEN_8(2, 5);
                GEN_8(2, 9);
                GEN_8(2, 12);
                GEN_8(3, 13);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 2);
                GEN_J8(2, 6, 2);
                GEN_J8(2, 10, 2);
                GEN_J8(3, 7, 2);
                GEN_J8(3, 11, 2);
                GEN_J8(3, 14, 2);
                GEN_JR8(4, 15, 2);
            }break;
            case 14:{
                GEN_8(2, 6);
                GEN_8(2, 10);
                GEN_8(2, 12);
                GEN_8(3, 14);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_J8(2, 3, 1);
                GEN_J8(2, 5, 1);
                GEN_J8(2, 9, 1);
                GEN_J8(3, 7, 1);
                GEN_J8(3, 11, 1);
                GEN_J8(3, 13, 1);
                GEN_JR8(4, 15, 1);
            }break;
            case 15:{
                GEN_8(2, 3);
                GEN_8(2, 5);
                GEN_8(2, 6);
                GEN_8(2, 9);
                GEN_8(2, 10);
                GEN_8(2, 12);
                GEN_8(3, 7);
                GEN_8(3, 11);
                GEN_8(3, 13);
                GEN_8(3, 14);
                GEN_R8(4, 15);
                if(!jk) break; // ジョーカーがなければ飛ばす
                GEN_JR8(5, 15, 15);
            }break;
            default: UNREACHABLE; break;
        }
        mv += genAllSeq(mv, c); // 階段を生成
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    int genLegalLead(move_t *const mv0, const hand_t& hand){
        int cnt = genLead(mv0, hand);
        (mv0 + cnt)->setNULL();
        return cnt + 1;
    }
    
#undef GEN_BASE
#undef GEN8_BASE
    
#undef GEN
#undef GEN_J
#undef GEN_R
#undef GEN_8
#undef GEN_JR
#undef GEN_J8
#undef GEN_R8
#undef GEN_JR8
    
#define GEN_BASE(q, oc, op) {\
Cards tc = (oc);\
while(tc){\
IntCard ic = popIntCardLow(&tc);\
int r = IntCardToRank(ic);\
uint32_t s = IntCardToSuits(ic);\
mv->setNULL();\
mv->setSeq(q, r, s);\
if(isEightSeqRank(r, q)){ mv->setEight(); }\
op;\
++mv; }\
}

#define GEN(q, oc) GEN_BASE(q, oc,)
#define GEN_J(q, oc, jdr) GEN_BASE(q, oc, mv->setJokerRank(r + jdr);mv->setJokerSuits(s))
#define GEN_R(q, oc) GEN_BASE(q, oc, mv->setRev())
#define GEN_JR(q, oc, jdr) GEN_BASE(q, oc, mv->setJokerRank(r + jdr);mv->setJokerSuits(s);mv->setRev())
    
    template<class move_t>
    int genAllSeqWithJoker(move_t *const mv0, const Cards x){
        Cards c = maskJOKER(x);
        if(!c){ return 0; }
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
        GEN_R(5, seq5);
        // 2_2型を作成
        const Cards seq2_2 = c2_2 & ~seq5;
        GEN_JR(5, seq2_2, 2);
        // 3_1型と1_3型を作成
        const Cards seq3_1 = c3_1 & ~seq5;
        const Cards seq1_3 = c1_3 & ~seq5;
        GEN_JR(5, seq3_1, 3);
        GEN_JR(5, seq1_3, 1);
        // 4型を両側で作成
        const Cards seq4_ = c4 & ~seq5;
        const Cards seq_4 = (c4 >> 4) & ~seq5;
        GEN_JR(5, seq_4, 0);
        GEN_JR(5, seq4_, 4);
        
        // 6枚階段
        if(c3){
            const Cards c4_1 = c4 & (c3_1 >> 4);
            const Cards c1_4 = c1_3 & (c4 >> 8);
            const Cards c3_2 = c3_1 & (c2_2 >> 4);
            const Cards c2_3 = c2_2 & (c1_3 >> 4);
            const Cards c6 = c5 & c4_1;
            // プレーンを作成
            const Cards seq6 = c6;
            GEN_R(6, seq6);
            // 3_2型と2_3型を作成
            const Cards seq3_2 = c3_2 & ~seq6;
            const Cards seq2_3 = c2_3 & ~seq6;
            GEN_JR(6, seq3_2, 3);
            GEN_JR(6, seq2_3, 2);
            // 4_1型と1_4型を作成
            const Cards seq4_1 = c4_1 & ~seq6;
            const Cards seq1_4 = c1_4 & ~seq6;
            GEN_JR(6, seq4_1, 4);
            GEN_JR(6, seq1_4, 1);
            // 5型を両側で作成
            const Cards seq5_ = c5 & ~seq6;
            const Cards seq_5 = (c5 >> 4) & ~seq6;
            GEN_JR(6, seq_5, 0);
            GEN_JR(6, seq5_, 5);
        }
        
        // 7枚以上階段は未実装
        return mv - mv0;
    }
    
    template<class move_t>
    int genAllPlainSeq(move_t *const mv0, const Cards x){
        // ジョーカーを入れない階段のみ生成
        // 生成ランク制限無し
        move_t *mv = mv0;
        Cards c = polymRanks<3>(x);
        for(int q = 3; anyCards(c); ++q){
            Cards tmp = c;
            while(1){
                IntCard ic = popIntCardLow(&tmp);
                int rank = IntCardToRank(ic);
                uint32_t suit = IntCardToSuits(ic);
                mv->setNULL();
                mv->setSeq(q, rank, suit);
                if(isEightSeqRank(rank, q)){ mv->setEight(); }
                if(q >= 5){ mv->setRev(); }
                ++mv;
                if(!tmp){ break; }
            }
            // 次の枚数を考える
            c = polymRanks<2>(c); // ランク a かつランク a + 1 の n 枚階段があればランク a の n + 1 枚階段が出来る
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    int genAllSeq(move_t *const mv, const hand_t& hand){
        if(!hand.seq){ return 0; }
        return genAllSeq(mv, Cards(hand));
    }
    
    template<class move_t>
    int genAllSeq(move_t *const mv, const Cards& c){
        int cnt;
        if(!containsJOKER(c)){
            cnt = genAllPlainSeq(mv, c);
        }else{
            cnt = genAllSeqWithJoker(mv, c);
        }
        return cnt;
    }
    
    template<class move_t, class hand_t>
    int genFollowSeq(move_t *const mv, const hand_t& hand, const Board bd){
        if(!hand.seq){ return 0; }
        int cnt;
        if(!hand.containsJOKER()){
            cnt = genFollowPlainSeq(mv, Cards(hand), bd);
        }else{
            cnt = genFollowSeqWithJoker(mv, Cards(hand), bd);
        }
        return cnt;
    }
    
    template<class move_t>
    int genFollowSeq(move_t *const mv, Cards c, const Board bd){
        int cnt;
        if(!containsJOKER(c)){
            cnt = genFollowPlainSeq(mv, c, bd);
        }else{
            cnt = genFollowSeqWithJoker(mv, c, bd);
        }
        return cnt;
    }
    
    template<class move_t>
    int genFollowPlainSeq(move_t *const mv0, Cards c, Board bd){
        uint32_t r = bd.rank();
        uint32_t q = bd.qty();
        assert(q >= 3);
        if(!bd.isTmpOrderRev()){ // 通常
            if(r + (q << 1) > RANK_MAX + 1){ return 0; }
            c &= RankRangeToCards(r + q, RANK_MAX);
        }else{ // オーダー逆転中
            if(r < RANK_MIN + q){ return 0; }
            c &= RankRangeToCards(RANK_MIN, r - 1);
        }
        c = polymRanks<3>(c);
        if(!c){ return 0; }
        if(q > 3){
            c = polymRanks(c, q - 3 + 1);
        }
        if(!c){ return 0; }
        if(bd.suitsLocked()){
            c &= SuitsToCards(bd.suits());
        }
        move_t *mv = mv0;
        while(c){
            IntCard ic = popIntCardLow(&c);
            int r = IntCardToRank(ic);
            uint32_t s = IntCardToSuits(ic);
            mv->setNULL();
            mv->setSeq(q, r, s);
            if(isEightSeqRank(r, q)){ mv->setEight(); }
            if(q >= 5){ mv->setRev(); }
            ++mv;
        }
        return mv - mv0;
    }
    
    template<class move_t>
    int genFollowSeqWithJoker(move_t *const mv0, const Cards plain, const Board bd){
        const uint32_t r = bd.rank();
        const uint32_t qty = bd.qty();
        assert(qty >= 3);
        Cards c = plain;
        Cards rCards; // 合法カードゾーン
        Cards validSeqZone; // 合法階段ゾーン
        if(!bd.isTmpOrderRev()){ // 通常
            if(r + (qty << 1) > RANK_MAX + 2){ return 0; }
            rCards = RankRangeToCards(r + qty, RANK_MAX + 1);
            validSeqZone = RankRangeToCards(r + qty, RANK_MAX + 1);
        }else{ // オーダー逆転中
            if(r < qty + RANK_MIN - 1){ return 0; }
            rCards = RankRangeToCards(RANK_MIN - 1, r - 1);
            validSeqZone = RankRangeToCards(RANK_MIN - 1, r - qty);
        }
        c &= rCards;
        if(bd.suitsLocked()){
            c &= SuitsToCards(bd.suits());
        }
        if(!c){ return 0; }
        move_t *mv = mv0;
        switch(qty){
            case 0: UNREACHABLE; break;
            case 1: UNREACHABLE; break;
            case 2: UNREACHABLE; break;
            case 3:{
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
            }break;
            case 4:{
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
            }break;
            case 5:{
                const Cards c1_1 = polymJump(c);
                const Cards c2 = polymRanks<2>(c);
                
                const Cards c3 = c1_1 & c2;
                
                if(!c3){ break; }
                
                const Cards c2_1 = c2 & (c1_1 >> 4);
                const Cards c1_2 = c1_1 & (c2 >> 8);
                
                const Cards c4 = c3 & c2_1;
                const Cards c3_1 = c3 & (c2_1 >> 4);
                const Cards c1_3 = c1_2 & (c3 >> 8);
                const Cards c2_2 = c2_1 & (c1_2 >> 4);
                
                const Cards c5 = c4 & c3_1;
                
                // プレーンを作成
                const Cards seq5 = c5 & validSeqZone;
                GEN_R(5, seq5);
                // 2_2型を作成
                const Cards seq2_2 = c2_2 & validSeqZone & ~seq5;
                GEN_JR(5, seq2_2, 2);
                // 3_1型と1_3型を作成
                const Cards seq3_1 = c3_1 & validSeqZone & ~seq5;
                const Cards seq1_3 = c1_3 & validSeqZone & ~seq5;
                GEN_JR(5, seq3_1, 3);
                GEN_JR(5, seq1_3, 1);
                // 4型を両側で作成
                const Cards seq4_ = c4 & validSeqZone & ~seq5;
                const Cards seq_4 = (c4 >> 4) & validSeqZone & ~seq5;
                GEN_JR(5, seq_4, 0);
                GEN_JR(5, seq4_, 4);
            }break;
            default: break; // 6枚以上階段は未実装
        }
        return mv - mv0;
    }
    
#undef GEN_BASE
#undef GEN
#undef GEN_J
#undef GEN_R
#undef GEN_JR
    
    /**************************特別着手生成**************************/
    
    // 通常の着手生成では無視する着手のうち、
    // クライアントの手としては検討するものを生成
    
    // ジョーカーを使わずに作る事が出来る着手を
    // ジョーカーによって出す着手
    // 8か最強ランクであり、
    // 本来そのランクが無くなるはずの着手についてのみ検討する
    
    template<class move_t>
    int genNullPass(move_t *const mv0){
        // 空場でのパスを生成
        mv0->setNULL();
        return 1;
    }
    
#define GEN_BASE(q, s, op) {mv->setNULL();\
mv->setGroupByRank4x(q, r4x, s);\
op;\
if(r4x == RANK4X_8){ mv->setEight(); }\
if(q >= 4){ mv->setRev(); }\
++mv;}

#define GEN_J(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js))
    
    template<class move_t>
    int genJokerGroup(move_t *const mv0, Cards c, const Cards ops, const Board bd){
        
        assert(containsJOKER(c));
        assert(containsS3(ops));
        assert(bd.isGroup());
        
        move_t *mv = mv0;
        int br4x = bd.rank4x();
        if(!bd.isTmpOrderRev()){ // 通常
            c &= RankRange4xToCards(br4x + 4, RANK4X_MAX);
        }else{ // オーダー逆転中
            c &= RankRange4xToCards(RANK4X_MIN, br4x - 4);
        }
        uint32_t s = bd.suits();
        int tmpOrd = bd.tmpOrder();
        
        for(uint32_t r4x = RANK4X_3; r4x <= RANK4X_2; r4x += 4){
            uint32_t q = countCards(c & Rank4xToCards(r4x));
            if(q == bd.qty()){
                uint32_t x = CardsRank4xToSuits(c, r4x);
                
                if(r4x != RANK4X_8){
                    // 相手が返せるかどうかチェック
                    if(tmpOrd){
                        if(x == 15){
                            if(!holdsCards(RankRange4xToCards(RANK4X_3, r4x), ops))continue;
                        }else{
                            if(!holdsCards(RankRange4xToCards(r4x, RANK4X_2), ops))continue;
                        }
                    }else{
                        if(x == 15){
                            if(!holdsCards(RankRange4xToCards(r4x, RANK4X_2), ops))continue;
                        }else{
                            if(!holdsCards(RankRange4xToCards(RANK4X_3, r4x), ops))continue;
                        }
                    }
                }
                
                switch(x){
                    case 0: break;
                    case 1: break;
                    case 2: break;
                    case 3:{ if(bd.suitsLocked() && s != 3)break;
                        GEN_J(2, 3, 2);
                        GEN_J(2, 3, 1);
                    }break;
                    case 4: break;
                    case 5:{ if(bd.suitsLocked() && s != 5)break;
                        GEN_J(2, 5, 1);
                        GEN_J(2, 5, 4);
                    }break;
                    case 6:{ if(bd.suitsLocked() && s != 6)break;
                        GEN_J(2, 6, 2);
                        GEN_J(2, 6, 4);
                    }break;
                    case 7:{ if(bd.suitsLocked() && s != 7)break;
                        GEN_J(3, 7, 1);
                        GEN_J(3, 7, 2);
                        GEN_J(3, 7, 4);
                    }break;
                    case 8: break;
                    case 9:{ if(bd.suitsLocked() && s != 9)break;
                        GEN_J(2, 9, 1);
                        GEN_J(2, 9, 8);
                    }break;
                    case 10:{ if(bd.suitsLocked() && s != 10)break;
                        GEN_J(2, 10, 2);
                        GEN_J(2, 10, 8);
                    }break;
                    case 11:{ if(bd.suitsLocked() && s != 11)break;
                        GEN_J(3, 11, 1);
                        GEN_J(3, 11, 2);
                        GEN_J(3, 11, 8);
                    }break;
                    case 12:{ if(bd.suitsLocked() && s != 12)break;
                        GEN_J(2, 12, 4);
                        GEN_J(2, 12, 8);
                    }break;
                    case 13:{ if(bd.suitsLocked() && s != 13)break;
                        GEN_J(3, 13, 1);
                        GEN_J(3, 13, 4);
                        GEN_J(3, 13, 8);
                    }break;
                    case 14:{ if(bd.suitsLocked() && s != 14)break;
                        GEN_J(3, 14, 2);
                        GEN_J(3, 14, 4);
                        GEN_J(3, 14, 8);
                    }break;
                    case 15:{ if(bd.suitsLocked() && s != 15)break;
                        GEN_J(4, 15, 1);
                        GEN_J(4, 15, 2);
                        GEN_J(4, 15, 4);
                        GEN_J(4, 15, 8);
                    }break;
                    default: UNREACHABLE; break;
                }
            }
        }
        return mv - mv0;
    }
#undef GEN_BASE
#undef GEN_J
    
    template<class move_t>
    int genGroupDebug(move_t *const mv0, Cards c, Board bd){
        move_t *mv = mv0;
        int st, ed;
        // 速度は問わず、全ての合法着手を生成
        if(bd.isNF()){
            st = RANK_MIN; ed = RANK_MAX;
        }else{
            uint32_t r_bd = bd.rank();
            if(!bd.isTmpOrderRev()){ // 通常
                st = r_bd + 1; ed = RANK_MAX;
            }else{ // オーダー逆転中
                st = RANK_MIN; ed = r_bd - 1;
            }
        }
        for(int r = st; r <= ed; ++r){
            for(uint32_t s = 1; s <= SUITS_ALL; ++s){
                if(bd.isNull() || bd.suits() == s){
                    int q = countSuits(s);
                    if(holdsCards(c, RankSuitsToCards(r, s))){
                        mv->setNULL();
                        mv->setGroup(r, s, countSuits(s));
                        if(r == RANK_8){ mv->setEight(); }
                        if(q >= 4){ mv->setRev(); }
                        ++mv;
                    }else if(containsJOKER(c)){
                        for(uint32_t js = SUIT_MIN; s <= SUIT_MAX; s <<= 1){
                            if((s & js) && holdsCards(c, RankSuitsToCards(r, s - js))){
                                mv->setNULL();
                                mv->setGroup(r, s, countSuits(s));
                                if(r == RANK_8){ mv->setEight(); }
                                if(q >= 4){ mv->setRev(); }
                                mv->setJokerSuits(js);
                                ++mv;
                            }
                        }
                    }
                }
            }
        }
        return mv - mv0;
    }
    
    template<class move_t, class hand_t>
    inline int genFollow(move_t *mv, const hand_t& c, const Board bd){
        // 返り値は、生成した手の数
        int ret = 1;
        mv->setNULL();
        ++mv;
        if(!bd.isSeq()){
            uint32_t qty = bd.qty();
            switch(qty){
                case 0: break;
                case 1: ret += genFollowSingle<move_t>(mv, c, bd); break;
                case 2: ret += genFollowDouble<move_t>(mv, c, bd); break;
                case 3: ret += genFollowTriple<move_t>(mv, c, bd); break;
                case 4: ret += genFollowQuadruple<move_t>(mv, c, bd); break;
                default: break;
            }
        }else{
            ret += genFollowSeq<move_t>(mv, c, bd);
        }
        return ret;
    }
    
    template<class move_t, class hand_t>
    int genFollowExceptPASS(move_t *const mv, const hand_t& c, const Board bd){
        // 返り値は、生成した手の数
        int ret = 0;
        if(!bd.isSeq()){
            const uint32_t qty = bd.qty();
            switch(qty){
                case 0: break;
                case 1: ret += genFollowSingle(mv, c, bd); break;
                case 2: ret += genFollowDouble(mv, c, bd); break;
                case 3: ret += genFollowTriple(mv, c, bd); break;
                case 4: ret += genFollowQuadruple(mv, c, bd); break;
                default: break;
            }
        }else{
            ret += genFollowSeq(mv, c, bd);
        }
        return ret;
    }
    
    template<int IS_NF = _BOTH, class move_t, class hand_t>
    int genMove(move_t *mv, const hand_t& c, const Board bd){
        if(IS_NF == _YES || (IS_NF != _NO && bd.isNF())){
            return genLead(mv, c);
        }else{
            return genFollow(mv, c, bd);
        }
        UNREACHABLE;
    }
    
    template<int IS_NF = _BOTH, class move_t, class hand_t>
    int genLegal(move_t *mv, const hand_t& c, const Board bd){
        if(IS_NF == _YES || (IS_NF != _NO && bd.isNF())){
            return genLegalLead(mv, c);
        }else{
            return genFollow(mv, c, bd);
        }
        UNREACHABLE;
    }
    
    template<class move_t = Move, class hand_t = Cards>
    class MoveGenerator{
        // 合法着手生成マシン
        // 用途に応じて使う関数を変える
    private:
        static AtomicAnalyzer<7, 1> ana;
        
    public:
        // 通常生成
        // 比較的後の方に有効な役が多い
        int genFollowSingle(move_t *const mv, const hand_t& c, const Board bd){
            ana.start();
            int cnt = UECda::genFollowSingle(mv, c, bd);
            ana.end(0);
            return cnt;
        }
        int genFollowDouble(move_t *const mv, const hand_t c, const Board bd){
            ana.start();
            int cnt = UECda::genFollowDouble(mv, c, bd);
            ana.end(1);
            return cnt;
        }
        int genFollowTriple(move_t *const mv, const hand_t& c, const Board bd){
            ana.start();
            int cnt = UECda::genFollowTriple(mv, c, bd);
            ana.end(2);
            return cnt;
        }
        int genFollowQuadruple(move_t *const mv, const hand_t& c, const Board bd){
            ana.start();
            int cnt = UECda::genFollowQuadruple(mv, c, bd);
            ana.end(3);
            return cnt;
        }
        int genFollowSeq(move_t *const mv, const hand_t& c, const Board bd){
            ana.start();
            int cnt = UECda::genFollowSeq(mv, c, bd);
            ana.end(4);
            return cnt;
        }
        int genFollow(move_t *mv, const hand_t& c, const Board bd){
            // 返り値は、生成した手の数
            ana.start();
            int ret = 1;
            mv->setNULL();
            ++mv;
            ana.end(6);
            if(bd.isSeq()){
                ret += genFollowSeq(mv, c, bd);
            }else{
                const uint32_t qty = bd.qty();
                switch(qty){
                    case 0: break;
                    case 1: ret += genFollowSingle(mv, c, bd); break;
                    case 2: ret += genFollowDouble(mv, c, bd); break;
                    case 3: ret += genFollowTriple(mv, c, bd); break;
                    case 4: ret += genFollowQuadruple(mv, c, bd); break;
                    default: break;
                }
            }
            return ret;
        }
        int genFollowExceptPASS(move_t *mv, const hand_t& c, const Board bd){
            // 返り値は、生成した手の数
            int ret = 0;
            if(bd.isSeq()){
                ret += genFollowSeq(mv, c, bd);
            }else{
                const uint32_t qty = bd.qty();
                switch(qty){
                    case 0: break;
                    case 1: ret += genFollowSingle(mv, c, bd); break;
                    case 2: ret += genFollowDouble(mv, c, bd); break;
                    case 3: ret += genFollowTriple(mv, c, bd); break;
                    case 4: ret += genFollowQuadruple(mv, c, bd); break;
                    default: break;
                }
            }
            return ret;
        }
        
        int genLead(move_t *const mv, const hand_t& c){
            ana.start();
            int cnt = UECda::genLead(mv, c);
            ana.end(5);
            return cnt;
        }
        
        template<int IS_NF = _BOTH>
        int genMove(move_t *const mv0, const hand_t& c, const Board bd){
            int ret;
            if((IS_NF == _YES) || (IS_NF != _NO && bd.isNF())){
                ret = genLead(mv0, c);
            }else{
                ret = genFollow(mv0, c, bd);
            }
            return ret;
        }
    };
    
    template<>AtomicAnalyzer<7, 1> MoveGenerator<MoveInfo, Hand>::ana("MoveGenerator<MoveInfo, Hand>");
    template<>AtomicAnalyzer<7, 1> MoveGenerator<MoveInfo, Cards>::ana("MoveGenerator<MoveInfo, Cards>");
}