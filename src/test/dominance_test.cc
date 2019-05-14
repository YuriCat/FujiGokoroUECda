
// 支配性判定のテスト

#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "../core/dominance.hpp"
#include "test.h"

using namespace std;

static MoveInfo buffer[8192];
static Clock cl;
static std::mt19937 mt;

int outputDominanceJudgeResult() {
    // 気になるケースやコーナーケース、代表的なケースでの支配性判定の結果を出力する
    return 0;
}

// カード集合への支配性について合法着手生成関数を用いてチェック
bool dominatesSlow(const Board b, const Cards c) {
    if (b.isNull()) return false;
    int moves = genFollowExceptPASS(buffer, c, b);
    return moves <= 0;
}
bool dominatesSlow(const Move m, const Cards c, Board b) {
    if (m.isPASS()) return false;
    b.proc(m);
    if (b.isNull()) return true;
    return dominatesSlow(b, c);
}

int testRecordMoveDominance(const Record& record) {
    // 棋譜中の局面において支配性判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    
    uint64_t judgeTime[4] = {0};
    uint64_t judgeCount[2] = {0};
    uint64_t judgeMatrix[2][2][2] = {0};
    Field field;
    
    // プリミティブ型での支配性判定
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            int turn = field.turn();
            Cards opsCards = field.getOpsCards(turn);
            Board b = field.board;
            cl.start();
            bool dom = dominatesCards(move, opsCards, b);
            judgeTime[0] += cl.stop();
            judgeCount[0] += 1;
            
            cl.start();
            bool ans = dominatesSlow(move, opsCards, b);
            judgeTime[1] += cl.stop();
            
            /*if (dom != ans) {
                cerr << "judge " << dom << " <-> " << " answer " << ans << endl;
                cerr << move << " on " << b << endl;
                cerr << OutCardTable(opsCards);
                cerr << endl;
                getchar();
            }*/
            
            judgeMatrix[0][ans][dom] += 1;
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
    }

    // 場からの判定
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange
        (field, record.game(i),
        [&](const Field& field) {}, // first callback
        [&](const Field& field, const Move move, const uint64_t time)->int{ // play callback
            Cards remCards = field.remCards;
            Hand remHand;
            remHand.set(remCards);
            Board b = field.board;

            cl.start();
            bool dom = dominatesCards(b, remCards);
            judgeTime[2] += cl.stop();
            judgeCount[1] += 1;
            
            cl.start();
            bool ans = dominatesSlow(b, remCards);
            judgeTime[3] += cl.stop();
            
            judgeMatrix[1][ans][dom] += 1;
            return 0;
        },
        [&](const Field& field) {} // last callback
        );
    }
    
    cerr << "judge result (move - cards) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << judgeMatrix[0][i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "judge result (board - cards) = " << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << judgeMatrix[1][i][j] << " ";
        }
        cerr << endl;
    }
    
    cerr << "judge time (move  - cards) = " << judgeTime[0] / (double)judgeCount[0] << endl;
    cerr << "judge time (move  - slow)  = " << judgeTime[1] / (double)judgeCount[0] << endl;
    cerr << "judge time (board - cards) = " << judgeTime[2] / (double)judgeCount[1] << endl;
    cerr << "judge time (board - slow)  = " << judgeTime[3] / (double)judgeCount[1] << endl;

    return 0;
}

bool DominanceTest(const vector<string>& recordFiles) {

    mt.seed(1);
    
    if (outputDominanceJudgeResult()) {
        cerr << "failed case test." << endl;
        return false;
    }
    cerr << "passed case test." << endl;

    Record record(recordFiles);
    if (testRecordMoveDominance(record)) {
        cerr << "failed record move dominance judge test." << endl;
    }
    cerr << "passed record move dominance judge test." << endl;
    
    return true;
}
