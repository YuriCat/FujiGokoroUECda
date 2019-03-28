
// 支配性判定のテスト

#include "../include.h"
#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "../core/dominance.hpp"

using namespace UECda;

MoveInfo buffer[8192];
MoveGenerator<MoveInfo> mgCards;
Clock cl;
std::mt19937 mt;

int outputDominanceJudgeResult() {
    // 気になるケースやコーナーケース、代表的なケースでの支配性判定の結果を出力する
    return 0;
}

bool dominateCardsSlow(const Move move, const Cards cards, const Board abd) {
    // カード集合への支配性について合法着手生成関数を用いてチェック
    if (move.isPASS()) { return false; }
    Board bd = abd;
    bd.proc(move);
    if (bd.isNF()) { return true; }
    int moves = mgCards.genFollowExceptPASS(buffer, cards, bd);
    if (moves <= 0) { // pass only
        return true;
    }
    return false;
}

int testRecordMoveDominance(const std::vector<std::string>& logs) {
    // 棋譜中の局面において支配性判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog>>, 256> mLogs(logs);
    
    uint64_t judgeTime[3] = {0};
    uint64_t judgeCount[2] = {0};
    
    uint64_t judgeMatrix[2][2][2] = {0};
    Field field;
    
    // プリミティブ型での支配性判定
    iterateGameLogAfterChange
    (field, mLogs,
     [&](const auto& field) {}, // first callback
     [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
         int turnPlayer = field.getTurnPlayer();
         Cards opsCards = field.getOpsCards(turnPlayer);
         Board bd = field.getBoard();
         cl.start();
         bool dom = dominatesCards(move, opsCards, bd);
         judgeTime[0] += cl.stop();
         judgeCount[0] += 1;
         
         cl.start();
         bool ans = dominateCardsSlow(move, opsCards, bd);
         judgeTime[2] += cl.stop();
         
         /*if (dom != ans) {
             cerr << "judge " << dom << " <-> " << " answer " << ans << endl;
             cerr << move << " on " << bd << endl;
             cerr << OutCardTable(opsCards);
             cerr << endl;
             //getchar();
         }*/
         
         judgeMatrix[0][ans][dom] += 1;
         
         return 0;
     },
     [&](const auto& field) {} // last callback
     );
    
    // より複雑な型での支配性判定
    iterateGameLogAfterChange
    (field, mLogs,
     [&](const auto& field) {}, // first callback
     [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
         int turnPlayer = field.getTurnPlayer();
         const Hand& opsHand = field.getOpsHand(turnPlayer);
         Board bd = field.getBoard();
         cl.start();
         bool dom = dominatesHand(move, opsHand, bd);
         judgeTime[1] += cl.stop();
         judgeCount[1] += 1;
         
         cl.start();
         bool ans = dominateCardsSlow(move, opsHand.getCards(), bd);
         judgeTime[2] += cl.stop();
         
         judgeMatrix[1][ans][dom] += 1;
         
         return 0;
     },
     [&](const auto& field) {} // last callback
     );
    
    cerr << "judge result (cards) = " << endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            cerr << judgeMatrix[0][i][j] << " ";
        }cerr << endl;
    }
    cerr << "judge result (hand) = " << endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            cerr << judgeMatrix[1][i][j] << " ";
        }cerr << endl;
    }
    
    cerr << "judge time (cards) = " << judgeTime[0] / (double)judgeCount[0] << endl;
    cerr << "judge time (hand)  = " << judgeTime[1] / (double)judgeCount[1] << endl;
    cerr << "judge time (slow)  = " << judgeTime[2] / (double)(judgeCount[0] + judgeCount[1]) << endl;
    
    return 0;
}

int main(int argc, char* argv[]) {
    
    std::vector<std::string> logFileNames;
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-l")) {
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    mt.seed(1);
    
    if (outputDominanceJudgeResult()) {
        cerr << "failed case test." << endl; return -1;
    }
    cerr << "passed case test." << endl;
    if (testRecordMoveDominance(logFileNames)) {
        cerr << "failed record move dominance judge test." << endl;
    }
    cerr << "passed record move dominance judge test." << endl;
    
    return 0;
}
