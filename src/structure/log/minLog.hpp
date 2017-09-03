/*
 minLog.hpp
 Katsuki Ohto
 */

// 試合進行ログ
// 最小の情報のみ保持
// 一対戦（プレーヤーが変わらない、得点が累計）と
// 複数対戦を手軽に扱うためのクラスも準備

#ifndef UECDA_STRUCTURE_MINLOG_HPP_
#define UECDA_STRUCTURE_MINLOG_HPP_

#include "../primitive/prim.hpp"
#include "minLogIO.hpp"

namespace UECda{
    
    class MinPlayLog{
        // 1つの着手の記録
    public:
        constexpr MinPlayLog():
        move_(), time_(){}
        
        constexpr MinPlayLog(const Move amove, const uint32_t atime):
        move_(amove), time_(atime){}
        
        Move move()const noexcept{ return move_; }
        uint32_t time()const noexcept{ return time_; }
        
        void set(const Move amove, const uint32_t atime)noexcept{
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
    
    class MinClientPlayLog : public MinPlayLog{
        // クライアント用の着手記録
    public:
        constexpr MinClientPlayLog():
        MinPlayLog(), sbjTime_(){}
        
        constexpr MinClientPlayLog(const Move amove, const uint32_t atime, const uint32_t asbjTime):
        MinPlayLog(amove, atime), sbjTime_(asbjTime){}
        
        uint32_t sbjTime()const noexcept{ return sbjTime_; }
        
        void set(const Move amove, const uint32_t atime, const uint32_t asbjTime)noexcept{
            MinPlayLog::set(amove, atime); sbjTime_ = asbjTime;
        }
    protected:
        uint32_t sbjTime_;
    };
    
    class MinChangeLog{
        // 交換の記録
    public:
        constexpr MinChangeLog():
        from_(), to_(), c_(){}
        
        constexpr MinChangeLog(const uint32_t afrom, const uint32_t ato, const Cards ac):
        from_(afrom), to_(ato), c_(ac){}
        
        uint32_t from()const noexcept{ return from_; }
        uint32_t to()const noexcept{ return to_; }
        Cards cards()const noexcept{ return c_; }
        
    protected:
        uint32_t from_, to_; Cards c_;
    };
    
    template<class _playLog_t>
    class MinCommonGameLog{
    public:
        
        using playLog_t = _playLog_t;
        using changeLog_t = MinChangeLog;
        
        void init()noexcept{
            changes_ = 0; plays_ = 0;
            flags_.reset();
            infoClass_.clear(); infoSeat_.clear(); infoNewClass_.clear();
        }
        int plays()const noexcept{ return plays_; }
        int changes()const noexcept{ return changes_; }
        
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
        
        void setTerminated()noexcept{ flags_.set(0); }
        void setInitGame()noexcept{ flags_.set(1); }
        void resetInitGame()noexcept{ flags_.reset(0); }
        
        auto isTerminated()const noexcept{ return flags_.test(0); }
        auto isInitGame()const noexcept{ return flags_.test(1); }
        auto isChangeOver()const noexcept{ return flags_.test(2); }
        auto isPlayOver()const noexcept{ return flags_.test(3); }
        
        BitArray32<4, N_PLAYERS>& infoClass()noexcept{ return infoClass_; }
        BitArray32<4, N_PLAYERS>& infoSeat()noexcept{ return infoSeat_; }
        BitArray32<4, N_PLAYERS>& infoNewClass()noexcept{ return infoNewClass_; }
        
        const BitArray32<4, N_PLAYERS>& infoClass()const noexcept{ return infoClass_; }
        const BitArray32<4, N_PLAYERS>& infoSeat()const noexcept{ return infoSeat_; }
        const BitArray32<4, N_PLAYERS>& infoNewClass()const noexcept{ return infoNewClass_; }
        
        void setPlayerClass(int p, int cl)noexcept{ infoClass_.assign(p, cl); }
        void setPlayerSeat(int p, int s)noexcept{ infoSeat_.assign(p, s); }
        
    protected:
        static constexpr int N_CHANGES = (N_PLAYERS / 2) * 2; // TODO: まあ5人以下だからいいのか?
        int changes_;
        std::array<changeLog_t, N_CHANGES> change_;
        
        int plays_;
        
        // 128手を超えることは滅多にないので強制打ち切り
        // ここをvectorとかにすると連続対戦棋譜がvector<vector>みたいになって死ぬ
        std::array<playLog_t, 128> play_;
        
        BitSet32 flags_;
        
        BitArray32<4, N_PLAYERS> infoClass_;
        BitArray32<4, N_PLAYERS> infoSeat_;
        BitArray32<4, N_PLAYERS> infoNewClass_; // 先日手でサーバーが順位決定した場合に必要なデータ
    };
    
    template<class _playLog_t>
    class MinGameLog : public MinCommonGameLog<_playLog_t>{
    private:
        using base = MinCommonGameLog<_playLog_t>;
    public:
        void init(){
            base::init();
            dealtCards_.fill(CARDS_NULL);
            orgCards_.fill(CARDS_NULL);
        }
        
        Cards dealtCards(int p)const{ return dealtCards_[p]; }
        void setDealtCards(int p, Cards c){ dealtCards_[p] = c; }
        void addDealtCards(int p, Cards c){ addCards(&dealtCards_[p], c); }
        
        Cards orgCards(int p)const{ return orgCards_[p]; }
        void setOrgCards(int p, Cards c){ orgCards_[p] = c; }
        void addOrgCards(int p, Cards c){ addCards(&orgCards_[p], c); }
        
        const std::array<Cards, N_PLAYERS>& orgCards()const{ return orgCards_; }
        const std::array<Cards, N_PLAYERS>& dealtCards()const{ return dealtCards_; }
        
        std::string toString(int gn)const{
            std::ostringstream oss;
            
            oss << "/* " << endl;
            oss << "game " << gn << " " << endl;
            oss << "score " << endl;
            oss << "class ";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << base::infoClass().at(p) << " ";
            }
            oss << endl;
            oss << "seat ";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << base::infoSeat().at(p) << " ";
            }
            oss << endl;
            oss << "dealt ";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << OutCardsM(dealtCards(p)) << " ";
            }
            oss << endl;
            oss << "changed ";
            
            Cards changeCards[N_PLAYERS];
            for(int p = 0; p < N_PLAYERS; ++p){
                changeCards[p] = CARDS_NULL;
            }
            for(int c = 0; c < base::changes(); ++c){
                changeCards[base::change(c).from()] = base::change(c).cards();
            }
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << OutCardsM(changeCards[p]) << " ";
            }
            oss << endl;
            oss << "original ";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << OutCardsM(orgCards(p)) << " ";
            }
            oss << endl;
            oss << "play ";
            for(int t = 0; t < base::plays(); ++t){
                oss << base::play(t).toString() << " ";
            }
            oss << endl;
            oss << "result ";
            for(int p = 0; p < N_PLAYERS; ++p){
                oss << base::infoNewClass().at(p) << " ";
            }
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
        
    protected:
        std::array<Cards, N_PLAYERS> dealtCards_;
        std::array<Cards, N_PLAYERS> orgCards_;
    };
    
    template<class _playLog_t>
    class MinClientGameLog : public MinGameLog<_playLog_t>{
    private:
        using base = MinGameLog<_playLog_t>;
    public:
        void init(){
            base::init();
            NOrgCards_.clear();
            sentCards_ = recvCards_ = CARDS_NULL;
        }
        void setNOrgCards(int p, int n){ NOrgCards_.assign(p, n); }
        void setSentCards(int p, Cards c){ sentCards_ = c; }
        void setRecvCards(int p, Cards c){ recvCards_ = c; }
        
        int getNOrgCards(int p)const{ return NOrgCards_[p]; }
        Cards getSentCards(int p)const{ return sentCards_; }
        Cards getRecvCards(int p)const{ return recvCards_; }
        
        BitArray32<4, N_PLAYERS>& NOrgCards(){ return NOrgCards_; }
        const BitArray32<4, N_PLAYERS>& NOrgCards()const{ return NOrgCards_; }
    protected:
        BitArray32<4, N_PLAYERS> NOrgCards_;
        Cards sentCards_, recvCards_; // 本当はchangeにいれたいけど
    };
    
    template<class _gameLog_t>
    class MinMatchLog{
    public:
        using gameLog_t = _gameLog_t;
        
        int games()const noexcept{ return game_.size(); }
        
        const std::string& player(int p)const{ return player_[p]; }
        const std::string& fileName()const{ return fileName_; }
        const gameLog_t& game(int t)const{ return game_[t]; }
        
        void setPlayer(int p, const std::string& str){
            player_[p] = str;
        }
        void push_game(const gameLog_t& game){
            game_.emplace_back(game);
        }
        void reserve_game(std::size_t n){
            game_.reserve(n);
        }
        
        void init()noexcept{
            game_.clear();
            for(int p = 0; p < N_PLAYERS; ++p){
                player_[p] = "";
            }
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
        
        int matches()const noexcept{ return matches_; }
        int games()const noexcept{ return games_sum_[matches_]; }
        
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
        
        void addMatch()noexcept{ matches_++; }
        
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
        void init()noexcept{
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
        for(int m = 0; m < mLogs.matches(); ++m){
            callback(mLogs.match(m));
        }
    }
    template<class _matchLog_t, int N, typename callback_t>
    void iterateGameRandomly(const MinMatchLogAccessor<_matchLog_t, N>& mLogs,
                             int first, int end, const callback_t& callback){
        for(int i = max(0, first); i < min(mLogs.games(), end); ++i){
            callback(mLogs.access(i), mLogs.accessMatch(i));
        }
    }
    template<class _matchLog_t, int N, typename callback_t>
    void iterateGameRandomlyWithIndex(const MinMatchLogAccessor<_matchLog_t, N>& mLogs,
                                      int first, int end, const callback_t& callback){
        for(int i = max(0, first); i < min(mLogs.games(), end); ++i){
            callback(mLogs.accessIndex(i), mLogs.access(i), mLogs.accessMatch(i));
        }
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
        field.init1G();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        if(gLog.isInitGame()){
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass();
            field.infoClassPlayer = invert(gLog.infoClass());
        }
        field.infoSeat = gLog.infoSeat();
        field.infoSeatPlayer = invert(gLog.infoSeat());
    }
    
    template<class field_t, class gameLog_t,
    typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
    int iterateGameLogBeforePlay
    (field_t& field, const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}){
        setFieldBeforeAll(field, gLog);
        firstCallback(field);
        
        // deal cards
        for(int p = 0; p < N_PLAYERS; ++p){
            Cards tmp = gLog.dealtCards(p);
            field.hand[p].cards = tmp; // とりあえずcardsだけセットしておく
        }
        // present
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const typename gameLog_t::changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.to()) <= FUGO){
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
        field.fillRemHand(CARDS_ALL);
        
        dealtCallback(field);
        
        // change
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const typename gameLog_t::changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.from()) >= HINMIN){
                // subtr present cards
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
        lastCallback(field);
        return 0;
    }
    
    template<class field_t, class gameLog_t, class hand_t>
    void setFieldAfterChange(field_t& field, const gameLog_t& gLog,
                             const std::array<hand_t, N_PLAYERS>& hand){
        // カード交換が終わった後から棋譜を読み始める時の初期設定
        for(int p = 0; p < N_PLAYERS; ++p){
            Cards tmp = Cards(hand[p]);
            field.hand[p].setAll(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ field.hand[p].hash);
            field.addAttractedPlayer(p);
        }
        field.fillRemHand(CARDS_ALL);
        field.prepareAfterChange();
    }
    
    // 試合中のプレーヤーがこれまでの試合(交換後)を振り返る場合
    // 相手手札がわからないのでhandとして外部から与える
    template<class field_t, class gameLog_t, class hand_t,
    typename firstCallback_t, typename playCallback_t>
    int iterateGameLogInGame
    (field_t& field, const gameLog_t& gLog, const std::array<hand_t, N_PLAYERS>& hand,
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
        for(int t = 0; t < gLog.plays(); ++t){
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
        int ret = iterateGameLogInGame(field, gLog, gLog.orgCards(),
                                       firstCallback, playCallback, initialized);
        if(ret <= -2)return ret; // この試合もう進めない
        field.infoNewClass = gLog.infoNewClass();
        field.infoNewClassPlayer = invert(gLog.infoNewClass());
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
}

#endif // UECDA_STRUCTURE_MINLOG_HPP_
