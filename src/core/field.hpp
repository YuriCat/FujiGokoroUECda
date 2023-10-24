#pragma once

// 盤の情報表現
#include "../UECda.h"
#include "daifugo.hpp"
#include "hand.hpp"

/**************************完全情報空間**************************/

enum GameFlag {
    FLAG_INIT_GAME,
};

enum GamePhase {
    PHASE_UNINIT, PHASE_INIT,
    PHASE_PRESENT, PHASE_CHANGE, PHASE_PLAY,
};

// common information
struct CommonStatus {
    int turnCount;
    int turn;
    int firstTurn;
    int owner;
    std::bitset<16> flag;

    void clear() {
        turnCount = 0;
        turn = firstTurn = owner = -1;
        flag.reset();
    }
};

/**************************プレーヤー状態**************************/

struct PlayersState : public BitArray32<8, 4> {
    // Boardと合わせて、場の状態を表す
    // 0-7   Alive
    // 8-15  numAlive
    // 16-23 Awake
    // 24-31 numAwake
    using base_t = BitArray32<8, 4>;
    using data_t = typename base_t::data_type;
    constexpr static int N = N_PLAYERS;
    constexpr static uint32_t BMASK = 1U; // 基準ビット
    constexpr static uint32_t PMASK = (1 << 8) - 1; // プレーヤー全体
    constexpr static uint32_t REALPMASK = (1 << N) - 1; // 実際にいるプレーヤー全体
    constexpr static uint32_t NMASK = (1 << 8) - 1; // 数全体
    // set
    void setAsleep(int p) {
        assert(isAwake(p)); // 現在Awake
        base_t::data_ -= (BMASK << 24) + ((BMASK << 16) << p);
    }
    void setDead(int p) {
        // プレーヤーがあがった
        assert(isAwake(p)); assert(isAlive(p)); // 現在AliveかつAwakeの必要
        base_t::data_ -= (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
    }
    void setAllAsleepExcept(int p) {
        base_t::data_ &= (PMASK << 0) | (NMASK << 8);
        base_t::data_ += (BMASK << 24) + ((BMASK << 16) << p);
    }

    constexpr data_t isAlive(int p) const { return data() & ((BMASK << 0) << p); }
    constexpr data_t isAwake(int p) const { return data() & ((BMASK << 16) << p); }
    constexpr bool isExcluded(int p) const { return false; } // あがり以外の除外(都落ち)

    uint32_t searchOpsPlayer(int p) const {
        // p以外でaliveなプレーヤーを1人挙げる
        assert(numAlive() >= 1);
        return bsf32(data() & ~(BMASK << p));
    }

    constexpr data_t numAlive() const { return (*this)[1]; }
    constexpr data_t numAwake() const { return (*this)[3]; }

    unsigned countNAlive() const { return popcnt(part(0)); }
    unsigned countNAwake() const { return popcnt(part(2)); }

    constexpr data_t bestClass() const { return N - numAlive(); } // 最高の階級 = 全員 - 残っている人数
    constexpr data_t worstClass() const { return N - 1; } // 最低の階級 = 全員の最後

    int searchL1Player() const {
        // 最後に残ったプレーヤーを探す
        assert(popcnt32(data() & PMASK) == 1); // 1人だけ残っている
        return bsf32(data());
    }

    void flush() {
        // 場が流れる
        data_t alive = data() & ((PMASK << 0) | (NMASK << 8)); // alive情報
        base_t::data_ = alive | (alive << 16); // awake情報をalive情報に置き換える
    }
    void init() {
        base_t::data_ = (REALPMASK << 0) | (N << 8) | (REALPMASK << 16) | (N << 24);
    }

    bool exam() const;
};

extern std::ostream& operator <<(std::ostream& out, const PlayersState& arg);

struct GameRecord;

struct Field {
    int myPlayerNum = -1; // 主観的局面表現として使用する宣言を兼ねる
    int phase = PHASE_UNINIT;

    // シミュレーション用
    MoveInfo *mbuf = nullptr;
    std::bitset<32> attractedPlayers; // 結果を知りたいプレイヤー集合
    FieldAddInfo fieldInfo;

    CommonStatus common;
    Board board;
    PlayersState ps;

    std::array<int8_t, N_PLAYERS> infoClass, infoClassPlayer;
    std::array<int8_t, N_PLAYERS> infoSeat, infoSeatPlayer;
    std::array<int8_t, N_PLAYERS> infoNewClass, infoNewClassPlayer;
    std::array<int8_t, N_PLAYERS> infoPosition;

    // 手札情報
    Cards remCards;
    uint32_t remQty;
    uint64_t remKey;

    std::array<Hand, N_PLAYERS> hand, opsHand;
    std::array<Cards, N_PLAYERS> usedCards;
    std::array<Cards, N_PLAYERS> dealtCards;
    std::array<Cards, N_PLAYERS> sentCards, recvCards;

    bool isSubjective() const { return myPlayerNum >= 0; }
    bool know(int p) const { return !isSubjective() || myPlayerNum == p; }

    bool isL2Situation() const { return numPlayersAlive() == 2; }
    bool isEndGame() const { // 末端探索に入るべき局面かどうか。学習にも影響する
        if (isL2Situation()) return true;
        return false;
    }

    void setMoveBuffer(MoveInfo *pm) { mbuf = pm; }
    void addAttractedPlayer(int p) { attractedPlayers.set(p); }

    bool isNull() const { return board.isNull(); }
    int turnCount() const  { return common.turnCount; }

    void setInitGame() { common.flag.set(FLAG_INIT_GAME); }
    void resetInitGame() { common.flag.reset(FLAG_INIT_GAME); }
    bool isInitGame() const { return common.flag.test(FLAG_INIT_GAME); }
    bool isInChange() const { return phase == PHASE_PRESENT; }

    bool isAlive(int p) const { return ps.isAlive(p); }
    bool isAwake(int p) const { return ps.isAwake(p); }
    unsigned numPlayersAwake() const { return ps.numAwake(); }
    unsigned numPlayersAlive() const { return ps.numAlive(); }

    int bestClass() const { return ps.bestClass(); }
    int worstClass() const { return ps.worstClass(); }

    void setClassOf(int p, int cl) {
        infoClass[p] = cl;
        infoClassPlayer[cl] = p;
    }
    void setNewClassOf(int p, int c) {
        infoNewClass[p] = c;
        infoNewClassPlayer[c] = p;
    }
    void setSeatOf(int p, int s) {
        infoSeat[p] = s;
        infoSeatPlayer[s] = p;
    }
    void setPositionOf(int p, int pos) {
        infoPosition[p] = pos;
    }

    int classOf(int p) const { return infoClass[p]; }
    int newClassOf(int p) const { return infoNewClass[p]; }
    int seatOf(int p) const { return infoSeat[p]; }
    int positionOf(int p) const { return infoPosition[p]; }

    int classPlayer(int c) const { return infoClassPlayer[c]; }
    int newClassPlayer(int c) const { return infoNewClassPlayer[c]; }
    int seatPlayer(int s) const { return infoSeatPlayer[s]; }

    int turn() const { return common.turn; }
    int owner() const { return common.owner; }
    int firstTurn() const { return common.firstTurn; }

    void setTurn(int p) { common.turn = p; }
    void setOwner(int p) { common.owner = p; }
    void setFirstTurn(int p) { common.firstTurn = p; }

    Cards getCards(int p) const { return hand[p].cards; }
    Cards getOpsCards(int p) const { return opsHand[p].cards; }
    unsigned numCardsOf(int p) const { return hand[p].qty; }
    Cards getRemCards() const { return remCards; }
    Cards getNumRemCards() const { return remQty; }
    const Hand& getHand(int p) const { return hand[p]; }
    const Hand& getOpsHand(int p) const { return opsHand[p]; }

    Cards getDealtCards(int p) const { return dealtCards[p]; }
    Cards getUsedCards(int p) const { return usedCards[p]; }
    Cards getSentCards(int p) const { return sentCards[p]; }
    Cards getRecvCards(int p) const { return recvCards[p]; }

    int flushLeadPlayer() const;
    int nextSeatPlayer(int p) const {
        return seatPlayer(getNextSeat<N_PLAYERS>(seatOf(p)));
    }
    void rotateTurnPlayer(int turn) {
        do {
            turn = nextSeatPlayer(turn);
        } while (!isAwake(turn));
        setTurn(turn);
    }
    void flushTurnPlayer() {
        int turn = owner();
        if (isAlive(turn)) setTurn(turn);
        else rotateTurnPlayer(turn);
    }
    void flushState() { // 場を流す ただし b はすでに流れているとき
        ps.flush();
        flushTurnPlayer();
    }
    void flush() { // 場を流す
        board.flush();
        flushState();
    }

    void procHand(int tp, Move m);

    // 局面更新
    template <bool FAST> int procImpl(const MoveInfo m);
    int procFast(const MoveInfo m);
    int procFast(const Move m) { return procFast(MoveInfo(m)); }
    int proceed(const Move m);

    void makeChange(int from, int to, int dq, Cards dc,
                    bool sendOnly = false, bool recvOnly = false);

    void setHand(int p, Cards c) { hand[p].setAll(c); }
    void setOpsHand(int p, Cards c) { opsHand[p].setAll(c); }
    void setBothHand(int p, Cards c) {
        hand[p].setAll(c);
        opsHand[p].setAll(remCards - c, remQty - hand[p].qty, subCardKey(remKey, hand[p].key));
    }
    void setBothHandQty(int p, int qty) {
        hand[p].qty = qty;
        opsHand[p].qty = remQty - qty;
    }
    void setRemHand(Cards c) {
        remCards = c;
        remQty = c.count();
        remKey = subCardKey(HASH_CARDS_ALL, CardsToHashKey(CARDS_ALL - c));
    }

    void initGame();
    void prepareAfterChange();
    void prepareForPlay();

    bool exam() const;

    std::string toString() const;
    std::string toDebugString() const;

    // recordが定義された時用
    void setBeforeGame(const GameRecord& game, int playerNum);
    int passPresent(const GameRecord& game, int playerNum, bool sendPresent = false);
    void passChange(const GameRecord& game, int playerNum);
    void setAfterChange(const GameRecord& game, const std::array<Cards, N_PLAYERS>& cards);
    void fromRecord(const GameRecord& game, int playerNum, int tcnt = 256);
};

// シミュレーション時の状態コピー
extern void copyField(const Field& arg, Field *const dst);

/**************************仮想世界**************************/

struct World {
    // 仮想世界
    int builtTurn; // この世界がセットされたターン
    Cards cards[N_PLAYERS];
    uint64_t cardKey[N_PLAYERS];
    uint64_t key;

    void set(int turnCount, const Cards *c);
};

// 状態表現に世界情報を設定
extern void setWorld(const World& world, Field *const dst);