
// 必勝判定の動作テスト

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

template <int M = 0>
int searchCardsPWSlow(MoveInfo *const, const int, const int,
                      const Cards, const Cards,
                      Board, PlayersState, bool);

template <int M = 0>
bool judgeCardsPWSlow(MoveInfo *const buf,
                      const int p,
                      const Cards myCards, const Cards opsCards,
                      Board bd, PlayersState ps, bool flushLead) {
    uint64_t key = uint64_t(myCards) ^ -bd.toInt();
    if (bd.isNull()) {
        if (visitedCards.find(key) != visitedCards.end()) {
            return visitedCards[key];
        }
        //int ans = visitedCards.read(myCards | -uint32_t(bd));
        //if (ans >= 0) { return bool(ans); }
    }
    const int myMoves = genMove(buf, myCards, bd);
    bool pw = searchCardsPWSlow<M>(buf, myMoves, p, myCards, opsCards, bd, ps, flushLead) >= 0;
    if (bd.isNull()) {
        visitedCards[key] = pw;
        //visitedCards.regist(int(pw), myCards ^ -uint32_t(bd));
    }
    return pw;
}

template <int M = 0>
bool checkCardsPWSlow(MoveInfo *const buf,
                      const int p, const Move move,
                      Cards myCards, const Cards opsCards,
                      Board bd, PlayersState ps, bool flushLead) {
    // 自分以外のカード集合に対して完全勝利(PW)状態であるか合法着手生成関数を用いてチェック
    myCards = maskCards(myCards, move.cards());
    if (anyCards(myCards)) {
        if (move.isPASS()) {
            if (flushLead) {
                if (!ps.isAllAsleepExcept(p)) {
                    // 他のプレーヤーが出せるか確認
                    const int opsMoves = genFollowExceptPASS(buf, opsCards, bd);
                    if (opsMoves > 0) return false;
                }
                bd.flush();
                ps.flush();
                return judgeCardsPWSlow<M>(buf, p, myCards, opsCards, bd, ps, true);
            } else { // 残りの人がパスして自分のターンでない
                return false;
            }
        } else {
            bd.proc(move);
            if (!bd.isNull()) { // 流れていない
                if (!ps.isAllAsleepExcept(p)) {
                    // 他のプレーヤーが出せるか確認
                    const int opsMoves = genFollowExceptPASS(buf, opsCards, bd);
                    if (opsMoves > 0) {
                        // ここからBNPW判定
                        /*for (int m = 0; m < opsMoves; m++) {
                            Move opsMove = buf[m];
                            Board nextBd = bd;
                            nextBd.proc(opsMove);
                            if (nextBd.isNull())continue; // 相手に流されたらそこで終了
                            // ここに対して自分の必勝手があるか調べる
                            judgeCardsPWSlow(buf + opsMoves, p,
                                             myCards, subtrCards(opsCards, opsMove.cards()),
                                             nextBd, ps,
                        }*/
                        return false;
                    }
                    ps.setAllAsleep();
                    ps.setAwake(p);
                }
                // セルフフォロー
            } else {
                ps.flush();
            }
            return judgeCardsPWSlow<M>(buf, p, myCards, opsCards, bd, ps, true);
        }
    }
    return true;
}

template <int M>
int searchCardsPWSlow(MoveInfo *const buf, const int moves,
                      const int p,
                      const Cards myCards, const Cards opsCards,
                      Board bd, PlayersState ps, bool flushLead) {
    int mateIndex = -1;
    for (int i = 0; i < moves; i++) {
        if (buf[i].qty() >= countCards(myCards)) { // final move
            mateIndex = i;
            if (M) mateMoves.insert(buf[mateIndex].toInt());
            else return i;
        }
    }
    if (mateIndex >= 0) return mateIndex;
    for (int i = 0; i < moves; i++) {
        if (checkCardsPWSlow<M>(buf + moves, p, buf[i], myCards, opsCards, bd, ps, flushLead)) {
            if (M) mateIndex = i;
            else return i;
        }
    }
    return mateIndex;
}

int testRecordMoveMate(const Record& record) {
    // 棋譜中の局面において必勝判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    
    // judge(高速判定)
    uint64_t judgeTime[2] = {0};
    uint64_t judgeCount = 0;
    uint64_t judgeMatrix[2][2] = {0};
    Field field;
    
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board bd = field.board;
            if (!bd.isNull()) return 0;

            cl.start();
            bool mate = judgeHandPW_NF(myHand, opsHand, bd);
            judgeTime[0] += cl.stop();
            judgeCount += 1;
            
            cl.start();
            visitedCards.clear();
            bool pw = judgeCardsPWSlow(buffer, turnPlayer,
                                        myHand.cards, opsHand.cards, bd, field.ps, field.fieldInfo.isFlushLead());
            judgeTime[1] += cl.stop();         
            judgeMatrix[pw][mate] += 1;
    
            /*if (mate && !pw) {
                cerr << field.toDebugString() << endl;
                getchar();
            }*/
            //cerr << "next" << endl;
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
    }
    cerr << "judge result (hand) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << judgeMatrix[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "judge time (hand)    = " << judgeTime[0] / (double)judgeCount << endl;
    cerr << "judge time (pw-slow) = " << judgeTime[1] / (double)judgeCount << endl;
    
    // check
    uint64_t checkTime[2] = {0};
    uint64_t checkCount = 0;
    
    uint64_t checkMatrix[2][2] = {0};
    
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board bd = field.board;
            MoveInfo mi = MoveInfo(move);
            
            if (dominatesHand(bd, myHand)) return 0;
            
            cl.start();
            bool mate = checkHandMate(1, buffer, mi, myHand, opsHand, bd, field.fieldInfo);
            checkTime[0] += cl.stop();
            checkCount += 1;

            cl.start();
            visitedCards.clear();
            bool pw = checkCardsPWSlow(buffer, turnPlayer, move,
                                        myHand.cards, opsHand.cards, bd, field.ps, field.fieldInfo.isFlushLead());
            checkTime[1] += cl.stop();
            checkMatrix[pw][mate] += 1;
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
    }

    cerr << "check result (hand) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << checkMatrix[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "check time (hand)    = " << checkTime[0] / (double)checkCount << endl;
    cerr << "check time (pw-slow) = " << checkTime[1] / (double)checkCount << endl;
    
    // search
    uint64_t searchTime[2] = {0};
    uint64_t searchCount = 0;
    
    uint64_t searchMatrix[2][2] = {0};
    

    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board bd = field.board;
            
            const int moves = genMove(buffer, myHand, bd);
            if (moves <= 1) return 0;
            
            cl.start();
            int mateIndex = searchHandMate(1, buffer, moves, myHand, opsHand, bd, field.fieldInfo);
            searchTime[0] += cl.stop();
            searchCount += 1;
            
            /*for (int m = 0; m < moves; m++) {
                bool check = checkHandMate(0, buffer + moves, buffer[m], myHand, opsHand, bd, 1, 1);
                cerr << buffer[m] << " : " << check << endl;
            }*/
            
            cl.start();
            visitedCards.clear();
            int pwIndex = searchCardsPWSlow(buffer, moves, turnPlayer,
                                            myHand.cards, opsHand.cards,
                                            bd, field.ps, bool(field.fieldInfo.isFlushLead()));
            searchTime[1] += cl.stop();
            
            searchMatrix[(pwIndex >= 0)][(mateIndex >= 0)] += 1;
            
            /*if ((mateIndex >= 0) != (pwIndex >= 0)) {
                
                for (int m = 0; m < moves; m++) {
                    bool check = checkHandMate(0, buffer + moves, buffer[m], myHand, opsHand, bd, 1, 1);
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
                cerr << move << " on " << bd << endl;
                cerr << field.ps << " " << field.fieldInfo << endl;
                cerr << Out2CardTables(myHand.getCards(), opsHand.getCards()) << endl;
                cerr << field.toString();
                getchar();
            }*/
            
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
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
    
    return 0;
}

int analyzeMateDistribution(const Record& record) {
    
    // search
    std::array<uint64_t, 12> mateMovesDistribution = {0};
    Field field;
    
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.turn();
            const Hand& myHand = field.getHand(turnPlayer);
            const Hand& opsHand = field.getOpsHand(turnPlayer);
            Board bd = field.board;
            mateMoves.clear();
            visitedCards.clear();
            bool pw = judgeCardsPWSlow<1>(buffer, turnPlayer,
                                        myHand.cards, opsHand.cards, bd, field.ps, field.fieldInfo.isFlushLead());
            if (pw) mateMovesDistribution[bsf32(mateMoves.size())] += 1;
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
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
