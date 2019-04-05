#pragma once

// クライアント側から見た局面実況

#include "record.hpp"

namespace UECda{

    template <class matchLog_t>
    void broadcastBMatch(const matchLog_t& mLog) { // 全試合開始前実況
        // 自分のプロフィールや設定を確認する
        cerr << "Client's Name : " << MY_NAME << " " << MY_VERSION << endl;
        cerr << "Client's Coach :" << MY_COACH << endl;
        cerr << "Thinking Level : " << THINKING_LEVEL << "  (value of time = " << VALUE_PER_SEC << " pts/sec)" << endl;
        cerr << "My Common Number = " << mLog.getMyPlayerNum() << endl;
    }
    
    template <class matchLog_t>
    void broadcastBGame(const matchLog_t& mLog) { // 1ゲーム開始前実況
        const typename matchLog_t::gameLog_t& gLog = mLog.latestGame();
        cerr << "Game " << mLog.getLatestGameNum() << " is about to start." << endl;
        cerr << "Game Type...";
        if (gLog.isInitGame()) {
            cerr << "INIT GAME" << endl;
        } else {
            cerr << "ADVANTAGE GAME  " << endl;
        }
        for (int s = 0; s < N_PLAYERS; ++s) {
            int pNum = mLog.seatPlayer(s);
            int pos = mLog.positionOf(pNum) + 1; // 最上位を1位に変更
            cerr << "[Seat " << s << "] : Player " << pNum;
            cerr << "  Class " << gLog.classOf(pNum);
            cerr << "  Score " << mLog.getScore(pNum) << " ( " << pos;
            switch (pos % 100) {
                case 11: case 12:
                    cerr << "th"; break;
                default:
                    switch (pos % 10) {
                        case 1: cerr << "st"; break;
                        case 2: cerr << "nd"; break;
                        case 3: cerr << "rd"; break;
                        default: cerr << "th"; break;
                    }
                    break;
            }
            cerr << " place)";
            if (pNum == mLog.getMyPlayerNum()) { cerr << " <- Me"; }
            cerr << endl;
        }
    }
    
    template <class matchLog_t>
    void broadcastPlayerState(const matchLog_t& mLog) { // プレーヤー状態実況
        const typename matchLog_t::gameLog_t& gLog = mLog.latestGame();
        for (int s = 0; s < N_PLAYERS; ++s) {
            int pNum = mLog.seatPlayer(s);
            cerr << "[Seat " << s << "] : Player " << pNum;
            cerr << "  Class " << gLog.classOf(pNum);
            if (ps.isAlive(pNum)) {
                if (ps.isAwake(pNum)) {
                    cerr << "          ";
                } else {
                    cerr << "  ASLEEP  ";
                }
            }else if (ps.isExcluded(pNum)) {
                cerr << " EXCLUDED ";
            } else {
                cerr << "  DEAD(";
                cerr << newClassOf(pNum);
                cerr << ") ";
            }
            cerr << "last " << getNCards(pNum) << " cards ,";
            cerr << " used " << usedCards[pNum];
            cerr << endl;
        }
        cerr << "Players alive : " << getNAlivePlayers();
        cerr << "  Players awake : " << getNAwakePlayers() << endl;
    }
    
    void broadcastBP() const { // プレー前実況
        cerr << "********** Before Play **********";
        cerr << "<Game> " << getGameNum() << " <Turn> " << turnCount();
        cerr <<  "  TurnPlayer : " << turn() << "  Owner : " << owner() << endl;
        cerr << "Board : " << bd << endl;
        cerr << Out2CardTables(getMyCards(), subtrCards(getRemCards(), getMyCards()));
        broadcastPlayerState();
    }
    
    void broadcastAP() const { // プレー後実況
        cerr << "********** After Play **********";
        cerr << "<Game> " << getGameNum() << " <Turn> " << turnCount();
        cerr << "  TurnPlayer : " << turn() << "  Owner : " << owner() << endl;
        cerr << "Board : " << bd << endl;
        cerr << Out2CardTables(getMyCards(), subtrCards(getRemCards(), getMyCards()));
        broadcastPlayerState();
    }
    
    void broadcastPlay(uint32_t p, Move move) const { // プレー実況
        cerr << "[PLAY] ";
        if (p == getMyPlayerNum()) {
            cerr << "MY PLAY  ";
        } else {
            cerr << "Player " << p << "  ";
        }
        cerr << move;
        int qty = getNCards(p);
        if (qty <= 0) {
            cerr << " passed away...  Class " << newClassOf(p);
        } else {
            if (qty == 1) {
                cerr << " 1 card";
            } else {
                cerr << " " << qty << " cards";
            }
            cerr << " still remains.";
        }
        cerr << endl;
    }
    
    void broadcastMyChange(Cards cards) const { // 自分のプレー決定実況
        cerr << "My decided change : " << cards << endl;
    }
    
    void broadcastMyPlay(Move move) const { // 自分のプレー決定実況
        cerr << "My decided move : ";
        cerr << move;
        //決定手が決まっている場合はそれも表示
        for (int m = nextMoves.size() - 1; m >= 0; --m)
            cerr << " -> " << nextMoves.getData(m);
        cerr << endl;
    }
}