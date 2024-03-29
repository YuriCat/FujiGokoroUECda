#include "../core/action.hpp"
#include "../core/dominance.hpp"
#include "policy.hpp"

using namespace std;

#define LINEOUT(feature, str) { out << str << endl; int base = FEA_IDX(feature);\
for (int i = 0; i < FEA_NUM(feature); i++) { os(base + i); } out << endl; }

#define LINEOUTX(feature, str, x) { out << str << endl; int base = FEA_IDX(feature); int num = FEA_NUM(feature);\
for (int i = 0;;) { os(base + i); i++; if (i >= num) break; if (i % (x) == 0) { out << endl; }} out << endl; }

namespace PlayPolicySpace {
    int commentToPolicyParam(ostream& out, const policy_value_t param[FEA_NUM_ALL]) {
        auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };

        out << "****** MOVE POLICY ******" << endl;

        out << "HAND_SNOWL" << endl;
        int base = FEA_IDX(FEA_HAND_SNOWL);
        string situationString[3] = { "NF", "RF", "UR" };
        string orderString[2] = { "NML", "REV" };
        for (int s = 0; s < 3; s++) {
            for (int o = 0; o < 2; o++) {
                out << situationString[s] << " " << orderString[o] << endl;
                out << "  SEQ" << endl;
                for (int i = 0; i < 11; i++) os(base + i);
                out << endl;
                for (int i = 0; i < 10; i++) os(base + 11 + i);
                out << endl;
                for (int i = 0; i < 9; i++) os(base + 21 + i);
                out << endl;
                out << "  GROUP" << endl;
                for (int i = 0; i < 13; i++) {
                    for (int j = 0; j < 4; j++) os(base + 30 + 4 * i + j);
                    out << endl;
                }
                out << "  JOKER" << endl;
                os(base + 82); out << endl;
                base += 83;
            }
        }

        LINEOUT(FEA_HAND_S3, "JK_S3");
        LINEOUT(FEA_HAND_PQR_RANK, "AVG_PQR");
        LINEOUT(FEA_HAND_NF_PARTY, "NF_PARTY");
        LINEOUT(FEA_HAND_P8_JOKER, "P8_JOKER");
        LINEOUT(FEA_MOVE_QTY, "QTY");
        LINEOUT(FEA_SUITLOCK_EFFECT, "SUITLOCK_EFFECT");
        LINEOUT(FEA_SAME_QR, "SAME_QR");
        LINEOUT(FEA_REV_CLASS, "REV_CLASS");
        LINEOUT(FEA_MOVE_S3, "S3 ON JOKER");
        LINEOUT(FEA_MOVE_JOKER_AGAINST_S3, "JOKER AGAINST S3");
        LINEOUT(FEA_PASS_PHASE, "PASS_PHASE");
        LINEOUT(FEA_PASS_DOM, "PASS_DOM");
        LINEOUT(FEA_PASS_OWNER_DISTANCE, "PASS_OWNER_DISTANCE");
        LINEOUTX(FEA_PASS_NAWAKE_OWNER, "PASS_DOM_NAWAKE_OWNER", 8);
        LINEOUT(FEA_MOVE_SEQ, "SEQ");
        LINEOUT(FEA_MOVE_RF_GROUP_BREAK, "RF_GROUP_BREAK");
        LINEOUT(FEA_MOVE_NFDOM_PASSDOM , "NFDOM_PASSDOM");
        LINEOUT(FEA_MOVE_NF_EIGHT_MANY_WEAKERS, "NF_EIGHT_MANY_WEAKERS_NOREV");
        LINEOUT(FEA_MOVE_EIGHT_QTY, "EIGHT_QTY");
        LINEOUT(FEA_MOVE_MIN_RANK, "MIN_RANK");
        //LINEOUT(,);
        return 0;
    }
}
namespace ChangePolicySpace {
    int commentToPolicyParam(ostream& out, const policy_value_t param[FEA_NUM_ALL]) {
        auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };

        out << "****** CHANGE POLICY ******" << endl;
        {
            out << "1PQR or SEQ" << endl;
            int base = FEA_IDX(FEA_CHANGE_HAND_1PQR_SEQ);
            out << "  SEQ" << endl;
            for (int i = 0; i < 11; i++) { os(base + i); } out << endl;
            for (int i = 0; i < 10; i++) { os(base + 11 + i); } out << endl;
            for (int i = 0; i < 9; i++) { os(base + 21 + i); } out << endl;
            out << "  GROUP" << endl;
            for (int i = 0; i < 13; i++) {
                for (int j = 0; j < 4; j++) { os(base + 30 + 4 * i + j); } out << endl;
            }
        }

        LINEOUT(FEA_CHANGE_HAND_D3, "D3 BONUS");
        LINEOUT(FEA_CHANGE_HAND_JOKER_S3, "S3-JOKER LINK BONUS");
        LINEOUT(FEA_CHANGE_HAND_MAX1_RANK, "MAX1 RANK");
        LINEOUT(FEA_CHANGE_HAND_MAX2_RANK, "MAX2 RANK");
        LINEOUT(FEA_CHANGE_HAND_MIN1_RANK, "MIN1 RANK");
        LINEOUT(FEA_CHANGE_HAND_MIN2_RANK, "MIN2 RANK");
        LINEOUT(FEA_CHANGE_DOUBLE, "DOUBLE PRESENT");
        LINEOUT(FEA_CHANGE_PART_SEQ, "PART SEQ PRESENT");
        //LINEOUT(,);
        return 0;
    }
}

#undef LINEOUTX
#undef LINEOUT

double pqrRankScore(Cards pqr, int jk, int ord) {
    // 階級平均点を計算
    int r = 0;
    int cnt = countCards(pqr);
    for (IntCard ic : pqr) r += IntCardToRank(ic);
    if (ord != 0) r = (RANK_3 + RANK_2) * cnt - r;
    if (jk) {
        r += RANK_2 + 1;
        cnt += 1;
    }
    return r / (double)cnt;
}

#define Foo(f) { s += pol.param(f); FASSERT(s,); if (M) { pol.feedFeatureScore(i, (f), 1.0); }}
#define FooX(f, x) { s += pol.param(f) * (x); FASSERT(x,); FASSERT(s,); if (M) { pol.feedFeatureScore(i, (f), (x)); }}

// M = 0 通常系計算, 1 学習のため特徴ベクトル記録
template <int M, class policy_t>
int playPolicyScore(double *const dst, Move *const mbuf, const int numMoves,
                    const Field& field, policy_t& pol) { // learnerとして呼ばれうるため const なし

    using namespace PlayPolicySpace;

    // 恒常パラメータ
    const Board b = field.board;
    const int tp = field.turn();
    const int turnSeat = field.seatOf(tp);
    const int owner = field.owner();
    const int ownerSeat = field.seatOf(owner);
    const Hand& myHand = field.getHand(tp);
    const Hand& opsHand = field.getOpsHand(tp);
    const Cards myCards = myHand.cards;
    const int numMyCards = myHand.qty;
    const Cards curPqr = myHand.pqr;
    const FieldAddInfo& fieldInfo = field.fieldInfo;
    const int numPartition = divisionCount(mbuf + numMoves, myCards);

    // 元々の手札の最低、最高ランク
    const int myLR = IntCardToRank(pickIntCardLow(myCards));
    const int myHR = IntCardToRank(pickIntCardHigh(myCards));

    const int order = b.order();
    const double rankScore = pqrRankScore(curPqr, myCards.joker(), order);

    // 場役主から自分が何人目か数える
    int distanceToOwner = 0;
    if (owner != tp) {
        distanceToOwner = 1;
        for (int s = getPreviousSeat<N_PLAYERS>(turnSeat); s != ownerSeat; s = getPreviousSeat<N_PLAYERS>(s)) {
            if (field.isAlive(field.seatPlayer(s))) distanceToOwner++;
        }
        if (field.isAlive(owner)) distanceToOwner++;
    }

    // 自分以外のプレーヤーの手札の和
    const Cards opsCards = opsHand.cards;
    const Cards opsPlainCards = maskJOKER(opsCards);

    pol.initCalculatingScore(numMoves);

    for (int i = 0; i < numMoves; i++) {
        pol.initCalculatingCandidateScore();

        const MoveInfo m = mbuf[i];
        typename policy_t::real_t s = 0;

        if (m.isMate() || !anyCards(myCards - m.cards())) {
            s -= 1000; // Mateのときは低い点にする
        } else {
            // 着手パラメータ
            const int aftOrd = b.nextOrder(m);
            const int q4 = min(m.qty(), 4);

            const Cards afterCards = myCards - m.cards();
            const int numAfterCards = numMyCards - m.qty();
            const Cards afterPqr = CardsToPQR(afterCards);
            const Cards myAfterSeqCards = polymRanks<3>(afterCards);

            const int afterOrder = aftOrd;

            // snowl score
            {
                int base = FEA_IDX(FEA_HAND_SNOWL);

                if (!b.isNull()) {
                    if (fieldInfo.isUnrivaled()) base += 166 * 2;
                    else base += 166;
                }
                if (afterOrder != 0) base += 83;
                if (containsJOKER(afterCards)) Foo(base + 82)

                Cards tmp = maskJOKER(afterCards);

                Cards seq3 = polymRanks<3>(tmp);
                if (seq3) {
                    Cards seq4 = polymRanks<2>(seq3);
                    if (seq4) {
                        Cards seq5 = polymRanks<2>(seq4);
                        if (seq5) {
                            // 6枚以上の階段も5枚階段と同じパラメータで扱う(ダブルカウントしないように注意)
                            seq4.mask(extractRanks<2>(seq5));
                            seq3.mask(extractRanks<3>(seq5));
                            tmp.mask(extractRanks<5>(seq5));
                            for (IntCard ic : seq5) {
                                Foo(base + IntCardToRank(ic) - RANK_3);
                                seq5.mask(extractRanks<5>(IntCardToCards(ic)));
                                if (!seq5) break;
                            }
                        }
                        if (seq4) {
                            tmp.mask(extractRanks<4>(seq4));
                            seq3.mask(extractRanks<2>(seq4));
                            for (IntCard ic : seq4) {
                                Foo(base + 9 + IntCardToRank(ic) - RANK_3);
                                if (!seq4) break;
                            }
                        }
                    }
                    if (seq3) {
                        tmp.mask(extractRanks<3>(seq3));
                        for (IntCard ic : seq3) {
                            Foo(base + 19 + IntCardToRank(ic) - RANK_3);
                            if (!seq3) break;
                        }
                    }
                }
                if (tmp) {
                    base += 30 - 4;
                    tmp = CardsToPQR(tmp); // 枚数位置型に変換
                    for (IntCard ic : tmp) {
                        Foo(base + ic); // 枚数位置型なのでそのままインデックスになる
                        if (!tmp) break;
                    }
                }
            }

            // joker, s3 bonus
            {
                constexpr int base = FEA_IDX(FEA_HAND_S3);
                if (containsS3(afterCards) && containsJOKER(afterCards)) Foo(base + 0)
                else if (containsS3(afterCards) && containsJOKER(opsCards)) Foo(base + 1)
                else if (containsS3(opsCards) && containsJOKER(afterCards)) Foo(base + 2)
            }

            // avg pqr rank
            {
                constexpr int base = FEA_IDX(FEA_HAND_PQR_RANK);
                double rs = pqrRankScore(afterPqr, containsJOKER(afterCards) ? 1 : 0, aftOrd);
                FooX(base, numMyCards * log(max(rs / rankScore, 0.000001)))
            }

            // nf min party
            {
                constexpr int base = FEA_IDX(FEA_HAND_NF_PARTY);
                if (b.isNull()) FooX(base, divisionCount(mbuf + numMoves, afterCards) - numPartition)
                else FooX(base + 1, divisionCount(mbuf + numMoves, afterCards) - numPartition)
            }

            // after hand joker - p8
            if (polymJump(maskJOKER(afterCards)) && containsJOKER(afterCards)) {
                const int base = FEA_IDX(FEA_HAND_P8_JOKER);
                Foo(base)
            }

            // qty
            {
                constexpr int base = FEA_IDX(FEA_MOVE_QTY);
                if (b.isNull()) Foo(base + q4 - 1)
            }

            //same qr
            {
                constexpr int base = FEA_IDX(FEA_SAME_QR);
                if (b.isNull() && !m.isSeq()) {
                    int sameQ = countCards(curPqr & (PQR_1 << (q4 - 1)));
                    if (!m.containsJOKER()) FooX(base, sameQ)
                    else {
                        FooX(base + 1, sameQ);
                        // シングルジョーカーで無い場合
                        if (q4 > 0) FooX(base + 2, countCards(curPqr & (PQR_1 << (q4 - 2))))
                    }
                }
            }

            // suit lock
            {
                constexpr int base = FEA_IDX(FEA_SUITLOCK_EFFECT);
                if (!m.isPASS() && !b.isNull()) {
                    if (!fieldInfo.isLastAwake() && b.locksSuits(m)) { // LAでなく、スートロックがかかる
                        int key;
                        if (m.qty() > 1) key = base + 2; // 2枚以上のロックは強力
                        else {
                            // 最強のを自分が持っているか??
                            Cards lockedSuitCards = SuitsToCards(b.suits());
                            Cards mine = maskJOKER(afterCards) & lockedSuitCards;
                            // ジョーカーをここで使っていい場合にはロックしたいので、
                            // それは考慮に入れる
                            if (!anyCards(maskJOKER(afterCards))) key = base + 0;
                            else {
                                Cards ops = maskJOKER(opsCards) & lockedSuitCards;
                                bool hasStrong = afterOrder == 0 ?
                                                 (mine > ops) :
                                                 (pickLow(mine + CARDS_JOKER, 1) < pickLow(ops + CARDS_JOKER, 1));
                                if (hasStrong) key = base + 0;
                                else {
                                    if (!containsS3(opsCards) && !any2Cards(CardsToPQR(maskJOKER(afterCards)))) {
                                        // ジョーカー出して流してあがり
                                        key = base + 0;
                                    } else key = base + 1;
                                }
                            }
                        }
                        Foo(key);
                    }
                }
            }

            // rev(only normal order)
            {
                constexpr int base = FEA_IDX(FEA_REV_CLASS);
                if (!b.isRev() && m.isRev()) {
                    int relativeClass;
                    if (field.isInitGame()) {
                        relativeClass = (field.numPlayersAlive() - 1) / 2;
                    } else {
                        relativeClass = field.classOf(tp);
                        for (int r = 0; r < (int)field.classOf(tp); r++) {
                            if (!field.isAlive(field.classPlayer(r))) relativeClass--;
                        }
                    }
                    Foo(base + relativeClass)
                }
            }

            // pass(game phase)
            {
                constexpr int base = FEA_IDX(FEA_PASS_PHASE);
                if (m.isPASS()) {
                    int key = base + (field.getNumRemCards() > 30 ? 0 : (field.getNumRemCards() > 15 ? 1 : 2));
                    Foo(key)
                }
            }

            // pass(unrivaled)
            {
                constexpr int base = FEA_IDX(FEA_PASS_DOM);
                if (m.isPASS()) {
                    if (fieldInfo.isPassDom()) {
                        Foo(base)
                        if (!fieldInfo.isUnrivaled()) Foo(base + 1)
                    }
                }
            }

            // pass(owner distance)
            {
                constexpr int base = FEA_IDX(FEA_PASS_OWNER_DISTANCE);
                if (m.isPASS() && !fieldInfo.isUnrivaled()) Foo(base + (distanceToOwner - 1));
            }

            // pass nawake and qty
            {
                constexpr int base = FEA_IDX(FEA_PASS_NAWAKE_OWNER);
                if (m.isPASS() && owner != tp) {
                    if (field.isAlive(owner)) {
                        int key = base + (field.numPlayersAwake() - 2) * 8 + min(field.numCardsOf(owner), 8U) - 1;
                        Foo(key)
                    }
                }
            }

            // s3(move)
            {
                constexpr int base = FEA_IDX(FEA_MOVE_S3);
                if (b.isSingleJOKER() && m.isS3()) {
                    int key = base + (fieldInfo.isPassDom() ? 1 : 0);
                    Foo(key)
                }
            }

            // joker against S3
            {
                constexpr int base = FEA_IDX(FEA_MOVE_JOKER_AGAINST_S3);
                if (m.isSingleJOKER()) {
                    int key;
                    if (containsS3(afterCards)) key = base; // 自分でS3を被せられる
                    else if (fieldInfo.isLastAwake() || !(opsCards & CARDS_S3)) key = base + 1; // safe
                    else key = base + 2; // dangerous
                    Foo(key)
                }
            }

            // sequence
            {
                constexpr int base = FEA_IDX(FEA_MOVE_SEQ);
                if (b.isNull()) {
                    if (m.isSeq()) {
                        Foo(base)
                        FooX(base + 2, numMyCards)
                    }
                } else {
                    if (b.isSeq()) Foo(base + 1)
                }
            }

            // rf group break
            {
                constexpr int base = FEA_IDX(FEA_MOVE_RF_GROUP_BREAK);
                if (!b.isNull() && !fieldInfo.isUnrivaled() && m.isGroup() && !m.containsJOKER()) {
                    if (myHand.qr[m.rank()] != m.qty()) { // 崩して出した
                        if (m.domInevitably()) Foo(base) // 8切り
                        else {
                            if (aftOrd == 0) {
                                if (m.rank() >= IntCardToRank(pickIntCardHigh(opsPlainCards))) {
                                    if (containsJOKER(opsCards)) Foo(base + 2)
                                    else Foo(base + 1)
                                }
                            } else {
                                if (m.rank() <= IntCardToRank(pickIntCardLow(opsPlainCards))) {
                                    if (containsJOKER(opsCards)) Foo(base + 2)
                                    else Foo(base + 1)
                                }
                            }
                        }
                    }
                }
            }

            // NF_Dominance Move On PassDom
            {
                if (fieldInfo.isPassDom()) {
                    if (m.domInevitably() || dominatesHand(m, opsHand, OrderToNullBoard(order))) {
                        int key = FEA_IDX(FEA_MOVE_NFDOM_PASSDOM);
                        Foo(key)
                    }
                }
            }

            // NF_EIGHT
            if (b.isNull()) {
                if (m.domInevitably() && !m.isSeq()) {
                    if (isNoRev(afterCards)) {
                        Cards seq = myAfterSeqCards;
                        Cards weakers = (order == 0) ?
                        RankRangeToCards(RANK_3, RANK_7) : RankRangeToCards(RANK_9, RANK_2);
                        if (any2Cards(maskCards(afterCards & weakers, seq)) && any2Cards(maskCards(curPqr & weakers, seq))) {
                            int key = FEA_IDX(FEA_MOVE_NF_EIGHT_MANY_WEAKERS);
                            Foo(key)
                        }
                    }
                }
            }

            // eight
            {
                if (m.domInevitably()) {
                    int key = FEA_IDX(FEA_MOVE_EIGHT_QTY);
                    FooX(key, numAfterCards * numAfterCards)
                    key = FEA_IDX(FEA_MOVE_EIGHT_QTY) + 1;
                    FooX(key, numAfterCards)
                }
            }

            // min rank
            {
                constexpr int base = FEA_IDX(FEA_MOVE_MIN_RANK);
                if (b.order() == 0 && m.rank() == myLR) Foo(base)
                else if (b.order() != 0 && m.rank() == myHR) Foo(base + 1)
            }

            // MCパターン
            if (!m.isPASS()) {
                if (!m.isSeq()) {
                    // グループ
                    constexpr int base = FEA_IDX(FEA_GR_CARDS);
                    using Index = TensorIndexType<2, 16, 2, 16, N_PATTERNS_SUITS_SUITS>;
                    if (m.isSingleJOKER()) {
                        for (int f = RANK_MIN; f <= RANK_MAX; f++) {
                            uint32_t qb = afterPqr[f];
                            if (qb) {
                                int key = base + Index::get(order, RANK_JOKER, 0, f, qb);
                                // suits - suits のパターン数より少ないのでOK
                                Foo(key)
                            }
                        }
                    } else {
                        for (IntCard ic : afterPqr) { // ここは速度に関係するのでスパース用ループに
                            int f = IntCardToRank(ic);
                            uint32_t as = afterCards[f];
                            int key = base + Index::get(order, m.rank(), b.locksSuits(m) ? 1 : 0,
                                                        f, SSIndex[m.suits()][as]);
                            Foo(key)
                        }
                        if (containsJOKER(afterCards)) {
                            int key = base + Index::get(order, m.rank(), b.locksSuits(m) ? 1 : 0,
                                                        RANK_JOKER, m.qty());
                            // suits - suits のパターン数より少ないのでOK
                            Foo(key)
                        }
                    }
                } else {
                    // 階段
                    constexpr int base = FEA_IDX(FEA_SEQ_CARDS);
                    using Index = TensorIndexType<2, 16, 3, 16, N_PATTERNS_SUIT_SUITS>;
                    for (int f = RANK_MIN; f <= RANK_MAX; f++) {
                        uint32_t as = afterCards[f];
                        if (as) {
                            int key = base + Index::get(order, m.rank(), min(int(m.qty()) - 3, 2),
                                                        f, sSIndex[m.suits()][as]);
                            Foo(key)
                        }
                    }
                    if (containsJOKER(afterCards)) {
                        int key = base + Index::get(order, m.rank(), min(int(m.qty()) - 3, 2),
                                                    RANK_JOKER, 0);
                        Foo(key)
                    }
                }

                // MOパターン
                if (!m.isSeq()) {
                    // グループ
                    constexpr int base = FEA_IDX(FEA_GR_MO);
                    using Index = TensorIndexType<2, 16, 2, 16, N_PATTERNS_SUITS_SUITS>;
                    if (m.isSingleJOKER()) {
                        for (int f = RANK_MIN; f <= RANK_MAX; f++) {
                            uint32_t qb = opsHand.pqr[f];
                            if (qb) {
                                int key = base + Index::get(order, RANK_JOKER, 0, f, qb);
                                // suits - suits のパターン数より少ないのでOK
                                Foo(key)
                            }
                        }
                    } else {
                        for (int f = RANK_MIN; f <= RANK_MAX; ++f) {
                            uint32_t os = opsCards[f];
                            if (os) {
                                int key = base + Index::get(order, m.rank(), b.locksSuits(m) ? 1 : 0,
                                                            f, SSIndex[m.suits()][os]);
                                Foo(key)
                            }
                        }
                        if (containsJOKER(opsCards)) {
                            int key = base + Index::get(order, m.rank(), b.locksSuits(m) ? 1 : 0,
                                                        RANK_JOKER, m.qty());
                            // suits - suits のパターン数より少ないのでOK
                            Foo(key);
                        }
                    }
                } else {
                    // 階段
                    constexpr int base = FEA_IDX(FEA_SEQ_MO);
                    using Index = TensorIndexType<2, 16, 3, 16, N_PATTERNS_SUIT_SUITS>;
                    for (int f = RANK_MIN; f <= RANK_MAX; f++) {
                        uint32_t os = opsCards[f];
                        if (os) {
                            int key = base + Index::get(order, m.rank(), min(int(m.qty()) - 3, 2),
                                                        f, sSIndex[m.suits()][os]);
                            Foo(key);
                        }
                    }
                    if (containsJOKER(opsCards)) {
                        int key = base + Index::get(order, m.rank(), min(int(m.qty()) - 3, 2),
                                                    RANK_JOKER, 0);
                        Foo(key);
                    }
                }
            }
        }
        pol.feedCandidateScore(i, exp(s));

        if (!M || dst != nullptr) dst[i] = s;
    }
    pol.finishCalculatingScore();

    return 0;
}

template <int M, class policy_t>
int changePolicyScore(double *const dst, const Cards *const change, const int numChanges,
                      const Cards myCards, const int numChangeCards,
                      policy_t& pol) { // learnerとして呼ばれうるため const なし

    using namespace ChangePolicySpace;

    const Cards pqr = CardsToPQR(myCards);

    pol.initCalculatingScore(numChanges);

    for (int i = 0; i < numChanges; i++) {
        pol.initCalculatingCandidateScore();

        typename policy_t::real_t s = 0;
        const Cards changeCards = change[i];
        const Cards changePlainCards = changeCards.plain();
        const Cards afterCards = myCards - changeCards;
        const Cards afterPlainCards = afterCards.plain();

        const Cards afterPqr = CardsToPQR(afterPlainCards);

        // 1PQR, SEQ (snowl score except joker)
        {
            int base = FEA_IDX(FEA_CHANGE_HAND_1PQR_SEQ);

            Cards tmp = afterPlainCards;
            Cards seq3 = polymRanks<3>(tmp);
            if (seq3) {
                Cards seq4 = polymRanks<2>(seq3);
                if (seq4) {
                    Cards seq5 = polymRanks<2>(seq4);
                    if (seq5) {
                        // 6枚以上の階段も5枚階段と同じパラメータで扱う(ダブルカウントしないように注意)
                        seq4.mask(extractRanks<2>(seq5));
                        seq3.mask(extractRanks<3>(seq5));
                        tmp.mask(extractRanks<5>(seq5));
                        for (IntCard ic : seq5) {
                            int f = base + IntCardToRank(ic) - RANK_3;
                            Foo(f);
                            seq5.mask(extractRanks<5>(IntCardToCards(ic)));
                        }
                    }
                    if (seq4) {
                        tmp.mask(extractRanks<4>(seq4));
                        seq3.mask(extractRanks<2>(seq4));
                        for (IntCard ic : seq4) {
                            int f = base + 9 + IntCardToRank(ic) - RANK_3;
                            Foo(f);
                        }
                    }
                }
                if (seq3) {
                    tmp.mask(extractRanks<3>(seq3));
                    for (IntCard ic : seq3) {
                        int f = base + 19 + IntCardToRank(ic) - RANK_3;
                        Foo(f);
                        if (!seq3) break;
                    }
                }
            }

            if (tmp) {
                base += 30 - 4;
                tmp = CardsToPQR(tmp); // 枚数位置型に変換
                for (IntCard ic : tmp) {
                    int f = base + ic; // 枚数位置型なのでそのままインデックスになる
                    Foo(f);
                    if (!tmp) break;
                }
            }
        }

        { // D3 BONUS
            constexpr int base = FEA_IDX(FEA_CHANGE_HAND_D3);
            if (afterCards & CARDS_D3) Foo(base);
        }

        { // JOKER_S3 BONUS
            constexpr int base = FEA_IDX(FEA_CHANGE_HAND_JOKER_S3);
            if (afterCards & CARDS_S3) {
                if (afterCards.joker()) Foo(base) // mine
                else Foo(base + 1) // ops
            } else if (afterCards.joker()) Foo(base + 2)
        }

        { // MAX, MIN RANK
            Cards tmpPqr = afterPqr;
            int hr1;
            if (containsJOKER(afterCards)) {
                hr1 = RANK_MAX + 1;
            } else {
                IntCard ic = tmpPqr.popHighest();
                hr1 = IntCardToRank(ic);
            }
            const int hr2 = IntCardToRank(tmpPqr.highest());
            const int lr1 = IntCardToRank(tmpPqr.popLowest());
            const int lr2 = IntCardToRank(tmpPqr.lowest());

            FooX(FEA_IDX(FEA_CHANGE_HAND_MAX1_RANK), hr1);
            FooX(FEA_IDX(FEA_CHANGE_HAND_MAX2_RANK), hr2);
            FooX(FEA_IDX(FEA_CHANGE_HAND_MIN1_RANK), lr1);
            FooX(FEA_IDX(FEA_CHANGE_HAND_MIN2_RANK), lr2);
        }

        { // DOUBLE PRESENT, PART SEQ PRESENT
            if (numChangeCards == 2) {
                if (!any2Cards(CardsToER(changePlainCards))) { // 同じ階級のカードをあげる
                    Foo(FEA_IDX(FEA_CHANGE_DOUBLE));
                } else if (polymRanks<2>(changePlainCards)) { // 同じスートの連続した2階級
                    Foo(FEA_IDX(FEA_CHANGE_PART_SEQ));
                } else if (polymJump(changePlainCards)) { // 同じスートの1つ飛ばし階級
                    Foo(FEA_IDX(FEA_CHANGE_PART_SEQ) + 1);
                }
            }
        }

        { // 2 CARDS
            constexpr int base = FEA_IDX(FEA_CHANGE_CC);
            using Index = TensorIndexType<16, 16, N_PATTERNS_SUITS_SUITS>;
            if (containsJOKER(changeCards)) {
                for (int r = RANK_MIN; r <= RANK_MAX; ++r) {
                    FooX(base + Index::get(RANK_MAX + 2, r, pqr[r]), -1);
                    Foo(base + Index::get(RANK_MAX + 2, r, afterPqr[r]));
                    // suits - suits のパターン数より少ないのでOK
                }
            }

            // 交換するランクのカードと他のカードとの関係
            const Cards diffRanks = CardsToER(changeCards);
            for (IntCard ic : diffRanks) {
                unsigned int rank = IntCardToRank(ic);
                // プレーンカード同士の関係
                for (int r = RANK_MIN; r <= RANK_MAX; r++) {
                    FooX(base + Index::get(rank, r, SSIndex[myCards[rank]][myCards[r]]), -1);
                    Foo(base + Index::get(rank, r, SSIndex[afterCards[rank]][afterCards[r]]));
                }
                // ジョーカーとの関係
                FooX(base + Index::get(rank, RANK_JOKER, pqr[rank]), -1);
                Foo(base + Index::get(rank, RANK_JOKER, afterPqr[rank]));
            }
        }
        pol.feedCandidateScore(i, exp(s));
        if (!M || dst != nullptr) dst[i] = s;
    }
    pol.finishCalculatingScore();
    return 0;
}

#undef FooX
#undef Foo

int playPolicyScore(double *const dst, MoveInfo *const mbuf, const int numMoves,
                    const Field& field, const PlayPolicy& pol, int mode) {
    if (mode == 0) return playPolicyScore<0>(dst, mbuf, numMoves, field, pol);
    else return playPolicyScore<1>(dst, mbuf, numMoves, field, pol);
}
int playPolicyScore(double *const dst, MoveInfo *const mbuf, const int numMoves,
                    const Field& field, PlayPolicyLearner& pol, int mode) {
    if (mode == 0) return playPolicyScore<0>(dst, mbuf, numMoves, field, pol);
    else return playPolicyScore<1>(dst, mbuf, numMoves, field, pol);
}
int changePolicyScore(double *const dst, const Cards *const change, const int numChanges,
                      const Cards myCards, const int numChangeCards,
                      const ChangePolicy& pol, int mode) {
    if (mode == 0) return changePolicyScore<0>(dst, change, numChanges, myCards, numChangeCards, pol);
    else return changePolicyScore<1>(dst, change, numChanges, myCards, numChangeCards, pol);
}
int changePolicyScore(double *const dst, const Cards *const change, const int numChanges,
                      const Cards myCards, const int numChangeCards,
                      ChangePolicyLearner& pol, int mode) {
    if (mode == 0) return changePolicyScore<0>(dst, change, numChanges, myCards, numChangeCards, pol);
    else return changePolicyScore<1>(dst, change, numChanges, myCards, numChangeCards, pol);
}