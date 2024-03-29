#include "../base/util.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"
#include "mate.hpp"
#include "last2.hpp"

using namespace std;

namespace L2 {
    TwoValueBook<(1 << 20) - 3> book;
    void init() {
        book.clear();
    }
}

inline L2Field procAndFlushL2Field(const L2Field& cur, const Move m) {
    L2Field f;
    f.b = cur.b;
    f.b.procAndFlush(m);
    f.lastAwake = false;
    f.flushLead = true;
    return f;
}

bool procL2Field(const L2Field& cur, L2Field *const pnext, const MoveInfo m) {
    Board b = cur.b;
    bool lastAwake = cur.lastAwake;
    bool flushLead = cur.flushLead;
    bool flipped = false;
    if (m.isPASS()) {
        if (lastAwake) {
            b.flush();
            if (!flushLead) flipped = true;
            lastAwake = false;
            flushLead = true;
        } else {
            flipped = true;
            lastAwake = true;
            flushLead = !flushLead;
        }
    } else {
        if (m.dominatesAll()
            || (lastAwake && m.dominatesMe())) {
            b.procAndFlush(m);
            lastAwake = false;
            flushLead = true;
        } else {
            b.proc(m);
            if (b.isNull()) { // 流れた
                lastAwake = false;
                flushLead = true;
            } else {
                if (lastAwake || m.dominatesOthers()) {
                    lastAwake = flushLead = true;
                } else {
                    flipped = true;
                    lastAwake = flushLead = false;
                }
            }
        }
    }
    pnext->b = b;
    pnext->lastAwake = lastAwake;
    pnext->flushLead = flushLead;
    return flipped;
}

bool judgeHandL2L_NF(const Hand& myHand, const Hand& opsHand, const Board b) {
    // PQRND判定済みを仮定 TODO: ラスト2人では必勝も別に書くべき?
    assert(myHand.qty > 1);
    if (myHand.seq) return false;
    if (opsHand.qty == 1) return true;

    if (!(myHand.pqr & PQR_234) && !(myHand.cards & (CARDS_S3 | CARDS_JOKER))) {
        if (b.order() == 0) {
            Cards mine = myHand.cards, ops = opsHand.cards;
            int myHigh = IntCardToRank(mine.highest());
            int opsLow = IntCardToRank(ops.lowest());
            if (myHigh < opsLow) return true;
            Cards tmp = maskCards(ops, RankToCards(opsLow));
            if (tmp) {
                int opsLow = IntCardToRank(tmp.lowest());
                if (myHigh < opsLow) return true;
            }
        } else {
            Cards mine = myHand.cards, ops = maskJOKER(opsHand.cards);
            assert(ops.any());
            int myHigh = IntCardToRank(mine.lowest());
            int opsLow = IntCardToRank(ops.highest());
            if (myHigh > opsLow) return true;
            Cards tmp = maskCards(ops, RankToCards(opsLow));
            if (tmp) {
                int opsLow = IntCardToRank(tmp.highest());
                if (myHigh > opsLow) return true;
            }
        }
    }
    return false;
}

int L2Judge::judge(const int depth, MoveInfo *const buf,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {
    // 判定を返す
    nodes++;
    uint64_t fkey = -1;

    if (field.b.isNull()) {
        // 局面や相手の手札も考えた必勝判定
        assert(myHand.exam1stHalf() && opsHand.exam1stHalf());
        if (judgeHandPW_NF(myHand, opsHand, field.b)) return L2_WIN;
        //if (judgeHandMate(0, buf, myHand, opsHand, field.b)) return L2_WIN;

        // 簡易必敗判定
        if (judgeHandL2L_NF(myHand, opsHand, field.b)) return L2_LOSE;

        // 局面登録を検索(NFのみ)
        assert(myHand.exam_key() && opsHand.exam_key());
        fkey = knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b));
        int result = L2::book.read(fkey);
        if (result != -1) return result; // 結果登録あり
    } else {
        // フォローで全部出せる場合は勝ち
        //if (field.b.qty() == myHand.qty && !dominatesHand(field.b, myHand)) return L2_WIN;
    }

    if (nodes > NODE_LIMIT) { failed = 1; return L2_DRAW; }

    int numMoves = genMove(buf, myHand, field.b);

    // 即上がり判定
    if (field.b.qty() == myHand.qty && numMoves >= 2) return L2_WIN;
    // 探索
    int winIndex = -2;

    for (int i = numMoves - 1; i >= 0; i--) {
        if (!field.b.isNull() || buf[i].isSeq()) {
            if (buf[i].qty() >= myHand.qty || checkDomMate(depth, buf + numMoves, buf[i], myHand, opsHand, field)) { winIndex = i; break; }
            buf[i].setChecked();
        }
    }
    if (winIndex < 0) {
        bool unFound = false;
        for (int i = numMoves - 1; i >= 0; i--) {
            int res = check(depth, buf + numMoves, buf[i], myHand, opsHand, field, buf[i].isChecked());
            if (res == L2_WIN) { searchCount++; searchIndex += numMoves - i; winIndex = i; break; }
            if (res == L2_DRAW) unFound = true;
        }
        if (winIndex < 0 && !unFound) winIndex = -1;
    }

    if (winIndex >= -1) {
        int result = winIndex >= 0 ? L2_WIN : L2_LOSE;
        if (field.b.isNull()) L2::book.regist(result, fkey);
        return result;
    }
    return L2_DRAW;
}

bool L2Judge::checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& m,
                           const Hand& myHand, const Hand& opsHand, const L2Field& field) {
    if (!m.isPASS()) {
        if (field.lastAwake
            || dominatesCards(m, opsHand.cards, field.b)) { // 他支配チェック
            if (myHand.qty - m.qty() <= 1) return true; // 支配して残り1枚なら勝ち

            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, m);
            if (judgeMate_Easy_NF(nextHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, m);
            if (judgeHandPW_NF(nextHand, opsHand, nextField.b)) return true;
            //nextHand.key = subCardKey(myHand.key, CardsToHashKey(m.cards()));
            m.setDomOthers();
        }
    } else {
        if (field.flushLead && field.lastAwake) {
            if (myHand.qty <= 1) return true; // 支配して残り1枚なら勝ち
            if (judgeMate_Easy_NF(myHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, m);
            if (judgeHandPW_NF(myHand, opsHand, nextField.b)) return true;
            m.setDomOthers();
        }
    }
    return false;
}

int L2Judge::check(const int depth, MoveInfo *const buf, MoveInfo& m,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {
    if (!checkedEasy) {
        if (m.qty() >= myHand.qty) return L2_WIN;
        if (checkDomMate(depth, buf, m, myHand, opsHand, field)) return L2_WIN;
    }

    if (!field.lastAwake && !m.dominatesOthers() && m.qty() == opsHand.qty) {
        if (!dominatesHand(m, opsHand, field.b)) return L2_LOSE;
    }

    childs++;

    // 支配性判定
    if (!m.isPASS() && (field.lastAwake || m.dominatesOthers())) {
        if (dominatesCards(m, myHand.cards, field.b)) m.setDomMe();
    }

    L2Field nextField;
    bool flipped = procL2Field(field, &nextField, m);
    if (!m.isPASS()) {
        Hand nextHand;
        makeMoveAll(myHand, &nextHand, m);
        if (!flipped) {
            return judge(depth + 1, buf, nextHand, opsHand, nextField, true);
        } else {
            return L2_WIN + L2_LOSE - judge(depth + 1, buf, opsHand, nextHand, nextField);
        }
    } else { // PASS
        if (!flipped) {
            return judge(depth + 1, buf, myHand, opsHand, nextField, true);
        } else {
            return L2_WIN + L2_LOSE - judge(depth + 1, buf, opsHand, myHand, nextField);
        }
    }
}

int judgeLast2(MoveInfo *const buf, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo, int node_limit, bool stats) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    L2Judge judge(node_limit, buf);
    return judge.judge(0, buf, myHand, opsHand, L2Field(b, fieldInfo));
}

int checkLast2(MoveInfo *const buf, const MoveInfo move, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo, int node_limit, bool stats) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    L2Judge judge(node_limit, buf);
    MoveInfo m = move;
    return judge.check(0, buf, m, myHand, opsHand, L2Field(b, fieldInfo));
}