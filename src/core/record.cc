#include "record.hpp"

using namespace std;

namespace Recorder {
    const vector<string> commandList = {
        "//", "/*", "*/"
        "player", "game", "score", "seat", "class",
        "dealt", "changed", "original",
        "play", "result",
    };
    
    map<string, int> commandMap;
    
    bool isCommand(const string& str) {
        return commandMap.find(str) != commandMap.end();
    }
    void initCommandSet() {
        commandMap.clear();
        int cnt = 0;
        for (string command : commandList) commandMap[command] = cnt++;
    }
}
    
int StringQueueToCardsM(queue<string>& q, Cards *const dst) {
    *dst = CARDS_NULL;
    const string& str = q.front();
    if (Recorder::isCommand(str)) {
        cerr << "com" << endl; return -1;
    }
    if (str != "{") {
        cerr << "not {" << endl; return -1;
    }
    q.pop();
    while (q.front() != "}") {
        if (!q.size()) {
            cerr << "not q size" << endl; return -1;
        }
        const string& str = q.front();
        if (Recorder::isCommand(str)) {
            cerr << "com" << endl; return -1;
        }
        IntCard ic = StringToIntCard(str);
        if (!examIntCard(ic)) {
            cerr << "bad intcard " << ic << " by " << str << endl; return -1;
        }
        dst->insert(ic);
        q.pop();
    }
    q.pop();
    return 0;
}

int StringToMoveTimeM(const string& str, Move *const dstMv, uint64_t *const dstTime) {
    *dstMv = MOVE_PASS;
    *dstTime = 0;
    
    Move mv;
    vector<string> v = split(str, " []\n");
    
    for (const string& tstr : v) DERR << tstr << ", ";
    DERR << endl;
    
    if (v.size() <= 0) {
        DERR << "failed to read move-time : length = 0" << endl;
        return -1;
    }
    mv = StringToMoveM(v[0]);

    if (mv == MOVE_NONE) {
        DERR << "illegal move" << endl;
        return -1;
    }
    *dstMv = mv;

    if (v.size() <= 1) {
        DERR << "failed to read move-time : length = 1" << endl;
        return 0;
    }
    char *end;
    *dstTime = strtol(v[1].c_str(), &end, 10);
    return 0;
}

#define Foo() {DERR << "unexpected command : " << q.front() << endl; goto NEXT;}
#define ToI(str) stoll(str)

int readMatchLogFile(const string& fName, MatchRecord *const pmLog) {
    
    using game_t = typename MatchRecord::game_t;
    using playLog_t = typename game_t::playLog_t;
    using change_t = typename game_t::change_t;
    
    constexpr int N = N_PLAYERS;
    
    Recorder::initCommandSet();
    
    ifstream ifs;
    
    ifs.open(fName, ios::in);
    
    if (!ifs) {
        cerr << "readMatchLogFile() : no log file." << endl;
        return -1;
    }
    
    pmLog->init();
    
    game_t gLog;
    BitArray32<4, N> infoClass, infoSeat, infoNewClass;
    map<int, change_t> cLogMap;
    Move lastMove;
    
    bitset<32> flagGame, flagMatch;
    int startedGame = -1;
    int failedGames = 0;
    
    int t;
    
    queue<string> q;
    string str;
    
    // ここから読み込みループ
    cerr << "start reading log..." << endl;
    while (1) {
        while (q.size() < 1000) {
            // コマンドを行ごとに読み込み
            if (!getline(ifs, str, '\n')) {
                q.push("//");
                break;
            }
            vector<string> v = split(str, ' ');
            for (const string& str : v) {
                if (str.size() > 0) q.push(str);
            }
        }
        
        const string cmd = q.front();
        q.pop();
        
        DERR << "command = " << cmd << endl;
        
        if (cmd == "//") break; // 全試合終了
        else if (cmd == "/*") { // game開始合図
            if (startedGame >= 0) {
                cerr << "failed to read game " << (startedGame + failedGames) << endl;
                failedGames += 1;
            }
            startedGame = pmLog->games();
            gLog.init();
            infoClass.clear();
            infoSeat.clear();
            infoNewClass.clear();
            cLogMap.clear();
            lastMove = MOVE_PASS;
            flagGame.reset();
        } else if (cmd == "*/") { // game終了合図
            // ログとして必要なデータが揃っているか確認
            // if (!flagGame.test(0)) {}//ゲーム番号不明
            // if (!flagGame.test(0)) {}//累積スコア不明
            if (!lastMove.isPASS()) {
                gLog.setTerminated();
            }
            gLog.infoClass = infoClass;
            gLog.infoSeat = infoSeat;
            gLog.infoNewClass = infoNewClass;
            pmLog->pushGame(gLog);
            startedGame = -1;
        } else if (cmd == "match") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            q.pop();
            flagMatch.set(0);
        } else if (cmd == "player") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                DERR << "pname : " << str << endl;
                pmLog->setPlayer(i, str);
                q.pop();
            }
            flagMatch.set(1);
        } else if (cmd == "game") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            char *end;
            int gn = strtol(str.c_str(), &end, 10);
            DERR << "game " << gn << endl;
            q.pop();
            flagGame.set(0);
        } else if (cmd == "score") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                q.pop();
            }
            flagGame.set(1);
        } else if (cmd == "class") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoClass.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(2);
        } else if (cmd == "seat") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoSeat.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(3);
        } else if (cmd == "dealt") {
            for (int i = 0; i < N; i++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                gLog.setDealtCards(i, c);
                DERR << c << endl;
            }
            flagGame.set(4);
        } else if (cmd == "changed") {
            bool anyChange = false;
            BitArray32<4, N> infoClassPlayer = invert(infoClass);
            
            for (int i = 0; i < N; i++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                if (anyCards(c)) {
                    anyChange = true;
                    change_t cLog(i, infoClassPlayer[getChangePartnerClass(infoClass[i])], c);
                    
                    cLogMap[-infoClass[i]] = cLog;
                }
            }
            if (!anyChange) {
                gLog.setInitGame();
                DERR << "init game." << endl;
            } else {
                for (auto c : cLogMap) gLog.push_change(c.second);
            }
            flagGame.set(5);
        } else if (cmd == "original") {
            for (int i = 0; i < N; i++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo(); 
                gLog.setOrgCards(i, c);
                DERR << c << endl;
            }
            flagGame.set(6);
        } else if (cmd == "play") {
            while (1) {
                Move mv; uint64_t time;
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                if (StringToMoveTimeM(str, &mv, &time) < 0) Foo();
                playLog_t pLog(mv, time);
                gLog.push_play(pLog);
                lastMove = mv;
                q.pop();
            }
            flagGame.set(7);
        } else if (cmd == "result") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoNewClass.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(8);
        } else q.pop(); // いかなるコマンドでもないので読み飛ばし
    NEXT:;
    }
END:;
    
    cerr << pmLog->games() << " games were loaded." << endl;
    ifs.close();
    return 0;
}
#undef ToI
#undef Foo