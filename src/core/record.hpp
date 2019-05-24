#pragma once

#include <boost/container/static_vector.hpp>
#include "daifugo.hpp"
#include "field.hpp"

struct PlayRecord { // 1つの着手の記録
    Move move; unsigned time;

    PlayRecord(): move(), time() {}
    PlayRecord(Move m, unsigned t): move(m), time(t) {}
    void set(Move m, unsigned t) {
        move = m; time = t;
    }
    std::string toString() const;
};

class EnginePlayRecord : public PlayRecord {
    // 思考エンジン用の着手記録
    // ルールが微妙に違っても対処できるようにこのクラスが局面の一次情報クラスとして振舞う
public:
    int turn() const { return turn_; }
    int owner() const { return owner_; }
    int firstTurn() const { return firstTurn_; }
    int getNCards(int p) const { return infoNCards[p]; }
    void set(Move m, Cards dc, unsigned t, unsigned st) {
        PlayRecord::set(m, t);
        subjectiveTime = st;
        usedCards = dc;
    }
    bool procByServer(Move move, Cards dc) {
        // あがりの場合に1を返す
        bool dead = false;
        int tp = turn();
        if (move.isPASS()) { // パス
            ps.setAsleep(tp);
        } else { // パスでない
            bd.procExceptFlush(move);
            infoNCards.sub(tp, countCards(dc));
            if (getNCards(tp) <= 0) {//あがり
                ps.setDead(tp);
                dead = true;
            }
            setOwner(tp);
        }
        return dead;
    }
    void initGame() {
        bd.init();
        ps.init();
    }
    void setTurn(int p) { turn_ = p; }
    void setOwner(int p) { owner_ = p; }
    void setFirstTurn(int p) { firstTurn_ = p; }
    void setNCards(int p, int n) {
        infoNCards.assign(p, n);
    }
    void flush(BitArray32<4, N_PLAYERS> infoSeat) {
        bd.flush();
        ps.flush();
        flushTurnPlayer(infoSeat);
    }
    void flushTurnPlayer(BitArray32<4, N_PLAYERS> infoSeat) {
        int tp = owner();
        if (ps.isAlive(tp)) setTurn(tp);
        else rotateTurnPlayer(tp, infoSeat);
    }
    void rotateTurnPlayer(int tp, BitArray32<4, N_PLAYERS> infoSeat) {
        BitArray32<4, N_PLAYERS> infoSeatPlayer = invert(infoSeat);
        do {
            tp = infoSeatPlayer[getNextSeat<N_PLAYERS>(infoSeat[tp])];
        } while (!ps.isAwake(tp));
        setTurn(tp);
    }
    
    char turn_, owner_, firstTurn_;
    Board bd;
    PlayersState ps;
    BitArray32<4, N_PLAYERS> infoNCards;
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
        changes.clear();
        plays.clear();
        flags_.reset();
        infoClass.clear();
        infoSeat.clear();
        infoNewClass.clear();
        infoPosition.clear();
    }
    void push_change(const change_t& change) {
        if (changes.size() < N_CHANGES) changes.push_back(change);
        else flags_.set(2);
    }
    void push_play(const playLog_t& play) {
        if (plays.size() < 128) plays.push_back(play);
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

    static constexpr int N_CHANGES = (N_PLAYERS / 2) * 2; // TODO: まあ5人以下だからいいのか?
    boost::container::static_vector<change_t, N_CHANGES> changes;
    // 128手を超えることは滅多にないので強制打ち切り
    // ここをvectorとかにすると連続対戦棋譜がvector<vector>みたいになって死ぬ
    boost::container::static_vector<playLog_t, 128> plays;
    std::bitset<32> flags_;
};

template <class _playLog_t>
class GameRecord : public CommonGameRecord<_playLog_t> {
protected:
    using base = CommonGameRecord<_playLog_t>;
public:
    void init() {
        base::init();
        dealtCards.fill(CARDS_NULL);
        orgCards.fill(CARDS_NULL);
    }
    
    static constexpr bool isSubjective() { return false; }
    
    Cards getDealtCards(int p) const { return dealtCards[p]; }
    void setDealtCards(int p, Cards c) { dealtCards[p] = c; }
    
    Cards getOrgCards(int p) const { return orgCards[p]; }
    void setOrgCards(int p, Cards c) { orgCards[p] = c; }
    void addOrgCards(int p, Cards c) { orgCards[p] += c; }

    std::array<Cards, N_PLAYERS> dealtCards, orgCards;
};

class ServerGameRecord : public GameRecord<PlayRecord> {
public:
    std::string toString(int gn) const;
    int fadd(const std::string& fName, int g) {
        // ファイルに追加書き込み
        std::ofstream ofs(fName, std::ios::app);
        if (!ofs) return -1;
        ofs << toString(g);
        return 0;
    }
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
    void initGame() {
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
    void pushGame(const game_t& gLog) {
        game_.emplace_back(gLog);
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

template <class game_t>
void setFieldBeforeAll(Field& field, const game_t& gLog) {
    // 棋譜を読んでの初期設定
    field.initGame();
    field.setMoveBuffer(nullptr);
    if (gLog.isInitGame()) field.setInitGame();
    field.infoNewClass.fill(-1);
    field.infoNewClassPlayer.fill(-1);
    for (int p = 0; p < N_PLAYERS; p++) {
        field.setClassOf(p, gLog.classOf(p));
        field.setSeatOf(p, gLog.seatOf(p));
        field.setPositionOf(p, gLog.positionOf(p));
    }
}

template <class game_t,
typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t, typename lastCallback_t>
int iterateGameLogBeforePlay
(Field& field, const game_t& gLog,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const dealtCallback_t& dealtCallback = [](const Field&)->void{},
 const changeCallback_t& changeCallback = [](const Field&, const int, const int, const Cards)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{},
 bool stopBeforeChange = false) {
    setFieldBeforeAll(field, gLog);
    firstCallback(field);
    
    // deal cards
    for (int p = 0; p < N_PLAYERS; p++) {
        Cards tmp = gLog.getDealtCards(p);
        field.hand[p].cards = tmp; // とりあえずcardsだけセットしておく
    }
    // 献上が先にある場合
    for (const auto& change : gLog.changes) {
        if (field.classOf(change.to) < HEIMIN) {
            Cards present = change.cards;
            field.hand[change.to].cards += present;
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
    for (const auto& change : gLog.changes) {
        int fromClass = field.classOf(change.from);
        if (fromClass < HEIMIN) {
            // 上位側の交換選択機会
            int ret = changeCallback(field, change.from, change.to, change.cards);
            if (ret <= -2) {
                cerr << "error on change " << change.from << " -> " << change.to << endl;
                return ret;
            } else if (ret == -1) {
                break;
            }
        }
        // 配布時献上ルールならば献上側はあげるだけ
        bool sendOnly = fromClass > HEIMIN;
        field.makeChange(change.from, change.to, change.cards, sendOnly);
    }
    field.resetInChange();
    lastCallback(field);
    return 0;
}

template <class game_t>
void setFieldAfterChange(Field& field, const game_t& gLog,
                         const std::array<Cards, N_PLAYERS>& hand) {
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
template <class game_t,
typename firstCallback_t, typename playCallback_t>
int iterateGameLogInGame
(Field& field, const game_t& gLog, int turns, const std::array<Cards, N_PLAYERS>& hand,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move, const uint64_t)->int{ return 0; },
 bool initialized = false) {
    field.myPlayerNum = -1;
    if (!initialized) {
        setFieldBeforeAll(field, gLog);
        if (!gLog.isSubjective()) {
            for (int p = 0; p < N_PLAYERS; p++) {
                field.dealtCards[p] = gLog.dealtCards[p];
            }
            for (const auto& change : gLog.changes) {
                field.sentCards[change.from] = change.cards;
                field.recvCards[change.to] = change.cards;
            }
        }
        setFieldAfterChange(field, gLog, hand); // 交換後まで進める
    }
    field.prepareAfterChange();
    firstCallback(field);
    // play
    for (int t = 0; t < turns; t++) {
        field.prepareForPlay();
        const typename game_t::playLog_t& play = gLog.plays[t];
        int ret = playCallback(field, play.move, play.time);
        if (ret <= -2) {
            cerr << "error on play turn " << t << endl;
            return ret;
        } else if (ret == -1) {
            break;
        }
        // proceed field
        field.procSlowest(play.move);
    }
    return 0;
}

template <class game_t,
typename firstCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLogAfterChange
(Field& field, const game_t& gLog,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{},
 bool initialized = false) {
    int ret = iterateGameLogInGame(field, gLog, gLog.plays.size(), gLog.orgCards,
                                   firstCallback, playCallback, initialized);
    if (ret <= -2) return ret; // この試合もう進めない
    for (int p = 0; p < N_PLAYERS; p++) field.setNewClassOf(p, gLog.newClassOf(p));
    lastCallback(field);
    return 0;
}

// 1試合全体
template <class game_t,
typename firstCallback_t, typename dealtCallback_t, typename changeCallback_t,
typename afterChangeCallback_t, typename playCallback_t, typename lastCallback_t>
int iterateGameLog
(Field& field, const game_t& gLog,
 const firstCallback_t& firstCallback = [](const Field&)->void{},
 const dealtCallback_t& dealtCallback = [](const Field&)->void{},
 const changeCallback_t& changeCallback = [](const Field&, const int, const int, const Cards)->int{ return 0; },
 const afterChangeCallback_t& afterChangeCallback = [](const Field&)->void{},
 const playCallback_t& playCallback = [](const Field&, const Move&, const uint64_t)->int{ return 0; },
 const lastCallback_t& lastCallback = [](const Field&)->void{}) {
    // 交換
    int ret = iterateGameLogBeforePlay(field, gLog, firstCallback, dealtCallback, changeCallback);
    if (ret <= -2) return ret; // この試合もう進めない
    // 役提出
    return iterateGameLogAfterChange(field, gLog, afterChangeCallback, playCallback, lastCallback, true);
}

// 盤面情報のセット
template <class game_t>
void setFieldFromLog(const game_t& gLog, Field *const pfield, int turns) {
    // ルールを信頼する
    if (turns < 0) { // turns < 0 で交換中を指定する
        iterateGameLogBeforePlay(*pfield, gLog,
                                 [](const Field&)->void{},
                                 [](const Field&)->void{},
                                 [](const Field&, const int, const int, const Cards)->int{ return 0; },
                                 [](const Field&)->void{},
                                 true);
    } else {
        iterateGameLogInGame(*pfield, gLog, turns, gLog.orgCards,
                             [](const Field&)->void{},
                             [](const Field&, const Move, const uint64_t)->int{ return 0; });
    }
}
template <class game_t>
void setFieldFromClientLog(const game_t& gLog, int myPlayerNum, Field *const dst) {
    // TODO: ルールを信頼しないようにする
    setFieldBeforeAll(*dst, gLog);
    dst->myPlayerNum = myPlayerNum;
    if (gLog.classOf(myPlayerNum) != HEIMIN && gLog.isInChange()) {
        // 自分の手札だけわかるので設定
        Cards dealt = gLog.getDealtCards(myPlayerNum);
        dst->dealtCards[myPlayerNum] = dealt;
        for (int p = 0; p < N_PLAYERS; p++) {
            dst->hand[p].qty = gLog.getNDealtCards(p);
        }
        dst->hand[myPlayerNum].setAll(dealt);
        dst->setInChange();
    } else {
        dst->sentCards[myPlayerNum] = gLog.getSentCards();
        dst->recvCards[myPlayerNum] = gLog.getRecvCards();
        dst->dealtCards[myPlayerNum] = gLog.getDealtCards(myPlayerNum);
 
        // 使用カードの処理
        for (const auto& play : gLog.plays) {
            int turn = play.turn();
            Cards dc = play.usedCards;
            if (anyCards(dc)) {
                uint32_t dq = countCards(dc);
                uint64_t dkey = CardsToHashKey(dc);
                
                // 全体の残り手札の更新
                dst->usedCards[turn] += dc;
                dst->remCards -= dc;
                dst->remQty -= dq;
                dst->remKey = subCardKey(dst->remKey, dkey);
                // あがり処理
                if (countCards(turn) == play.getNCards(turn)) {
                    dst->setNewClassOf(turn, play.ps.getBestClass());
                }
            }
        }
        for (int p = 0; p < N_PLAYERS; p++) {
            dst->hand[p].qty = gLog.current.getNCards(p);
        }
        DERR << "dc = " << gLog.getDealtCards(myPlayerNum) << endl;
        DERR << "oc = " << gLog.getOrgCards(myPlayerNum) << endl;
        DERR << "uc = " << dst->usedCards[myPlayerNum] << endl;
        DERR << "rc = " << dst->remCards << endl;

        // 自分の手札を設定
        Cards myCards = gLog.getOrgCards(myPlayerNum) - dst->usedCards[myPlayerNum];
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
        // 現時点の盤面を設定
        dst->board = gLog.current.bd;
        dst->ps = gLog.current.ps;

        dst->common.turnCount = gLog.plays.size();
        dst->common.turn = gLog.current.turn();
        dst->common.owner = gLog.current.owner();
        dst->common.firstTurn = gLog.current.firstTurn();
    }
}