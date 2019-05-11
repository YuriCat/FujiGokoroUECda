#pragma once

// 盤の情報表現
#include "../UECda.h"
#include "daifugo.hpp"
#include "prim2.hpp"
#include "hand.hpp"

/**************************完全情報空間**************************/

enum Phase {
    PHASE_IN_CHANGE,
    PHASE_IN_PLAY,
    PHASE_INIT_GAME,
};

// common information
struct CommonStatus {
    int turnCount;
    int turn;
    int firstTurn;
    int owner;
    std::bitset<16> phase;

    void clear() {
        turnCount = 0;
        turn = firstTurn = owner = -1;
        phase.reset();
    }
};

/**************************プレーヤー状態**************************/

struct PlayersState : public BitArray32<8, 4> {
    // Boardと合わせて、場の状態を表す
    // 0-7   Alive
    // 8-15   NAlive
    // 16-23  Awake
    // 24-31 Nawake
    using base_t = BitArray32<8, 4>;
    using data_t = typename base_t::data_type;
    constexpr static int N = N_PLAYERS;
    constexpr static uint32_t BMASK = 1U; // 基準ビット
    constexpr static uint32_t PMASK = (1 << 8) - 1; // プレーヤー全体
    constexpr static uint32_t REALPMASK = (1 << N) - 1; // 実際にいるプレーヤー全体
    constexpr static uint32_t NMASK = (1 << 8) - 1; // 数全体
    // set
    void setAsleep(const int p) {
        assert(isAwake(p)); // 現在Awake
        base_t::data_ -= (BMASK << 24) + ((BMASK << 16) << p);
    }
    void setDead(const int p) {
        // プレーヤーがあがった
        assert(isAwake(p)); assert(isAlive(p)); // 現在AliveかつAwakeの必要
        base_t::data_ -= (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
    }
    void setDeadOnly(const int p) {
        // プレーヤーがあがった
        assert(isAlive(p)); // 現在Aliveの必要
        base_t::data_ -= (BMASK << 8) + ((1U << 0) << p);
    }
    void setAwake(const int p) {
        assert(!isAwake(p));
        base_t::data_ += (BMASK << 24) + ((BMASK << 16) << p);
    }
    void setAlive(const int p) {
        // プレーヤー復活
        assert(!isAwake(p)); assert(!isAlive(p));
        base_t::data_ += (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
    }
    
    void setAllAsleep() {
        base_t::data_ &= (PMASK << 0) | (NMASK << 8);
    }
    
    void setNAlive(const int n) { assign(1, n); }
    void setNAwake(const int n) { assign(3, n); }

    constexpr data_t isAlive(int p) const { return data() & ((BMASK << 0) << p); }
    constexpr data_t isAwake(int p) const { return data() & ((BMASK << 16) << p); }
    constexpr bool isExcluded(int p) const { return false; } // あがり以外の除外(都落ち)
    
    constexpr bool isAllAsleepExcept(int p) const { // p以外全員asleep
        return !(data() & ((PMASK << 16) ^ ((BMASK << 16) << p)));
    }
    
    uint32_t searchOpsPlayer(int p) const {
        // p以外でaliveなプレーヤーを1人挙げる
        // pがaliveであることは保証される
        assert(isAlive(p)); assert(getNAlive() >= 2);
        return bsf32(data() ^ (BMASK << p));
    }
    
    constexpr data_t getNAlive() const { return (*this)[1]; }
    constexpr data_t getNAwake() const { return (*this)[3]; }
    
    unsigned countNAlive() const { return popcnt(part(0)); }
    unsigned countNAwake() const { return popcnt(part(2)); }
    
    constexpr data_t anyAlive() const { return data() & (PMASK <<  0); }
    constexpr data_t anyAwake() const { return data() & (PMASK << 16); }
    
    bool isSoloAlive() const { return part(1)  == (1U <<  8); }
    bool isSoloAwake() const { return part(3)  == (1U << 24); }
    
    constexpr data_t getBestClass() const { return N - getNAlive(); } // 最高の階級 = 全員 - 残っている人数
    constexpr data_t getWorstClass() const { return N - 1; } // 最低の階級 = 全員の最後
    
    uint32_t searchL1Player() const {
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
    
    bool exam_alive() const;
    bool exam_awake() const;
    bool exam() const;
    bool examNF() const;
    bool examSemiNF() const;
    
    constexpr PlayersState() : base_t() {}
    constexpr PlayersState(const PlayersState& arg) : base_t(arg) {}
};

extern std::ostream& operator <<(std::ostream& out, const PlayersState& arg);

struct Field {
    
    int myPlayerNum = -1; // 主観的局面表現として使用する宣言を兼ねる
    // tools for playout
    MoveInfo *mbuf = nullptr; // buffer of move
    
    // playout result
    BitArray64<11, N_PLAYERS> infoReward; // rewards
    
    // information for playout
    std::bitset<32> attractedPlayers; // players we want playout-result
    FieldAddInfo fieldInfo;
    
    CommonStatus common;
    Board board;
    PlayersState ps;
    
    std::array<int8_t, N_PLAYERS> infoClass, infoClassPlayer;
    std::array<int8_t, N_PLAYERS> infoSeat, infoSeatPlayer;
    std::array<int8_t, N_PLAYERS> infoNewClass, infoNewClassPlayer;
    std::array<int8_t, N_PLAYERS> infoPosition;
    
    uint32_t remQty;
    Cards remCards;
    uint64_t remKey;
    
    // 手札
    std::array<Hand, N_PLAYERS> hand, opsHand;
    // 手札情報
    std::array<Cards, N_PLAYERS> usedCards;
    std::array<Cards, N_PLAYERS> sentCards, recvCards;
    std::array<Cards, N_PLAYERS> dealtCards;
    
    bool isL2Situation() const { return getNAlivePlayers() == 2; }
    bool isEndGame() const { // 末端探索に入るべき局面かどうか。学習にも影響する
        if (isL2Situation()) return true;
        return false;
    }
    uint32_t getRivalPlayersFlag(int myPlayerNum) const;
    
    void setMoveBuffer(MoveInfo *pm) { mbuf = pm; }
    void addAttractedPlayer(int p) { attractedPlayers.set(p); }

    bool isNull() const { return board.isNull(); }
    int turnCount() const  { return common.turnCount; }
    
    void setInitGame() { common.phase.set(PHASE_INIT_GAME); }
    void setInChange() { common.phase.set(PHASE_IN_CHANGE); }

    void resetInitGame() { common.phase.reset(PHASE_INIT_GAME); }
    void resetInChange() { common.phase.reset(PHASE_IN_CHANGE); }
    
    bool isInitGame() const { return common.phase.test(PHASE_INIT_GAME); }
    bool isInChange() const { return common.phase.test(PHASE_IN_CHANGE); }
    bool isSubjective() const { return myPlayerNum >= 0; }
    
    bool isAlive(const int p) const { return ps.isAlive(p); }
    bool isAwake(const int p) const { return ps.isAwake(p); }
    uint32_t getNAwakePlayers() const { return ps.getNAwake(); }
    uint32_t getNAlivePlayers() const { return ps.getNAlive(); }
    
    uint32_t searchOpsPlayer(const int p) const {
        return ps.searchOpsPlayer(p);
    }
    int getBestClass() const { return ps.getBestClass(); }
    int getWorstClass() const { return ps.getWorstClass(); }
    
    void clearSeats() {
        infoSeat.fill(-1); infoSeatPlayer.fill(-1);
    }
    void clearClasses() {
        infoClass.fill(-1); infoClassPlayer.fill(-1);
    }
    
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
    uint32_t getNCards(int p) const { return hand[p].qty; }
    Cards getRemCards() const { return remCards; }
    Cards getNRemCards() const { return remQty; }
    const Hand& getHand(int p) const { return hand[p]; }
    const Hand& getOpsHand(int p) const { return opsHand[p]; }

    Cards getDealtCards(int p) const { return dealtCards[p]; }
    Cards getUsedCards(int p) const { return usedCards[p]; }
    Cards getSentCards(int p) const { return sentCards[p]; }
    Cards getRecvCards(int p) const { return recvCards[p]; }

    uint32_t getFlushLeadPlayer() const {
        // 全員パスの際に誰から始まるか
        if (isNull()) return turn();
        uint32_t own = owner();
        if (!isAlive(own)) { // すでにあがっている
            // own~tp間のaliveなプレーヤーを探す
            while (1) {
                own = getNextSeatPlayer(own);
                if (isAlive(own)) break;
            }
        }
        return own;
    }
    uint32_t getNextSeatPlayer(const int p) const {
        return seatPlayer(getNextSeat<N_PLAYERS>(seatOf(p)));
    }
    void rotateTurnPlayer(uint32_t tp) {
        do {
            tp = getNextSeatPlayer(tp);
        } while (!isAwake(tp));
        setTurn(tp);
    }
    
    void flushTurnPlayer() {
        uint32_t tp = owner();
        if (isAlive(tp)) setTurn(tp);
        else rotateTurnPlayer(tp);
    }
    
    void setPlayerAsleep(const int p) { ps.setAsleep(p); }
    void setAllAsleep() { ps.setAllAsleep(); }
    void setPlayerDead(const int p) { ps.setDead(p); }
    void setPlayerAwake(const int p) { ps.setAwake(p); }
    void setPlayerAlive(const int p) { ps.setAlive(p); }
    
    void flushState() { // 場を流す ただし b はすでに流れているとき
        ps.flush();
        flushTurnPlayer();
    }
    void flush() { // 場を流す
        board.flush();
        flushState();
    }
    
    uint32_t searchOpsMinNCards(int pn) const { // 自分以外の最小枚数
        uint32_t nc = N_CARDS;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAlive(p) && p != pn) nc = min(nc, getNCards(p));
        }
        return nc;
    }
    uint32_t searchOpsMinNCardsAwake(int pn) const { // 自分以外のAwakeなプレーヤーの最小枚数
        uint32_t nc = N_CARDS;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAwake(p) && p != pn) nc = min(nc, getNCards(p));
        }
        return nc;
    }
    uint32_t searchOpsMaxNCards(int pn) const { // 自分以外の最大枚数
        uint32_t nc = 0;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAlive(p) && p != pn) nc = max(nc, getNCards(p));
        }
        return nc;
    }
    uint32_t searchOpsMaxNCardsAwake(int pn) const { // 自分以外のAwakeなプレーヤーの最小枚数
        uint32_t nc = 0;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAwake(p) && p != pn) nc = max(nc, getNCards(p));
        }
        return nc;
    }
    
    uint32_t getOpsMinNCards(int pn) const { return searchOpsMinNCards(pn); }
    uint32_t getOpsMinNCardsAwake(int pn) const { return searchOpsMinNCardsAwake(pn); }
    uint32_t getOpsMaxNCards(int pn) const { return searchOpsMaxNCards(pn); }
    uint32_t getOpsMaxNCardsAwake(int pn) const { return searchOpsMaxNCardsAwake(pn); }

    void procHand(int tp, Move m);

    // 局面更新
    template <bool FAST> int procImpl(const MoveInfo m);
    int proc(const MoveInfo m);
    int proc(const Move m) { return proc(MoveInfo(m)); }
    int procSlowest(const Move m);
    
    void makeChange(int from, int to, Cards dc, bool sendOnly = false);
    void makePresents();
    void removePresentedCards();
    
    void setHand(int p, Cards c) {
        hand[p].setAll(c);
    }
    void setOpsHand(int p, Cards c) {
        opsHand[p].setAll(c);
    }
    void setRemHand(Cards c) {
        remCards = c;
        remQty = c.count();
        remKey = subCardKey(HASH_CARDS_ALL, CardsToHashKey(CARDS_ALL - c));
    }

    void prepareForPlay();
    void initGame();
    void prepareAfterChange();
    
    bool exam() const;

    std::string toString() const;
    std::string toDebugString() const;
};

// copy Field arg to dst before playout
extern void copyField(const Field& arg, Field *const dst);

/**************************仮想世界**************************/

struct ImaginaryWorld {
    // 仮想世界
    double weight; // この世界の存在確率比
    int builtTurn; // この世界がセットされたターン
    uint64_t key; // 世界識別ハッシュ。着手検討中ターンにおいて世界を識別出来れば形式は問わない
    
    Cards cards[N_PLAYERS];
    uint64_t cardKey[N_PLAYERS];

    void clear() {
        weight = 1.0;
    }
    void set(int turnCount, const Cards c[]) {
        clear();
        for (int p = 0; p < N_PLAYERS; p++) {
            cards[p] = c[p];
            cardKey[p] = CardsToHashKey(c[p]);
        }
        builtTurn = turnCount;
    }
};

// set estimated information
extern void setWorld(const ImaginaryWorld& world, Field *const dst);