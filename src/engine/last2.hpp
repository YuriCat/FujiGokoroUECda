#pragma once

#include "../core/daifugo.hpp"
#include "../core/hand.hpp"

// ラスト二人探索

namespace L2 {
    extern void init();
}

enum L2Result {
    L2_NONE = -1, L2_WIN = 0,
    L2_DRAW = 1, L2_LOSE = 2
};

struct L2Field;

class L2Judge {
    // L2判定
    // judge...局面L2判定を返す
    // check...着手L2判定を返す
    // search...与えられた着手集合に勝利着手があればインデックス、なければ-1を返す

protected:
    const int NODE_LIMIT;
    Move *const mbuf;
    int nodes, childs, failed; // 統計情報
    
public:
    void init() {
        nodes = childs = failed = 0;
    }
    
    L2Judge(int nl, Move *const mb):
    NODE_LIMIT(nl), mbuf(mb) { init(); }

    // 再帰版
    template <int S_LEVEL, int E_LEVEL>
    int judge(const int depth, Move *const buf, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    int check(const int depth, Move *const buf, Move& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy = false);
    bool checkDomMate(const int depth, Move *const buf, Move& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    int search(const int depth, Move *const buf, const int NMoves, const Hand& myHand, const Hand& opsHand, const L2Field& field);
    
    int start_judge(const Hand& myHand, const Hand& opsHand, const Board b);
    int start_check(const Move mi, const Hand& myHand, const Hand& opsHand, const Board b);
};
