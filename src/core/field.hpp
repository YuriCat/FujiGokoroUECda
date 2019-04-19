#pragma once

// 盤の情報表現

#include "daifugo.hpp"
#include "prim2.hpp"
#include "hand.hpp"
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
    int firstTurn;
    std::bitset<16> phase;

    void clear() {
        turnCount = 0;
        firstTurn = -1;
        phase.reset();
    }
};

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
    BoardState board;
    
    std::array<int, N_PLAYERS> infoClass;
    std::array<int, N_PLAYERS> infoClassPlayer;
    std::array<int, N_PLAYERS> infoSeat;
    std::array<int, N_PLAYERS> infoSeatPlayer;
    std::array<int, N_PLAYERS> infoNewClass;
    std::array<int, N_PLAYERS> infoNewClassPlayer;
    std::array<int, N_PLAYERS> infoPosition;
    
    uint32_t remQty;
    Cards remCards;
    uint64_t remKey;
    
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
    
    bool isAlive(int p) const { return board.alive(seatOf(p)); }
    bool isAwake(int p) const { return board.awake(seatOf(p)); }
    int getNAwakePlayers() const { return board.numAwake(); }
    int getNAlivePlayers() const { return board.numAlive(); }

    int bestClass() const { return board.bestClass(); }
    int worstClass() const { return board.worstClass(); }
    
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
    
    
    int turn() const { return seatPlayer(turnSeat()); }
    int turnSeat() const { return board.turnSeat; }
    int owner() const { return seatPlayer(ownerSeat()); }
    int ownerSeat() const { return board.ownerSeat; }
    int firstTurn() const { return common.firstTurn; }
    
    void setTurn(int p) { board.turnSeat = seatOf(p); }
    void setOwner(int p) { board.ownerSeat = seatOf(p); }
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
    
    /*void setPlayerAsleep(const int p) { ps.setAsleep(p); }
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
    }*/
    
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

    void playHand(int tp, Move m) {
        if (m.isPASS()) return;
        int dq = m.qty();
        Cards dc = m.cards();
        uint64_t dkey = CardsToHashKey(dc);
        
        // 全体の残り手札の更新
        usedCards[tp] += dc;
        remCards -= dc;
        remQty -= dq;
        remKey = subCardKey(remKey, dkey);

        // 出したプレーヤーの手札とそれ以外のプレーヤーの相手手札を更新
        for (int p = 0; p < N_PLAYERS; p++) {
            if (p == tp) hand[p].makeMoveAll(m, dc, dq, dkey);
            else if (isAlive(p)) opsHand[p].makeMoveAll(m, dc, dq, dkey);
        }
    }

    // 局面更新
    // Move型以外も対応必須?
    //int play(int tp, MoveInfo mv);
    //int play(int tp, Move mv);
    int play(Move m) {
        assert(exam());
        int ts = turnSeat();
        int tp = seatPlayer(ts);
        bool last = m.qty() >= hand[tp].qty;
        if (last) setNewClassOf(seatPlayer(ts), bestClass());
        board.play(m, last);
        playHand(tp, m);
        common.turnCount++;
        if (last && board.gameEnd()) {
            // 残りの1人の処理
            setNewClassOf(seatPlayer(turnSeat()), bestClass());
            return -1;
        }
        assert(exam());
        return turn();
    }
    
    void makeChange(int from, int to, Cards dc, int dq) {
        ASSERT(hand[from].exam(), cerr << hand[from] << endl;);
        ASSERT(hand[to].exam(), cerr << hand[to] << endl;);
        ASSERT(examCards(dc), cerr << dc << endl;);
        ASSERT(holdsCards(hand[from].cards, dc),
               cerr << hand[from] << " -> " << dc << endl;);
        ASSERT(!anyCards(andCards(dc, hand[to].cards)),
               cerr << dc << " -> " << hand[to] << endl;)
        uint64_t dkey = CardsToHashKey(dc);
        hand[from].subtrAll(dc, dq, dkey);
        hand[to].addAll(dc, dq, dkey);
        opsHand[from].addAll(dc, dq, dkey);
        opsHand[to].subtrAll(dc, dq, dkey);
    }
    void makeChange(int from, int to, Cards dc) {
        makeChange(from, to, dc, countCards(dc));
    }
    void makePresents() {
        // 献上を一挙に行う
        for (int cl = 0; cl < MIDDLE; cl++) {
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
            uint64_t dkey = CardsToHashKey(dc);
            int dq = N_CHANGE_CARDS(cl);
            
            hand[from].subtrAll(dc, dq, dkey);
            opsHand[from].addAll(dc, dq, dkey);
        }
    }
    
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
    bool exam() const {
        // validator
        if (!board.exam()) {
            cerr << "Field::exam() illegal BoardState" << endl;
            cerr << board << endl; return false;
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
                if (board.numAwake() == 1) { // SF ではないが LA
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
    }
    
    void initGame() {
        board.init(N_PLAYERS);
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
        remKey = HASH_CARDS_ALL;
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
        oss << "turnCount = " << turnCount() << endl;
        oss << "turnSeat = " << turnSeat() << endl;
        oss << "turnPlayer = " << turn() << endl;
        oss << "class = " << infoClass << endl;
        oss << "seat = " << infoSeat << endl;
        oss << "board = " << board << endl;
        oss << "state = " << PlayerState(board) << endl;
        oss << "hand = " << endl;
        for (int s = 0; s < N_PLAYERS; s++) {
            int p = seatPlayer(s);
            oss << p << (isAwake(p) ? " " : "*") << ": " << hand[p] << endl;
        }
        return oss.str();
    }
};

/*int Field::play(const int tp, const MoveInfo mv) {
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
                    playHand(tp, mv);
                } else {
                    playHand(tp, mv);
                }
            }
        } else {
            playHand(tp, mv);
        }
        
        board.play(mv);
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
inline int Field::play(const int tp, const Move mv) {
    return play(tp, MoveInfo(mv));
}*/

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
    dst->remKey = arg.remKey;
}

/**************************仮想世界**************************/

struct ImaginaryWorld {
    //仮想世界
    constexpr static int ACTIVE = 0;
    constexpr static int USED = 1;
    
    std::bitset<32> flags;
    
    double weight; // この世界の存在確率比
    int builtTurn; // この世界がセットされたターン
    uint64_t key; // 世界識別ハッシュ。着手検討中ターンにおいて世界を識別出来れば形式は問わない
    
    Cards cards[N_PLAYERS];
    uint64_t cardKey[N_PLAYERS];

    void clear() {
        flags.reset();
        weight = 1.0;
    }
    void activate() {
        flags.set(ACTIVE);
        DERR << "ACTIVEATED!" << endl;
    }
    
    int isActive() const { return flags.test(ACTIVE); }
    
    void set(const Field& field, const Cards c[]) {
        clear();
        for (int p = 0; p < N_PLAYERS; p++) {
            cards[p] = c[p];
            cardKey[p] = CardsToHashKey(c[p]);
        }
        builtTurn = field.turnCount();
    }
    
    void play(const int p, const Move mv, const Cards dc) {
        // 世界死がおきずに進行した
    }
    ImaginaryWorld() { clear(); }
    ~ImaginaryWorld() { clear(); }
};

// set estimated information
inline void setWorld(const ImaginaryWorld& world, Field *const dst) {
    Cards remCards = dst->remCards;
    uint64_t remKey = dst->remKey;
    for (int p = 0; p < N_PLAYERS; p++) {
        if (dst->isAlive(p)) {
            // only alive players
            uint64_t myKey = world.cardKey[p];
            dst->hand[p].set(world.cards[p]);
            dst->hand[p].setKey(myKey);
            dst->opsHand[p].set(remCards - world.cards[p]);
            dst->opsHand[p].setKey(subCardKey(remKey, myKey));
        } else {
            // alive でないプレーヤーも手札枚数だけセットしておく
            dst->hand[p].qty = 0;
        }
    }
}
