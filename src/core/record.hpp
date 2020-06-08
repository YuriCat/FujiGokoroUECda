#pragma once

#include <queue>
#include <boost/container/static_vector.hpp>
#include "daifugo.hpp"
#include "field.hpp"

struct PlayRecord { // 1つの着手の記録
    Move move; unsigned time;

    operator Move() const { return move; }
    void set(Move m, unsigned t) { move = m; time = t; }
    std::string toString() const;
};

struct ChangeRecord { // 交換の記録
    int from; int to;
    bool already; // 既に手札が相手型に渡っている
    int qty; Cards cards;

    void set(int f, int t, int q, Cards c, bool a) {
        from = f; to = t;
        already = a;
        qty = q; cards = c;
    }
};

struct GameRecord {
    void init(int playerNum = -1);
    void pushChange(const ChangeRecord& change) {
        if (changes.size() < N_CHANGES) changes.push_back(change);
        else flags_.set(2);
    }
    void pushPlay(const PlayRecord& play) {
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

    int myPlayerNum;
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

    std::string toString() const;
};

struct MatchRecord {
    const GameRecord& latestGame() const { return games.back(); }
    GameRecord& latestGame() { return games.back(); }
    int getLatestGameNum() const { return int(games.size()) - 1; }

    int getScore(int p) const { return score[p]; }
    int positionOf(int p) const { // score から順位を調べる
        int pos = 0;
        for (int pp = 0; pp < N_PLAYERS; pp++) {
            if (score[p] < score[pp]) pos += 1;
        }
        return pos;
    }
    void initGame() {
        games.emplace_back(GameRecord());
        auto& g = latestGame();
        g.init(myPlayerNum);
        for (int p = 0; p < N_PLAYERS; p++) {
            g.setPositionOf(p, positionOf(p));
        }
    }
    void closeGame() {
        const auto& g = latestGame();
        for (int p = 0; p < N_PLAYERS; p++) {
            score[p] += REWARD(g.newClassOf(p));
        }
    }
    void pushGame(const GameRecord& game) {
        games.emplace_back(game);
    }
    void init(int playerNum = -1) {
        myPlayerNum = playerNum;
        games.clear();
        playerName.fill("");
        score.fill(0);
    }
    std::string toHeaderString() const;
    int fin(std::string path);
    int fout(std::string path);
    MatchRecord() { init(); }
    MatchRecord(std::string path) {
        init(); fin(path);
    }
    int myPlayerNum = -1;
    std::string filePath;
    std::array<std::string, N_PLAYERS> playerName;
    std::vector<GameRecord> games;
    std::array<int64_t, N_PLAYERS> score;
};

struct Record {
    // MatchRecordのリストをランダム順なアクセス可能にしたもの
    std::deque<MatchRecord> match;
    std::vector<int> rindex, startIndex;

    Record() { init(); }
    Record(const std::vector<std::string>& paths) {
        init(); fin(paths);
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
    const GameRecord& game(int idx) const { return gameImpl(idx); }
    const GameRecord& rgame(int idx) const { return gameImpl(rindex[idx]); }
    const GameRecord& gameImpl(int idx) const {
        auto itr = std::upper_bound(startIndex.begin(), startIndex.end(), idx); // 二分探索
        int midx = itr - startIndex.begin() - 1; // itrは超えた位置を指すので1引く
        return match[midx].games[idx - startIndex[midx]];
    }
    int fin(std::string path);
    int fin(const std::vector<std::string>& paths);
};

// ここから棋譜から局面を正方向に読む実装

template <class actionRecord_t>
class RecordIteratorBase : public std::iterator<std::input_iterator_tag, actionRecord_t> {
    friend Field;
public:
    bool operator !=(const RecordIteratorBase<actionRecord_t>& itr) const {
        return field != itr.field || game != itr.game || ply != itr.ply;
    }
protected:
    explicit RecordIteratorBase(Field *const f, const GameRecord *const g, int p):
    field(f), game(g), ply(p) {}
    Field *const field;
    const GameRecord *const game;
    int ply;
};

class RollerBase {
public:
    RollerBase(Field& f, const GameRecord& g): field(&f), game(&g) {
        field->phase = PHASE_UNINIT; // 初期化状態でなくてもOKに
    }
    ~RollerBase() {
        for (int p = 0; p < N_PLAYERS; p++) {
            if (game->newClassOf(p) >= 0) field->setNewClassOf(p, game->newClassOf(p));
        }
    }
protected:
    Field *const field;
    const GameRecord *const game;
};

class ChangeRoller : public RollerBase {
public:
    class const_iterator : public RecordIteratorBase<ChangeRecord> {
    public:
        ChangeRecord operator *() const { return game->changes[ply]; }
        const_iterator& operator ++() {
            ChangeRecord change = game->changes[ply++];
            field->makeChange(change.from, change.to, change.qty, change.cards,
                              change.already, false);
            return *this;
        }
        explicit const_iterator(Field *const f, const GameRecord *const g, int p):
        RecordIteratorBase<ChangeRecord>(f, g, p) {}
    };

    const_iterator begin() const { return const_iterator(field, game, presentCount); }
    const_iterator end() const { return const_iterator(field, game, game->changes.size()); }

    ChangeRoller(Field& f, const GameRecord& g): RollerBase(f, g) {
        presentCount = field->passPresent(*game, game->myPlayerNum);
    }
protected:
    int presentCount;
};

class PlayRoller : public RollerBase {
public:
    class const_iterator : public RecordIteratorBase<PlayRecord> {
    public:
        PlayRecord operator *() const { return game->plays[ply]; }
        const_iterator& operator ++() {
            field->proceed(game->plays[ply++]);
            return *this;
        }
        explicit const_iterator(Field *const f, const GameRecord *const g, int p):
        RecordIteratorBase<PlayRecord>(f, g, p) {}
    };

    const_iterator begin() const { return const_iterator(field, game, 0); }
    const_iterator end() const { return const_iterator(field, game, game->plays.size()); }

    PlayRoller(Field& f, const GameRecord& g): RollerBase(f, g) {
        field->passChange(*game, game->myPlayerNum);
    }
    PlayRoller(Field& f, const GameRecord& g, const std::array<Cards, N_PLAYERS>& hand):
    PlayRoller(f, g) {
        // 手札を外部からセットする場合
        field->setAfterChange(*game, hand);
    }
};