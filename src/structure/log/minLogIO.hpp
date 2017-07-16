/*
 minLogIO.hpp
 Katsuki Ohto
 */

#ifndef UECDA_STRUCTURE_MINLOGIO_HPP_
#define UECDA_STRUCTURE_MINLOGIO_HPP_

#ifdef LOGGING

#include "../primitive/prim.hpp"

// ファイルからのログ読み込み
// ファイルへの書き込みはminLog.hpp内のクラスのメンバ関数

namespace UECda{
    
    //入力
    const int N_COMMANDS = 13;
    const std::string command[N_COMMANDS] = {
        "//",
        "/*",
        "*/"
        "player",
        "game",
        "score",
        "seat",
        "class",
        "dealt",
        "changed",
        "original",
        "play",
        "result",
    };
    
    std::map<std::string, int> cmdMap;
    
    bool isCommand(const std::string& str){
        if(cmdMap.find(str) != cmdMap.end()){ return true; }
        return false;
    }
    
    int StringsToCardsM(std::vector<std::string>& vec, Cards *const dst){
        *dst = CARDS_NULL;
        for(auto itr = vec.begin(); itr != vec.end(); ++itr){
            IntCard ic = StringToIntCardM(*itr);
            if(ic < 0){ return -1; }
            addIntCard(dst, ic);
        }
        return 0;
    }
    
    int StringQueueToCardsM(std::queue<std::string>& q, Cards *const dst){
        *dst = CARDS_NULL;
        const std::string& str = q.front();
        if(isCommand(str)){
            cerr << "com" << endl; return -1;
        }
        if(str != "{"){
            cerr << "not {" << endl; return -1;
        }
        q.pop();
        while(q.front() != "}"){
            if(!q.size()){
                cerr << "not q size" << endl; return -1;
            }
            const std::string& str = q.front();
            if(isCommand(str)){
                cerr << "com" << endl; return -1;
            }
            IntCard ic = StringToIntCardM(str);
            if(!examIntCard(ic)){
                cerr << "bad intcard " << ic << " by " << str << endl; return -1;
            }
            addIntCard(dst, ic);
            q.pop();
        }
        q.pop();
        return 0;
    }
    
    int StringToMoveTimeM(const std::string& str, Move *const dstMv, uint64_t *const dstTime){
        
        *dstMv = MOVE_PASS;
        *dstTime = 0;
        
        Move mv;
        std::vector<std::string> v = split(str, " []\n");
        
        for(const std::string& tstr : v){
            DERR << tstr << ", ";
        }DERR << endl;
        
        if(v.size() <= 0){
            DERR << "failed to read move-time : length = 0" << endl;
            return -1;
        }
        
        mv = StringToMoveM(v[0]);

        if(mv == MOVE_NONE){
            DERR << "illegal move" << endl;
            return -1;
        }
        
        *dstMv = mv;
        
        if(v.size() <= 1){
            DERR << "failed to read move-time : length = 1" << endl;
            return 0;
        }
        
        char *end;
        *dstTime = strtol(v[1].c_str(), &end, 10);
        
        return 0;
    }
    
    void initCommandMap(){
        cmdMap.clear();
        for(int i = 0; i < N_COMMANDS; ++i){
            cmdMap[command[i]] = i;
        }
    }
    
#define Foo() DERR << "unexpected command : " << q.front() << endl; goto NEXT;
#define ToI(str) atoi(str.c_str())
    
    template<class matchLog_t>
    int readMatchLogFile(const std::string& fName, matchLog_t *const pmLog){
        
        using std::string;
        using std::vector;
        using std::queue;
        
        using gameLog_t = typename matchLog_t::gameLog_t;
        using playLog_t = typename gameLog_t::playLog_t;
        using changeLog_t = typename gameLog_t::changeLog_t;
        
        constexpr int N = gameLog_t::players();
        
        initCommandMap();
        
        std::ifstream fs;
        
        fs.open(fName, std::ios::in);
        
        if(!fs){
            cerr << "UECda::readMatchLogFile() : no log file." << endl;
            return -1;
        }
        
        pmLog->init();
        
        gameLog_t gLog;
        BitArray32<4, N> infoClass;
        BitArray32<4, N> infoSeat;
        BitArray32<4, N> infoNewClass;
        std::map<int, changeLog_t> cLogMap;
        Move lastMove;
        
        BitSet32 flagGame;
        BitSet32 flagMatch;
        int startedGame = -1;
        int failedGames = 0;
        
        int t;
        
        queue<string> q;
        string str;
        
        // ここから読み込みループ
        cerr << "start reading log..." << endl;
        while(1){
            
            while(q.size() < 1000){
                // コマンドを行ごとに読み込み
                if(!getline(fs, str, '\n')){
                    q.push("//");
                    break;
                }
                vector<string> v = split(str, ' ');
                for(auto itr = v.begin(); itr != v.end(); ++itr){
                    if(itr->size() > 0){
                        q.push(*itr);
                    }
                }
            }
            
            const string cmd = q.front();
            q.pop();
            
            DERR << "command = " << cmd << endl;
            
            if(cmd == "//"){ // 全試合終了
                break;
            }
            else if(cmd == "/*"){ // game開始合図
                if(startedGame >= 0){
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
                flagGame.init();
            }
            else if(cmd == "*/"){ // game終了合図
                // ログとして必要なデータが揃っているか確認
                // if(!flagGame.test(0)){}//ゲーム番号不明
                // if(!flagGame.test(0)){}//累積スコア不明
                if(!lastMove.isPASS()){
                    gLog.setTerminated();
                }
                gLog.infoClass() = infoClass;
                gLog.infoSeat() = infoSeat;
                gLog.infoNewClass() = infoNewClass;
                pmLog->push_game(gLog);
                startedGame = -1;
            }
            else if(cmd == "match"){
                const string& str=q.front();
                if(isCommand(str)){ Foo(); }
                q.pop();
                flagMatch.set(0);
            }
            else if(cmd == "player"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    DERR << "pname : " << str << endl;
                    pmLog->setPlayer(i, str);
                    q.pop();
                }
                flagMatch.set(1);
            }
            else if(cmd == "game"){
                const string& str = q.front();
                if(isCommand(str)){ Foo(); }
                char *end;
                int gn = strtol(str.c_str(), &end, 10);
                //cerr << "game " << gn << endl;
                q.pop();
                flagGame.set(0);
            }
            else if(cmd == "score"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    q.pop();
                }
                flagGame.set(1);
            }
            else if(cmd == "class"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    infoClass.replace(i, ToI(str));
                    q.pop();
                }
                flagGame.set(2);
            }
            else if(cmd == "seat"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if( isCommand(str) ){ Foo(); }
                    infoSeat.replace(i, ToI(str));
                    q.pop();
                }
                flagGame.set(3);
            }
            else if(cmd == "dealt"){
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueToCardsM(q, &c) < 0){ Foo(); }
                    gLog.setDealtCards(i, c);
                    DERR << OutCards(c) << endl;
                }
                flagGame.set(4);
            }
            else if(cmd == "changed"){
                bool anyChange = false;
                BitArray32<4, N> infoClassPlayer = invert(infoClass);
                
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueToCardsM(q, &c) < 0){ Foo(); }
                    if(anyCards(c)){
                        anyChange = true;
                        changeLog_t cLog(i, infoClassPlayer[getChangePartnerClass(infoClass[i])], c);
                        
                        cLogMap[-infoClass[i]] = cLog;
                    }
                }
                if(!anyChange){
                    gLog.setInitGame();
                    DERR << "init game." << endl;
                }else{
                    for(auto c : cLogMap){
                        gLog.push_change(c.second);
                    }
                }
                flagGame.set(5);
            }
            else if(cmd == "original"){
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueToCardsM(q, &c) < 0){ Foo(); }
                    gLog.setOrgCards(i, c);
                    DERR << OutCards(c) << endl;
                }
                flagGame.set(6);
            }
            else if(cmd == "play"){
                while(1){
                    Move mv; uint64_t time;
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    if(StringToMoveTimeM(str, &mv, &time) < 0){ Foo(); }
                    playLog_t pLog(mv, time);
                    gLog.push_play(pLog);
                    lastMove = mv;
                    q.pop();
                }
                flagGame.set(7);
            }
            else if(cmd == "result"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    infoNewClass.replace(i, ToI(str));
                    q.pop();
                }
                flagGame.set(8);
            }
            else{
                // いかなるコマンドでもないので読み飛ばし
                q.pop();
            }
        NEXT:;
        }
    END:;
        
        cerr << pmLog->games() << " games were loaded." << endl;
        
        // これで全試合が読み込めたはず
        fs.close();
        return 0;
    }
#undef ToI
#undef Foo
    
    //出力
}

#endif // LOGGING
#endif // UECDA_STRUCTURE_MINLOGIO_HPP_
