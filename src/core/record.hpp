#pragma once

#include "daifugo.hpp"

// 試合棋譜
// 最小の情報のみ保持
// 一対戦（プレーヤーが変わらない、得点が累計）と
// 複数対戦を手軽に扱うためのクラスも準備

namespace UECda {
    const std::vector<std::string> commandList = {
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
    
    std::map<std::string, int> commandMap;
    
    bool isCommand(const std::string& str) {
        return commandMap.find(str) != commandMap.end();
    }
    
    int StringsToCardsM(std::vector<std::string>& vec, Cards *const dst) {
        *dst = CARDS_NULL;
        for (const std::string& str : vec) {
            IntCard ic = StringToIntCardM(str);
            if (ic < 0) return -1;
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
    
    void initCommandSet() {
        commandMap.clear();
        int cnt = 0;
        for (std::string command : commandList) commandMap[command] = cnt++;
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
        
        constexpr int N = N_PLAYERS;
        
        initCommandSet();
        
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
                gLog.infoClass = infoClass;
                gLog.infoSeat = infoSeat;
                gLog.infoNewClass = infoNewClass;
                pmLog->pushGame(gLog);
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
    
    class MinPlayLog {
        // 1つの着手の記録
    public:
        MinPlayLog():
        move_(), time_(){}
        
        MinPlayLog(Move amove, uint32_t atime):
        move_(amove), time_(atime){}
        
        Move move() const { return move_; }
        uint32_t time() const { return time_; }
        
        void set(Move amove, uint32_t atime) {
            move_ = amove; time_ = atime;
        }
        std::string toString()const{
            std::ostringstream oss;
            oss << LogMove(move()) << "[" << time() << "]";
            return oss.str();
        }
        
    protected:
        Move move_;
        uint32_t time_;
    };
    
    class ClientPlayLog : public MinPlayLog {
        // クライアント用の着手記録
        // ルールが微妙に違っても対処できるようにこのクラスが局面の一次情報クラスとして振舞う
    public:
        uint32_t getTurnPlayer()const{
            return infoSpecialPlayer[0];
        }
        uint32_t getPMOwner()const{
            return infoSpecialPlayer[1];
        }
        uint32_t getNCards(int p)const{
            return infoNCards[p];
        }
        void set(Move amove, Cards adc,
                 uint32_t atime, uint32_t asbjTime){
            MinPlayLog::set(amove, atime);
            subjectiveTime = asbjTime;
            usedCards = adc;
        }
        bool procByServer(Move move, Cards dc){
            // あがりの場合に1を返す
            bool dead = false;
            uint32_t tp = getTurnPlayer();
            if(move.isPASS()){ // パス
                ps.setAsleep(tp);
            }else{ // パスでない
                bd.procExceptFlush(move);
                infoNCards.subtr(tp, countCards(dc));
                if(getNCards(tp) <= 0){//あがり
                    ps.setDead(tp);
                    dead = true;
                }
                setPMOwner(tp);
            }
            return dead;
        }
        void initGame(){
            bd.init();
            ps.init();
            infoSpecialPlayer.clear();
        }
        void setPMOwner(int p){
            infoSpecialPlayer.assign(1, p);
        }
        void setFirstTurnPlayer(int p){
            infoSpecialPlayer.assign(3, p);
        }
        void setTurnPlayer(int p){
            infoSpecialPlayer.assign(0, p);
        }
        void setNCards(int p, int n){
            infoNCards.assign(p, n);
        }
        void flush(BitArray32<4, N_PLAYERS> infoSeat){
            bd.flush();
            ps.flush();
            flushTurnPlayer(infoSeat);
        }
        void flushTurnPlayer(BitArray32<4, N_PLAYERS> infoSeat){
            uint32_t tp = getPMOwner();
            if(ps.isAlive(tp)){
                setTurnPlayer(tp);
            }else{
                rotateTurnPlayer(tp, infoSeat);
            }
        }
        void rotateTurnPlayer(uint32_t tp, BitArray32<4, N_PLAYERS> infoSeat){
            BitArray32<4, N_PLAYERS> infoSeatPlayer = invert(infoSeat);
            do{
                tp = infoSeatPlayer[getNextSeat<N_PLAYERS>(infoSeat[tp])];
            }while(!ps.isAwake(tp));
            setTurnPlayer(tp);
        }
        
        Board bd;
        PlayersState ps;
        BitArray32<4> infoSpecialPlayer;
        BitArray32<4, N_PLAYERS> infoNCards;
        Cards usedCards; // javaサーバは役表現と構成する手札表現が合わないことがある...
        uint32_t subjectiveTime;
    };
    
    class MinChangeLog{
        // 交換の記録
    public:
        constexpr MinChangeLog():
        from_(), to_(), c_(){}
        
        constexpr MinChangeLog(const uint32_t afrom, const uint32_t ato, const Cards ac):
        from_(afrom), to_(ato), c_(ac){}
        
        uint32_t from()const { return from_; }
        uint32_t to()const { return to_; }
        Cards cards()const { return c_; }
        
    protected:
        uint32_t from_, to_; Cards c_;
    };
    
    template<class _playLog_t>
    class MinCommonGameLog{
    public:
        
        using playLog_t = _playLog_t;
        using changeLog_t = MinChangeLog;
        
        void init(){
            changes_ = 0;
            plays_ = 0;
            flags_.reset();
            infoClass.clear();
            infoSeat.clear();
            infoNewClass.clear();
            infoPosition.clear();
        }
        int plays()const { return plays_; }
        int changes()const { return changes_; }
        
        const playLog_t& play(int t)const{ return play_[t]; }
        const changeLog_t& change(int c)const{ return change_[c]; }
        
        void push_change(const changeLog_t& change){
            if(changes_ < N_CHANGES){
                change_[changes_++] = change;
            }else{
                flags_.set(2);
            }
        }
        void push_play(const playLog_t& play){
            if(plays_ < 128){
                play_[plays_++] = play;
            }else{
                flags_.set(3);
            }
        }
        
        void setTerminated(){ flags_.set(0); }
        void setInitGame(){ flags_.set(1); }
        void resetInitGame(){ flags_.reset(0); }
        
        bool isInChange()const{ return !isInitGame() && changes() < N_CHANGES; }
        
        bool isTerminated()const{ return flags_.test(0); }
        bool isInitGame()const{ return flags_.test(1); }
        bool isChangeOver()const{ return flags_.test(2); }
        bool isPlayOver()const{ return flags_.test(3); }
        
        int getPlayerClass(int p)const{ return infoClass[p]; }
        int getPlayerSeat(int p)const{ return infoSeat[p]; }
        int getPlayerNewClass(int p)const{ return infoNewClass[p]; }
        int getPosition(int p)const{ return infoPosition[p]; }
        
        // 逆置換
        int getClassPlayer(int cl)const{ return invert(infoClass)[cl]; }
        int getSeatPlayer(int s)const{ return invert(infoSeat)[s]; }
        int getNewClassPlayer(int ncl)const{ return invert(infoNewClass)[ncl]; }
        
        void setPlayerClass(int p, int cl){ infoClass.assign(p, cl); }
        void setPlayerSeat(int p, int s){ infoSeat.assign(p, s); }
        void setPlayerNewClass(int p, int ncl){ infoNewClass.assign(p, ncl); }
        void setPosition(int p, int pos){ infoPosition.assign(p, pos); }
        
        BitArray32<4, N_PLAYERS> infoClass, infoSeat, infoNewClass, infoPosition;
        
    protected:
        static constexpr int N_CHANGES = (N_PLAYERS / 2) * 2; // TODO: まあ5人以下だからいいのか?
        int changes_;
        std::array<changeLog_t, N_CHANGES> change_;
        int plays_;
        
        // 128手を超えることは滅多にないので強制打ち切り
        // ここをvectorとかにすると連続対戦棋譜がvector<vector>みたいになって死ぬ
        std::array<playLog_t, 128> play_;
        
        BitSet32 flags_;
    };
    
    template<class _playLog_t>
    class MinGameLog : public MinCommonGameLog<_playLog_t>{
    private:
        using base = MinCommonGameLog<_playLog_t>;
    public:
        void init(){
            base::init();
            dealtCards.fill(CARDS_NULL);
            orgCards.fill(CARDS_NULL);
        }
        
        static constexpr bool isSubjective(){ return false; }
        
        Cards getDealtCards(int p)const{ return dealtCards[p]; }
        void setDealtCards(int p, Cards c){ dealtCards[p] = c; }
        void addDealtCards(int p, Cards c){ addCards(&dealtCards[p], c); }
        
        Cards getOrgCards(int p)const{ return orgCards[p]; }
        void setOrgCards(int p, Cards c){ orgCards[p] = c; }
        void addOrgCards(int p, Cards c){ addCards(&orgCards[p], c); }
        
        std::string toString(int gn)const{
            std::ostringstream oss;
            
            oss << "/* " << endl;
            oss << "game " << gn << " " << endl;
            oss << "score " << endl;
            oss << "class ";
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << base::getPlayerClass(p) << " ";
            oss << endl;
            oss << "seat ";
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << base::getPlayerSeat(p) << " ";
            oss << endl;
            oss << "dealt ";
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << OutCardsM(getDealtCards(p)) << " ";
            oss << endl;
            oss << "changed ";
            
            Cards changeCards[N_PLAYERS];
            for(int p = 0; p < N_PLAYERS; ++p)
                changeCards[p] = CARDS_NULL;
            for(int c = 0; c < base::changes(); ++c)
                changeCards[base::change(c).from()] = base::change(c).cards();
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << OutCardsM(changeCards[p]) << " ";
            oss << endl;
            oss << "original ";
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << OutCardsM(getOrgCards(p)) << " ";
            oss << endl;
            oss << "play ";
            for(int t = 0; t < base::plays(); ++t)
                oss << base::play(t).toString() << " ";
            oss << endl;
            oss << "result ";
            for(int p = 0; p < N_PLAYERS; ++p)
                oss << base::getPlayerNewClass(p) << " ";
            oss << endl;
            oss << "*/ " << endl;
            
            return oss.str();
        }
        
        int fadd(const std::string& fName, int g){
            // ファイルに追加書き込み
            std::ofstream ofs(fName, std::ios::app);
            if(!ofs){ return -1; }
            ofs << toString(g);
            return 0;
        }
        
        std::array<Cards, N_PLAYERS> dealtCards, orgCards;
    };
    
    template<class _playLog_t>
    class ClientGameLog : public MinGameLog<_playLog_t>{
    private:
        using base = MinGameLog<_playLog_t>;
    public:
        void init(){
            base::init();
            infoNDealtCards.clear();
            infoNOrgCards.clear();
            sentCards = recvCards = CARDS_NULL;
        }
        
        static constexpr bool isSubjective(){ return true; }
        
        void setNDealtCards(int p, int n){ infoNDealtCards.assign(p, n); }
        void setNOrgCards(int p, int n){ infoNOrgCards.assign(p, n); }
        void setSentCards(Cards c){ sentCards = c; }
        void setRecvCards(Cards c){ recvCards = c; }
        
        int getNOrgCards(int p)const{ return infoNOrgCards[p]; }
        int getNDealtCards(int p)const{ return infoNDealtCards[p]; }
        Cards getSentCards()const{ return sentCards; }
        Cards getRecvCards()const{ return recvCards; }
        
        // 交換中かどうかの判定はオーバーロードしているので注意
        bool isInChange()const{ return !base::isInitGame() && sentCards == CARDS_NULL; }
        
        BitArray32<4, N_PLAYERS> infoNDealtCards, infoNOrgCards;
        Cards sentCards, recvCards; // 本当はchangeにいれたいけど
        ClientPlayLog current;
    protected:
    };
    
    template<class _gameLog_t>
    class MinMatchLog{
    public:
        using gameLog_t = _gameLog_t;
        
        int games()const { return game_.size(); }
        
        const std::string& player(int p)const{ return player_[p]; }
        const std::string& fileName()const{ return fileName_; }
        const gameLog_t& game(int g)const{ return game_[g]; }
        
        const gameLog_t& latestGame()const{ return game_.back(); }
        gameLog_t& latestGame(){ return game_.back(); }
        int getLatestGameNum()const{ return games() - 1; }
        
        int getScore(int p)const{ return score[p]; }
        int getPosition(int p)const{ // score から順位を調べる
            int pos = 0;
            for(int pp = 0; pp < N_PLAYERS; ++pp)
                if(score[pp] < score[p]) pos += 1;
            return pos;
        }
        
        void setPlayer(int p, const std::string& str){
            player_[p] = str;
        }
        
        void reserveGames(std::size_t n){
            game_.reserve(n);
        }
        void initGame(){
            game_.emplace_back(gameLog_t());
            gameLog_t& gLog = latestGame();
            gLog.init();
            for(int p = 0; p < N_PLAYERS; ++p)
                gLog.setPosition(p, getPosition(p));
        }
        void closeGame(){
            const gameLog_t& gLog = latestGame();
            for(int p = 0; p < N_PLAYERS; ++p)
                score[p] += REWARD(gLog.getPlayerNewClass(p));
        }
        void pushGame(const gameLog_t& gLog){
            game_.emplace_back(gLog);
        }
        void init(){
            game_.clear();
            player_.fill("");
            score.fill(0);
        }
        
        std::string toHeaderString()const{
            // ヘッダ部分の出力
            std::ostringstream oss;
            oss << "player";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << " " << player_[p];
            }
            oss << endl;
            return oss.str();
        }
        
        int fin(const std::string& fName){
            // ファイルから読み込み
            fileName_ = fName;
            return readMatchLogFile(fName, this);
        }
        
        int fout(const std::string& fName){
            // ファイルに書き込み
            fileName_ = fName;
            std::ofstream ofs(fName, std::ios::out);
            if(!ofs){ return -1; }
            ofs << toHeaderString();
            for(int g = 0, gend = games(); g < gend; ++g){
                const gameLog_t& gLog = game(g);
                ofs << gLog.toString(g);
            }
            return 0;
        }
        int foutLast(const std::string& fName){
            // ファイルに最後の試合を書き込み
            std::ofstream ofs(fName, std::ios::app);
            if(!ofs){ return -1; }
            int g = (int)games() - 1;
            if(g < 0){ return -1; }
            const gameLog_t& gLog = game(g);
            ofs << gLog.toString(g);
            return 0;
        }
        
        MinMatchLog(){
            init();
        }
        MinMatchLog(const std::string& logFile){
            init();
            fin(logFile);
        }
    protected:
        std::string fileName_;
        std::array<std::string, N_PLAYERS> player_;
        std::vector<gameLog_t> game_;
        std::array<int32_t, N_PLAYERS> score;
    };
    
    template<class _gameLog_t>
    class ClientMatchLog : public MinMatchLog<_gameLog_t>{
        // クライアントの観測は基本ここから見えるようにしておく
    public:
        int getMyPlayerNum()const{ return myPlayerNum; }
        void setMyPlayerNum(int p){ myPlayerNum = p; }
        
        ClientMatchLog(){ myPlayerNum = -1; }
    protected:
        int myPlayerNum; // これも一応観測
    };
    
    template<class _matchLog_t, int N>
    class MinMatchLogAccessor{
        // ランダム順なアクセス用
    public:
        using matchLog_t = _matchLog_t;
        using gameLog_t = typename matchLog_t::gameLog_t;
        
        MinMatchLogAccessor(){
            init();
        }
        MinMatchLogAccessor(const std::string& logFile){
            init();
            fin(logFile);
        }
        MinMatchLogAccessor(const std::vector<std::string>& logFiles){
            init();
            fin(logFiles);
        }
        
        ~MinMatchLogAccessor(){
            if(list != nullptr){ free(list); list=nullptr; }
        }
        
        int matches()const { return matches_; }
        int games()const { return games_sum_[matches_]; }
        
        void initRandomList(){
            if(list != nullptr){ free(list); }
            list = new int[games()];
            for(int g = 0, gend = games(); g < gend; ++g){
                list[g] = g;
            }
        }
        template<class dice_t>
        void shuffleRandomList(const int first, const int end, dice_t& dice){
            if(list == nullptr){return;}
            // random shuffle
            for(int i = min(games(), end) - 1; i > max(0, first); --i){
                std::swap(list[i], list[dice() % ((i - first) + 1)]);
            }
        }
        int accessIndex(const int s)const{
            assert(list != nullptr);
            assert(s < games());
            return list[s];
        }
        const gameLog_t& access(const int s)const{
            int idx = accessIndex(s);
            int m = -1;
            for(int i = 0; i < matches() + 1; ++i){
                if(idx < games_sum_[i]){
                    m = i - 1;
                    break;
                }
            }
            assert(m >= 0);
            //cerr << "match " << m << " game " << (idx - games_sum_[m]) << endl;
            return match_[m].game(idx - games_sum_[m]);
        }
        const matchLog_t& accessMatch(const int s)const{
            int idx = accessIndex(s);
            int m = -1;
            for(int i = 0; i < matches() + 1; ++i){
                if(idx < games_sum_[i]){
                    m = i - 1;
                    break;
                }
            }
            assert(m >= 0);
            return match_[m];
        }
        
        matchLog_t& match(int n){ return match_[n]; }
        const matchLog_t& match(int n)const{ return match_[n]; }
        
        void addMatch(){ matches_++; }
        
        int fin(const std::string& logFile, int index = 0){
            // 棋譜ファイルを1つ読み込み
            cerr << "using log file [ " << logFile << " ]" << endl;
            int err = match(index).fin(logFile);
            if(err < 0){
                cerr << "failed to read " << logFile << "." << endl;
                return err;
            }else{ // エラーがなければ試合数を追加
                games_sum_[index + 1] = games_sum_[index] + match(index).games();
                addMatch();
            }
            return 0;
        }
        
        int fin(const std::vector<std::string>& logFiles, bool forced = false){
            // 棋譜ファイルを複数読み込み
            for(int i = 0; i < logFiles.size(); ++i){
                int err = fin(logFiles.at(i), matches());
                if(err < 0){
                    if(!forced){
                        return err;
                    }
                }
            }
            cerr << matches() << " matches were loaded." << endl;
            return 0;
        }
        
    protected:
        void init(){
            games_sum_[0] = 0;
            matches_ = 0;
            list = nullptr;
        }
        
        std::array<matchLog_t, N> match_;
        int matches_;
        int *list;
        std::array<int, N + 1> games_sum_;
    };
    template<class _matchLog_t, int N, typename callback_t>
    void iterateMatch(const MinMatchLogAccessor<_matchLog_t, N>& mLogs, const callback_t& callback){
        for(int m = 0; m < mLogs.matches(); ++m)
            callback(mLogs.match(m));
    }
    template<class _matchLog_t, int N, typename callback_t>
    void iterateGameRandomly(const MinMatchLogAccessor<_matchLog_t, N>& mLogs,
                             int first, int end, const callback_t& callback){
        for(int i = max(0, first); i < min(mLogs.games(), end); ++i)
            callback(mLogs.access(i), mLogs.accessMatch(i));
    }
    template<class _matchLog_t, int N, typename callback_t>
    void iterateGameRandomlyWithIndex(const MinMatchLogAccessor<_matchLog_t, N>& mLogs,
                                      int first, int end, const callback_t& callback){
        for(int i = max(0, first); i < min(mLogs.games(), end); ++i)
            callback(mLogs.accessIndex(i), mLogs.access(i), mLogs.accessMatch(i));
    }
    
    // ここからログから局面を正方向に読む関数たち
    
    // 交換やプレー時のコールバッックには返り値を要求する
    // 0 が通常
    // -1 でそのフェーズ終了
    // -2 で試合ログ読み終了
    // -3 で1マッチ(連続対戦)終了
    // -4 で全マッチ終了
    
    template<class field_t, class gameLog_t>
    void setFieldBeforeAll(field_t& field, const gameLog_t& gLog){
        // 棋譜を読んでの初期設定
        field.initGame();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        if(gLog.isInitGame()){
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass;
            field.infoClassPlayer = invert(gLog.infoClass);
        }
        field.infoSeat = gLog.infoSeat;
        field.infoSeatPlayer = invert(gLog.infoSeat);
        field.infoNewClass.clear();
        field.infoNewClassPlayer.clear();
        field.infoPosition = gLog.infoPosition;
    }
    
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
    int iterateGameLogBeforePlay
    (field_t& field, const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{},
     bool stopBeforeChange = false){
        setFieldBeforeAll(field, gLog);
        firstCallback(field);
        
        // deal cards
        for(int p = 0; p < N_PLAYERS; ++p){
            Cards tmp = gLog.getDealtCards(p);
            field.hand[p].cards = tmp; // とりあえずcardsだけセットしておく
        }
        // present
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const typename gameLog_t::changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.to()) < HEIMIN){
                Cards present = change.cards();
                addCards(&field.hand[change.to()].cards, present);
            }
        }
        // set card info
        for(int p = 0; p < N_PLAYERS; ++p){
            Cards c = field.hand[p].cards;
            field.hand[p].setAll(c);
            field.opsHand[p].set(subtrCards(CARDS_ALL, c));
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ field.hand[p].hash);
            field.addAttractedPlayer(p);
        }
        
        field.setInChange();
        if(stopBeforeChange)
            return -1;
        
        dealtCallback(field);
        
        // change
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const typename gameLog_t::changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.from()) >= HINMIN){
                // 献上以外
                field.hand[change.from()].subtrAll(change.cards());
                field.opsHand[change.from()].addAll(change.cards());
            }else{
                int ret = changeCallback(field, change.from(), change.to(), change.cards());
                if(ret <= -2){
                    cerr << "error on change "
                    << change.from() << " -> " << change.to() << endl;
                    return ret;
                }else if(ret == -1){
                    break;
                }
                // proceed field
                field.makeChange(change.from(), change.to(), change.cards());
            }
        }
        field.resetInChange();
        lastCallback(field);
        return 0;
    }
    
    template<class field_t, class gameLog_t, class hand_t>
    void setFieldAfterChange(field_t& field, const gameLog_t& gLog,
                             const std::array<hand_t, N_PLAYERS>& hand){
        // カード交換が終わった後から棋譜を読み始める時の初期設定
        // 全体初期化はされていると仮定する
        for(int p = 0; p < N_PLAYERS; ++p){
            Cards tmp = Cards(hand[p]);
            field.hand[p].setAll(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ field.hand[p].hash);
            field.addAttractedPlayer(p);
        }
        field.prepareAfterChange();
    }
    
    // 試合中のプレーヤーがこれまでの試合(交換後)を振り返る場合
    // 相手手札がわからないのでhandとして外部から与える
    template<class field_t, class gameLog_t, class hand_t,
    typename firstCallback_t, typename playCallback_t>
    int iterateGameLogInGame
    (field_t& field, const gameLog_t& gLog, int turns, const std::array<hand_t, N_PLAYERS>& hand,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
     bool initialized = false){
        if(!initialized){
            setFieldBeforeAll(field, gLog);
            setFieldAfterChange(field, gLog, hand); // 交換後まで進める
        }
        field.prepareAfterChange();
        firstCallback(field);
        // play
        for(int t = 0; t < turns; ++t){
            field.prepareForPlay();
            const typename gameLog_t::playLog_t& play = gLog.play(t);
            int ret = playCallback(field, play.move(), play.time());
            if(ret <= -2){
                cerr << "error on play turn " << t << endl;
                return ret;
            }else if(ret == -1){
                break;
            }
            // proceed field
            field.procSlowest(play.move());
        }
        return 0;
    }
    
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
    int iterateGameLogAfterChange
    (field_t& field, const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{},
     bool initialized = false){
        int ret = iterateGameLogInGame(field, gLog, gLog.plays(), gLog.orgCards,
                                       firstCallback, playCallback, initialized);
        if(ret <= -2)return ret; // この試合もう進めない
        field.infoNewClass = gLog.infoNewClass;
        field.infoNewClassPlayer = invert(gLog.infoNewClass);
        lastCallback(field);
        return 0;
    }
    
    // 1試合全体
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t,
    typename afterChangeCallback_t, typename playCallback_t, typename lastCallback_t>
    int iterateGameLog
    (field_t& field, const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const afterChangeCallback_t& afterChangeCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        // 交換
        int ret = iterateGameLogBeforePlay(field, gLog, firstCallback, dealtCallback, changeCallback);
        if(ret <= -2)return ret; // この試合もう進めない
        // 役提出
        return iterateGameLogAfterChange(field, gLog, afterChangeCallback, playCallback, lastCallback, true);
    }
    
    // 1ゲームずつ順番に読む
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
    int iterateGameLogBeforePlay
    (field_t& field, const MinMatchLog<gameLog_t>& mLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            int err = iterateGameLogBeforePlay<field_t>
            (field, gLog, firstCallback, dealtCallback, changeCallback, lastCallback);
            if(err <= -3){
                cerr << "error on game " << g << endl;
                return err;
            }
        }
        return 0;
    }
    
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
    int iterateGameLogAfterChange
    (field_t& field, const MinMatchLog<gameLog_t>& mLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            int err = iterateGameLogAfterChange<field_t>
            (field, gLog, firstCallback, playCallback, lastCallback);
            if(err <= -3){
                cerr << "error on game " << g << endl;
                return err;
            }
        }
        return 0;
    }
    
    // 1ゲームずつ順番に読む
    template<class field_t, class matchLog_t, int N,
    typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
    int iterateGameLogBeforePlay
    (field_t& field, const MinMatchLogAccessor<matchLog_t, N>& mLogs,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        for(int m = 0; m < mLogs.matches(); ++m){
            int err = iterateGameLogBeforePlay<field_t>
            (field, mLogs.match(m), firstCallback, dealtCallback, changeCallback, lastCallback);
            if(err <= -4){
                cerr << "error on match " << m << endl;
                return err;
            }
        }
        return 0;
    }
    
    template<class field_t, class matchLog_t, int N,
    typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
    int iterateGameLogAfterChange
    (field_t& field, const MinMatchLogAccessor<matchLog_t, N>& mLogs,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        for(int m = 0; m < mLogs.matches(); ++m){
            int err = iterateGameLogAfterChange<field_t>
            (field, mLogs.match(m), firstCallback, playCallback, lastCallback);
            if(err <= -4){
                cerr << "error on match " << m << endl;
                return err;
            }
        }
        return 0;
    }
    
    // 盤面情報のセット
    template<class gameLog_t, class field_t>
    void setFieldFromLog(const gameLog_t& gLog, field_t *const pfield, int turns){
        // ルールを信頼する
        if(turns < 0){ // turns < 0 で交換中を指定する
            iterateGameLogBeforePlay(*pfield, gLog,
                                     [](const field_t&)->void{},
                                     [](const field_t&)->void{},
                                     [](const field_t&, const int, const int, const Cards)->int{ return 0; },
                                     [](const field_t&)->void{},
                                     true);
        }else{
            iterateGameLogInGame(*pfield, gLog, turns, gLog.orgCards,
                                 [](const field_t&)->void{},
                                 [](const field_t&, const Move, const uint64_t)->int{ return 0; });
        }
    }
    template<class gameLog_t, class field_t>
    void setFieldFromClientLog(const gameLog_t& gLog, int myPlayerNum, field_t *const dst){
        // TODO: ルールを信頼しないようにする
        setFieldBeforeAll(*dst, gLog);
        
        if(gLog.getPlayerClass(myPlayerNum) != HEIMIN && gLog.isInChange()){
            // 自分の手札だけわかるので設定
            Cards dealt = gLog.getDealtCards(myPlayerNum);
            //cerr << "dc = " << OutCards(dealt) << endl;
            dst->dealtCards[myPlayerNum] = dealt;
            for(int p = 0; p < N_PLAYERS; ++p)
                dst->hand[p].qty = gLog.getNDealtCards(p);
            dst->hand[myPlayerNum].setAll(dealt);
            dst->setInChange();
        }else{
            dst->sentCards[myPlayerNum] = gLog.getSentCards();
            dst->recvCards[myPlayerNum] = gLog.getRecvCards();
            dst->dealtCards[myPlayerNum] = gLog.getDealtCards(myPlayerNum);
            
            for(int t = 0; t < gLog.plays(); ++t){
                const auto& pLog = gLog.play(t);
                int p = pLog.getTurnPlayer();
                
                Cards dc = pLog.usedCards;
                if(anyCards(dc)){
                    uint32_t dq = countCards(dc);
                    uint64_t dkey = CardsToHashKey(dc);
                    
                    // 全体の残り手札の更新
                    addCards(&dst->usedCards[p], dc);
                    subtrCards(&dst->remCards, dc);
                    dst->remQty -= dq;
                    dst->remHash ^= dkey;
                    
                    // あがり処理
                    if(countCards(p) == pLog.getNCards(p))
                        dst->setPlayerNewClass(p, pLog.ps.getBestClass());
                }
            }
            for(int p = 0; p < N_PLAYERS; ++p)
                dst->hand[p].qty = gLog.current.getNCards(p);
            
            DERR << "dc = " << OutCards(gLog.getDealtCards(myPlayerNum)) << endl;
            DERR << "oc = " << OutCards(gLog.getOrgCards(myPlayerNum)) << endl;
            DERR << "uc = " << OutCards(dst->usedCards[myPlayerNum]) << endl;
            DERR << "rc = " << OutCards(dst->remCards) << endl;
            Cards myCards = subtrCards(gLog.getOrgCards(myPlayerNum),
                                       dst->usedCards[myPlayerNum]);
            uint32_t myQty = countCards(myCards);
            uint64_t myKey = CardsToHashKey(myCards);
            if(anyCards(myCards))
                dst->hand[myPlayerNum].setAll(myCards, myQty, myKey);
            Cards opsCards = subtrCards(dst->remCards, myCards);
            if(anyCards(opsCards))
                dst->opsHand[myPlayerNum].setAll(opsCards, dst->remQty - myQty, dst->remHash ^ myKey);
            
            dst->turnNum = gLog.plays();
            dst->bd = gLog.current.bd;
            dst->ps = gLog.current.ps;
            dst->infoSpecialPlayer = gLog.current.infoSpecialPlayer;
            
            dst->originalKey = CardsToHashKey(gLog.getOrgCards(myPlayerNum));
            dst->recordKey = CardsArrayToHashKey<N_PLAYERS>(dst->usedCards.data());
            dst->numCardsKey = NumCardsToHashKey<N_PLAYERS>([&](int p)->int{ return gLog.current.getNCards(p); });
            dst->boardKey = BoardToHashKey(dst->bd);
            dst->aliveKey = StateToAliveHashKey(dst->ps);
            dst->fullAwakeKey = StateToFullAwakeHashKey(dst->ps);
            dst->stateKey = StateToHashKey(dst->aliveKey, dst->ps, dst->getTurnPlayer());
        }
    }
}