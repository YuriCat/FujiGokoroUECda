
// ラスト2人完全情報状態の勝敗判定動作テスト

#include "../UECda.h"
#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "../engine/lastTwo.hpp"

using namespace UECda;

Move buffer[8192];
Clock cl;
std::mt19937 mt;

int outputMateJudgeResult() {
    // 気になるケースやコーナーケース、代表的なケースでの支配性判定の結果を出力する
    //Cards myCards = CARDS_C3 | CARDS_H3 | CARDS_D9 | CARDS_CQ | CARDS_HQ;
    //Cards oppCards = CARDS_C4 | CARDS_D4 | CARDS_S5 | CARDS_D6 /*| CARDS_H9*/ | CARDS_DK | CARDS_HK /*| CARDS_SK*/ | CARDS_JOKER;
    
    /*
     turn = 71
     player = 3
     class = {2,4,1,3,0}
     seat = {0,3,4,2,1}
     board = NULL  Order : NORMAL  Suits : FREE
     state = al{13} aw{13}
     hand =
     0 : { SQ }(0)
     1 : { H4 C7 D7 S7 H8 C9 ST }(7)
     2 : { C3 H3 }(0)
     3 : { S3 S4 H7 H9 DT HQ SK }(7)
     4 : { D6 H6 }(0)
     
     game 1503
     score
     class 2 4 1 3 0
     seat 0 3 4 2 1
     dealt { d3 d4 s5 c6 s6 d8 s8 sj sq hk } { c7 d7 s7 h8 c9 st dj hj ca ha c2 } { c3 h3 c4 hq dk sa d2 h2 s2 jo } { s3 s4 c5 h5 h7 c8 h9 dt cj sk da } { h4 d5 d6 h6 d9 s9 ct ht cq dq ck }
     changed { } { ha c2 } { hq } { da } { h4 d5 }
     original { d3 d4 s5 c6 s6 d8 s8 sj sq hk } { h4 d5 c7 d7 s7 h8 c9 st dj hj ca } { c3 h3 c4 dk da sa d2 h2 s2 jo } { s3 s4 c5 h5 h7 c8 h9 dt cj hq sk } { d6 h6 d9 s9 ct ht cq dq ck ha c2 }
     play d-3[1916109] c-2[1231495] p[100] p[65] jk[140708] p[85] p[58] p[59] c-4[1128775] s-5[1021434] c-t[1041389] p[2147739] c-a[2630023] p[107] p[63] p[51] p[53] d-5[4272813] d-k[158] p[72] p[51] p[39] p[42] d-a[36290] p[39187] dhs-2[39991] p[67] p[50] p[38] p[43] p[38963] s-a[39934] p[66] p[48] p[36] p[3023] p[35971] ch-3[40470] cs-6[250549] ds-9[150878] p[82] dh-j[1059628] p[95] cd-q[193318] p[82] p[65] h-t[279505] p[1373844] p[92] s-j[372380] c-k[88] p[29] h-a[39294] p[39479] dh-6[39999] p[85] p[448619] ds-8[240285] d-4[306337] c-j[777196] p[94] h-k[116] p[67] p[36722] s-q[39949] p[119] p[55] ch-5[43264] p[2241] p[37364] c-8[40833] p[38852] cd-7[788] p[38650] h-8[40111] c-9[39515] d-t[211] p[39037] p[121] h-9[39896] p[144] p[39385] s-3[39981] s-7[127] s-k[39219] p[78] p[39514] h-7[39995] s-t[124] h-q[39258] p[74] p[39531] s-4[39964]
     result 2 4 0 3 1
     */
    
    Cards myCards = "s3 s4 h7 h9 dt hq sk";
    Cards oppCards = "h4 c7 d7 s7 h8 c9 st";
    {
        Hand myHand, oppHand;
        myHand.setAll(myCards);
        oppHand.setAll(oppCards);
        Board bd = OrderToNullBoard(0);
        bd.setFlushLead();
        
        L2Judge judge(300000, buffer);
        int judgeResult = judge.start_judge(myHand, oppHand, bd);
        cerr << myHand << oppHand << " -> " << judgeResult << endl;
        
        genMove(buffer, myCards, bd);
    }
    
    {
        Hand myHand, oppHand;
        myHand.setAll(oppCards);
        oppHand.setAll(myCards);
        Board bd = OrderToNullBoard(0);
        bd.setFlushLead();
        
        L2Judge judge(300000, buffer);
        int judgeResult = judge.start_judge(myHand, oppHand, bd);
        cerr << myHand << oppHand << " -> " << judgeResult << endl;
    }
    
    
    return 0;
}

int testRecordL2(const Record& record) {
    // 棋譜中の局面においてL2判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    // 正解を調べるのがきついこともあるのでとりあえず棋譜の結果を正解とする
    
    // judge(高速判定)
    uint64_t judgeTime[2] = {0};
    uint64_t judgeCount = 0;
    uint64_t judgeMatrix[2][3] = {0};
    
    int judgeResult = -1;
    int l2TurnPlayer = -1;
    Field field;
    
    for (int i = 0; i < record.games(); i++) {
        iterateGameLogAfterChange(field, record.game(i),
        [&](const auto& field) {}, // first callback
        [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
            
            if (field.getNAlivePlayers() == 2) {
                const int turnPlayer = field.turn();
                const int oppPlayer = field.ps.searchOpsPlayer(turnPlayer);
                const Hand& myHand = field.getHand(turnPlayer);
                const Hand& oppHand = field.getHand(oppPlayer);
                Board b = field.board;
                
                cl.start();
                L2Judge judge(65536, buffer);
                judgeResult = judge.start_judge(myHand, oppHand, b);
                judgeTime[0] += cl.stop();
                judgeCount += 1;
                
                l2TurnPlayer = turnPlayer;
                
                /*if (mate != pw) {
                cerr << "judge " << mate << " <-> " << " answer " << pw << endl;
                cerr << move << " on " << b << endl;
                cerr << field.ps << " " << toInfoString(b) << endl;
                cerr << Out2CardTables(myHand.cards, opsHand.cards);
                cerr << endl;
                getchar();
                }*/
                
                // L2に入った瞬間だけテストする
                //cerr << field.toString() << endl;
                return -1;
            }
            return 0;
        },
        [&](const auto& field) { // last callback
            // ここで新しい順位を得ることができる
            if (judgeResult != L2_NONE && l2TurnPlayer >= 0) {
                int turnResult = field.newClassOf(l2TurnPlayer);
                judgeMatrix[turnResult == N_PLAYERS - 2][judgeResult == L2_WIN ? 2 : (judgeResult == L2_DRAW ? 1 : 0)] += 1;
                /*if (turnResult == N_PLAYERS - 1 && judgeResult == L2_WIN) {
                    cerr << "bad result" << endl;
                    getchar();
                }*/
            }
        });
    }
    
    cerr << "judge result (hand) = " << endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 3; ++j) {
            cerr << judgeMatrix[i][j] << " ";
        }cerr << endl;
    }
    cerr << "judge time (hand)    = " << judgeTime[0] / (double)judgeCount << endl;
    //cerr << "judge time (pw-slow) = " << judgeTime[1] / (double)judgeCount << endl;
    
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
    
    if (outputMateJudgeResult()) {
        cerr << "failed case test." << endl; return -1;
    }
    cerr << "passed case test." << endl;
    
    Record record(logFileNames);
    
    if (testRecordL2(record)) {
        cerr << "failed record L2 judge test." << endl;
    }
    cerr << "passed record L2 judge test." << endl;
    
    return 0;
}
