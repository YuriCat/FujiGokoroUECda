#include "../core/action.hpp"
#include "mate.hpp"
#include "policy.hpp"
#include "estimation.hpp"

using namespace std;

// Walker's Alias Method はこちらを参考に
// http://qiita.com/ozwk/items/6d62a0717bdc8eac8184

namespace Settings {
    const double estimationTemperatureChange = 1.0;
    const double estimationTemperaturePlay = 1.1;
    const int maxRejection = 800; // 採択棄却法時の棄却回数限度
    constexpr int BUCKET_MAX = 32;
}

float estimationTable[EST_FEATURES];

namespace Deal {
    constexpr uint64_t AfterChangeWeight[INTCARD_MAX + 1][5] =
    {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {40187, 46546, 64949, 84544, 91452},
        {65332, 65330, 65584, 64965, 66466},
        {39672, 47128, 66286, 83651, 90940},
        {50584, 55582, 65449, 74930, 81133},
        {35999, 45033, 65227, 84912, 96507},
        {36852, 45822, 65033, 84837, 95133},
        {35970, 45000, 65274, 85753, 95680},
        {35988, 44561, 65002, 85243, 96884},
        {37288, 48815, 64768, 82475, 94331},
        {38364, 48902, 65997, 81338, 93077},
        {37267, 48838, 65411, 82359, 93803},
        {37226, 48437, 64545, 82317, 95153},
        {40736, 51090, 65475, 79123, 91254},
        {41346, 52258, 65492, 78157, 90424},
        {40479, 51784, 65692, 78306, 91416},
        {40452, 51744, 65109, 78250, 92122},
        {43869, 55224, 65175, 75323, 88087},
        {43904, 55647, 65353, 74690, 88083},
        {43438, 55366, 65306, 75162, 88405},
        {43318, 55501, 65319, 74975, 88565},
        {66168, 65825, 65954, 64771, 64960},
        {65723, 64782, 65259, 66174, 65739},
        {65066, 65846, 65482, 65427, 65857},
        {65650, 64407, 65996, 65827, 65797},
        {52611, 60501, 65365, 70037, 79165},
        {52243, 61009, 65670, 69421, 79334},
        {52700, 61574, 64736, 68785, 79882},
        {52666, 61262, 65303, 70209, 78238},
        {56397, 62638, 65057, 69090, 74496},
        {56834, 63327, 65488, 68190, 73838},
        {56937, 62447, 65557, 68633, 74103},
        {57557, 62619, 65358, 68219, 73923},
        {62103, 63767, 65126, 67037, 69645},
        {62665, 64509, 65919, 66638, 67947},
        {63263, 64814, 65124, 67043, 67434},
        {65270, 64905, 65302, 66524, 65676},
        {69342, 65909, 64905, 65853, 61669},
        {71208, 66233, 65996, 64553, 59688},
        {73523, 67138, 65136, 64318, 57562},
        {75799, 68557, 65696, 63199, 54426},
        {81158, 69157, 65792, 62293, 49277},
        {84611, 70547, 65673, 61197, 45650},
        {88791, 71562, 65913, 59500, 41911},
        {90933, 74389, 66582, 57471, 38302},
        {100006, 76303, 66276, 54205, 30887},
        {104324, 79576, 65577, 52278, 25923},
        {108602, 84369, 66081, 48192, 20433},
        {113764, 88234, 66318, 43611, 15752},
        {119492, 94538, 66498, 37370, 9779},
        {124468, 100891, 66124, 30924, 5271},
        {127085, 109902, 65995, 22792, 1902},
        {128964, 120523, 65898, 12292, 1},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {129187, 132712, 65779, 1, 1},
    };
}

#define F(index) { if (v != nullptr) v->push_back(make_pair(index, 1.0f)); score += table[index]; }

float inverseEstimationScore(const Cards orgCards, const Cards usedCards, const Cards sentCards, int playerClass, vector<pair<int, float>> *const v) {
    const float *const table = estimationTable;
    float score = 0;
    int classIndex = playerClass > HEIMIN ? (playerClass - 1) : playerClass;

    if (playerClass != HEIMIN) {
        Cards lowerCards = CARDS_NULL;
        for (IntCard ic : orgCards) {
            F(64 * classIndex + ic);
            for (IntCard ic2 : lowerCards) F(64 * 4 + classIndex * 64 * 64 + ic * 64 + ic2);
            lowerCards.insert(ic);
        }
    }

    Cards unusedCards = orgCards - usedCards;
    for (IntCard ic : usedCards) {
        for (IntCard ic2 : unusedCards) F(64 * 4 + 64 * 64 * 4 + 64 * ic + ic2);
    }

    for (IntCard ic2 : sentCards) {
        for (IntCard ic : unusedCards) F(64 * 4 + 64 * 64 * 4 + 64 * 64 + ic * 64 + ic2);
    }

    return score;
}

#undef F

// 拘束条件分割
bool dist2Rest_64(int numRest,
                  uint64_t *const goal0, uint64_t *const goal1,
                  const uint64_t arg, int N0, int N1,
                  const uint64_t rest0, const uint64_t rest1, Dice& dice) {
    // 0が交換上手側、1が下手側
    uint64_t tmp0 = 0ULL, tmp1 = 0ULL;
    uint64_t all = arg | rest0 | rest1;

    // まず確定ビットを探す
    if (rest0) {
        uint64_t low = highestNBits(all, N1 + numRest);
        uint64_t set0 = rest0 & ~low;
        if (set0) {
            int NSet0 = popcnt(set0);
            tmp0 |= set0; all -= set0; N0 -= NSet0;
        }
    }
    assert((int)popcnt(all) == N0 + N1);
    dist2_64(&tmp0, &tmp1, all, N0, N1, dice);
    // 献上
    uint64_t highNRest = highestNBits(tmp1, numRest);
    tmp0 |= highNRest; tmp1 -= highNRest;

    if (!holdsBits(tmp0, rest0)
        || !holdsBits(tmp1, rest1 & ~highNRest)) return false;

    *goal0 = tmp0;
    *goal1 = tmp1;

    return true;
}

void RandomDealer::dealAllRand(Cards *const dst, Dice& dice) const {
    // 完全ランダム分配
    // 自分の分だけは実際のものにする
    BitCards tmp[N] = {0};
    auto tmpNOwn = NOwn;
    tmpNOwn[myClass] = 0;
    dist64<N>(tmp, remCards - myCards, tmpNOwn.data(), dice);
    tmp[myClass] = myCards;
    for (int p = 0; p < N; p++) {
        dst[p] = tmp[infoClass[p]];
    }
    checkDeal(dst, false);
}

void RandomDealer::dealWithSubjectiveInfo(Cards *const dst, Dice& dice) const {
    // 主観情報のうち完全な情報のみ扱い、それ以外は完全ランダムとする
    BitCards tmp[N] = {0};
    if (!initGame && myClass < MIDDLE) {
        // 献上が無矛盾になるように交換相手に配布
        int qty = N_CHANGE_CARDS(myClass);
        Cards myHigh = highestNBits<uint64_t>(myDealtCards, qty);
        int partnerClass = getChangePartnerClass(myClass);
        Cards okCards = pickLower(myHigh) & dealCards;
        tmp[partnerClass] = pickNBits64(okCards, NDeal[partnerClass], okCards.count() - NDeal[partnerClass], dice);
        assert(myHigh.lowest() > Cards(detCards[partnerClass] + tmp[partnerClass] - sentCards).highest());

        auto tmpNDeal = NDeal;
        tmpNDeal[partnerClass] = 0;
        dist64<N>(tmp, dealCards - tmp[partnerClass], tmpNDeal.data(), dice);
    } else {
        dist64<N>(tmp, dealCards, NDeal.data(), dice);
    }
    for (int cl = 0; cl < N; cl++) {
        dst[infoClassPlayer[cl]] = detCards[cl] + tmp[cl] - usedCards[cl];
    }
    checkDeal(dst);
}

void RandomDealer::dealWithBias(Cards *const dst, Dice& dice) const {
    // 逆関数法でバイアスを掛けて分配
    if (initGame) return dealWithSubjectiveInfo(dst, dice);

    array<Cards, N> tmp = detCards;
    int tmpNDeal[N];
    for (int r = 0; r < N; r++) tmpNDeal[r] = NDeal[r];
    Cards fromCards = dealCards;

    if (!initGame && myClass < MIDDLE) {
        // 献上によるバイアスを反映して配布
        int ptClass = getChangePartnerClass(myClass);
        if (NDeal[ptClass]) {
            Cards tmpDist = selectInWA(dice.random());
            Cards dealt = pickNBits64(tmpDist, NDeal[ptClass], tmpDist.count() - NDeal[ptClass], dice);
            tmp[ptClass] += dealt;
            tmpNDeal[ptClass] = 0;
            fromCards -= dealt;
        }
    }

    while (fromCards) {
        IntCard ic = fromCards.popHighest();

        uint64_t weightSum[N + 1];
        weightSum[0] = 0;
        for (int r = 0; r < N; r++) {
            weightSum[r + 1] = weightSum[r] + tmpNDeal[r] * Deal::AfterChangeWeight[ic][r];
        }
        assert(weightSum[N] > 0);
        uint64_t ran = dice() % weightSum[N];

        int r;
        for (r = 0; r < N; r++) if (weightSum[r + 1] > ran) break;
        tmp[r].insert(ic);
        tmpNDeal[r]--;
    }

    for (int cl = 0; cl < N; cl++) {
        dst[infoClassPlayer[cl]] = tmp[cl] - usedCards[cl];
    }
    checkDeal(dst);
}

void RandomDealer::dealWithNewBias(Cards *const dst, Dice& dice) const {
    const int K = 64;
    array<Cards, N> cards[K];
    double score[K] = {0};
    for (int i = 0; i < K; i++) dealWithBias(cards[i].data(), dice);
    for (int i = 0; i < K; i++) {
        float s = 0;
        for (int p = 0; p < N; p++) {
            int cl = infoClass[p];
            int realClass = initGame ? HEIMIN : cl;
            if (NDeal[cl] > 0) {
                assert(p != myNum && cards[i][p].count() > 0);
                Cards recv = realClass == getChangePartnerClass(myClass) && myClass > MIDDLE ? recvCards : Cards(CARDS_NULL);
                s += inverseEstimationScore(cards[i][p] | usedCards[cl], usedCards[cl], recv, realClass);
            }
        }
        score[i] = s;
    }
    SoftmaxSelector<double> selector(score, K, 0.1);
    int index = selector.select(dice.random());
    for (int p = 0; p < N; p++) dst[p] = cards[index][p];
}

void RandomDealer::dealWithRejection(Cards *const dst, const GameRecord& game,
                                     const SharedData& shared, ThreadTools *const ptools) {
    if (inChange) {
        // 交換時は献上まで
        // 献上によるバイアスを反映して配布
        BitCards tmp[N] = {0};
        Cards fromCards = dealCards;
        auto tmpNDeal = NDeal;
        int ptClass = getChangePartnerClass(myClass);
        if (NDeal[ptClass]) {
            Cards tmpDist = selectInWA(ptools->dice.random());
            Cards dealt = pickNBits64(tmpDist, NDeal[ptClass], tmpDist.count() - NDeal[ptClass], ptools->dice);
            tmp[ptClass] += dealt;
            tmpNDeal[ptClass] = 0;
            fromCards -= dealt;
        }
        dist64<N>(tmp, fromCards, tmpNDeal.data(), ptools->dice);
        for (int cl = MIDDLE + 1; cl < N; cl++) {
            if (cl == ptClass) continue;
            Cards presented = getPresentCards(tmp[cl] + detCards[cl], N_CHANGE_RISE_CARDS(cl));
            tmp[cl] -= presented;
            tmp[getChangePartnerClass(cl)] += presented;
        }
        for (int cl = 0; cl < N; cl++) {
            dst[infoClassPlayer[cl]] = detCards[cl] + tmp[cl];
        }
        return;
    }

    // 採択棄却法メイン
    Cards deal[Settings::BUCKET_MAX][N]; // カード配置候補
    bool rejectionDeal[Settings::BUCKET_MAX] = {false};
    int bestDeal = 0;
    for (int i = 0; i < buckets; i++) {
        if (failed) {
            dealWithNewBias(deal[i], ptools->dice);
        } else {
            bool ok = dealWithChangeRejection(deal[i], shared, ptools);
            if (ok) rejectionDeal[i] = true;
            // 失敗の回数が一定値を超えた以降は逆関数法に移行
            if (!ok && ++failures > 1) failed = true;
        }
    }
    // 役提出の尤度を計算して使用する手札配置を決定
    if (buckets > 1) {
        double lhs[Settings::BUCKET_MAX];
        for (int i = 0; i < buckets; i++) {
            lhs[i] = playLikelihood(deal[i], game, shared, ptools);
            if (!rejectionDeal[i] && !initGame && myClass > MIDDLE) {
                // 逆関数で配った場合はここで交換相手の交換尤度を評価
                int ptClass = getChangePartnerClass(myClass);
                if (NDeal[ptClass]) {
                    int partner = infoClassPlayer[ptClass];
                    double prob = oneChangeLikelihood(partner, deal[i][partner] + usedCards[ptClass] + recvCards, recvCards, shared);
                    //cerr << NDeal << Cards(deal[i][partner] + usedCards[ptClass] + recvCards) << " -> " << recvCards << " " << prob << " " << lhs[i] << endl;
                    lhs[i] += log(prob);
                }
            }
        }
        SoftmaxSelector<double> selector(lhs, buckets, 0.3);
        bestDeal = selector.select(ptools->dice.random());
    }
    for (int p = 0; p < N; p++) dst[p] = deal[bestDeal][p];
    checkDeal(dst);
}

void RandomDealer::set(const Field& field, int playerNum) {
    // 最初に繰り返し使う情報をセット
    detCards.fill(CARDS_NULL);
    usedCards.fill(CARDS_NULL);
    NOwn.fill(0);
    NOrg.fill(0);
    NDet.fill(0);
    NDeal.fill(0);
    infoClassPlayer.fill(-1);
    infoClass.fill(-1);
    failures = 0;
    failed = false;
    initGame = field.isInitGame();
    inChange = field.isInChange();
    turnCount = inChange ? -1 : field.turnCount();

    // 交換ありの場合には配列のインデックスは階級を基本とする
    // 交換がない場合にはプレーヤー番号で代用する
    for (int p = 0; p < N; p++) {
        int cl = initGame ? p : field.classOf(p);
        infoClassPlayer[cl] = p;
        infoClass[p] = cl;
    }
    myNum = playerNum;
    myCards = field.getCards(myNum);
    myClass = infoClass[myNum];
    firstTurnClass = inChange ? -1 : infoClass[field.firstTurn()];

    for (int p = 0; p < N; p++) {
        int cl = infoClass[p];
        int org = field.numCardsOf(p) + field.getUsedCards(p).count();
        int own = field.numCardsOf(p);

        if (inChange) {
            int nch = N_CHANGE_CARDS(cl);
            if (cl == getChangePartnerClass(myClass)
                || (cl < MIDDLE && cl != myClass)) {
                org -= nch; own -= nch;
            }
        }

        NOwn[cl] = NDeal[cl] = own;
        NOrg[cl] = org;
        NDet[cl] = org - own;
        usedCards[cl] = field.getUsedCards(p);
        detCards[cl] += usedCards[cl]; // 既に使用されたカードは確定
    }
    // サーバー視点の場合はdealtとrecvが分かれて入っていて、
    // プレーヤー視点ではdealtに全て入っているので足す処理にする
    myDealtCards = field.getDealtCards(myNum) + field.getRecvCards(myNum);
    remCards = field.getRemCards();

    // 交換中でなく、カード交換に関与した場合に動いたカードを設定
    if (!initGame && !inChange && myClass != MIDDLE) {
        sentCards = field.getSentCards(myNum);
        recvCards = field.getRecvCards(myNum);
    }

    // すでに分かっている情報から確実な部分を分配
    prepareSubjectiveInfo();

    // 交換相手の献上効果反映準備
    if (!field.isInitGame() && myClass < MIDDLE) setWeightInWA();

    // 採択棄却法の使用不可を設定
    if (!okForRejection()) failed = true;
}

void RandomDealer::prepareSubjectiveInfo() {
    // 自分以外の未使用カード
    dealCards = remCards - myCards;

    // 自分
    detCards[myClass] += myCards;
    NDeal[myClass] = 0;
    NDet[myClass] = NOrg[myClass];

    // 初手がすでに済んでいる場合、初手プレーヤーにD3
    if (!inChange && turnCount > 0
        && dealCards.contains(INTCARD_D3)) {
        addDetectedCards(firstTurnClass, CARDS_D3);
    }
    if (!initGame && !inChange) {
        if (myClass < MIDDLE) { // 自分が上位のとき
            // 交換であげたカードのうちまだ確定扱いでないもの
            int changePartner = getChangePartnerClass(myClass);
            Cards sent = sentCards;
            sent.mask(detCards[changePartner]);
            if (sent) addDetectedCards(changePartner, sent);
        }
    }

    // 結局配る枚数
    NdealCards = dealCards.count();
    ASSERT(NdealCards == accumulate(NDeal.begin(), NDeal.begin() + N, 0),
           cerr << NdealCards << " " << NDeal << " " << accumulate(NDeal.begin(), NDeal.begin() + N, 0) << endl;);

    buckets = 1;
    if (!inChange && turnCount > 0) {
        // 他人が使用したカード枚数から採択棄却の際の候補数を設定
        unsigned NOppUsedCards = Cards(CARDS_ALL - remCards - usedCards[myClass]).count();
        // 1 -> ... -> max -> ... -> 1 と台形に変化
        int line2 = Settings::BUCKET_MAX;
        int line1 = (line2 - 1) * NOppUsedCards / 4 + 1;
        int line3 = (line2 - 1) * NdealCards / 16 + 1;
        buckets = NOppUsedCards > 0 ? min(min(line1, line2), line3) : 1;
        // 全てのカードが明らかになっていないプレーヤーは着手を検討する必要があるのでフラグを立てる
        playFlag.reset();
        for (int p = 0; p < N; p++) {
            if (p != myNum && NDeal[infoClass[p]] > 0) playFlag.set(p);
        }
    }

    for (int p = 0; p < N; p++) {
        DERR << "cl:" << p << " p:" << (int)infoClassPlayer[p] << " Org:" << (int)NOrg[p] << " Own:" << (int)NOwn[p]
        << " Det:" << (int)NDet[p] << " Deal:" << (int)NDeal[p] << " " << detCards[p] << endl;
    }
}

void RandomDealer::checkDeal(const Cards *dst, bool sbj) const {
    for (int p = 0; p < N; p++) DERR << dst[p] << endl;
    for (int r = 0; r < N; r++) {
        ASSERT(dst[infoClassPlayer[r]].count() == NOwn[r],
               cerr << "class = " << r << " NOwn = " << (int)NOwn[r];
               cerr << " cards = " << dst[infoClassPlayer[r]] << endl;);
    }
    if (sbj) {
        // 主観で確定した情報には従うとき
        for (int p = 0; p < N; p++) {
            for (int pp = 0; pp < N; pp++) {
                if (p == pp) continue;
                assert(isExclusiveCards(dst[p], detCards[infoClass[pp]]));
            }
        }
    }
}

bool RandomDealer::okForRejection() const {
    // 採択棄却法使用可能性
    // 数値は経験的に決定している
    if (initGame) return true;
    switch (myClass) {
        case DAIFUGO:
            if (NDet[FUGO] >= 9 || NDet[HINMIN] >= 9) return false;
            break;
        case FUGO:
            if (NDet[DAIFUGO] >= 9 || NDet[DAIHINMIN] >= 9) return false;
            break;
        case HEIMIN:
            if (NDet[DAIFUGO] >= 7 || NDet[FUGO] >= 7
                || NDet[HINMIN] >= 8 || NDet[DAIHINMIN] >= 8) return false;
            break;
        case HINMIN:
            if (NDet[DAIFUGO] >= 5 || NDet[FUGO] >= 3 || NDet[DAIHINMIN] >= 5) return false;
            break;
        case DAIHINMIN:
            if (NDet[DAIFUGO] >= 4 || NDet[FUGO] >= 5 || NDet[HINMIN] >= 5) return false;
            break;
        default: exit(1); break;
    }
    return true;
}

Cards RandomDealer::change(const int p, const Cards cards, const int qty,
                           const SharedData& shared, ThreadTools *const ptools) const {
    // 採択棄却法のためのカード交換モデル
    // 交換方策に従った交換を行う
    Cards change[N_MAX_CHANGES];
    double score[N_MAX_CHANGES];
    int numChanges = genChange(change, cards, qty);
    changePolicyScore(score, change, numChanges, cards, qty, shared.baseChangePolicy);
    if (shared.playerModel.trained) {
        for (int i = 0; i < numChanges; i++) score[i] += shared.playerModel.changeBiasScore(p, cards, change[i]);
    }
    SoftmaxSelector<double> selector(score, numChanges, Settings::estimationTemperatureChange);
    int index = selector.select(ptools->dice.random());
    return change[index];
}

bool RandomDealer::dealWithChangeRejection(Cards *const dst,
                                           const SharedData& shared,
                                           ThreadTools *const ptools) const {
    // 採択棄却法のカード交換パート
    auto& dice = ptools->dice;
    if (initGame) {
        dealWithSubjectiveInfo(dst, dice);
        return true;
    }

    bool success = false;
    int trials = 0;

    BitCards R[N] = {0};

    for (int t = 0; t < Settings::maxRejection; t++) {
        // 1. 交換相手に配る
        int ptClass = getChangePartnerClass(myClass);
        if (myClass < MIDDLE) {
            // Walker's Alias methodで献上下界を決めて交換相手に分配
            R[ptClass] = detCards[ptClass];
            if (NDeal[ptClass]) {
                Cards tmpDist = selectInWA(dice.random());
                R[ptClass] += pickNBits64(tmpDist, NDeal[ptClass], tmpDist.count() - NDeal[ptClass], dice);
            }
        } else if (myClass > MIDDLE) {
            // 交換相手のカードを決め打ち
            R[ptClass] = detCards[ptClass];
            if (NDeal[ptClass]) {
                BitCards remained = CARDS_NULL;
                int numDealOthers = NdealCards - NDeal[ptClass];
                dist2_64(&remained, &R[ptClass], dealCards, numDealOthers, NDeal[ptClass], dice);
            }
        }

        // 2. 残りカードを平民と自分が関与した以外の交換系にそれぞれ分ける
        Cards rem = dealCards;
        BitCards remained[2] = {0};
        if (myClass == HEIMIN) {
            dist2_64(&remained[0], &remained[1], rem, NDeal[0] + NDeal[4], NDeal[1] + NDeal[3], dice);
        } else {
            rem -= R[ptClass] - detCards[ptClass];
            R[HEIMIN] = detCards[HEIMIN];
            if (NDeal[HEIMIN]) {
                BitCards otherPair = CARDS_NULL;
                dist2_64(&otherPair, &R[HEIMIN], rem, rem.count() - NDeal[HEIMIN], NDeal[HEIMIN], dice);
                rem = otherPair;
            }
            // 残りを自分が関わっていない交換系へ
            remained[1 - min(myClass, ptClass)] = rem;
        }

        // 関与しない交換系内での分配
        bool ok = true;
        for (int cl = FUGO; cl >= 0; cl--) {
            int rich = cl, poor = getChangePartnerClass(cl);
            if (rich == myClass || poor == myClass) continue;
            int numChange = N_CHANGE_CARDS(rich);
            if (!dist2Rest_64(numChange, &R[rich], &R[poor], remained[cl],
                              NOrg[rich], NOrg[poor], detCards[rich], detCards[poor], dice)) {
                ok = false; break;
            }
        }
        if (!ok) continue;

        // 交換処理
        if (myClass > MIDDLE) {
            int ptClass = getChangePartnerClass(myClass);
            if (NDeal[ptClass]) {
                Cards cc = change(infoClassPlayer[ptClass], R[ptClass] + recvCards, N_CHANGE_CARDS(ptClass), shared, ptools);
                if (cc != recvCards) continue;
            }
        }

        ok = true;
        for (int cl = FUGO; cl >= 0; cl--) {
            int rich = cl, poor = getChangePartnerClass(cl);
            if (rich == myClass || poor == myClass) continue;
            Cards cc = change(infoClassPlayer[rich], R[rich], N_CHANGE_CARDS(rich), shared, ptools);
            if (!Cards(R[poor] + cc).holds(detCards[poor])
                || !Cards(R[rich] - cc).holds(detCards[rich])) {
                ok = false; break;
            }
            R[rich] -= cc;
            R[poor] += cc;
        }
        if (!ok) continue;

        success = true;
        break;
    }

    if (success) {
        for (int cl = 0; cl < N; cl++) {
            dst[infoClassPlayer[cl]] = R[cl] - usedCards[cl];
        }
        dst[myNum] = myCards;
        checkDeal(dst);
    } else {
        DERR << "DEAL_REJEC FAILED..." << endl;
        // 失敗の場合は逆関数法に変更
        dealWithNewBias(dst, dice);
        return false;
    }
    return true;
}

Cards RandomDealer::selectInWA(double urand) const {
    double v = urand * candidatesInWA;
    int k = (int)v;
    double u = 1 + k - v;
    if (u < thresholdInWA[k]) return dealCardsUnderInWA[k];
    else return dealCardsOverInWA[k];
}

void RandomDealer::setWeightInWA() {
    vector<double> probs;
    const int T = NDeal[getChangePartnerClass(myClass)]; // 交換相手の配布枚数
    if (T == 0) return; // どうしようもない

    // 相手の献上後の所持カードで判明しているもの
    const Cards partnerDealtCards = maskCards(detCards[getChangePartnerClass(myClass)], myDealtCards);
    // 相手の献上後の所持カードの上界より高い札のみ献上でもらっている可能性がある
    const Cards partnerDealtMask = anyCards(partnerDealtCards) ? pickHigher(partnerDealtCards) : CARDS_ALL;

    Cards tmp = (myDealtCards - pickHigh(myDealtCards, N_CHANGE_CARDS(myClass) - 1)) & partnerDealtMask;
    while (tmp) {
        const IntCard ic = tmp.popLowest();
        // ic が献上によって得られたカードの下界だった場合のパターン数を計算
        const Cards c = IntCardToCards(ic);
        const Cards lowerDist = pickLower(c) & dealCards;
        const int lowerDists = countCards(lowerDist);

        if (lowerDists < T) continue; // 配布不可能

        double combinations = dCombination(lowerDists, T);
        if (N_CHANGE_CARDS(myClass) > 1) {
            // 下界が確定したときの他の献上札のパターン数をかける
            combinations *= dCombination(countCards(pickHigher(c) & myDealtCards), N_CHANGE_CARDS(myClass) - 1);
        }
        dealCardsUnderInWA[probs.size()] = lowerDist;
        probs.push_back(combinations);
    }
    assert(probs.size() > 0);

    // WA mathod 用の配列に入れる
    double sum = 0;
    for (double prob : probs) sum += prob;
    for (double& prob : probs) prob *= probs.size() / sum;

    queue<int> small, large;
    for (int i = 0; i < (int)probs.size(); i++) {
        if (probs[i] < 1) small.push(i);
        else large.push(i);
    }

    while (small.size() > 0 && large.size() > 0) {
        int l = small.front();
        int g = large.front();
        small.pop();
        large.pop();

        thresholdInWA[l] = probs[l];
        dealCardsOverInWA[l] = dealCardsUnderInWA[g];
        probs[g] += probs[l] - 1;
        if (probs[g] < 1) small.push(g);
        else large.push(g);
    }
    while (large.size() > 0) {
        int g = large.front();
        large.pop();
        thresholdInWA[g] = 1;
    }
    while (small.size() > 0) {
        int l = small.front();
        small.pop();
        thresholdInWA[l] = 1;
    }
    candidatesInWA = probs.size();
}

double RandomDealer::oneChangeLikelihood(int p, const Cards cards, const Cards changeCards, const SharedData& shared) const {
    Cards change[N_MAX_CHANGES];
    double score[N_MAX_CHANGES];
    int qty = changeCards.count();
    int numChanges = genChange(change, cards, qty);
    changePolicyScore(score, change, numChanges, cards, qty, shared.baseChangePolicy);
    if (shared.playerModel.trained) {
        for (int i = 0; i < numChanges; i++) score[i] += shared.playerModel.changeBiasScore(p, cards, change[i]);
    }
    SoftmaxSelector<double> selector(score, numChanges, 1.1);
    int index = find(change, change + numChanges, changeCards) - change;
    assert(0 <= index && index < numChanges);
    double prob = selector.prob(index);
    return max(prob, 1 / 128.0);
}

double RandomDealer::onePlayLikelihood(const Field& field, Move move,
                                       const SharedData& shared, ThreadTools *const ptools) const {
    int turn = field.turn();
    const Board b = field.board;

    MoveInfo *const mbuf = ptools->mbuf;
    int numMoves = genMove(mbuf, field.getCards(turn), b);
    if (numMoves <= 1) return 1;

    // 場の情報をまとめる
    for (int i = 0; i < numMoves; i++) {
        bool mate = checkHandMate(0, mbuf + numMoves, mbuf[i], field.hand[turn],
                                  field.opsHand[turn], b, field.fieldInfo);
        if (mate) mbuf[i].setMPMate();
    }

    // 選ばれた手が生成されているか調べる
    int moveIndex = searchMove(mbuf, numMoves, move);
    if (moveIndex == -1) return 0.1 / double(numMoves + 1);

    double score[N_MAX_MOVES];
    playPolicyScore(score, mbuf, numMoves, field, shared.basePlayPolicy);
    if (shared.playerModel.trained) {
        for (int i = 0; i < numMoves; i++) score[i] += shared.playerModel.playBiasScore(field, turn, mbuf[i]);
    }

    // Mateの手のスコアを設定
    double maxScore = *max_element(score, score + numMoves);
    for (int i = 0; i < numMoves; i++) {
        if (mbuf[i].isMate()) score[i] = maxScore + 4;
    }

    SoftmaxSelector<double> selector(score, numMoves, Settings::estimationTemperaturePlay);
    return max(selector.prob(moveIndex), 1 / 256.0);
}

double RandomDealer::playLikelihood(const Cards *c, const GameRecord& game,
                                    const SharedData& shared, ThreadTools *const ptools) const {
    // 想定した手札配置から、試合進行がどの程度それっぽいか考える
    if (inChange) return 0;
    double playllh = 0; // 対数尤度

    array<Cards, N> orgCards;
    for (int p = 0; p < N; p++) {
        orgCards[p] = c[p] + usedCards[infoClass[p]];
    }

    Field field;
    for (Move move : PlayRoller(field, game, orgCards)) {
        int turn = field.turn();
        if (field.turnCount() >= turnCount) break; // 決めたところまで読み終えた(オフラインでの判定用)
        if (!holdsCards(field.getCards(turn), move.cards())) break; // 終了(エラー)
        // カードが全確定しているプレーヤー(主に自分と、既に上がったプレーヤー)以外を考慮
        if (playFlag.test(turn)) {
            double lh = onePlayLikelihood(field, move, shared, ptools);
            if (lh < 1) playllh += log(lh);
        }
    }
    return playllh;
}