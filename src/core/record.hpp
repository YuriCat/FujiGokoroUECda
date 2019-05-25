#pragma once

#include <boost/container/static_vector.hpp>
#include "daifugo.hpp"
#include "field.hpp"

struct PlayRecord { // 1つの着手の記録
    Move move; unsigned time;

    void set(Move m, unsigned t) { move = m; time = t; }
    std::string toString() const;
};

struct ChangeRecord { // 交換の記録
    int from; int to;
    bool already; // 既に手札が相手型に渡っている
    int qty; Cards cards;

    void set(int f, int t, bool a, int q, Cards c) {
        from = f; to = t;
        already = a;
        qty = q; cards = c;
    }
};

struct GameRecord {
    void init() {
        changes.clear();
        plays.clear();
        flags_.reset();
        infoClass.clear();
        infoSeat.clear();
        infoNewClass.clear();
        infoPosition.clear();

        dealtCards.fill(CARDS_NULL);
        orgCards.fill(CARDS_NULL);

        numDealtCards.fill(-1);
        numOrgCards.fill(-1);

        firstTurn = -1;
    }
    void push_change(const ChangeRecord& change) {
        if (changes.size() < N_CHANGES) changes.push_back(change);
        else flags_.set(2);
    }
    void push_play(const PlayRecord& play) {
        if (plays.size() < 128) plays.push_back(play);
        else flags_.set(3);
    }

    void setTerminated() { flags_.set(0); }
    void setInitGame() { flags_.set(1); }
    void resetInitGame() { flags_.reset(0); }

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
    
    void setClassOf(int p, int cl) { infoClass.assign(p, cl); }
    void setSeatOf(int p, int s) { infoSeat.assign(p, s); }
    void setNewClassOf(int p, int ncl) { infoNewClass.assign(p, ncl); }
    void setPositionOf(int p, int pos) { infoPosition.assign(p, pos); }
    
    int firstTurn;
    BitArray32<4, N_PLAYERS> infoClass, infoSeat, infoNewClass, infoPosition;
    std::array<int8_t, N_PLAYERS> numDealtCards, numOrgCards;
    std::array<Cards, N_PLAYERS> dealtCards, orgCards;

    static constexpr int N_CHANGES = (N_PLAYERS / 2) * 2; // TODO: まあ5人以下だからいいのか?
    boost::container::static_vector<ChangeRecord, N_CHANGES> changes;
    // 128手を超えることは滅多にないので強制打ち切り
    // ここをvectorとかにすると連続対戦棋譜がvector<vector>みたいになって死ぬ
    boost::container::static_vector<PlayRecord, 128> plays;
    std::bitset<32> flags_;

    std::string toString(int gn) const;
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
    void initGame() {
        game_.emplace_back(game_t());
        game_t& g = latestGame();
        g.init();
        for (int p = 0; p < N_PLAYERS; p++) {
            g.setPositionOf(p, positionOf(p));
        }
    }
    void closeGame() {
        const game_t& g = latestGame();
        for (int p = 0; p < N_PLAYERS; p++) {
            score[p] += REWARD(g.newClassOf(p));
        }
    }
    void pushGame(const game_t& game) {
        game_.emplace_back(game);
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
    int fin(std::string fName);
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

struct EngineMatchRecord : public MatchRecordBase<GameRecord> {
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

using MatchRecord = MatchRecordBase<ServerGameRecord>;
using Record = MatchRecordAccessor<MatchRecordBase<ServerGameRecord>>;

extern int readMatchLogFile(const std::string& fName, MatchRecord *const pmLog);

template <> inline int MatchRecord::fin(std::string fName) {
    // ファイルから読み込み
    fileName_ = fName;
    return readMatchLogFile(fName, this);
}

// ここから棋譜から局面を正方向に読む関数たち

// 交換やプレー時のコールバッックには返り値を要求する
// 0 が通常
// -1 でそのフェーズ終了
// -2 で試合ログ読み終了
// -3 で1マッチ(連続対戦)終了
// -4 で全マッチ終了

template <typename firstCallback_t, typename dealtCallback_t,
          typename changeCallback_t, typename lastCallback_t>
int iterateGameLogBeforePlay
(Field& field, const GameRecord& game,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const dealtCallback_t& dealtCallback = [](const Field&)->void{},
 const changeCallback_t& changeCallback = [](const Field&, const int, const int, const Cards)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{},
 bool stopBeforeChange = false) {
    field.setBeforeGame(game, -1);
    firstCallback(field);
    field.passChange(game, -1);
    field.setInChange();
    if (stopBeforeChange) return -1;
    
    dealtCallback(field);
    
    // change
    for (const auto& change : game.changes) {
        if (!change.already) {
            // 上位側の交換選択機会
            int ret = changeCallback(field, change.from, change.to, change.cards);
            if (ret <= -2) {
                cerr << "error on change " << change.from << " -> " << change.to << endl;
                return ret;
            }
            if (ret == -1) break;
        }
        field.makeChange(change.from, change.to, change.qty, change.cards, change.already);
    }
    field.resetInChange();
    lastCallback(field);
    return 0;
}

// 試合中のプレーヤーがこれまでの試合(交換後)を振り返る場合
// 相手手札がわからないのでhandとして外部から与える
template <typename firstCallback_t, typename playCallback_t>
int iterateGameLogInGame
(Field& field, const GameRecord& game, int turns, const std::array<Cards, N_PLAYERS>& hand,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move, const uint64_t)->int{ return 0; },
 bool initialized = false) {
    if (!initialized) {
        field.passChange(game, -1);
        // 交換後の手札を配置
        field.setAfterChange(game, hand);
    }
    field.prepareAfterChange();
    firstCallback(field);
    // play
    for (int t = 0; t < turns; t++) {
        field.prepareForPlay();
        const auto& play = game.plays[t];
        int ret = playCallback(field, play.move, play.time);
        if (ret <= -2) {
            cerr << "error on play turn " << t << endl;
            return ret;
        }
        if (ret == -1) break;
        // proceed field
        field.procSlowest(play.move);
    }
    return 0;
}

template <class game_t,
typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLogAfterChange
(Field& field, const game_t& game,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{},
 bool initialized = false) {
    int ret = iterateGameLogInGame(field, game, game.plays.size(), game.orgCards,
                                   firstCallback, playCallback, initialized);
    if (ret <= -2) return ret; // この試合もう進めない
    for (int p = 0; p < N_PLAYERS; p++) field.setNewClassOf(p, game.newClassOf(p));
    lastCallback(field);
    return 0;
}

// 1試合全体
template <typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t,
          typename afterChangeCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLog
(Field& field, const GameRecord& game,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const dealtCallback_t& dealtCallback = [](const Field&)->void{},
 const changeCallback_t& changeCallback = [](const Field&, const int, const int, const Cards)->int{ return 0; },
 const afterChangeCallback_t& afterChangeCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move&, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{}) {
    // 交換
    int ret = iterateGameLogBeforePlay(field, game, firstCallback, dealtCallback, changeCallback);
    if (ret <= -2) return ret; // この試合もう進めない
    // 役提出
    return iterateGameLogAfterChange(field, game, afterChangeCallback, playCallback, lastCallback, true);
}