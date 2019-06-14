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

    bool isNull() const { return b.isNull(); }
    int order() const { return b.order(); }

    bool isLastAwake() const { return b.isLastAwake(); }
    bool isFlushLead() const { return b.isFlushLead(); }
    bool isUnrivaled() const { return b.isUnrivaled(); }

    void setSelfFollow() { b.setSelfFollow(); }
    void setLastAwake() { b.setLastAwake(); }
    void setFlushLead() { b.setFlushLead(); }

#ifdef DEBUG
    int p; // プレーヤー確認用
    int turn() const { return p; }
    void flipTurnPlayer() { p = 1 - p; }
    L2Field(): b(), p(0) {}
#else
    int turn() const { return 0; }
    void flipTurnPlayer() {}
    L2Field(): b() {}
#endif
};

// L2局面表現へのチェンジ
L2Field convL2Field(const Board& b) {
    L2Field f;
    f.b = b;
    return f;
}
L2Field procAndFlushL2Field(const L2Field& cur, const Move m) {
    L2Field f;
    f.b = cur.b;
    f.b.procAndFlush(m);
    f.b.initInfo();
    return f;
}
int procL2Field(const L2Field& cur, L2Field *const pnext, const Move m) {
    *pnext = cur;
    pnext->b.initInfo();

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

template <int S_LEVEL, int E_LEVEL>
int L2Judge::judge(const int depth, Move *const buf,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field) {
    // 判定を返す
    nodes++;
    uint64_t fkey = -1;

    switch (S_LEVEL) {
        case 0: if (E_LEVEL <= 0) break; // fallthrough
        case 1:
            if (field.isNull() && myHand.qty == 1) return L2_WIN;
            if (E_LEVEL <= 1) break; // fallthrough
        case 2:
            if (field.isNull() && judgeMate_Easy_NF(myHand)) return L2_WIN;
            if (E_LEVEL <= 2) break; // fallthrough
        case 3:
            // 局面や相手の手札も考えた必勝判定
            assert(myHand.exam1stHalf() && opsHand.exam1stHalf());
            if (field.isNull()) {
                if (judgeHandPW_NF(myHand, opsHand, field.b)) return L2_WIN;
            }
            if (E_LEVEL <= 3) break; // fallthrough
        case 4:
            // 局面登録を検索
            // NFのみ
            if (field.isNull()) {
                assert(myHand.exam_key() && opsHand.exam_key());
                fkey = knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b));
                int result = L2::book.read(fkey);
                if (result != -1) return result; // 結果登録あり
            }
            if (E_LEVEL <= 4) break; // fallthrough
        case 5:
            // 簡易必敗判定
            if (field.isNull()) {
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
            }
            if (E_LEVEL <= 5) break; // fallthrough
        default: // 完全探索
            if (nodes > NODE_LIMIT) { failed = 1; return L2_DRAW; }

            const int NMoves = genMove(buf, myHand, field.b);
            childs += NMoves;

            // 即上がり判定
            if (field.b.qty() >= myHand.qty && NMoves >= 2) return L2_WIN;
            // 探索
            int winIndex = search(depth, buf, NMoves, myHand, opsHand, field);
            if (winIndex >= 0 || winIndex == -1) {
                int result = winIndex >= 0 ? L2_WIN : L2_LOSE;
                if (field.isNull()) L2::book.regist(result, fkey);
                return result;
            }
            break;
    }
    return L2_DRAW;
}

bool L2Judge::checkDomMate(const int depth, Move *const buf, Move& tmp,
                           const Hand& myHand, const Hand& opsHand, const L2Field& field) {
    if (field.isUnrivaled() || tmp.dominatesOthers()
        || dominatesCards(tmp, opsHand.cards, field.b)) { // 他支配チェック
        tmp.setDO();

        // 支配して残り1枚数なら勝ち
        if (myHand.qty - tmp.qty() <= 1) return true;

        int result;
        if (!tmp.isPASS()) {
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, tmp);
            if (judgeMate_Easy_NF(nextHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, tmp);
            nextHand.key = subCardKey(myHand.key, CardsToHashKey(tmp.cards()));
            result = judge<3, 4>(depth + 1, buf, nextHand, opsHand, nextField);
        } else {
            if (judgeMate_Easy_NF(myHand)) return true;
            L2Field nextField = procAndFlushL2Field(field, tmp);
            result = judge<3, 4>(depth + 1, buf, myHand, opsHand, nextField);
        }
        if (result == L2_WIN) return true;
    }
    return false;
}

int L2Judge::check(const int depth, Move *const buf, Move& tmp,
                   const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {

    if (!checkedEasy) {
        if (tmp.qty() >= myHand.qty) return L2_WIN;
        if (checkDomMate(depth, buf, tmp, myHand, opsHand, field)) return L2_WIN;
    }
    childs++;

    // 支配性判定
    if (!tmp.isPASS() && (field.isLastAwake() || tmp.dominatesOthers())) {
        if (dominatesCards(tmp, myHand.cards, field.b)) {
            tmp.setDM();
        }
    }

    Hand nextHand;
    L2Field nextField;
    int nextPlayer = procL2Field(field, &nextField, tmp);
    if (!tmp.isPASS()) {
        makeMoveAll(myHand, &nextHand, tmp);
        if (nextPlayer == 0) {
            return judge<1, 1024>(depth + 1, buf, nextHand, opsHand, nextField);
        } else {
            return L2_WIN + L2_LOSE - judge<1, 1024>(depth + 1, buf, opsHand, nextHand, nextField);
        }
    } else { // PASS
        if (nextPlayer == 0) {
            return judge<1, 1024>(depth + 1, buf, myHand, opsHand, nextField);
        } else {
            return L2_WIN + L2_LOSE - judge<1, 1024>(depth + 1, buf, opsHand, myHand, nextField);
        }
    }
}

int L2Judge::search(const int depth, Move *const buf, const int NMoves,
                    const Hand& myHand, const Hand& opsHand, const L2Field& field) {
    // 勝利手があればインデックス(>=0)、結果不明なら-2、負け確定なら-1を返す
    // 支配からの簡単詰み
    for (int i = NMoves - 1; i >= 0; i--) {
        if (checkDomMate(depth, buf + NMoves, buf[i], myHand, opsHand, field)) return i;
    }
    // 探索
    bool unFound = false;
    for (int i = NMoves - 1; i >= 0; i--) {
        int res = check(depth, buf + NMoves, buf[i], myHand, opsHand, field, true);
        if (res == L2_WIN) return i;
        if (res == L2_DRAW) unFound = true;
    }
    if (unFound) return -2;
    else return -1;
}

int L2Judge::start_judge(const Hand& myHand, const Hand& opsHand, const Board b) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    init();
    L2Field field = convL2Field(b); // L2型へのチェンジ
    return judge<1, 1024>(0, mbuf, myHand, opsHand, field);
}

int L2Judge::start_check(const Move m, const Hand& myHand, const Hand& opsHand, const Board b) {
    assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
    init();
    L2Field field = convL2Field(b); // L2型へのチェンジ
    Move tm = m;
    return check(0, mbuf, tm, myHand, opsHand, field);
}
