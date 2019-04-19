#pragma once

#include "daifugo.hpp"
#include "field.hpp"

// 試合棋譜
// 最小の情報のみ保持
// 一対戦（プレーヤーが変わらない、得点が累計）と
// 複数対戦を手軽に扱うためのクラスも準備

namespace Recorder {
    static const std::vector<std::string> commandList = {
        "//", "/*", "*/"
        "player", "game", "score", "seat", "class",
        "dealt", "changed", "original",
        "play", "result",
    };
    
    std::map<std::string, int> commandMap;
    
    inline bool isCommand(const std::string& str) {
        return commandMap.find(str) != commandMap.end();
    }
    static void initCommandSet() {
        commandMap.clear();
        int cnt = 0;
        for (std::string command : commandList) commandMap[command] = cnt++;
    }
}
    
static int StringQueueToCardsM(std::queue<std::string>& q, Cards *const dst) {
    *dst = CARDS_NULL;
    const std::string& str = q.front();
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
        const std::string& str = q.front();
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
static int StringToMoveTimeM(const std::string& str, Move *const dstMv, uint64_t *const dstTime) {
    *dstMv = MOVE_PASS;
    *dstTime = 0;
    
    Move mv;
    std::vector<std::string> v = split(str, " []\n");
    
    for (const std::string& tstr : v) DERR << tstr << ", ";
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

template <class matchLog_t>
int readMatchLogFile(const std::string& fName, matchLog_t *const pmLog) {
    
    using std::string;
    using std::vector;
    using std::queue;
    
    using game_t = typename matchLog_t::game_t;
    using playLog_t = typename game_t::playLog_t;
    using change_t = typename game_t::change_t;
    
    constexpr int N = N_PLAYERS;
    
    Recorder::initCommandSet();
    
    std::ifstream ifs;
    
    ifs.open(fName, std::ios::in);
    
    if (!ifs) {
        cerr << "readMatchLogFile() : no log file." << endl;
        return -1;
    }
    
    pmLog->init();
    
    game_t gr;
    BitArray32<4, N> infoClass, infoSeat, infoNewClass;
    std::map<int, change_t> cLogMap;
    Move lastMove;
    
    std::bitset<32> flagGame, flagMatch;
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
        
        if (cmd == "//") { // 全試合終了
            break;
        }
        else if (cmd == "/*") { // game開始合図
            if (startedGame >= 0) {
                cerr << "failed to read game " << (startedGame + failedGames) << endl;
                failedGames += 1;
            }
            startedGame = pmLog->games();
            gr.init();
            infoClass.clear();
            infoSeat.clear();
            infoNewClass.clear();
            cLogMap.clear();
            lastMove = MOVE_PASS;
            flagGame.reset();
        }
        else if (cmd == "*/") { // game終了合図
            // ログとして必要なデータが揃っているか確認
            // if (!flagGame.test(0)) {}//ゲーム番号不明
            // if (!flagGame.test(0)) {}//累積スコア不明
            if (!lastMove.isPASS()) {
                gr.setTerminated();
            }
            gr.infoClass = infoClass;
            gr.infoSeat = infoSeat;
            gr.infoNewClass = infoNewClass;
            pmLog->pushGame(gr);
            startedGame = -1;
        }
        else if (cmd == "match") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            q.pop();
            flagMatch.set(0);
        }
        else if (cmd == "player") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                DERR << "pname : " << str << endl;
                pmLog->setPlayer(i, str);
                q.pop();
            }
            flagMatch.set(1);
        }
        else if (cmd == "game") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            char *end;
            int gn = strtol(str.c_str(), &end, 10);
            DERR << "game " << gn << endl;
            q.pop();
            flagGame.set(0);
        }
        else if (cmd == "score") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                q.pop();
            }
            flagGame.set(1);
        }
        else if (cmd == "class") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoClass.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(2);
        }
        else if (cmd == "seat") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoSeat.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(3);
        }
        else if (cmd == "dealt") {
            for (int i = 0; i < N; i++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                gr.setDealtCards(i, c);
                DERR << c << endl;
            }
            flagGame.set(4);
        }
        else if (cmd == "changed") {
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
                gr.setInitGame();
                DERR << "init game." << endl;
            } else {
                for (auto c : cLogMap) {
                    gr.push_change(c.second);
                }
            }
            flagGame.set(5);
        }
        else if (cmd == "original") {
            for (int i = 0; i < N; i++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo(); 
                gr.setOrgCards(i, c);
                DERR << c << endl;
            }
            flagGame.set(6);
        }
        else if (cmd == "play") {
            while (1) {
                Move mv; uint64_t time;
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                if (StringToMoveTimeM(str, &mv, &time) < 0) Foo();
                playLog_t pLog(mv, time);
                gr.push_play(pLog);
                lastMove = mv;
                q.pop();
            }
            flagGame.set(7);
        }
        else if (cmd == "result") {
            for (int i = 0; i < N; i++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoNewClass.assign(i, ToI(str));
                q.pop();
            }
            flagGame.set(8);
        }
        else {
            // いかなるコマンドでもないので読み飛ばし
            q.pop();
        }
    NEXT:;
    }
END:;
    
    cerr << pmLog->games() << " games were loaded." << endl;
    
    // これで全試合が読み込めたはず
    ifs.close();
    return 0;
}
#undef ToI
#undef Foo

struct PlayRecord { // 1つの着手の記録
    Move move; unsigned time;

    PlayRecord(): move(), time() {}
    PlayRecord(Move m, unsigned t): move(m), time(t) {}
    void set(Move m, unsigned t) {
        move = m; time = t;
    }
    std::string toString() const {
        std::ostringstream oss;
        oss << LogMove(move) << "[" << time << "]";
        return oss.str();
    }
};

struct Observation {
    BoardState bs;
    char numCards[N_PLAYERS];
};

class EnginePlayRecord : public PlayRecord {
    // 思考エンジン用の着手記録
    // ルールが微妙に違っても対処できるようにこのクラスが局面の一次情報クラスとして振舞う
public:
    void set(Move m, Cards dc, unsigned t, unsigned st) {
        PlayRecord::set(m, t);
        subjectiveTime = st;
        usedCards = dc;
    }
    template <class field_t>
    bool playByServer(Move move, Cards dc, const field_t& f) {
        // あがりの場合にtrueを返す
        bool agari = move.qty() <= o.numCards[f.seatPlayer(o.bs.turnSeat)];
        o.bs.play(move, agari, false);
        return agari;
    }
    void initGame(int n) { o.bs.init(n); }
    void setTurnSeat(int s) { o.bs.turnSeat = s; }
    void setNumCards(int p, int n) { o.numCards[p] = n; }
    void flush() { o.bs.flush(); }

    Observation o;
    Cards usedCards; // javaサーバは役表現と構成する手札表現が合わないことがある...
    uint32_t subjectiveTime;
};

struct ChangeRecord { // 交換の記録
    int from, to; Cards cards;
    ChangeRecord(): from(), to(), cards() {}
    ChangeRecord(int f, int t, Cards c): from(f), to(t), cards(c) {}
};

template <class _playLog_t>
class CommonGameRecord {
public:
    
    using playLog_t = _playLog_t;
    using change_t = ChangeRecord;
    
    void init() {
        changes_ = 0;
        plays_ = 0;
        flags_.reset();
        infoClass.clear();
        infoSeat.clear();
        infoNewClass.clear();
        infoPosition.clear();
    }
    int plays() const { return plays_; }
    int changes() const { return changes_; }
    
    const playLog_t& play(int t) const { return play_[t]; }
    const change_t& change(int c) const { return change_[c]; }
    
    void push_change(const change_t& change) {
        if (changes_ < N_CHANGES) change_[changes_++] = change;
        else flags_.set(2);
    }
    void push_play(const playLog_t& play) {
        if (plays_ < 128) play_[plays_++] = play;
        else flags_.set(3);
    }
    
    void setTerminated() { flags_.set(0); }
    void setInitGame() { flags_.set(1); }
    void resetInitGame() { flags_.reset(0); }
    
    bool isInChange() const { return !isInitGame() && changes() < N_CHANGES; }
    
    bool isTerminated() const { return flags_.test(0); }
    bool isInitGame() const { return flags_.test(1); }
    bool isChangeOver() const { return flags_.test(2); }
    bool isPlayOver() const { return flags_.test(3); }
    
    int classOf(int p) const { return infoClass[p]; }
    int seatOf(int p) const { return infoSeat[p]; }
    int newClassOf(int p) const { return infoNewClass[p]; }
    int positionOf(int p) const { return infoPosition[p]; }
    
    // 逆置換
    int classPlayer(int cl) const { return invert(infoClass)[cl]; }
    int seatPlayer(int s) const { return invert(infoSeat)[s]; }
    int newClassPlayer(int ncl) const { return invert(infoNewClass)[ncl]; }
    
    void setPlayerClass(int p, int cl) { infoClass.assign(p, cl); }
    void setPlayerSeat(int p, int s) { infoSeat.assign(p, s); }
    void setPlayerNewClass(int p, int ncl) { infoNewClass.assign(p, ncl); }
    void setPosition(int p, int pos) { infoPosition.assign(p, pos); }
    
    BitArray32<4, N_PLAYERS> infoClass, infoSeat, infoNewClass, infoPosition;
    
protected:
    static constexpr int N_CHANGES = (N_PLAYERS / 2) * 2; // TODO: まあ5人以下だからいいのか?
    int changes_;
    std::array<change_t, N_CHANGES> change_;
    int plays_;
    
    // 128手を超えることは滅多にないので強制打ち切り
    // ここをvectorとかにすると連続対戦棋譜がvector<vector>みたいになって死ぬ
    std::array<playLog_t, 128> play_;
    std::bitset<32> flags_;
};

template <class _playLog_t>
class GameRecord : public CommonGameRecord<_playLog_t> {
private:
    using base = CommonGameRecord<_playLog_t>;
public:
    void init() {
        base::init();
        dealtCards.fill(CARDS_NULL);
        orgCards.fill(CARDS_NULL);
    }
    
    bool isSubjective() { return false; }
    
    Cards getDealtCards(int p) const { return dealtCards[p]; }
    void setDealtCards(int p, Cards c) { dealtCards[p] = c; }
    void addDealtCards(int p, Cards c) { dealtCards[p] |= c; }
    
    Cards getOrgCards(int p) const { return orgCards[p]; }
    void setOrgCards(int p, Cards c) { orgCards[p] = c; }
    void addOrgCards(int p, Cards c) { orgCards[p] |= c; }
    
    std::string toString(int gn) const {
        std::ostringstream oss;
        
        oss << "/* " << endl;
        oss << "game " << gn << " " << endl;
        oss << "score " << endl;
        oss << "class ";
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << base::classOf(p) << " ";
        }
        oss << endl;
        oss << "seat ";
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << base::seatOf(p) << " ";
        }
        oss << endl;
        oss << "dealt ";
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << tolower(getDealtCards(p).toString()) << " ";
        }
        oss << endl;
        oss << "changed ";
        
        Cards changeCards[N_PLAYERS];
        for (int p = 0; p < N_PLAYERS; p++) changeCards[p].clear();
        for (int c = 0; c < base::changes(); c++) {
            changeCards[base::change(c).from] = base::change(c).cards;
        }
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << tolower(changeCards[p].toString()) << " ";
        }
        oss << endl;
        oss << "original ";
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << tolower(getOrgCards(p).toString()) << " ";
        }
        oss << endl;
        oss << "play ";
        for (int t = 0; t < base::plays(); t++) {
            oss << base::play(t).toString() << " ";
        }
        oss << endl;
        oss << "result ";
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << base::newClassOf(p) << " ";
        }
        oss << endl;
        oss << "*/ " << endl;
        
        return oss.str();
    }
    
    int fadd(const std::string& fName, int g) {
        // ファイルに追加書き込み
        std::ofstream ofs(fName, std::ios::app);
        if (!ofs) return -1;
        ofs << toString(g);
        return 0;
    }
    int firstTurnSeat;
    std::array<Cards, N_PLAYERS> dealtCards, orgCards;
};

class EngineGameRecord : public GameRecord<EnginePlayRecord> {
public:
    void init() {
        GameRecord<EnginePlayRecord>::init();
        infoNDealtCards.clear();
        infoNOrgCards.clear();
        sentCards = recvCards = CARDS_NULL;
    }
    
    static constexpr bool isSubjective() { return true; }
    
    void setNDealtCards(int p, int n) { infoNDealtCards.assign(p, n); }
    void setNOrgCards(int p, int n) { infoNOrgCards.assign(p, n); }
    void setSentCards(Cards c) { sentCards = c; }
    void setRecvCards(Cards c) { recvCards = c; }
    
    int getNOrgCards(int p) const { return infoNOrgCards[p]; }
    int getNDealtCards(int p) const { return infoNDealtCards[p]; }
    Cards getSentCards() const { return sentCards; }
    Cards getRecvCards() const { return recvCards; }
    
    // 交換中かどうかの判定はオーバーロードしているので注意
    bool isInChange() const {
        return !GameRecord<EnginePlayRecord>::isInitGame() && sentCards == CARDS_NULL;
    }
    
    BitArray32<4, N_PLAYERS> infoNDealtCards, infoNOrgCards;
    Cards sentCards, recvCards; // 本当はchangeにいれたいけど
    EnginePlayRecord current;
};

template <class game_t_>
class MatchRecordBase {
public:
    using game_t = game_t_;
    
    int games() const { return game_.size(); }
    
    const std::string& player(int p) const { return player_[p]; }
    const std::string& fileName() const { return fileName_; }
    const game_t& game(int g) const { return game_[g]; }
    
    const game_t& latestGame() const { return game_.back(); }
    game_t& latestGame() { return game_.back(); }
    int getLatestGameNum() const { return games() - 1; }
    
    int getScore(int p) const { return score[p]; }
    int positionOf(int p) const { // score から順位を調べる
        int pos = 0;
        for (int pp = 0; pp < N_PLAYERS; pp++) {
            if (score[pp] < score[p]) pos += 1;
        }
        return pos;
    }
    
    void setPlayer(int p, const std::string& str) {
        player_[p] = str;
    }
    
    void reserveGames(std::size_t n) {
        game_.reserve(n);
    }
    void initGame(int n) {
        game_.emplace_back(game_t());
        game_t& g = latestGame();
        g.init();
        for (int p = 0; p < N_PLAYERS; p++) {
            g.setPosition(p, positionOf(p));
        }
    }
    void closeGame() {
        const game_t& g = latestGame();
        for (int p = 0; p < N_PLAYERS; p++) {
            score[p] += REWARD(g.newClassOf(p));
        }
    }
    void pushGame(const game_t& gr) {
        game_.emplace_back(gr);
    }
    void init() {
        game_.clear();
        player_.fill("");
        score.fill(0);
    }
    
    std::string toHeaderString() const {
        // ヘッダ部分の出力
        std::ostringstream oss;
        oss << "player";
        for (int p = 0; p < N_PLAYERS; p++) oss << " " << player_[p];
        oss << endl;
        return oss.str();
    }
    int fin(std::string fName) {
        // ファイルから読み込み
        fileName_ = fName;
        return readMatchLogFile(fName, this);
    }
    int fout(std::string fName) {
        // ファイルに書き込み
        fileName_ = fName;
        std::ofstream ofs(fName, std::ios::out);
        if (!ofs) return -1;
        ofs << toHeaderString();
        for (int i = 0; i < games(); i++) ofs << game(i).toString(i);
        return 0;
    }
    int foutLast(std::string fName) {
        // ファイルに最後の試合を書き込み
        std::ofstream ofs(fName, std::ios::app);
        if (!ofs) return -1;
        int g = (int)games() - 1;
        if (g < 0) return -1;
        ofs << game(g).toString(g);
        return 0;
    }
    MatchRecordBase() { init(); }
    MatchRecordBase(const std::string& logFile) {
        init(); fin(logFile);
    }
protected:
    std::string fileName_;
    std::array<std::string, N_PLAYERS> player_;
    std::vector<game_t> game_;
    std::array<int64_t, N_PLAYERS> score;
};

struct EngineMatchRecord : public MatchRecordBase<EngineGameRecord> {
    EngineMatchRecord() {
        init();
        myPlayerNum = -1;
    }
    int myPlayerNum;
};

template <class match_t_>
struct MatchRecordAccessor { // ランダム順なアクセス
    using match_t = match_t_;
    using game_t = typename match_t::game_t;

    std::deque<match_t> match;
    std::vector<int> rindex, startIndex;
    
    MatchRecordAccessor() { init(); }
    MatchRecordAccessor(std::string logFiles) {
        init(); fin(logFiles);
    }
    MatchRecordAccessor(std::vector<std::string> logFiles) {
        init(); fin(logFiles);
    }
    void init() {
        match.clear();
        rindex.clear();
        startIndex.clear();
        startIndex.push_back(0);
    }
    int matches() const { return match.size(); }
    int games() const { return startIndex.back(); }
    template <class dice_t>
    void shuffle(int first, int end, dice_t& dice) {
        std::shuffle(rindex.begin() + first, rindex.begin() + end, dice);
    }
    template <class dice_t>
    void shuffle(dice_t& dice) { shuffle(0, rindex.size(), dice); }
    const game_t game(int idx) const { return gameImpl(idx); }
    const game_t& rgame(int idx) const { return gameImpl(rindex[idx]); }
    const game_t& gameImpl(int idx) const {
        auto itr = std::upper_bound(startIndex.begin(), startIndex.end(), idx); // 二分探索
        int midx = itr - startIndex.begin() - 1; // itrは超えた位置を指すので1引く
        return match[midx].game(idx - startIndex[midx]);
    }
    int fin(const std::string& logFile) {
        // 棋譜ファイルを1つ読み込み
        cerr << "using log file [ " << logFile << " ]" << endl;
        match.resize(match.size() + 1);
        int err = match.back().fin(logFile);
        if (err < 0) {
            cerr << "failed to read " << logFile << "." << endl;
            match.resize(match.size() - 1);
            return err;
        }
        // インデックスの準備
        int g = match.back().games();
        for (int i = 0; i < g; i++) rindex.push_back(startIndex.back() + i);
        startIndex.push_back(startIndex.back() + g);
        return 0;
    }
    int fin(const std::vector<std::string>& logFiles, bool forced = false) {
        // 棋譜ファイルを複数読み込み
        for (int i = 0; i < logFiles.size(); i++) {
            int err = fin(logFiles.at(i));
            if (err < 0 && !forced) return err;
        }
        cerr << matches() << " matches were loaded." << endl;
        return 0;
    }
};

using MatchRecord = MatchRecordBase<GameRecord<PlayRecord>>;
using Record = MatchRecordAccessor<MatchRecordBase<GameRecord<PlayRecord>>>;

// ここから棋譜から局面を正方向に読む関数たち

// 交換やプレー時のコールバッックには返り値を要求する
// 0 が通常
// -1 でそのフェーズ終了
// -2 で試合ログ読み終了
// -3 で1マッチ(連続対戦)終了
// -4 で全マッチ終了

template <class field_t, class game_t>
void setFieldBeforeAll(field_t& field, const game_t& gr) {
    // 棋譜を読んでの初期設定
    field.initGame();
    field.setMoveBuffer(nullptr);
    field.setDice(nullptr);
    if (gr.isInitGame()) field.setInitGame();
    field.infoNewClass.fill(-1);
    field.infoNewClassPlayer.fill(-1);
    for (int p = 0; p < N_PLAYERS; p++) {
        field.setClassOf(p, gr.classOf(p));
        field.setSeatOf(p, gr.seatOf(p));
        field.setPositionOf(p, gr.positionOf(p));
    }
}

template <class field_t, class game_t,
typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
int iterateGameLogBeforePlay
(field_t& field, const game_t& gr,
 const firstCallback_t& firstCallback = [](const field_t&)->void{},
 const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
 const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const field_t&)->void{},
 bool stopBeforeChange = false) {
    setFieldBeforeAll(field, gr);
    firstCallback(field);
    
    // deal cards
    for (int p = 0; p < N_PLAYERS; p++) {
        Cards tmp = gr.getDealtCards(p);
        field.hand[p].cards = tmp; // とりあえずcardsだけセットしておく
    }
    // present
    for (int t = 0, tend = gr.changes(); t < tend; t++) {
        const typename game_t::change_t& change = gr.change(t);
        if (field.classOf(change.to) < HEIMIN) {
            Cards present = change.cards;
            field.hand[change.to].cards |= present;
        }
    }
    // set card info
    for (int p = 0; p < N_PLAYERS; p++) {
        Cards c = field.hand[p].cards;
        field.hand[p].setAll(c);
        field.opsHand[p].set(CARDS_ALL - c);
        field.opsHand[p].setKey(subCardKey(HASH_CARDS_ALL, field.hand[p].key));
        field.addAttractedPlayer(p);
    }
    
    field.setInChange();
    if (stopBeforeChange) return -1;
    
    dealtCallback(field);
    
    // change
    for (int t = 0, tend = gr.changes(); t < tend; t++) {
        const typename game_t::change_t& change = gr.change(t);
        if (field.classOf(change.from) >= HINMIN) {
            // 献上以外
            field.hand[change.from].subtrAll(change.cards);
            field.opsHand[change.from].addAll(change.cards);
        } else {
            int ret = changeCallback(field, change.from, change.to, change.cards);
            if (ret <= -2) {
                cerr << "error on change "
                << change.from << " -> " << change.to << endl;
                return ret;
            } else if (ret == -1) {
                break;
            }
            // proceed field
            field.makeChange(change.from, change.to, change.cards);
        }
    }
    field.resetInChange();
    lastCallback(field);
    return 0;
}

template <class field_t, class game_t, class hand_t>
void setFieldAfterChange(field_t& field, const game_t& gr,
                         const std::array<hand_t, N_PLAYERS>& hand) {
    // カード交換が終わった後から棋譜を読み始める時の初期設定
    // 全体初期化はされていると仮定する
    for (int p = 0; p < N_PLAYERS; p++) {
        Cards tmp = Cards(hand[p]);
        field.hand[p].setAll(tmp);
        field.opsHand[p].set(CARDS_ALL - tmp);
        field.opsHand[p].setKey(subCardKey(HASH_CARDS_ALL, field.hand[p].key));
        field.addAttractedPlayer(p);
    }
    field.prepareAfterChange();
}

// 試合中のプレーヤーがこれまでの試合(交換後)を振り返る場合
// 相手手札がわからないのでhandとして外部から与える
template <class field_t, class game_t, class hand_t,
typename firstCallback_t, typename playCallback_t>
int iterateGameLogInGame
(field_t& field, const game_t& gr, int turns, const std::array<hand_t, N_PLAYERS>& hand,
 const firstCallback_t& firstCallback = [](const field_t&)->void{},
 const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
 bool initialized = false) {
    if (!initialized) {
        setFieldBeforeAll(field, gr);
        setFieldAfterChange(field, gr, hand); // 交換後まで進める
    }
    field.prepareAfterChange();
    firstCallback(field);
    // play
    for (int t = 0; t < turns; t++) {
        field.prepareForPlay();
        const typename game_t::playLog_t& play = gr.play(t);
        int ret = playCallback(field, play.move, play.time);
        if (ret <= -2) {
            cerr << "error on play turn " << t << endl;
            return ret;
        } else if (ret == -1) {
            break;
        }
        // proceed field
        field.play(play.move);
    }
    return 0;
}

template <class field_t, class game_t,
typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLogAfterChange
(field_t& field, const game_t& gr,
 const firstCallback_t& firstCallback = [](const field_t&)->void{},
 const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const field_t&)->void{},
 bool initialized = false) {
    int ret = iterateGameLogInGame(field, gr, gr.plays(), gr.orgCards,
                                   firstCallback, playCallback, initialized);
    if (ret <= -2) return ret; // この試合もう進めない
    for (int p = 0; p < N_PLAYERS; p++) field.setNewClassOf(p, gr.newClassOf(p));
    lastCallback(field);
    return 0;
}

// 1試合全体
template <class field_t, class game_t,
typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t,
typename afterChangeCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLog
(field_t& field, const game_t& gr,
 const firstCallback_t& firstCallback = [](const field_t&)->void{},
 const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
 const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
 const afterChangeCallback_t& afterChangeCallback = [](const field_t&)->void{},
 const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const field_t&)->void{}) {
    // 交換
    int ret = iterateGameLogBeforePlay(field, gr, firstCallback, dealtCallback, changeCallback);
    if (ret <= -2) return ret; // この試合もう進めない
    // 役提出
    return iterateGameLogAfterChange(field, gr, afterChangeCallback, playCallback, lastCallback, true);
}

// 盤面情報のセット
template <class game_t, class field_t>
void setFieldFromLog(const game_t& gr, field_t *const pfield, int turns) {
    // ルールを信頼する
    if (turns < 0) { // turns < 0 で交換中を指定する
        iterateGameLogBeforePlay(*pfield, gr,
                                 [](const field_t&)->void{},
                                 [](const field_t&)->void{},
                                 [](const field_t&, const int, const int, const Cards)->int{ return 0; },
                                 [](const field_t&)->void{},
                                 true);
    } else {
        iterateGameLogInGame(*pfield, gr, turns, gr.orgCards,
                             [](const field_t&)->void{},
                             [](const field_t&, const Move, const uint64_t)->int{ return 0; });
    }
}
template <class game_t, class field_t>
void setFieldFromClientLog(const game_t& gr, int myPlayerNum, field_t *const dst) {
    // TODO: ルールを信頼しないようにする
    setFieldBeforeAll(*dst, gr);
    
    if (gr.classOf(myPlayerNum) != HEIMIN && gr.isInChange()) {
        // 自分の手札だけわかるので設定
        Cards dealt = gr.getDealtCards(myPlayerNum);
        dst->dealtCards[myPlayerNum] = dealt;
        for (int p = 0; p < N_PLAYERS; p++) {
            dst->hand[p].qty = gr.getNDealtCards(p);
        }
        dst->hand[myPlayerNum].setAll(dealt);
        dst->setInChange();
    } else {
        dst->sentCards[myPlayerNum] = gr.getSentCards();
        dst->recvCards[myPlayerNum] = gr.getRecvCards();
        dst->dealtCards[myPlayerNum] = gr.getDealtCards(myPlayerNum);
        
        for (int t = 0; t < gr.plays(); t++) {
            const auto& pr = gr.play(t);
            const Observation o = pr.o;
            int p = gr.seatPlayer(o.bs.turnSeat);
            
            Cards dc = pr.usedCards;
            if (anyCards(dc)) {
                uint32_t dq = countCards(dc);
                uint64_t dkey = CardsToHashKey(dc);
                
                // 全体の残り手札の更新
                dst->usedCards[p] |= dc;
                dst->remCards -= dc;
                dst->remQty -= dq;
                dst->remKey = subCardKey(dst->remKey, dkey);
                // あがり処理
                if (countCards(p) >= o.numCards[p]) {
                    dst->setNewClassOf(p, o.bs.bestClass());
                }
            }
        }
        for (int p = 0; p < N_PLAYERS; p++) {
            dst->hand[p].qty = gr.current.o.numCards[p];
        }
        
        DERR << "dc = " << gr.getDealtCards(myPlayerNum) << endl;
        DERR << "oc = " << gr.getOrgCards(myPlayerNum) << endl;
        DERR << "uc = " << dst->usedCards[myPlayerNum] << endl;
        DERR << "rc = " << dst->remCards << endl;
        Cards myCards = gr.getOrgCards(myPlayerNum) - dst->usedCards[myPlayerNum];
        uint32_t myQty = countCards(myCards);
        uint64_t myKey = CardsToHashKey(myCards);
        if (anyCards(myCards)) {
            dst->hand[myPlayerNum].setAll(myCards, myQty, myKey);
        }
        Cards opsCards = dst->remCards - myCards;
        if (anyCards(opsCards)) {
            dst->opsHand[myPlayerNum].setAll(opsCards, dst->remQty - myQty,
                                             subCardKey(dst->remKey, myKey));
        }
        dst->board = gr.current.o.bs;
        dst->common.turnCount = gr.plays();
        dst->common.firstTurn = gr.seatPlayer(gr.firstTurnSeat);
    }
}