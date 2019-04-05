/*
 maxn_test.cc
 Katsuki Ohto
 */

// 不完全情報でのMaxN探索テスト

#include "../include.h"
#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "../fuji/maxN.hpp"

using namespace UECda;
using namespace UECda::Fuji;

MoveInfo buffer[8192];
Clock cl;
std::mt19937 mt;
XorShift64 dice;

int outputMateJudgeResult() {
    // 気になるケースやコーナーケース、代表的なケースでの支配性判定の結果を出力する
    
    Cards c[N_PLAYERS] = {0};
    
    c[0] = StringToCards("c6 ha");
    c[1] = StringToCards("ct sk");
    c[2] = StringToCards("s7 s2");
    
    Cards rem = CARDS_NULL;
    for (int p = 0; p < N_PLAYERS; ++p) {
        rem |= c[p];
    }
    
    Field field;
    
    field.initGame();
    field.setMoveBuffer(buffer);
    field.setDice(&dice);
    
    // game state
    field.bd = OrderToNullBoard(0);
    field.clearSeats();
    field.clearClasses();
    field.setTurnPlayer(0);
    for (int p = 0; p < N_PLAYERS; ++p) {
        if (anyCards(c[p])) {
            field.setHand(p, c[p]);
            field.setOpsHand(p, subtrCards(rem, c[p]));
        } else {
            field.hand[p].qty = 0;
            field.ps.setDead(p);
        }
        field.setOpsHand(p, subtrCards(rem, c[p]));
        field.setPlayerClass(p, p);
        field.setPlayerSeat(p, p);
    }
    field.setRemHand(rem);
    field.prepareForPlay();
    
    MaxNResult result;
    searchMaxN(&result, field, buffer);
    //cerr << field.toDebugString();
    cerr << result << endl;
    
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
    dice.srand((unsigned int)time(NULL));
    
    if (outputMateJudgeResult()) {
        cerr << "failed case test." << endl; return -1;
    }
    cerr << "passed case test." << endl;
    
    /*MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog<N_PLAYERS>>>, 256> mLogs(logFileNames);
    
    if (testRecordL2(mLogs)) {
        cerr << "failed record MaxN judge test." << endl;
    }
    cerr << "passed record MaxN judge test." << endl;
    */
    return 0;
}
