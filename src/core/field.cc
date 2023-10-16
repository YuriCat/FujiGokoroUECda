#include "../UECda.h"
#include "dominance.hpp"
#include "field.hpp"

using namespace std;

bool PlayersState::exam() const {
    // 各要素
    if (numAlive() <= 0 || N < numAlive()) {
        cerr << "PlayersState : illegal NAlive " << numAlive() << endl;
        return false;
    }
    if (numAlive() != countNAlive()) {
        cerr << "PlayersState : NAlive != count()" << endl;
        return false;
    }
    if (numAwake() <= 0 || N < numAwake()) {
        cerr << "PlayersState : illegal NAwake " << numAwake() << endl;
        return false;
    }
    if (numAwake() != countNAwake()) {
        cerr << "PlayersState : NAwake != count()" << endl;
        return false;
    }

    // awakeとaliveの関係
    if (numAlive() < numAwake()) {
        cerr << "PlayersState : NAlive < NAwake" << endl;
        return false;
    }
    if (!holdsBits((*this)[0], (*this)[2])) {
        cerr << "PlayersState : !holds( alive, awake )" << endl;
        return false;
    }
    return true;
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

int Field::flushLeadPlayer() const {
    // 全員パスの際に誰から始まるか
    if (isNull()) return turn();
    int p = owner();
    if (!isAlive(p)) { // すでにあがっている
        // p ~ tp 間の残っているプレーヤーを探す
        while (1) {
            p = nextSeatPlayer(p);
            if (isAlive(p)) break;
        }
    }
    return p;
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
            if (know(p)) {
                assert(hand[p].holds(dc));
                if (dq >= hand[p].qty) hand[p].clear();
                else hand[p].makeMoveAll(m, dc, dq, dkey);
            }
            else hand[p].qty -= dq;
        } else if (isAlive(p)) {
            if (know(p)) {
                assert(opsHand[p].holds(dc));
                opsHand[p].makeMoveAll(m, dc, dq, dkey);
            }
            else opsHand[p].qty -= dq;
        }
    }
}

void Field::makeChange(int from, int to, int dq, Cards dc,
                       bool sendOnly, bool recvOnly) {
    // sendOnly の時は受取側手札情報を更新しない
    // 不完全情報のプレーヤーのとき、交換で動いた札が不明のときは枚数更新のみ
    uint64_t dkey = CardsToHashKey(dc);
    if (!recvOnly) {
        if (dc.any() && know(from)) {
            assert(hand[from].holds(dc));
            hand[from].subtrAll(dc, dq, dkey);
            opsHand[from].addAll(dc, dq, dkey);
            assert(opsHand[from].exam());
        } else {
            hand[from].qty -= dq;
            opsHand[from].qty += dq;
        }
    }
    if (!sendOnly) {
        if (dc.any() && know(to)) {
            assert(opsHand[to].holds(dc));
            hand[to].addAll(dc, dq, dkey);
            opsHand[to].subtrAll(dc, dq, dkey);
            assert(hand[to].exam());
        } else {
            hand[to].qty += dq;
            opsHand[to].qty -= dq;
        }
    }
    if (dc.any()) {
        if (!recvOnly) sentCards[from] = dc;
        if (!sendOnly) recvCards[to] = dc;
    }
}

void Field::prepareForPlay() {
    int tp = turn();
    if (!know(tp)) return;
    fieldInfo.init();

    // 相手手札枚数情報
    unsigned minNum = 15, maxNum = 0;
    unsigned minNumAwake = 15, maxNumAwake = 0;
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p == tp) continue;
        unsigned num = numCardsOf(p);
        if (isAlive(p)) {
            minNum = min(minNum, num);
            maxNum = max(maxNum, num);
            if (isAwake(p)) {
                minNumAwake = min(minNumAwake, num);
                maxNumAwake = max(maxNumAwake, num);
            }
        }
    }
    fieldInfo.setMinNumCardsAwake(minNumAwake);
    fieldInfo.setMinNumCards(minNum);
    fieldInfo.setMaxNumCardsAwake(maxNumAwake);
    fieldInfo.setMaxNumCards(maxNum);

    // 場の特徴
    if (isNull()) {
        if (numPlayersAlive() == numPlayersAwake()) { // 空場パスがない
            fieldInfo.setFlushLead();
        }
    } else {
        if (owner() == tp) { // セルフフォロー
            fieldInfo.setSelfFollow();
        } else {
            if (numPlayersAwake() == 1) { // SF ではないが LA
                fieldInfo.setLastAwake();
            }
            if (tp == flushLeadPlayer()) { // 全員パスしたら自分から
                fieldInfo.setFlushLead();
                if (!fieldInfo.isLastAwake()
                    && dominatesHand(board, opsHand[tp])) {
                    // 場が全員を支配しているので、パスをすれば自分から
                    fieldInfo.setBDO();
                    fieldInfo.setPassDom(); // fl && bdo ならパス支配
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
    remQty = N_CARDS;
    remKey = HASH_CARDS_ALL;
}

void Field::prepareAfterChange() {
    // 初手のプレーヤーを探す
    for (int p = 0; p < N_PLAYERS; p++) {
        if (!know(p)) continue;
        if (containsD3(hand[p].cards)) {
            setTurn(p);
            setFirstTurn(p);
            setOwner(p);
            break;
        }
    }
    prepareForPlay();
    assert(exam());
}


bool Field::exam() const {
    // validator
    if (!ps.exam()) {
        cerr << "Field::exam() illegal PlayersState" << endl;
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
            // 包含関係
            if (!holdsCards(r, c)) {
                cerr << remCards << endl;
                cerr << hand[p] << endl;
                cerr << "hand[" << p << "]hol" << endl;
                return false;
            }
            sum += c;
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
    oss << "info = " << fieldInfo << endl;
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
        if (numPlayersAwake() == 1) {
            flush();
        } else {
            ps.setAsleep(tp);
            rotateTurnPlayer(tp);
        }
    } else {
        bool agari = m.qty() >= hand[tp].qty;
        // 速度重視での即上がりまたはMATE処理
        if (FAST && (agari || m.isMate())) {
            if (numPlayersAlive() == 2) { // ゲーム終了
                setNewClassOf(tp, bestClass());
                setNewClassOf(ps.searchOpsPlayer(tp), bestClass() + 1);
                return -1;
            }
            attractedPlayers.reset(tp);
            // 結果が欲しいプレーヤーがすべて上がったので終了
            if (attractedPlayers.count() == 0) {
                setNewClassOf(tp, bestClass());
                return -1;
            }
            // ゲーム継続
            if (agari) {
                setNewClassOf(tp, bestClass());
                ps.setDead(tp);
            }
        }
        procHand(tp, m);
        board.proc(m);
        setOwner(tp);
        // 通常の上がり処理
        if (!FAST && agari) {
            setNewClassOf(tp, bestClass());
            ps.setDead(tp);
            if (numPlayersAlive() == 1) {
                setNewClassOf(ps.searchL1Player(), bestClass());
                // 最後のプレーヤーはあえてsetDeadしない
                return -1;
            }
            attractedPlayers.reset(tp);
        }
        if (board.isNull()) { // 流れた
            flushState();
        } else {
            if (FAST && m.dominatesOthers()) { // 他人を支配
                if (FAST && m.dominatesMe()) { // 自分も支配したので流れる
                    flush();
                    if (!isAwake(tp)) rotateTurnPlayer(tp);
                } else { // 他人だけ支配
                    if (isAwake(tp)) { // 自分以外全員をasleepにして自分の手番
                        ps.setAllAsleepExcept(tp);
                    } else { // 流れる
                        flush();
                    }
                }
            } else { // 支配なし
                if (numPlayersAwake() > 0) {
                    rotateTurnPlayer(tp);
                } else {
                    flush();
                }
            }
        }
    }
    common.turnCount++;
    prepareForPlay();
    assert(exam());
    return turn();
}

int Field::procFast(const MoveInfo m) { return procImpl<true>(m); }
int Field::proceed(const Move m) { return procImpl<false>(MoveInfo(m)); }

void World::set(int turnCount, const Cards *c) {
    for (int p = 0; p < N_PLAYERS; p++) {
        cards[p] = c[p];
        cardKey[p] = CardsToHashKey(c[p]);
    }
    key = cross64<N_PLAYERS>(cardKey);
    builtTurn = turnCount;
}

// set estimated information
void setWorld(const World& world, Field *const dst) {
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