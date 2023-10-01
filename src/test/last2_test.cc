
// ラスト2人完全情報状態の勝敗判定動作テスト

#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "../engine/last2.hpp"
#include "test.h"

using namespace std;

static MoveInfo buffer[8192];
static Clock cl;
static std::mt19937 mt;

bool checkLast2Slow(MoveInfo *const, MoveInfo, Cards, Cards, Board, bool, bool);

bool judgeLast2Slow(MoveInfo *const buf,
                    const Cards myCards, const Cards opsCards, Board b,
                    bool lastAwake, bool flushLead) {
    int numMoves = genMove(buf, myCards, b);
    for (int i = 0; i < numMoves; i++) if (buf[i].qty() >= myCards.count()) return true;
    for (int i = numMoves - 1; i >= 0; i--) { // 逆順の方が速い
        if (checkLast2Slow(buf + numMoves, buf[i], myCards, opsCards, b, lastAwake, flushLead)) return true;
    }
    return false;
}

bool checkLast2Slow(MoveInfo *const buf, MoveInfo move,
                    Cards myCards, Cards opsCards, Board b,
                    bool lastAwake, bool flushLead) {
    myCards -= move.cards();
    if (!myCards.any()) return true;
    bool flipped = false;
    if (move.isPASS()) {
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
        b.proc(move);
        if (b.isNull()) { // 流れた
            lastAwake = false;
            flushLead = true;
        } else {
            if (lastAwake) {
                lastAwake = flushLead = true;
            } else {
                flipped = true;
                lastAwake = flushLead = false;
            }
        }
    }
    if (flipped) swap(myCards, opsCards);
    bool res = judgeLast2Slow(buf, myCards, opsCards, b, lastAwake, flushLead);
    return flipped ? !res : res;
}

int outputL2JudgeResult() {
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
        Hand myHand, opsHand;
        myHand.setAll(myCards);
        opsHand.setAll(oppCards);
        Board b = OrderToNullBoard(0);
        FieldAddInfo fieldInfo;
        fieldInfo.init();
        fieldInfo.setFlushLead();

        int judgeResult = judgeLast2(buffer, myHand, opsHand, b, fieldInfo, 300000);
        cerr << myHand << opsHand << " -> " << judgeResult << endl;

        genMove(buffer, myCards, b);
    }

    {
        Hand myHand, opsHand;
        myHand.setAll(oppCards);
        opsHand.setAll(myCards);
        Board b = OrderToNullBoard(0);
        FieldAddInfo fieldInfo;
        fieldInfo.init();
        fieldInfo.setFlushLead();

        int judgeResult = judgeLast2(buffer, myHand, opsHand, b, fieldInfo, 300000);
        cerr << myHand << opsHand << " -> " << judgeResult << endl;
    }

    return 0;
}

int testRecordL2(const Record& record) {
    // 棋譜中の局面においてL2判定の結果をテスト
    // 間違っていた場合に失敗とはせず、正解不正解の確率行列を確認するに留める
    // 棋譜の結果とナイーブな探索の結果と比較する

    L2::init();

    // judge(高速判定)
    long long judgeTime[2] = {0};
    long long judgeCount[2] = {0};
    long long judgeMatrix[2][2][3] = {0};

    for (int i = 0; i < record.games(); i++) {
        Field field;
        for (Move move : PlayRoller(field, record.game(i))) {
            if (field.numPlayersAlive() != 2) continue;
            const Hand& myHand = field.hand[field.turn()];
            const Hand& opsHand = field.hand[field.ps.searchOpsPlayer(field.turn())];
            const Board b = field.board;
            bool won = record.game(i).newClassOf(field.turn()) == N_PLAYERS - 2;

            cl.start();
            int judgeResult = judgeLast2(buffer, myHand, opsHand, b, field.fieldInfo);
            int judgeIndex = judgeResult == L2_WIN ? 2 : (judgeResult == L2_DRAW ? 1 : 0);
            judgeTime[0] += cl.stop();
            judgeCount[0] += 1;
            judgeMatrix[0][won][judgeIndex] += 1;

            // 問題が小さいとき完全読み結果をチェック
            if (myHand.qty + opsHand.qty <= 12) {
                cl.start();
                bool l2mate = judgeLast2Slow(
                    buffer, myHand.cards, opsHand.cards, b,
                    field.fieldInfo.isLastAwake(), field.fieldInfo.isFlushLead()
                );
                judgeTime[1] += cl.stop();
                judgeCount[1] += 1;
                judgeMatrix[1][l2mate][judgeIndex] += 1;
            }
            break;
        }
    }

    cerr << "judge result (hand) = " << endl;
    for (int d = 0; d < 2; d++) {
        cerr << (d == 0 ? "real" : "search") << endl;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 3; j++) {
                cerr << judgeMatrix[d][i][j] << " ";
            }
            cerr << endl;
        }
    }
    cerr << "judge time (hand)    = " << judgeTime[0] / judgeCount[0] << endl;
    cerr << "judge time (l2-slow) = " << judgeTime[1] / judgeCount[1] << endl;
    return 0;
}

bool Last2Test(const vector<string>& recordFiles) {

    mt.seed(1);

    if (outputL2JudgeResult()) {
        cerr << "failed case test." << endl;
        return false;
    }
    cerr << "passed case test." << endl;

    Record record(recordFiles);

    if (testRecordL2(record)) {
        cerr << "failed record L2 judge test." << endl;
    }
    cerr << "passed record L2 judge test." << endl;

    return true;
}
