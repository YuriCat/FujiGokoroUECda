#pragma once

// 盤の情報表現

#include "daifugo.hpp"
#include "prim2.hpp"
#include "hand.hpp"
#include "logic.hpp"
#include "dominance.hpp"

/**************************完全情報空間**************************/

enum Phase {
    PHASE_IN_CHANGE,
    PHASE_IN_PLAY,
    PHASE_INIT_GAME,
    PHASE_SUBJECTIVE,
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
        ASSERT(isAwake(p), cerr << "p = " << p << "," << std::hex << data() << endl;); // 現在Awake
        base_t::data_ -= (BMASK << 24) + ((BMASK << 16) << p);
    }
    void setDead(const int p) {
        // プレーヤーがあがった
        ASSERT(isAwake(p), cerr << "p = " << p <<endl;);
        ASSERT(isAlive(p), cerr << "p = " << p << endl;); // 現在AliveかつAwakeの必要
        base_t::data_ -= (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
    }
    void setDeadOnly(const int p) {
        // プレーヤーがあがった
        ASSERT(isAlive(p), cerr << "p = " << p << endl;); // 現在Aliveの必要
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
    
    void setNAlive(const int n) {
        assign(1, n);
    }
    void setNAwake(const int n) {
        assign(3, n);
    }
    // get
    
    constexpr data_t isAlive(int p) const { return data() & ((BMASK << 0) << p); }
    constexpr data_t isAwake(int p) const { return data() & ((BMASK << 16) << p); }
    constexpr bool isExcluded(int p) const { return false; } // あがり以外の除外(都落ち)
    
    constexpr bool isAllAsleepExcept(int p) const { // p以外全員asleep
        return !(data() & ((PMASK << 16) ^ ((BMASK << 16) << p)));
    }
    
    uint32_t searchOpsPlayer(int p) const {
        // p以外でaliveなプレーヤーを1人挙げる
        // pがaliveであることは保証される
        assert(isAlive(p));
        assert(getNAlive() >= 2);
        return bsf32(data() ^ (BMASK << p));
    }
    
    constexpr data_t getNAlive() const { return (*this)[1]; }
    constexpr data_t getNAwake() const { return (*this)[3]; }
    
    uint32_t countNAlive() const { return popcnt(part(0)); }
    uint32_t countNAwake() const { return popcnt(part(2)); }
    
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
        base_t::data_  = (REALPMASK << 0) | (N << 8) | (REALPMASK << 16) | (N << 24);
    }
    
    bool exam_alive() const {
        if (getNAlive() <= 0 || N < getNAlive()) {
            cerr << "PlayersState : illegal NAlive " << getNAlive() << endl;
            return false;
        }
        if (getNAlive() != countNAlive()) {
            cerr << "PlayersState : NAlive != count()" << endl;
            return false;
        }
        return true;
    }
    bool exam_awake() const {
        if (getNAwake() <= 0 || N < getNAwake()) {
            cerr << "PlayersState : illegal NAwake " << getNAwake() << endl;
            return false;
        }
        if (getNAwake() != countNAwake()) {
            cerr << "PlayersState : NAwake != count()" << endl;
            return false;
        }
        return true;
    }
    
    //validator
    bool exam() const {
        //各要素
        if (!exam_alive()) return false;
        if (!exam_awake()) return false;
        
        //awakeとaliveの関係
        if (getNAlive() < getNAwake()) {
            cerr << "PlayersState : NAlive < NAwake" << endl;
            return false;
        }
        if (!holdsBits((*this)[0], (*this)[2])) {
            cerr << "PlayersState : !holds( alive, awake )" << endl;
            return false;
        }
        return true;
    }
    bool examNF() const {
        // awake情報とalive情報が同じはず
        if (data() >> 16 != (data() & ((1U << 16) - 1))) return false;
        return true;
    }
    bool examSemiNF() const {
        return exam();
    }
    
    constexpr PlayersState() : base_t() {}
    constexpr PlayersState(const PlayersState& arg) : base_t(arg) {}
};

static std::ostream& operator <<(std::ostream& out, const PlayersState& arg) { // 出力
    // 勝敗
    out << "al{";
    for (int i = 0; i < PlayersState::N; i++) {
        if (arg.isAlive(i)) out << i;
    }
    out << "}";
    out << " aw{";
    for (int i = 0; i < PlayersState::N; i++) {
        if (arg.isAwake(i)) out << i;
    }	
    out << "}";
    return out;
}

struct Field {
    
    int myPlayerNum; // 主観的局面表現として使用する宣言を兼ねる
    // tools for playout
    MoveInfo *mv; // buffer of move
    XorShift64 *dice;
    
    // playout result
    BitArray64<11, N_PLAYERS> infoReward; // rewards
    uint32_t domFlags;
    uint32_t NNullFields;
    std::bitset<32> flags;
    
    // information for playout
    std::bitset<32> attractedPlayers; // players we want playout-result
    int NMoves;
    int NActiveMoves;
    MoveInfo playMove; // move chosen by player int playout
    FieldAddInfo fieldInfo;
    
    CommonStatus common;
    Board board;
    PlayersState ps;
    
    std::array<int8_t, N_PLAYERS> infoClass;
    std::array<int8_t, N_PLAYERS> infoClassPlayer;
    std::array<int8_t, N_PLAYERS> infoSeat;
    std::array<int8_t, N_PLAYERS> infoSeatPlayer;
    std::array<int8_t, N_PLAYERS> infoNewClass;
    std::array<int8_t, N_PLAYERS> infoNewClassPlayer;
    std::array<int8_t, N_PLAYERS> infoPosition;
    
    uint32_t remQty;
    Cards remCards;
    uint64_t remHash;
    
    // 手札
    std::array<Hand, N_PLAYERS> hand;
    std::array<Hand, N_PLAYERS> opsHand;
    // 手札情報
    std::array<Cards, N_PLAYERS> usedCards;
    std::array<Cards, N_PLAYERS> sentCards;
    std::array<Cards, N_PLAYERS> recvCards;
    std::array<Cards, N_PLAYERS> dealtCards;
    
    bool isL2Situation() const { return getNAlivePlayers() == 2; }
    bool isEndGame() const { // 末端探索に入るべき局面かどうか。学習にも影響する
#ifdef SEARCH_LEAF_L2
        if (isL2Situation()) return true;
#endif
        return false;
    }
    uint32_t getRivalPlayersFlag(int myPlayerNum) const {
        // ライバルプレーヤー集合を得る
        uint32_t ret = 0U;
        int best = 99999;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (p != myPlayerNum) {
                int pos = positionOf(p);
                if (pos < best) {
                    ret = (1U << p);
                    best = pos;
                } else if (pos == best) {
                    ret |= (1U << p);
                }
            }
        }
        assert(ret != 0U);
        return ret;
    }
    
    void setMoveBuffer(MoveInfo *const pmv) { mv = pmv; }
    void setDice(XorShift64 *const pdice) { dice = pdice; }
    void setPlayMove(MoveInfo ami) { playMove = ami; }
    
    void addAttractedPlayer(int p) { attractedPlayers.set(p); }

    bool isNull() const { return board.isNull(); }
    int turnCount() const  { return common.turnCount; }
    
    void setInitGame() { common.phase.set(PHASE_INIT_GAME); }
    void setInChange() { common.phase.set(PHASE_IN_CHANGE); }
    void setSubjective() { common.phase.set(PHASE_SUBJECTIVE); }

    void resetInitGame() { common.phase.reset(PHASE_INIT_GAME); }
    void resetInChange() { common.phase.reset(PHASE_IN_CHANGE); }
    
    bool isInitGame() const { return common.phase.test(PHASE_INIT_GAME); }
    bool isInChange() const { return common.phase.test(PHASE_IN_CHANGE); }
    bool isSubjective() const { return common.phase.test(PHASE_SUBJECTIVE); }
    
    uint32_t isAlive(const int p) const { return ps.isAlive(p); }
    uint32_t isAwake(const int p) const { return ps.isAwake(p); }
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
    
    uint64_t getRemCardsHash() const { return remHash; }
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

    void procHand(int tp, Move mv, Cards dc, uint32_t dq) {
        ASSERT(anyCards(dc) && mv.cards() == dc && countCards(dc) == dq,
               cerr << mv << " " << dc << " " << dq << endl;
               cerr << toDebugString(););
        
        uint64_t dhash = CardsToHashKey(dc);
        
        // 全体の残り手札の更新
        usedCards[tp] |= dc;
        remCards -= dc;
        remQty -= dq;
        remHash ^= dhash;
        
        // 出したプレーヤーの手札とそれ以外のプレーヤーの相手手札を更新
        for (int p = 0; p < tp; p++) {
            if (isAlive(p)) opsHand[p].makeMoveAll(mv, dc, dq, dhash);
        }
        hand[tp].makeMoveAll(mv, dc, dq, dhash);
        for (int p = tp + 1; p < N_PLAYERS; p++) {
            if (isAlive(p)) opsHand[p].makeMoveAll(mv, dc, dq, dhash);
        }
    }
    
    void procAndKillHand(int tp, Move mv, Cards dc, uint32_t dq) {
        // あがりのときは手札を全更新しない
        uint64_t dhash = CardsToHashKey(dc);
        
        // 全体の残り手札の更新
        usedCards[tp] |= dc;
        remCards -= dc;
        remQty -= dq;
        remHash ^= dhash;
        
        hand[tp].qty = 0; // qty だけ 0 にしておくことで上がりを表現
        
        assert(!isAlive(tp)); // agari player is not alive
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAlive(p)) opsHand[p].makeMoveAll(mv, dc, dq, dhash);
        }
    }

    // 局面更新
    // Move型以外も対応必須?
    int proc(const int tp, const MoveInfo mv);
    int proc(const int tp, const Move mv);
    int procSlowest(const MoveInfo mv);
    int procSlowest(const Move mv);
    
    void makeChange(int from, int to, Cards dc, int dq) {
        ASSERT(hand[from].exam(), cerr << hand[from] << endl;);
        ASSERT(hand[to].exam(), cerr << hand[to] << endl;);
        ASSERT(examCards(dc), cerr << dc << endl;);
        ASSERT(holdsCards(hand[from].cards, dc),
               cerr << hand[from] << " -> " << dc << endl;);
        ASSERT(!anyCards(andCards(dc, hand[to].cards)),
               cerr << dc << " -> " << hand[to] << endl;)
        uint64_t dhash = CardsToHashKey(dc);
        hand[from].subtrAll(dc, dq, dhash);
        hand[to].addAll(dc, dq, dhash);
        opsHand[from].addAll(dc, dq, dhash);
        opsHand[to].subtrAll(dc, dq, dhash);
    }
    void makeChange(int from, int to, Cards dc) {
        makeChange(from, to, dc, countCards(dc));
    }
    void makePresents() {
        // 献上を一挙に行う
        for (int cl = 0; cl < MIDDLE; ++cl) {
            const int oppClass = getChangePartnerClass(cl);
            const int from = classPlayer(oppClass);
            const int to = classPlayer(cl);
            const Cards presentCards = pickHigh(getCards(from), N_CHANGE_CARDS(cl));
            
            makeChange(from, to, presentCards, N_CHANGE_CARDS(cl));
        }
    }
    void removePresentedCards() {
        // UECdaにおいてdealt後の状態で献上カードが2重になっているので
        // 献上元のカードを外しておく
        for (int cl = 0; cl < MIDDLE; cl++) {
            const int oppClass = getChangePartnerClass(cl);
            const int from = classPlayer(oppClass);
            const int to = classPlayer(cl);
            const Cards dc = andCards(getCards(from), getCards(to));
            uint64_t dhash = CardsToHashKey(dc);
            int dq = N_CHANGE_CARDS(cl);
            
            hand[from].subtrAll(dc, dq, dhash);
            opsHand[from].addAll(dc, dq, dhash);
        }
    }
    
    void setHand(int p, Cards ac) {
        hand[p].setAll(ac);
    }
    void setOpsHand(int p, Cards ac) {
        opsHand[p].setAll(ac);
    }
    void setRemHand(Cards ac) {
        remCards = ac;
        remQty = countCards(ac);
        remHash = HASH_CARDS_ALL ^ CardsToHashKey(subtrCards(CARDS_ALL, ac));
    }
    void fillRemHand(Cards ac) {
        remCards = CARDS_ALL;
        remQty = countCards(CARDS_ALL);
        remHash = HASH_CARDS_ALL;
    }
    
    bool exam() const {
        // validator
        
        if (!ps.exam()) {
            cerr << "Field::exam() illegal PlayersState" << endl;
            cerr << ps << endl; return false;
        }
        if (board.isNull() && !ps.examSemiNF()) {
            cerr << "Field::exam() illegal PlayersState on NullField" << endl;
            cerr << ps << endl; return false;
        }
        // 置換列
        if (!isInitGame()) {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (infoClassPlayer[infoClass[p]] != p) {
                    cerr << "Field::exam() illegal PlayerClass <-> ClassPlayer" << endl;
                    return false;
                }
            }
        }
        for (int p = 0; p < N_PLAYERS; p++) {
            if (infoSeatPlayer[infoSeat[p]] != p) {
                cerr << "Field::exam() illegal PlayerSeat <-> SeatPlayer" << endl;
                return false;
            }
        }
        
        // 手札
        Cards sum = CARDS_NULL;
        int NSum = 0;
        Cards r = remCards;
        int NR = remQty;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (isAlive(p)) {
                // 上がっていないのに手札が無い場合
                // ただし主観的に使う場合には仕方が無い
                if (!isSubjective()) {
                    if (!hand[p].any()) {
                        cerr << "Field::exam() alive but no card" << endl;
                        return false;
                    }
                    if (!hand[p].exam()) {
                        cerr << "Field::exam() alive but invalid hand" << endl;
                        return false;
                    }
                }

                Cards c = hand[p].cards;
                // 排他性
                if (!isExclusiveCards(sum, c)) {
                    cerr << sum << endl;
                    cerr << hand[p] << endl;
                    cerr << "hand[" << p << "]excl" << endl;
                    return false;
                }
                // 包括性
                if (!holdsCards(r, c)) {
                    cerr << remCards << endl;
                    cerr << hand[p] << endl;
                    cerr << "hand[" << p << "]hol" << endl;
                    return false;
                }
                
                sum |= c;
                NSum += hand[p].qty;
            } else {
                // 上がっているのに手札がある場合があるかどうか(qtyは0にしている)
                if (hand[p].qty > 0) {
                    cerr << "dead but qty > 0" << endl;
                }
            }
        }
        if (!isSubjective()) {
            if (sum != r) {
                for (int p = 0; p < N_PLAYERS; p++) {
                    cerr << hand[p].cards << endl;
                }
                cerr << "sum cards - rem cards" << endl;
                return false;
            }
            if (NR != NSum) {
                cerr << "nsum cards - nrem cards" << endl;
                return false;
            }
        }
        return true;
    }
    
    void initForPlayout() {
        flags.reset();
    }
    
    void prepareForPlay(bool isRoot = false) {
        
        int tp = turn();
        
        fieldInfo.init();
        fieldInfo.setMinNCardsAwake(getOpsMinNCardsAwake(tp));
        fieldInfo.setMinNCards(getOpsMinNCards(tp));
        fieldInfo.setMaxNCardsAwake(getOpsMaxNCardsAwake(tp));
        fieldInfo.setMaxNCards(getOpsMaxNCards(tp));
        
        if (isNull()) {
            if (getNAlivePlayers() == getNAwakePlayers()) { // 空場パスがない
                fieldInfo.setFlushLead();
            }
        } else {
            if (owner() == tp) { // セルフフォロー
                fieldInfo.setSelfFollow();
            } else {
                if (ps.isSoloAwake()) { // SF ではないが LA
                    fieldInfo.setLastAwake();
                }
                uint32_t fLPlayer = getFlushLeadPlayer();
                if (fLPlayer == tp) { // 全員パスしたら自分から
                    fieldInfo.setFlushLead();
                    if (fieldInfo.isLastAwake()) {
                    } else {
                        if (dominatesHand(board, opsHand[tp])) {
                            // 場が全員を支配しているので、パスをすれば自分から
                            fieldInfo.setBDO();
                            fieldInfo.setPassDom(); // fl && bdo ならパス支配
                        }
                    }
                }
            }
        }
        
        if (isRoot) {
            // オーダー固定か否か
            if (isNoBack(getCards(tp), getOpsCards(tp))
               && isNoRev(getCards(tp), getOpsCards(tp))) {
                // 革命もJバックもなし
                DERR << "ORDER SETTLED." << endl;
                fieldInfo.settleTmpOrder();
            }
        }
    }
    
    void initGame() {
        board.init();
        ps.init();
        attractedPlayers.reset();
        infoSeat.fill(-1);
        infoSeatPlayer.fill(-1);
        infoClass.fill(-1);
        infoClassPlayer.fill(-1);
        infoNewClass.fill(-1);
        infoNewClassPlayer.fill(-1);
        
        common.clear();
        
        usedCards.fill(CARDS_NULL);
        dealtCards.fill(CARDS_NULL);
        sentCards.fill(CARDS_NULL);
        recvCards.fill(CARDS_NULL);
        
        remCards = CARDS_ALL;
        remQty = countCards(CARDS_ALL);
        remHash = HASH_CARDS_ALL;
    }
    
    void prepareAfterChange() {
        // 初手のプレーヤーを探す
        for (int p = 0; p < N_PLAYERS; p++) {
            if (containsD3(hand[p].cards)) {
                setTurn(p);
                setFirstTurn(p);
                setOwner(p);
                break;
            }
        }
        ASSERT(exam(), cerr << toDebugString() << endl;);
    }
    
    std::string toString() const {
        std::ostringstream oss;
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << p << (isAwake(p) ? " " : "*") << ": ";
            if (hand[p].qty > 0) {
                oss << hand[p].cards << "(" << hand[p].qty << ")";
            } else {
                oss << "{}(0)";
            }
            oss << endl;
        }
        return oss.str();
    }
    std::string toDebugString() const {
        std::ostringstream oss;
        oss << "turn = " << turnCount() << endl;
        oss << "player = " << turn() << endl;
        oss << "class = " << infoClass << endl;
        oss << "seat = " << infoSeat << endl;
        oss << "board = " << board << endl;
        oss << "state = " << ps << endl;
        oss << "hand = " << endl;
        for (int p = 0; p < N_PLAYERS; p++) {
            oss << p << (isAwake(p) ? " " : "*") << ": " << hand[p] << endl;
        }
        return oss.str();
    }
};

int Field::proc(const int tp, const MoveInfo mv) {
    // 丁寧に局面更新
    ASSERT(exam(), cerr << toDebugString() << endl;); // should be valid before Play
    if (mv.isPASS()) {
        if (ps.isSoloAwake()) {
            flush();
        } else {
            setPlayerAsleep(tp);
            rotateTurnPlayer(tp);
        }
        common.turnCount++;
    } else {
        if (mv.isMate() || mv.qty() >= hand[tp].qty) { // 即上がりまたはMATE宣言のとき
            if (isOnlyValue(attractedPlayers, tp)) {
                // 結果が欲しいプレーヤーがすべて上がったので、プレイアウト終了
                setNewClassOf(tp, getBestClass());
                return -1;
            } else if (getNAlivePlayers() == 2) {
                // ゲームが終了
                setNewClassOf(tp, getWorstClass() - 1);
                setNewClassOf(ps.searchOpsPlayer(tp), getWorstClass());
                return -1;
            } else {
                // 通常の上がり/MATE処理
                if (mv.qty() >= hand[tp].qty) { // 即上がり
                    setNewClassOf(tp, getBestClass());
                    ps.setDead(tp);
                    attractedPlayers.reset(tp);
                    procAndKillHand(tp, mv, mv.cards(), mv.qty());
                } else {
                    procHand(tp, mv, mv.cards(), mv.qty());
                }
            }
        } else {
            procHand(tp, mv, mv.cards(), mv.qty());
        }
        
        board.proc(mv);
        setOwner(tp);
        common.turnCount++;
        if (board.isNull()) { // 流れた
            flushState();
        } else {
            if (mv.isDO()) { // 他人を支配
                if (mv.isDM()) { // 自分も支配したので流れる
                    flush();
                    if (!isAwake(tp)) rotateTurnPlayer(tp);
                } else { // 他人だけ支配
                    if (isAwake(tp)) {
                        // 自分以外全員をasleepにして自分の手番
                        setAllAsleep();
                        setPlayerAwake(tp);
                    } else {
                        // 流れる
                        flush();
                    }
                }
            } else { // 支配なし
                if (ps.anyAwake()) {
                    rotateTurnPlayer(tp);
                } else {
                    flush();
                }
            }
        }
    }
    ASSERT(exam(), cerr << toDebugString() << endl;);
    return turn();
}
inline int Field::proc(const int tp, const Move mv) {
    return proc(tp, MoveInfo(mv));
}

inline int Field::procSlowest(const Move mv) {
    const int tp = turn();
    // 丁寧に局面更新
    ASSERT(exam(), cerr << toDebugString() << endl;); // should be valid before Play
    if (mv.isPASS()) {
        if (ps.isSoloAwake()) {
            flush();
        } else {
            setPlayerAsleep(tp);
            rotateTurnPlayer(tp);
        }
        common.turnCount++;
    } else {
        if (mv.qty() >= hand[tp].qty) { // agari
            setNewClassOf(tp, getBestClass());
            ps.setDead(tp);
            if (ps.isSoloAlive()) {
                setNewClassOf(ps.searchL1Player(), getBestClass());
                return -1;
            }
            procAndKillHand(tp, mv, mv.cards(), mv.qty());
        } else {
            procHand(tp, mv, mv.cards(), mv.qty());
        }
        
        board.proc(mv);
        setOwner(tp);
        common.turnCount++;
        if (board.isNull()) { // 流れた
            flushState();
        } else {
            if (ps.anyAwake()) {
                rotateTurnPlayer(tp);
            } else {
                flush();
            }
        }
    }
    ASSERT(exam(), cerr << toDebugString() << endl;);
    return turn();
}
inline int Field::procSlowest(const MoveInfo mv) {
    return procSlowest(mv);
}

// copy Field arg to dst before playout
inline void copyField(const Field& arg, Field *const dst) {
    // playout result
    dst->infoReward = 0ULL;
    dst->domFlags = 0;
    dst->NNullFields = 0;
    
    // playout info
    dst->attractedPlayers = arg.attractedPlayers;
    dst->mv = arg.mv;
    dst->dice = arg.dice;
    
    // game info
    dst->board = arg.board;
    dst->ps = arg.ps;
    
    dst->infoSeat = arg.infoSeat;
    dst->infoSeatPlayer = arg.infoSeatPlayer;
    dst->infoNewClass = arg.infoNewClass;
    dst->infoNewClassPlayer = arg.infoNewClassPlayer;
    dst->infoClass = arg.infoClass;
    dst->infoClassPlayer = arg.infoClassPlayer;
    
    dst->infoPosition = arg.infoPosition;
    
    dst->common = arg.common;
    
    // we don't have to copy each player's hand,
    // because card-position will be set in the opening of playout.
    dst->usedCards = arg.usedCards;
    dst->sentCards = arg.sentCards;
    dst->recvCards = arg.recvCards;
    dst->dealtCards = arg.dealtCards;

    dst->remCards = arg.remCards;
    dst->remQty = arg.remQty;
    dst->remHash = arg.remHash;
}

/**************************仮想世界**************************/

struct ImaginaryWorld {
    //仮想世界
    constexpr static int ACTIVE = 0;
    constexpr static int USED = 1;
    
    std::bitset<32> flags;
    
    double weight; // この世界の存在確率比
    int builtTurn; // この世界がセットされたターン
    uint64_t hash; // 世界識別ハッシュ。着手検討中ターンにおいて世界を識別出来れば形式は問わない
    
    Cards cards[N_PLAYERS];
    uint64_t hash_cards[N_PLAYERS];
    
    Cards getCards(int p) const { return cards[p]; }
    uint64_t getCardsHash(int p) const { return hash_cards[p]; }
    
    void clear() {
        flags.reset();
        weight = 1.0;
    }
    void activate() {
        flags.set(ACTIVE);
        DERR << "ACTIVEATED!" << endl;
    }
    
    int isActive() const { return flags.test(ACTIVE); }
    
    template <class field_t>
    void set(const field_t& field, const Cards argCards[]) {
        clear();
        for (int p = 0; p < N_PLAYERS; p++) {
            cards[p] = argCards[p];
            hash_cards[p] = CardsToHashKey(argCards[p]);
        }
        builtTurn = field.turnCount();
    }
    
    void proc(const int p, const Move mv, const Cards dc) {
        // 世界死がおきずに進行した
        //uint64_t dhash = CardsToHashKey(dc);
        //uint32_t dq = mv.qty();
        
        /*wField.rHand.makeMoveAll(mv,dc,dq,dhash);
         
         wField.hand[p].makeMove(mv,dc);
         wField.hand[p].hash^=dhash;
         wField.opsHand[p].makeMove(mv,dc);
         wField.opsHand[p].hash^=dhash;*/
    }
    ImaginaryWorld() { clear(); }
    ~ImaginaryWorld() { clear(); }
};

// set estimated information
template <class world_t>
inline void setWorld(const world_t& world, Field *const dst) {
    Cards remCards = dst->getRemCards();
    uint64_t remHash = dst->getRemCardsHash();
    
    for (int p = 0; p < N_PLAYERS; p++) {
        if (dst->isAlive(p)) {
            // only alive players
            uint64_t myHash = world.getCardsHash(p);
            
            dst->hand[p].set(world.getCards(p));
            dst->hand[p].setHash(myHash);
            dst->opsHand[p].set(remCards - world.getCards(p));
            dst->opsHand[p].setHash(remHash ^ myHash);
        } else {
            // alive でないプレーヤーも手札枚数だけセットしておく
            dst->hand[p].qty = 0;
        }
    }
}
