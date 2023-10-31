#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"
#include "mate.hpp"

using namespace std;

bool judgeHandMate(const int depth, MoveInfo *const mbuf,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo) {
    if (b.isNull()) {
        if (judgeHandPW_NF(myHand, opsHand, b)) return true;
    }

    if (b.isNull() && depth == 0) {
        // 階段パターンのみ検討
        // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
        if (myHand.seq) {
            int numMoves = genAllSeq(mbuf, myHand.cards);
            for (int i = 0; i < numMoves; i++) {
                const MoveInfo& m = mbuf[i];
                if (m.qty() >= myHand.qty) return true; // 即上がり

                Hand nextHand;
                makeMove1stHalf(myHand, &nextHand, m);

                // 5枚以上の階段は支配としておく
                if (m.qty() > 4) {
                    Board nb = b;
                    nb.procOrder(m);
                    // いずれかのオーダーで必勝ならOK
                    if (judgeHandPW_NF(nextHand, opsHand, b)
                        || judgeHandPW_NF(nextHand, opsHand, nb)) return true;
                } else {
                    if (dominatesHand(m, opsHand, b)) {
                        if (judgeHandPW_NF(nextHand, opsHand, b)) return true;
                    } else {
                        // 支配的でない場合、この役を最後に出すことを検討
                        if (judgeHandPPW_NF(nextHand.cards, nextHand.pqr, nextHand.jk, opsHand.nd, b)) return true;
                    }
                }
            }
        }
    } else {
        // depth > 0 のとき 空場でない場合は合法手生成して詳しく判定
        int numMoves = genMove(mbuf, myHand, b);
        if (searchHandMate(depth, mbuf, numMoves, myHand, opsHand, b, fieldInfo) != -1) return true;
    }
    return false;
}

bool checkHandBNPW(const int depth, MoveInfo *const mbuf, const MoveInfo m,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo) {
    // 間に他プレーヤーの着手を含んだ必勝を検討
    // 支配的でないことは前提とする
    if (m.isPASS()) return false; // パスからのBNPWはない

    // 相手に間で上がられる可能性がある場合
    if (fieldInfo.minNumCardsAwake() <= m.qty() && m.qty() <= fieldInfo.maxNumCardsAwake()) return false;

    // S3分岐必勝を検討
    // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
    // ジョーカー -> S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
    if (m.isSingle() // シングルのみ
        && containsS3(myHand.cards - m.cards())) { // 残り手札にS3がある
        Cards zone = ORToGValidZone(b.order(), m.rank());
        if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());
        if (!(zone & opsHand.cards)) { // ジョーカー以外は出せない
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, m);
            if (judgeHandPW_NF(nextHand, opsHand, b)) { // 流れた場合
                Move s3; s3.setSingle(INTCARD_S3);
                nextHand.makeMove1stHalf(s3, CARDS_S3, 1); // S3 で進める
                if (judgeHandPW_NF(nextHand, opsHand, b)) { // S3で返した場合
                    // 両方で勝ったので必勝
                    DERR << "BRPW(S3)!!!" << std::endl;
                    return true;
                }
            }
        }
    }

    int curOrder = b.prmOrder();
    Cards ops8 = opsHand.cards & CARDS_8;

    // BNPWを検討
    if (m.isSingle()) {
        // シングルジョーカーは少なくともBNPWではない
        if (m.isSingleJOKER()) return false;
        if (myHand.jk && !containsS3(opsHand.cards)) { // まずジョーカーを検討
            // 相手に8切りで返される可能性があればだめ
            if (b.locksSuits(m)) ops8 &= SuitsToCards(m.suits());
            if (ops8 && isValidGroupRank(RANK_8, curOrder, m.rank())) return false;
            // 残り1枚がジョーカーなら勝ち
            if (myHand.qty == m.qty() + 1) return true;

            // 簡単な勝ちの条件にはまらなかったので手札を更新して空場必勝を確認
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, m);
            Move sj; sj.setSingleJOKER();
            nextHand.makeMove1stHalf(sj, CARDS_JOKER, 1);
            FieldAddInfo nextFieldInfo;
            flushFieldAddInfo(fieldInfo, &nextFieldInfo);
            int n = std::min(fieldInfo.minNumCards(), fieldInfo.minNumCardsAwake() - m.qty());
            nextFieldInfo.setMinNumCards(n);
            nextFieldInfo.setMinNumCardsAwake(n);
            if (judgeHandMate(depth, mbuf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                return true;
            }
            // ジョーカー -> S3 からの空場必勝を確認
            if (containsS3(nextHand.cards)) {
                if (nextHand.qty == 1) return true;
                Move s3; s3.setSingle(INTCARD_S3);
                nextHand.makeMove1stHalf(s3, CARDS_S3, 1);
                if (judgeHandMate(depth, mbuf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                    return true;
                }
            }
        }
        // TODO: 他の役で検討
    }
    return false;
}

bool checkHandMate(const int depth, MoveInfo *const mbuf, MoveInfo& m,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo) {

    if (fieldInfo.isUnrivaled()) { // 独断場のとき
        m.setDO(); // 支配フラグ付加
        if (m.isPASS()) {
            Board nb = b;
            nb.flush();
            FieldAddInfo nextFieldInfo;
            flushFieldAddInfo(fieldInfo, &nextFieldInfo);
            return judgeHandMate(depth, mbuf, myHand, opsHand, nb, nextFieldInfo);
        } else {
            if (m.qty() >= myHand.qty) return true; // あがり
            Hand nextHand;
            Board nb = b;
            FieldAddInfo nextFieldInfo;
            makeMove1stHalf(myHand, &nextHand, m);
            if (dominatesCards(m, nextHand.cards, b)) { // 自己支配
                m.setDM(); // 支配フラグ付加
                nb.procAndFlush(m);
                flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                return judgeHandMate(depth, mbuf, nextHand, opsHand, nb, nextFieldInfo);
            } else { // セルフフォロー必勝を判定
                nb.proc(m);
                procUnrivaled(fieldInfo, &nextFieldInfo);
                return judgeHandMate(depth, mbuf, nextHand, opsHand, nb, nextFieldInfo);
            }
        }
        return false;
    }

    if (m.isPASS()) {
        // パス支配でない場合は判定終了
        if (!fieldInfo.isPassDom()) return false;

        // Unrivaledでないがパス支配の場合がある?
        // パス支配の場合, 流れてからの必勝を判定
        m.setDO(); // 支配フラグ付加
        FieldAddInfo nextFieldInfo;
        flushFieldAddInfo(fieldInfo, &nextFieldInfo);
        return judgeHandMate(depth, mbuf, myHand, opsHand,
                             OrderToNullBoard(b.prmOrder()), nextFieldInfo);
    }

    // 通常
    if (m.qty() >= myHand.qty) return true; // 即上がり
    if (dominatesHand(m, opsHand, b)
        || m.qty() > fieldInfo.maxNumCardsAwake()) { // 支配
        m.setDO(); // 支配フラグ付加

        Board nb = b;
        nb.proc(m);
        Hand nextHand;
        makeMove1stHalf(myHand, &nextHand, m);

        // セルフフォロー

        // 自分の出した役を流してからの必勝チェック
        // 永続的パラメータ変更を起こす場合はBNPW判定を続け、起こさない場合はPWのみ検討
        nb.flush();
        FieldAddInfo nextFieldInfo;
        flushFieldAddInfo(fieldInfo, &nextFieldInfo);
        if (judgeHandMate(m.isRev() ? depth : 0,
                          mbuf, nextHand, opsHand, nb, nextFieldInfo)) {
            return true;
        }
        // 自分の出したジョーカーをS3で返してからの必勝チェック
        if (m.isSingleJOKER() && containsS3(nextHand.cards)) {
            Move s3; s3.setSingle(INTCARD_S3);
            nextHand.makeMove1stHalf(s3, CARDS_S3, 1);
            return judgeHandMate(0, mbuf, nextHand, opsHand, nb, nextFieldInfo);
        }
    } else { // 支配しない
        if (depth <= 0) return false;

        // BNPWを検討
        assert(!fieldInfo.isLastAwake());
        if (checkHandBNPW(depth - 1, mbuf, m, myHand, opsHand, b, fieldInfo)) {
            DERR << "BNPW - " << m << " ( " << fieldInfo.minNumCardsAwake() << " ) " << std::endl;
            return true;
        }
    }
    return false;
}

int searchHandMate(const int depth, MoveInfo *const mbuf, int numMoves,
                   const Hand& myHand, const Hand& opsHand,
                   const Board& b, const FieldAddInfo& fieldInfo) {
    // 必勝手探し
    for (int i = numMoves - 1; i >= 0; i--) {
        if (mbuf[i].qty() == myHand.qty) return i;
    }
    for (int i = numMoves - 1; i >= 0; i--) {
        if (checkHandMate(depth, mbuf + numMoves, mbuf[i], myHand, opsHand, b, fieldInfo)) return i;
    }
    return -1;
}