#pragma once

#include "../core/daifugo.hpp"
#include "../core/prim2.hpp"
#include "../core/hand.hpp"
#include "mate.hpp"

// ラスト二人探索

namespace L2 {
    // L2関連
    extern TwoValueBook<(1 << 20) - 3> book;
    inline void init() {
        book.init();
    }
}

enum L2Result {
    L2_NONE = -1, L2_WIN = 0,
    L2_DRAW = 1, L2_LOSE = 2
};

struct L2Field;

class L2Judge {
    
    // L2判定人
    // 単純な構造でデータを受け取り、
    
    // judge...局面L2判定を返す
    // check...着手L2判定を返す
    // search...与えられた着手集合に勝利着手があればインデックス、なければ-1を返す
    
private:
    const int NODE_LIMIT;
    
    MoveInfo *const buf;
    
    // 統計情報
    int nodes, childs, failed;
    
public:
    void init() {
        nodes = childs = failed = 0;
    }
    
    L2Judge(int nl,MoveInfo *const argMI):
    NODE_LIMIT(nl), buf(argMI) { init(); }
    ~L2Judge() {}

    // 再帰版
    template <int S_LEVEL, int E_LEVEL>
    int judge(const int depth, MoveInfo *const buf, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    int check(const int depth, MoveInfo *const buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy = false);
    int checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    int search(const int depth, MoveInfo *const buf, const int NMoves, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    
    int start_judge(const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fInfo);
    int start_check(const MoveInfo mi, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fInfo);
};