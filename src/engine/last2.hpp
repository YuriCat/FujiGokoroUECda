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

// L2局面表現
struct L2Field {
    Board b;
    bool lastAwake;
    bool flushLead;

    L2Field(): b(), lastAwake(), flushLead() {}
    L2Field(const Board& b, const FieldAddInfo& info):
    b(b), lastAwake(info.isLastAwake()), flushLead(info.isFlushLead()) {}
};

class L2Judge {
    // L2判定
    // judge...局面L2判定を返す
    // check...着手L2判定を返す

protected:
    const int NODE_LIMIT;
    MoveInfo *const mbuf;

public:
    // 統計情報
    int nodes, childs, failed;
    int searchCount, searchIndex;

    void init() {
        nodes = childs = failed = 0;
        searchCount = searchIndex = 0;
    }

    L2Judge(int nl, MoveInfo *const mb):
    NODE_LIMIT(nl), mbuf(mb) { init(); }

    int judge(const int depth, MoveInfo *const buf, const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy = false);
    int check(const int depth, MoveInfo *const buf, MoveInfo& m, const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy = false);
    bool checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& m, const Hand& myHand, const Hand& opsHand, const L2Field& field);
};

extern bool judgeHandL2L_NF(const Hand& myHand, const Hand& opsHand, const Board b);

extern int judgeLast2(MoveInfo *const buf, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo, int node_limit = 65536, bool stats = true);
extern int checkLast2(MoveInfo *const buf, const MoveInfo move, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo, int node_limit = 65536, bool stats = true);