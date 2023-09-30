
// 必勝判定の動作テスト

#include <unordered_map>
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../core/record.hpp"
#include "../engine/mate.hpp"
#include "test.h"

using namespace std;

static MoveInfo buffer[8192];
static Clock cl;
static std::mt19937 mt;

int outputMateJudgeResult() {
    // 気になるケースやコーナーケース、代表的なケースでの支配性判定の結果を出力する
    return 0;
}

std::set<uint32_t> mateMoves;
std::unordered_map<uint64_t, bool> visitedCards;

int searchCardsPWSlow(bool, bool,
                      MoveInfo *const, const int, const int,
                      const Cards, const Cards,
                      Board, PlayersState, bool, int, int);

bool judgeCardsPWSlow(bool analyze, bool ppw,
                      MoveInfo *const buf,
                      const int p,
                      const Cards myCards, const Cards opsCards,
                      Board b, PlayersState ps, bool flushLead, int maxNumCardsAwake, int maxNumCards) {
    if (!anyCards(myCards)) return true;
    uint64_t key = uint64_t(myCards) ^ -b.toInt();
    if (b.isNull() && visitedCards.find(key) != visitedCards.end()) return visitedCards[key];
    const int myMoves = genMove(buf, myCards, b);
    bool pw = searchCardsPWSlow(analyze, ppw, buf, myMoves, p, myCards, opsCards, b, ps, flushLead, maxNumCardsAwake, maxNumCards) >= 0;
    if (b.isNull()) visitedCards[key] = pw;
    return pw;
}

bool checkCardsPWSlow(bool analyze, bool ppw,
                      MoveInfo *const buf,
                      const int p, const Move move,
                      Cards myCards, const Cards opsCards,
                      Board b, PlayersState ps, bool flushLead, int maxNumCardsAwake, int maxNumCards) {
    // 自分以外のカード集合に対して完全勝利(PW)状態であるか合法着手生成関数を用いてチェック
    myCards = maskCards(myCards, move.cards());
    if (!ppw && !anyCards(myCards)) return true;
    if (move.isPASS()) {
        if (!flushLead) return false; // 残りの人がパスして自分のターンでない
        if (ps.numAwake() >= 2) {
            // 他のプレーヤーが出せるか確認
            if (b.qty() <= maxNumCardsAwake) {
                const int opsMoves = genFollowExceptPASS(buf, opsCards, b);
                if (opsMoves > 0) return false;
            }
        }
        b.flush();
        ps.flush();
        maxNumCardsAwake = maxNumCards;
    } else {
        b.proc(move);
        if (!b.isNull()) { // 流れていない
            if (ps.numAwake() >= 2) {
                // 他のプレーヤーが出せるか確認
                if (b.qty() <= maxNumCardsAwake) {
                    const int opsMoves = genFollowExceptPASS(buf, opsCards, b);
                    bool ok = false;
                    if (opsMoves > 0) {
                        if (ppw) return false;
                        // ここからBNPW判定
                        // S3分岐必勝
                        if (opsMoves == 1 && buf[0].isSingleJOKER() && containsS3(myCards) && maxNumCardsAwake > 1) {
                            Board nb = b; nb.flush();
                            PlayersState nps = ps; nps.flush();
                            if (judgeCardsPWSlow(analyze, ppw, buf, p, myCards, opsCards, nb, nps, true, maxNumCards, maxNumCards)
                                && judgeCardsPWSlow(analyze, ppw, buf, p, myCards - CARDS_S3, opsCards - CARDS_JOKER, nb, nps, true, maxNumCards, maxNumCards)) return true;
                        }
                        return false;

                        /*for (int m = 0; m < opsMoves; m++) {
                            Move opsMove = buf[m];
                            Board nb = b;
                            nb.proc(opsMove);
                            if (nb.isNull()) { // 相手に流されたらそこで終了
                                ok = false;
                                break;
                            }
                        }

                            // ここに対して自分の必勝手があるか調べる
                            judgeCardsPWSlow(buf + opsMoves, p,
                                            myCards, subtrCards(opsCards, opsMove.cards()),
                                            nb, ps,
                        }*/
                        return false;
                    }
                }
                ps.setAllAsleepExcept(p);
            }
            // セルフフォロー
            maxNumCardsAwake = 0;
        } else {
            ps.flush();
            maxNumCardsAwake = maxNumCards;
        }
    }
    // 支配性の確認が完了
    if (ppw && !anyCards(myCards)) return true;
    return judgeCardsPWSlow(analyze, ppw, buf, p, myCards, opsCards, b, ps, true, maxNumCardsAwake, maxNumCards);
}

int searchCardsPWSlow(bool analyze, bool ppw,
                      MoveInfo *const buf, const int numMoves,
                      const int p,
                      const Cards myCards, const Cards opsCards,
                      Board b, PlayersState ps, bool flushLead, int maxNumCardsAwake, int maxNumCards) {
    int mateIndex = -1;
    for (int i = 0; i < numMoves; i++) {
        if (!ppw && buf[i].qty() >= countCards(myCards)) { // final move
            mateIndex = i;
            if (analyze) mateMoves.insert(buf[mateIndex].toInt());
            else return i;
        }
    }
    if (mateIndex >= 0) return mateIndex;
    for (int i = 0; i < numMoves; i++) {
        if (checkCardsPWSlow(analyze, ppw, buf + numMoves, p, buf[i], myCards, opsCards, b, ps, flushLead, maxNumCardsAwake, maxNumCards)) {
            if (analyze) mateIndex = i;
            else return i;
        }
    }
    return mateIndex;
}

int testRecordMoveMate(const Record& record) {
    // 棋譜中の局面において必勝判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    Field field;

    // judge(高速判定)
    long long judgeTime[4] = {0};
    long long judgeCount = 0;
    long long judgeMatrix[3][2][2] = {0};

    for (int i = 0; i < record.games(); i++) {
        for (Move move : PlayRoller(field, record.game(i))) {
            if (!field.isNull()) continue;

            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board b = field.board;

            cl.start();
            bool mate0 = judgeMate_Easy_NF(myHand);
            judgeTime[0] += cl.stop();
            judgeCount += 1;

            cl.start();
            bool mate1 = judgeHandPW_NF(myHand, opsHand, b);
            judgeTime[1] += cl.stop();

            cl.start();
            bool mate2 = judgeHandMate(0, buffer, myHand, opsHand, b, field.fieldInfo);
            judgeTime[2] += cl.stop();

            cl.start();
            visitedCards.clear();
            bool pw = judgeCardsPWSlow(
                false, false,
                buffer, turnPlayer,
                myHand.cards, opsHand.cards, b, field.ps, field.fieldInfo.isFlushLead(),
                field.fieldInfo.maxNumCardsAwake(), field.fieldInfo.maxNumCards()
            );
            judgeTime[3] += cl.stop();

            judgeMatrix[0][pw][mate0] += 1;
            judgeMatrix[1][pw][mate1] += 1;
            judgeMatrix[2][pw][mate2] += 1;
            //if (mate2 && !pw && !canMakeSeq(myHand.cards, 5)) { cerr << field.toDebugString(); getchar(); }
        }
    }

    cerr << "judge result (hand) = " << endl;
    const string type[3] = {"easy", "pqr-nd", "hand"};
    for (int d = 0; d < 3; d++) {
        cerr << type[d] << ":" << endl;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                cerr << judgeMatrix[d][i][j] << " ";
            }
            cerr << endl;
        }
    }
    cerr << "judge time (easy)    = " << judgeTime[0] / (double)judgeCount << endl;
    cerr << "judge time (pqr-nd)  = " << judgeTime[1] / (double)judgeCount << endl;
    cerr << "judge time (hand)    = " << judgeTime[2] / (double)judgeCount << endl;
    cerr << "judge time (pw-slow) = " << judgeTime[3] / (double)judgeCount << endl;

    // check
    long long checkTime[4] = {0};
    long long checkCount = 0;
    long long checkMatrix[3][2][2] = {0};

    for (int i = 0; i < record.games(); i++) {
        for (Move move : PlayRoller(field, record.game(i))) {
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board b = field.board;
            MoveInfo m = MoveInfo(move);

            if (dominatesHand(b, myHand)) continue;

            cl.start();
            bool mate0 = checkHandMate(0, buffer, m, myHand, opsHand, b, field.fieldInfo);
            checkTime[0] += cl.stop();
            checkCount += 1;

            cl.start();
            bool mate1 = checkHandMate(1, buffer, m, myHand, opsHand, b, field.fieldInfo);
            checkTime[1] += cl.stop();

            cl.start();
            bool mate2 = checkHandMate(2, buffer, m, myHand, opsHand, b, field.fieldInfo);
            checkTime[2] += cl.stop();

            cl.start();
            visitedCards.clear();
            bool pw = checkCardsPWSlow(
                false, false,
                buffer, turnPlayer, move,
                myHand.cards, opsHand.cards, b, field.ps, field.fieldInfo.isFlushLead(),
                field.fieldInfo.maxNumCardsAwake(), field.fieldInfo.maxNumCards()
            );
            checkTime[3] += cl.stop();

            checkMatrix[0][pw][mate0] += 1;
            checkMatrix[1][pw][mate1] += 1;
            checkMatrix[2][pw][mate2] += 1;
            //if (mate1 && !pw && !canMakeSeq(myHand.cards, 5)) { cerr << field.toDebugString() << move << endl; getchar(); }
        }
    }

    cerr << "check result (hand) = " << endl;
    for (int d = 0; d < 3; d++) {
        cerr << "depth" << d << endl;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                cerr << checkMatrix[d][i][j] << " ";
            }
            cerr << endl;
        }
    }
    cerr << "check time (hand d0) = " << checkTime[0] / (double)checkCount << endl;
    cerr << "check time (hand d1) = " << checkTime[1] / (double)checkCount << endl;
    cerr << "check time (hand d2) = " << checkTime[2] / (double)checkCount << endl;
    cerr << "check time (pw-slow) = " << checkTime[3] / (double)checkCount << endl;

    // search
    long long searchTime[2] = {0};
    long long searchCount = 0;
    long long searchMatrix[2][2] = {0};

    for (int i = 0; i < record.games(); i++) {
        for (Move move : PlayRoller(field, record.game(i))) {
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board b = field.board;

            const int moves = genMove(buffer, myHand, b);
            if (moves <= 1) continue;

            cl.start();
            int mateIndex = searchHandMate(1, buffer, moves, myHand, opsHand, b, field.fieldInfo);
            searchTime[0] += cl.stop();
            searchCount += 1;

            /*for (int m = 0; m < moves; m++) {
                bool check = checkHandMate(0, buffer + moves, buffer[m], myHand, opsHand, b, 1, 1);
                cerr << buffer[m] << " : " << check << endl;
            }*/

            cl.start();
            visitedCards.clear();
            int pwIndex = searchCardsPWSlow(
                false, false,
                buffer, moves, turnPlayer,
                myHand.cards, opsHand.cards,
                b, field.ps, field.fieldInfo.isFlushLead(),
                field.fieldInfo.maxNumCardsAwake(), field.fieldInfo.maxNumCards()
            );
            searchTime[1] += cl.stop();

            searchMatrix[(pwIndex >= 0)][(mateIndex >= 0)] += 1;

            /*if ((mateIndex >= 0) != (pwIndex >= 0)) {

                for (int m = 0; m < moves; m++) {
                    bool check = checkHandMate(0, buffer + moves, buffer[m], myHand, opsHand, b, 1, 1);
                    cerr << buffer[m] << " : " << check << endl;
                }

                cerr << "search ";
                if (mateIndex >= 0) {
                    cerr << buffer[mateIndex];
                } else {
                    cerr << "none";
                }
                cerr << " <-> " << " answer ";
                if (pwIndex >= 0) {
                    cerr << buffer[pwIndex];
                } else {
                    cerr << "none";
                }
                cerr << endl;
                cerr << move << " on " << b << endl;
                cerr << field.ps << " " << field.fieldInfo << endl;
                cerr << Out2CardTables(myHand.getCards(), opsHand.getCards()) << endl;
                cerr << field.toString();
                getchar();
            }*/
        }
    }

    cerr << "search result (hand) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << searchMatrix[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "search time (hand)    = " << searchTime[0] / (double)searchCount << endl;
    cerr << "search time (pw-slow) = " << searchTime[1] / (double)searchCount << endl;

    // ppw (最後の役まで支配する完全詰み)
    long long ppwTime[2] = {0};
    long long ppwCount = 0;
    long long ppwMatrix[2][2] = {0};

    for (int i = 0; i < record.games(); i++) {
        for (Move move : PlayRoller(field, record.game(i))) {
            if (!field.isNull()) continue;

            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board b = field.board;

            cl.start();
            bool judged = judgeHandPPW_NF(myHand.cards, myHand.pqr, myHand.jk, opsHand.nd, b);
            ppwTime[0] += cl.stop();
            ppwCount += 1;

            cl.start();
            visitedCards.clear();
            bool ppw = judgeCardsPWSlow(
                false, true,
                buffer, turnPlayer,
                myHand.cards, opsHand.cards, b, field.ps, field.fieldInfo.isFlushLead(),
                field.fieldInfo.maxNumCardsAwake(), field.fieldInfo.maxNumCards()
            );
            ppwTime[1] += cl.stop();

            ppwMatrix[ppw][judged] += 1;
        }
    }

    cerr << "ppw result (hand) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << ppwMatrix[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "ppw time (pqr-nd)   = " << ppwTime[0] / (double)ppwCount << endl;
    cerr << "ppw time (ppw-slow) = " << ppwTime[1] / (double)ppwCount << endl;

    return 0;
}

int analyzeMateDistribution(const Record& record) {

    // search
    std::array<uint64_t, 12> mateMovesDistribution = {0};
    Field field;

    for (int i = 0; i < record.games(); i++) {
        for (Move move : PlayRoller(field, record.game(i))) {
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board b = field.board;
            mateMoves.clear();
            visitedCards.clear();
            bool pw = judgeCardsPWSlow(
                true, false,
                buffer, turnPlayer,
                myHand.cards, opsHand.cards, b, field.ps, field.fieldInfo.isFlushLead(),
                field.fieldInfo.maxNumCardsAwake(), field.fieldInfo.maxNumCards()
            );
            if (pw) mateMovesDistribution[bsf32(mateMoves.size())] += 1;
        }
    }

    cerr << "number of mate moves = " << mateMovesDistribution << endl;
    return 0;
}

bool MateTest(const vector<string>& recordFiles) {
    std::vector<std::string> logFileNames;

    mt.seed(1);

    if (outputMateJudgeResult()) {
        cerr << "failed case test." << endl;
        return false;
    }
    cerr << "passed case test." << endl;

    Record record(recordFiles);

    if (testRecordMoveMate(record)) {
        cerr << "failed record move mate judge test." << endl;
    }
    cerr << "passed record move mate judge test." << endl;

    analyzeMateDistribution(record);
    cerr << "finished analyzing mate moves distribution." << endl;

    return true;
}
