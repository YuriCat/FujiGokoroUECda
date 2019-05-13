#include "../UECda.h"
#include "dominance.hpp"
#include "field.hpp"

using namespace std;

bool PlayersState::exam_alive() const {
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
bool PlayersState::exam_awake() const {
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
bool PlayersState::exam() const {
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
bool PlayersState::examNF() const {
    // awake情報とalive情報が同じはず
    if (data() >> 16 != (data() & ((1U << 16) - 1))) return false;
    return true;
}
bool PlayersState::examSemiNF() const {
    return exam();
}

ostream& operator <<(ostream& out, const PlayersState& arg) { // 出力
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

uint32_t Field::getRivalPlayersFlag(int myPlayerNum) const {
    // ライバルプレーヤー集合を得る
    uint32_t ret = 0U;
    int best = 99999;
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p != myPlayerNum) {
            int pos = positionOf(p);
            if (pos < best) {
                ret = 1U << p;
                best = pos;
            } else if (pos == best) {
                ret |= 1U << p;
            }
        }
    }
    assert(ret != 0U);
    return ret;
}
void Field::procHand(int tp, Move m) {
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
        if (p == tp) {
            if (dq >= hand[tp].qty) hand[p].setAll(CARDS_NULL, 0, 0);
            else hand[p].makeMoveAll(m, dc, dq, dkey);
        } else if (isAlive(p)) opsHand[p].makeMoveAll(m, dc, dq, dkey);
    }
}

void Field::makeChange(int from, int to, Cards dc, bool sendOnly) {
    assert(hand[from].exam()); assert(hand[to].exam());
    assert(dc.exam()); assert(hand[from].cards.holds(dc));
    int dq = dc.count();
    uint64_t dkey = CardsToHashKey(dc);
    hand[from].subtrAll(dc, dq, dkey);
    opsHand[from].addAll(dc, dq, dkey);
    if (sendOnly) {
        // 送る側オンリーの時は相手側手札を更新しない
        assert(hand[to].holds(dc));
    } else {
        hand[to].addAll(dc, dq, dkey);
        opsHand[to].subtrAll(dc, dq, dkey);
    }
    sentCards[from] = dc;
    recvCards[to] = dc;
}
void Field::makePresents() {
    // 献上を一挙に行う
    for (int cl = 0; cl < MIDDLE; cl++) {
        const int oppClass = getChangePartnerClass(cl);
        const int from = classPlayer(oppClass);
        const int to = classPlayer(cl);
        const Cards presentCards = pickHigh(getCards(from), N_CHANGE_CARDS(cl));
        makeChange(from, to, presentCards);
    }
}
void Field::removePresentedCards() {
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

void Field::prepareForPlay() {
    
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
}

void Field::initGame() {
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
    remKey = HASH_CARDS_ALL;
}

void Field::prepareAfterChange() {
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


bool Field::exam() const {
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

string Field::toString() const {
    ostringstream oss;
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
string Field::toDebugString() const {
    ostringstream oss;
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

template <bool FAST>
int Field::procImpl(const MoveInfo m) {
    // 局面更新
    assert(exam());
    const int tp = turn();
    if (m.isPASS()) {
        if (ps.isSoloAwake()) {
            flush();
        } else {
            setPlayerAsleep(tp);
            rotateTurnPlayer(tp);
        }
    } else {
        bool agari = m.qty() >= hand[tp].qty;
        if (agari || (FAST && m.isMate())) { // 即上がりまたはMATE宣言のとき
            if (FAST) {
                if (isOnlyValue(attractedPlayers, tp)) {
                    // 結果が欲しいプレーヤーがすべて上がったので終了
                    setNewClassOf(tp, getBestClass());
                    return -1;
                } else if (getNAlivePlayers() == 2) {
                    // ゲーム終了
                    setNewClassOf(tp, getBestClass());
                    setNewClassOf(ps.searchOpsPlayer(tp), getBestClass() + 1);
                    return -1;
                }
            }
            // 通常の上がり処理
            if (agari) { // 即上がり
                setNewClassOf(tp, getBestClass());
                ps.setDead(tp);
                if (!FAST && ps.isSoloAlive()) {
                    setNewClassOf(ps.searchL1Player(), getBestClass());
                    return -1;
                }
                attractedPlayers.reset(tp);
            }
        }
        procHand(tp, m);
        board.proc(m);
        setOwner(tp);
        if (board.isNull()) { // 流れた
            flushState();
        } else {
            if (FAST && m.isDO()) { // 他人を支配
                if (FAST && m.isDM()) { // 自分も支配したので流れる
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
    common.turnCount++;
    assert(exam());
    return turn();
}

int Field::proc(const MoveInfo m) { return procImpl<true>(m); }
int Field::procSlowest(const Move m) { return procImpl<false>(MoveInfo(m)); }

void copyField(const Field& arg, Field *const dst) {
    // playout result
    dst->infoReward = 0ULL;

    // playout info
    dst->attractedPlayers = arg.attractedPlayers;
    dst->mbuf = arg.mbuf;
    
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
    dst->remKey = arg.remKey;
}

// set estimated information
void setWorld(const ImaginaryWorld& world, Field *const dst) {
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