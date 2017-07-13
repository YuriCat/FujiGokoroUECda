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
#include "logIteration.hpp"

namespace UECda{
    
    template<int _ARGN>
    class MinPlayLog{
    private:
        constexpr static int N_ = _ARGN;
    public:
        constexpr static int players()noexcept{ return N_; }
        
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
    
    template<int _ARGN>
    class MinClientPlayLog : public MinPlayLog<_ARGN>{
    public:
        constexpr MinClientPlayLog():
        MinPlayLog<_ARGN>(), sbjTime_(){}
        
        constexpr MinClientPlayLog(const Move amove, const uint32_t atime, const uint32_t asbjTime):
        MinPlayLog<_ARGN>(amove, atime), sbjTime_(asbjTime){}
        
        uint32_t sbjTime()const noexcept{return sbjTime_;}
        
        void set(const Move amove, const uint32_t atime, const uint32_t asbjTime)noexcept{
            MinPlayLog<_ARGN>::set(amove, atime); sbjTime_ = asbjTime;
        }
    protected:
        uint32_t sbjTime_;
    };
    
    template<int _ARGN>
    class MinChangeLog{
    public:
        constexpr MinChangeLog():
        from_(), to_(), c_(){}
        
        constexpr MinChangeLog(const uint32_t afrom, const uint32_t ato, const Cards ac):
        from_(afrom), to_(ato), c_(ac){}
        
        uint32_t from()const noexcept{ return from_; }
        uint32_t to()const noexcept{ return to_; }
        Cards cards()const noexcept{ return c_; }
        
    protected:
        constexpr static int N_=_ARGN;
        uint32_t from_, to_; Cards c_;
    };
    
    template<class _playLog_t>
    class MinCommonGameLog{
    private:
        constexpr static int N_ = _playLog_t::players();
    public:
        
        using playLog_t = _playLog_t;
        using changeLog_t = MinChangeLog<N_>;
        
        void init()noexcept{
            changes_ = 0; plays_ = 0;
            flags_.reset();
            infoClass_.clear(); infoSeat_.clear(); infoNewClass_.clear();
        }
        
        constexpr static int players()noexcept{ return N_; }
        int plays()const noexcept{ return plays_; }
        int changes()const noexcept{ return changes_; }
        
        const playLog_t& play(int t)const{ return play_[t]; }
        const changeLog_t& change(int c)const{ return change_[c]; }
        
        void push_change(const changeLog_t& change){
            if(changes_ < N_){
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
        
        auto isTerminated()const noexcept{ return flags_.test(0); }
        auto isInitGame()const noexcept{ return flags_.test(1); }
        auto isChangeOver()const noexcept{ return flags_.test(2); }
        auto isPlayOver()const noexcept{ return flags_.test(3); }
        
        BitArray32<4, N_>& infoClass()noexcept{ return infoClass_; }
        BitArray32<4, N_>& infoSeat()noexcept{ return infoSeat_; }
        BitArray32<4, N_>& infoNewClass()noexcept{ return infoNewClass_; }
        
        const BitArray32<4, N_>& infoClass()const noexcept{ return infoClass_; }
        const BitArray32<4, N_>& infoSeat()const noexcept{ return infoSeat_; }
        const BitArray32<4, N_>& infoNewClass()const noexcept{ return infoNewClass_; }
        
    protected:
        int changes_;
        std::array<changeLog_t, N_> change_;
        
        int plays_;
        std::array<playLog_t, 128> play_;
        
        BitSet32 flags_;
        
        BitArray32<4, N_> infoClass_;
        BitArray32<4, N_> infoSeat_;
        BitArray32<4, N_> infoNewClass_; // 先日手でサーバーが順位決定した場合に必要なデータ
    };
    
    template<class _playLog_t>
    class MinGameLog : public MinCommonGameLog<_playLog_t>{
    private:
        using base = MinCommonGameLog<_playLog_t>;
    public:
        
        Cards dealtCards(int p)const{ return dealtCards_[p]; }
        void setDealtCards(int p, Cards c){ dealtCards_[p] = c; }
        void addDealtCards(int p, Cards c){ addCards(&dealtCards_[p], c); }
        
        Cards orgCards(int p)const{ return orgCards_[p]; }
        void setOrgCards(int p, Cards c){ orgCards_[p] = c; }
        void addOrgCards(int p, Cards c){ addCards(&orgCards_[p], c); }
        
        std::string toString(int gn)const{
            std::ostringstream oss;
            
            oss << "/* " << endl;
            oss << "game " << gn << " " << endl;
            oss << "score " << endl;
            oss << "class ";
            for(int p = 0; p < base::players(); ++p){
                oss << base::infoClass().at(p) << " ";
            }
            oss << endl;
            oss << "seat ";
            for(int p = 0; p < base::players(); ++p){
                oss << base::infoSeat().at(p) << " ";
            }
            oss << endl;
            oss << "dealt ";
            for(int p = 0; p < base::players(); ++p){
                oss << OutCardsM(dealtCards(p)) << " ";
            }
            oss << endl;
            oss << "changed ";
            
            Cards changeCards[base::players()];
            for(int p = 0; p < base::players(); ++p){
                changeCards[p] = CARDS_NULL;
            }
            for(int c = 0; c < base::changes(); ++c){
                changeCards[base::change(c).from()] = base::change(c).cards();
            }
            for(int p = 0; p < base::players(); ++p){
                oss << OutCardsM(changeCards[p]) << " ";
            }
            oss << endl;
            oss << "original ";
            for(int p = 0; p < base::players(); ++p){
                oss << OutCardsM(orgCards(p)) << " ";
            }
            oss << endl;
            oss << "play ";
            for(int t = 0; t < base::plays(); ++t){
                oss << base::play(t).toString() << " ";
            }
            oss << endl;
            oss << "result ";
            for(int p = 0; p < base::players(); ++p){
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
        std::array<Cards, MinCommonGameLog<_playLog_t>::players()> dealtCards_;
        std::array<Cards, MinCommonGameLog<_playLog_t>::players()> orgCards_;
    };
    
    template<class _playLog_t>
    class MinClientGameLog : public MinCommonGameLog<_playLog_t>{
    private:
        using base = MinCommonGameLog<_playLog_t>;
    public:
        void setDealtCards(int p, Cards c){}
        
        Cards orgCards(int p)const{ return orgCards_[p]; }
        void setOrgCards(int p, Cards c){ orgCards_[p] = c; }
        void addOrgCards(int p, Cards c){ addCards(&orgCards_[p], c); }
        
        std::string toString(int gn)const{
            std::ostringstream oss;
            
            oss << "/* " << endl;
            oss << "game " << gn << " " << endl;
            oss << "score " << endl;
            oss << "class ";
            for(int p = 0; p < base::players(); ++p){
                oss << base::infoClass().at(p) << " ";
            }
            oss << endl;
            oss << "seat ";
            for(int p = 0; p < base::players(); ++p){
                oss << base::infoSeat().at(p) << " ";
            }
            oss << endl;
            oss << "original ";
            for(int p = 0; p < base::players(); ++p){
                oss << OutCardsM(orgCards(p)) << " ";
            }
            oss << endl;
            oss << "play ";
            for(int t = 0; t < base::plays(); ++t){
                oss << base::play(t).toString() << " ";
            }
            oss << endl;
            oss << "result ";
            for(int p = 0; p < base::players(); ++p){
                oss << base::infoNewClass().at(p) << " ";
            }
            oss << endl;
            oss << "*/ " << endl;
            
            return oss.str();
        }
        
    protected:
        std::array<Cards, MinCommonGameLog<_playLog_t>::players()> orgCards_;
    };
    
    template<class _gameLog_t>
    class MinMatchLog{
        constexpr static int N_ = _gameLog_t::players();
        
    public:
        using gameLog_t = _gameLog_t;
        
        constexpr static int players()noexcept{ return N_; }
        
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
            for(int p = 0; p < N_; ++p){
                player_[p] = "";
            }
        }
        
        std::string toHeaderString()const{
            // ヘッダ部分の出力
            std::ostringstream oss;
            oss << "player";
            for(int p = 0; p < N_; ++p){
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
        std::array<std::string, N_> player_;
        std::vector<gameLog_t> game_;
        
    };
    
    // 1ゲームずつ順番に読む
    template<
    class field_t,
    class gameLog_t,
    typename firstCallback_t,
    typename dealtCallback_t,
    typename changeCallback_t,
    typename lastCallback_t>
    int iterateGameLogBeforePlay
    (const MinMatchLog<gameLog_t>& mLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            int err = iterateGameLogBeforePlay<field_t>
            (gLog, firstCallback, dealtCallback, changeCallback, lastCallback);
            if(err <= -3){
                cerr << "error on game " << g << endl;
                return err;
            }
        }
        return 0;
    }
    
    template<
    class field_t,
    class gameLog_t,
    typename firstCallback_t,
    typename playCallback_t,
    typename lastCallback_t>
    int iterateGameLogAfterChange
    (const MinMatchLog<gameLog_t>& mLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            int err = iterateGameLogAfterChange<field_t>
            (gLog, firstCallback, playCallback, lastCallback);
            if(err <= -3){
                cerr << "error on game " << g << endl;
                return err;
            }
        }
        return 0;
    }

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
    
    // 1ゲームずつ順番に読む
    template<
    class field_t,
    class matchLog_t,
    int N,
    typename firstCallback_t,
    typename dealtCallback_t,
    typename changeCallback_t,
    typename lastCallback_t>
    int iterateGameLogBeforePlay
    (const MinMatchLogAccessor<matchLog_t, N>& mLogs,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        for(int m = 0; m < mLogs.matches(); ++m){
            const auto& mLog = mLogs.match(m);
            int err = iterateGameLogBeforePlay<field_t>
            (mLog, firstCallback, dealtCallback, changeCallback, lastCallback);
            if(err <= -4){
                cerr << "error on match " << m << endl;
                return err;
            }
        }
        return 0;
    }
    
    template<
    class field_t,
    class matchLog_t,
    int N,
    typename firstCallback_t,
    typename playCallback_t,
    typename lastCallback_t>
    int iterateGameLogAfterChange
    (const MinMatchLogAccessor<matchLog_t, N>& mLogs,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        for(int m = 0; m < mLogs.matches(); ++m){
            const auto& mLog = mLogs.match(m);
            int err = iterateGameLogAfterChange<field_t>
            (mLog, firstCallback, playCallback, lastCallback);
            if(err <= -4){
                cerr << "error on match " << m << endl;
                return err;
            }
        }
        return 0;
    }
}

#endif // UECDA_STRUCTURE_MINLOG_HPP_
