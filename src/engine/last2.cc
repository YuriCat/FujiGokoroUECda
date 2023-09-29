#include "../base/util.hpp"
#include "mate.hpp"
#include "last2.hpp"

using namespace std;

namespace L2 {
    TwoValueBook<(1 << 20) - 3> book;
    void init() {
        book.clear();
    }
}

// L2局面表現
struct L2Field {
    Board b;
    FieldAddInfo info;

    bool isNull() const { return b.isNull(); }
    int order() const { return b.order(); }

    bool isLastAwake() const { return info.isLastAwake(); }
    bool isFlushLead() const { return info.isFlushLead(); }
    bool isUnrivaled() const { return info.isUnrivaled(); }

    void setSelfFollow() { info.setSelfFollow(); }
    void setLastAwake() { info.setLastAwake(); }
    void setFlushLead() { info.setFlushLead(); }

#ifdef DEBUG
    int p; // プレーヤー確認用
    int turn() const { return p; }
    void flipTurnPlayer() { p = 1 - p; }
    L2Field(): b(), info(), p(0) {}
#else
    int turn() const { return 0; }
    void flipTurnPlayer() {}
    L2Field(): b(), info() {}
#endif
};

// L2局面表現へのチェンジ
L2Field convL2Field(const Board& b, const FieldAddInfo& info) {
    L2Field f;
    f.info = info;
    f.b = b;
    return f;
}
L2Field procAndFlushL2Field(const L2Field& cur, const Move m) {
    L2Field f;
    f.b = cur.b;
    f.info = cur.info;
    f.info.init();
    f.b.procAndFlush(m);
    return f;
}
int procL2Field(const L2Field& cur, L2Field *const pnext, const MoveInfo m) {
    *pnext = cur;
    pnext->info.init();
    if (cur.isUnrivaled()) { // 独壇場
        if (m.isPASS()) {
            pnext->b.flush();
        } else if (m.dominatesMe()) {
            // 自己支配がかかるので、流れて自分から
            pnext->b.procAndFlush(m);
        } else {
            // 流れなければSFが続く
            pnext->b.proc(m);
            if (!pnext->b.isNull()) {
                pnext->setSelfFollow();
            }
        }
    } else if (cur.isNull()) {
        if (m.dominatesAll()) {
            pnext->b.procAndFlush(m);
        } else if (m.dominatesOthers()) {
            pnext->b.proc(m);
            if (!pnext->b.isNull()) {
                pnext->setSelfFollow();
            }
        } else {
            pnext->b.proc(m);
            if (!pnext->b.isNull()) {
                pnext->flipTurnPlayer(); return 1;
            }
        }
    } else { // 独壇場でない通常場
        if (m.isPASS()) { // pass
            if (cur.isLastAwake()) {
                pnext->b.flush();
                // ここがFlushLeadになるのは探索入り口のみ
                if (!cur.isFlushLead()) {
                    pnext->flipTurnPlayer(); return 1;
                }
            } else {
                pnext->setLastAwake();
                if (!cur.isFlushLead()) {
                    pnext->setFlushLead();
                }
                pnext->flipTurnPlayer();
                return 1;
            }
        } else { // not pass
            if (cur.isLastAwake()) {
                if (m.dominatesMe()) {
                    // 自己支配がかかるので、流れて自分から
                    pnext->b.procAndFlush(m);
                } else {
                    pnext->b.proc(m);
                    if (!pnext->b.isNull()) {
                        pnext->setSelfFollow();
                    }
                }
            } else {
                if (m.dominatesAll()) {
                    pnext->b.procAndFlush(m);
                } else if (m.dominatesOthers()) {
                    pnext->b.proc(m);
                    if (!pnext->b.isNull()) {
                        pnext->setSelfFollow();
                    }
                } else {
                    pnext->b.proc(m);
                    if (!pnext->b.isNull()) {
                        pnext->flipTurnPlayer(); return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int L2Judge::judge(const int depth, MoveInfo *const buf,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {
    // 判定を返す
    nodes++;
    uint64_t fkey = -1;

    if (field.isNull()) {
        if (judgeMate_Easy_NF(myHand)) return L2_WIN;
        // 局面や相手の手札も考えた必勝判定
        assert(myHand.exam1stHalf() && opsHand.exam1stHalf());
        if (judgeHandPW_NF(myHand, opsHand, field.b)) return L2_WIN;
        //if (judgeHandMate(0, buf, myHand, opsHand, field.b)) return L2_WIN;

        // 局面登録を検索(NFのみ)
        assert(myHand.exam_key() && opsHand.exam_key());
        fkey = knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b));
        int result = L2::book.read(fkey);
        if (result != -1) return result; // 結果登録あり

        // 簡易必敗判定
        if (!myHand.seq && !(myHand.pqr & PQR_234) && !myHand.jk && !containsS3(myHand.cards) && field.order() == 0) {
            int myHR = IntCardToRank(pickIntCardHigh(myHand.cards));
            int opsLR = IntCardToRank(pickIntCardLow(opsHand.cards));
            if (myHR < opsLR) return L2_LOSE;
            Cards tmp = maskCards(opsHand.cards, RankToCards(opsLR));
            if (tmp) {
                opsLR = IntCardToRank(pickIntCardLow(tmp));
                if (myHR < opsLR) return L2_LOSE;
            }
        }
    } else {
        // フォローで全部出せる場合は勝ち
        //if (field.b.qty() == myHand.qty && !dominatesHand(field.b, myHand)) return L2_WIN;
    }

    //if (depth <= 0) return L2_DRAW;
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
        if (field.isNull()) L2::book.regist(result, fkey);
        return result;
    }
    return L2_DRAW;
}

bool L2Judge::checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& tmp,
                           const Hand& myHand, const Hand& opsHand, const L2Field& field) {
    if (field.isUnrivaled()
        || dominatesCards(tmp, opsHand.cards, field.b)) { // 他支配チェック
        tmp.setDomOthers();

        // 支配して残り1枚なら勝ち
        if (myHand.qty - tmp.qty() <= 1) return true;

        if (!tmp.isPASS()) {
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, tmp);
            if (judgeMate_Easy_NF(nextHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, tmp);
            if (judgeHandPW_NF(nextHand, opsHand, nextField.b)) return true;
            nodes++;
            nextHand.key = subCardKey(myHand.key, CardsToHashKey(tmp.cards()));
            uint64_t fkey = knitL2NullFieldHashKey(nextHand.key, opsHand.key, NullBoardToHashKey(nextField.b));
            if (L2::book.read(fkey) == L2_WIN) return true;
        } else {
            if (judgeMate_Easy_NF(myHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, tmp);
            if (judgeHandPW_NF(myHand, opsHand, nextField.b)) return true;
        }
    }
    return false;
}

int L2Judge::check(const int depth, MoveInfo *const buf, MoveInfo& tmp,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {
    if (!checkedEasy) {
        if (tmp.qty() >= myHand.qty) return L2_WIN;
        if (checkDomMate(depth, buf, tmp, myHand, opsHand, field)) return L2_WIN;
    }

    if (!field.isLastAwake() && !tmp.dominatesOthers() && tmp.qty() == opsHand.qty) {
        if (!dominatesHand(tmp, opsHand, field.b)) return L2_LOSE;
    }

    childs++;

    // 支配性判定
    if (!tmp.isPASS() && (field.isLastAwake() || tmp.dominatesOthers())) {
        if (dominatesCards(tmp, myHand.cards, field.b)) tmp.setDomMe();
    }

    L2Field nextField;
    int nextPlayer = procL2Field(field, &nextField, tmp);
    if (!tmp.isPASS()) {
        Hand nextHand;
        makeMoveAll(myHand, &nextHand, tmp);
        if (nextPlayer == 0) {
            return judge(depth + 1, buf, nextHand, opsHand, nextField, true);
        } else {
            return L2_WIN + L2_LOSE - judge(depth - 1, buf, opsHand, nextHand, nextField);
        }
    } else { // PASS
        if (nextPlayer == 0) {
            return judge(depth + 1, buf, myHand, opsHand, nextField, true);
        } else {
            return L2_WIN + L2_LOSE - judge(depth - 1, buf, opsHand, myHand, nextField);
        }
    }
}

int L2Judge::start_judge(const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    init();
    L2Field field = convL2Field(b, fieldInfo); // L2型へのチェンジ
    return judge(0, mbuf, myHand, opsHand, field);
}

int L2Judge::start_check(const MoveInfo m, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fieldInfo) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    init();
    L2Field field = convL2Field(b, fieldInfo); // L2型へのチェンジ
    MoveInfo tm = m;
    return check(0, mbuf, tm, myHand, opsHand, field);
}