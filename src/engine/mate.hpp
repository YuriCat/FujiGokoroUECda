#pragma once

// 詰み(Mate)判定

#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"

inline bool judgeMate_Easy_NF(const Hand& myHand) {
    // とりあえず高速でNF_Mateかどうか判断したい場合
    if (myHand.qty <= 1) return true;
    Cards pqr = myHand.pqr;
    if (!any2Cards(maskCards(pqr, CARDS_8 | CARDS_JOKER))) return true;
    if (myHand.jk) { // ジョーカーあり
        if (myHand.seq) {
            if (myHand.qty == 3) return true;
            if (canMakeJokerSeq(myHand.cards.plain(), myHand.jk, myHand.qty)) return true;
        }
    } else {
        if (myHand.seq) {
            if (myHand.qty == 3) return true;
            if (canMakePlainSeq(myHand.cards, myHand.qty)) return true;
        }
    }
    return false;
}

bool judgeHandPPW_NF(const Cards cards, const Cards pqr, const int jk,
                     const Cards *const nd, const Board& b);
bool judgeHandPW_NF(const Hand& myHand, const Hand& opsHand, const Board& b);

bool judgeHandMate(const int depth, MoveInfo *const buf,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo);

bool checkHandBNPW(const int depth, MoveInfo *const buf, const MoveInfo mv,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo);

bool checkHandMate(const int depth, MoveInfo *const buf, MoveInfo& mv,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo);

inline int searchHandMate(const int depth, MoveInfo *const buf, const int NMoves,
                          const Hand& myHand, const Hand& opsHand,
                          const Board& b, const FieldAddInfo& fieldInfo) {
    // 必勝手探し
    for (int i = NMoves - 1; i >= 0; i--) {
        if (buf[i].qty() == myHand.qty) return i;
    }
    for (int i = NMoves - 1; i >= 0; i--) {
        if (checkHandMate(depth, buf + NMoves, buf[i], myHand, opsHand, b, fieldInfo)) return i;
    }
    return -1;
}