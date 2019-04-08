#pragma once

// 盤の情報表現

#include "daifugo.hpp"
#include "prim2.hpp"
#include "hand.hpp"
#include "hash.hpp"
#include "logic.hpp"
#include "dominance.hpp"

namespace UECda {
    
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
        
        BitArray32<4, N_PLAYERS> infoClass;
        BitArray32<4, N_PLAYERS> infoClassPlayer;
        BitArray32<4, N_PLAYERS> infoSeat;
        BitArray32<4, N_PLAYERS> infoSeatPlayer;
        BitArray32<4, N_PLAYERS> infoNewClass;
        BitArray32<4, N_PLAYERS> infoNewClassPlayer;
        
        BitArray32<4, N_PLAYERS> infoPosition;
        
        
        uint32_t remQty;
        Cards remCards;
        uint64_t remHash;
        
        // 局面ハッシュ値
        uint64_t originalKey; // 交換後の手札配置のハッシュ値
        uint64_t recordKey; // 着手の試合進行のハッシュ値(現在は使用済み手札集合のみ)
        uint64_t boardKey; // 現時点の場のハッシュ値
        uint64_t stateKey; // 現時点で誰がパスをしていて、手番が誰かを示すハッシュ値
        uint64_t numCardsKey; // 各プレーヤーの手札枚数のハッシュ値
        uint64_t aliveKey, fullAwakeKey;
        
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
                if (p != (int)myPlayerNum) {
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
        uint32_t getNAlivePlayers() const  { return ps.getNAlive(); }
        
        uint32_t searchOpsPlayer(const int p) const {
            return ps.searchOpsPlayer(p);
        }
        int getBestClass() const { return ps.getBestClass(); }
        int getWorstClass() const { return ps.getWorstClass(); }
        
        void clearSeats() {
            infoSeat.clear(); infoSeatPlayer.clear();
        }
        void clearClasses() {
            infoClass.clear(); infoClassPlayer.clear();
        }
        
        void setPlayerClass(int p, int cl) {
            infoClass.set(p, cl);
            infoClassPlayer.set(cl, p);
        }
        void setPlayerNewClass(int p, int c) {
            infoNewClass.set(p, c);
            infoNewClassPlayer.set(c, p);
        }
        void setPlayerSeat(int p, int s) {
            infoSeat.set(p, s);
            infoSeatPlayer.set(s, p);
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
        
        uint64_t getSubjectiveHashKey(int p) const {
            // プレーヤー p から見た盤面の主観的ハッシュ値を返す
            return hand[p].hash ^ boardKey ^ stateKey ^ recordKey;
        }
        
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
            flushBoardStateHash(turn());
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
        
        // ハッシュ値更新
        void procBoardStateDead(int p) {
            aliveKey ^= deadHashKeyTable[p];
            fullAwakeKey ^= awakeHashKeyTable[p];
            stateKey ^= deadHashKeyTable[p] ^ awakeHashKeyTable[p];
            ASSERT(aliveKey == StateToAliveHashKey(ps),
                   cerr << std::hex << aliveKey << " <-> " << StateToAliveHashKey(ps) << std::dec << endl;
                   cerr << toDebugString(););
            ASSERT(fullAwakeKey == StateToFullAwakeHashKey(ps),
                   cerr << std::hex << fullAwakeKey << " <-> " << StateToFullAwakeHashKey(ps) << std::dec << endl;
                   cerr << toDebugString(););
        }
        void procBoardStateHashNonPass(int p, int ntp) {
            // b, ps の更新後に呼ばれる必要がある
            boardKey = BoardToHashKey(board);
            stateKey = procStateHashKeyNonPass(stateKey, p, ntp);
            // 差分計算 <-> 一括計算 チェック
            ASSERT(boardKey == BoardToHashKey(board),
                   cerr << std::hex << boardKey << " <-> " << BoardToHashKey(board) << std::dec << endl;
                   cerr << toDebugString(););
            ASSERT(stateKey == StateToHashKey(aliveKey, ps, ntp),
                   cerr << std::hex << stateKey << " <-> " << StateToHashKey(aliveKey, ps, ntp) << std::dec << endl;
                   cerr << toDebugString(););
        }
        void procBoardStateHashPass(int p, int ntp) {
            // b, ps の更新後に呼ばれる必要がある
            stateKey = procStateHashKeyPass(stateKey, p, ntp);
            // 差分計算 <-> 一括計算 チェック
            ASSERT(stateKey == StateToHashKey(aliveKey, ps, ntp),
                   cerr << std::hex << stateKey << " <-> " << StateToHashKey(aliveKey, ps, ntp) << std::dec << endl;
                   cerr << toDebugString(););
        }
        void flushBoardStateHash(int ntp) {
            // b, ps の更新後に呼ばれる必要がある
            boardKey = NullBoardToHashKey(board);
            stateKey = NullStateToHashKey(aliveKey, fullAwakeKey, ps, ntp);
        }
        void procRecordHash(int p, uint64_t dkey) {
            recordKey = procCardsArrayHashKey<N_PLAYERS>(recordKey, p, dkey);
            // 差分計算 <-> 一括計算 チェック
            ASSERT(recordKey == CardsArrayToHashKey<N_PLAYERS>(usedCards.data()),
                   cerr << std::hex << recordKey << " <-> " << CardsArrayToHashKey<N_PLAYERS>(usedCards.data()) << std::dec << endl;
                   cerr << toDebugString(););
        }
        void procNumCardsHash(int p) {
            numCardsKey = procNumCardsHashKey<N_PLAYERS>(numCardsKey, p, hand[p].qty);
            // 差分計算 <-> 一括計算 チェック
            ASSERT(numCardsKey == NumCardsToHashKey<N_PLAYERS>([&](int pp)->int{ return this->hand[pp].qty; }),
                   cerr << std::hex << numCardsKey << " <-> " << NumCardsToHashKey<N_PLAYERS>([&](int pp)->int{
                return this->hand[pp].qty;
            }) << std::dec << endl;
                   cerr << toDebugString(););
        }
        
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
            procRecordHash(tp, dhash); // 棋譜ハッシュ値の更新
            procNumCardsHash(tp); // 手札枚数ハッシュ値の更新
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
            procRecordHash(tp, dhash); // 棋譜ハッシュ値の更新
            procNumCardsHash(tp); // 手札枚数ハッシュ値の更新
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
                if (infoClass != invert(infoClassPlayer)) {
                    cerr << "Field::exam() illegal PlayerClass <-> ClassPlayer" << endl;
                    return false;
                }
            }
            if (infoSeat != invert(infoSeatPlayer)) {
                cerr << "Field::exam() illegal PlayerSeat <-> SeatPlayer" << endl;
                return false;
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
            infoSeat.clear();
            infoSeatPlayer.clear();
            infoClass.clear();
            infoClassPlayer.clear();
            infoNewClass.clear();
            infoNewClassPlayer.clear();
            
            common.clear();
            
            usedCards.fill(CARDS_NULL);
            dealtCards.fill(CARDS_NULL);
            sentCards.fill(CARDS_NULL);
            recvCards.fill(CARDS_NULL);
            
            remCards = CARDS_ALL;
            remQty = countCards(CARDS_ALL);
            remHash = HASH_CARDS_ALL;
            
            originalKey = 0ULL;
            recordKey = 0ULL;
            boardKey = 0ULL;
            aliveKey = StateToAliveHashKey(ps);
            fullAwakeKey = StateToFullAwakeHashKey(ps);
            stateKey = 0ULL;
            numCardsKey = 0ULL;
        }
        
        void prepareAfterChange() {
            // 初手のプレーヤーを探す
            for (int p = 0; p < N_PLAYERS; p++) {
                if (containsD3(hand[p].cards)) {
                    setTurn(p);
                    setFirstTurn(p);
                    setOwner(p);
                    stateKey = StateToHashKey(aliveKey, ps, p);
                    break;
                }
            }
            // ハッシュ値を設定
            numCardsKey = NumCardsToHashKey<N_PLAYERS>([&](int p)->int{ return this->hand[p].qty; });
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
            oss << std::hex;
            oss << "board key = " << boardKey << endl;
            oss << "alive key = " << aliveKey << endl;
            oss << "full awake key = " << fullAwakeKey << endl;
            oss << "state key = " << stateKey << endl;
            oss << "record key = " << recordKey << endl;
            oss << "num cards key = " << numCardsKey << endl;
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
                procBoardStateHashPass(tp, turn());
            }
            common.turnCount++;
        } else {
            if (mv.isMate() || mv.qty() >= hand[tp].qty) { // 即上がりまたはMATE宣言のとき
                if (isOnlyValue(attractedPlayers, tp)) {
                    // 結果が欲しいプレーヤーがすべて上がったので、プレイアウト終了
                    setPlayerNewClass(tp, getBestClass());
                    return -1;
                } else if (getNAlivePlayers() == 2) {
                    // ゲームが終了
                    setPlayerNewClass(tp, getWorstClass() - 1);
                    setPlayerNewClass(ps.searchOpsPlayer(tp), getWorstClass());
                    return -1;
                } else {
                    // 通常の上がり/MATE処理
                    if (mv.qty() >= hand[tp].qty) { // 即上がり
                        setPlayerNewClass(tp, getBestClass());
                        ps.setDead(tp);
                        attractedPlayers.reset(tp);
                        procBoardStateDead(tp);
                        procAndKillHand(tp, mv.mv(), mv.cards(), mv.qty());
                    } else {
                        procHand(tp, mv.mv(), mv.cards(), mv.qty());
                    }
                }
            } else {
                procHand(tp, mv.mv(), mv.cards(), mv.qty());
            }
            
            board.proc(mv.mv());
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
                            stateKey = aliveKey ^ awakeHashKeyTable[tp] ^ tp;
                            procBoardStateHashNonPass(tp, tp);
                        } else {
                            // 流れる
                            flush();
                        }
                    }
                } else { // 支配なし
                    if (ps.anyAwake()) {
                        rotateTurnPlayer(tp);
                        procBoardStateHashNonPass(tp, turn());
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
                procBoardStateHashPass(tp, turn());
            }
            common.turnCount++;
        } else {
            if (mv.qty() >= hand[tp].qty) { // agari
                setPlayerNewClass(tp, getBestClass());
                ps.setDead(tp);
                if (ps.isSoloAlive()) {
                    setPlayerNewClass(ps.searchL1Player(), getBestClass());
                    return -1;
                }
                procBoardStateDead(tp);
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
                    procBoardStateHashNonPass(tp, turn());
                } else {
                    flush();
                }
            }
        }
        ASSERT(exam(), cerr << toDebugString() << endl;);
        return turn();
    }
    inline int Field::procSlowest(const MoveInfo mv) {
        return procSlowest(mv.mv());
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
        
        // copy hash_value
        dst->originalKey = arg.originalKey;
        dst->recordKey = arg.recordKey;
        dst->numCardsKey = arg.numCardsKey;
        dst->boardKey = arg.boardKey;
        dst->aliveKey = arg.aliveKey;
        dst->fullAwakeKey = arg.fullAwakeKey;
        dst->stateKey = arg.stateKey;
        
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
}
