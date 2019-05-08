#include "mate.hpp"
#include "linearPolicy.hpp"
#include "estimation.hpp"

using namespace std;

// Walker's Alias Method はこちらを参考に
// http://qiita.com/ozwk/items/6d62a0717bdc8eac8184

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

// 拘束条件分割
bool dist2Rest_64(int numRest, uint64_t *const goal0, uint64_t *const goal1,
                  const uint64_t arg, int N0, int N1,
                  const uint64_t rest0, const uint64_t rest1, Dice& dice) {
    // 0が交換上手側、1が下手側
    uint64_t tmp0 = 0ULL, tmp1 = 0ULL;
    uint64_t all = arg | rest0 | rest1;

    // まず確定ビットを探す
    if (rest0) {
        // 上位者の確定札のうち、全体の下位 num1 - numRest 枚は献上で移動した
        // 訳では無いことが確定するので先に配っておく
        uint64_t set0 = rest0 & lowestNBits(all, N1 - numRest);
        if (set0) {
            int NSet0 = popcnt64(set0);
            tmp0 |= set0; all -= set0; N0 -= NSet0;
        }
    }
    // 残った札を枚数で分ける
    dist2_64(&tmp0, &tmp1, all, N0, N1, dice);
    // 献上
    uint64_t highNRest = highestNBits(tmp1, numRest);
    tmp0 |= highNRest;
    // 上位者の所持札はここから増えることは無い
    if (!holdsBits(tmp0, rest0)) return false;
    tmp1 -= highNRest;
    // 下位者は上位者からもらうことで手札が増えるが枚数には限度あり
    if (popcnt64(maskCards(rest1, tmp1)) > numRest) return false;
    
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
    // 主観情報のうち完全な（と定義した）情報のみ扱い、それ以外は完全ランダムとする
    BitCards tmp[N] = {0};
    dist64<N>(tmp, dealCards, NDeal.data(), dice);
    for (int cl = 0; cl < N; cl++) {
        dst[infoClassPlayer[cl]] = detCards[cl] + tmp[cl] - usedCards[cl];
    }
    checkDeal(dst);
}

void RandomDealer::dealWithBias(Cards *const dst, Dice& dice) const {
    //　逆関数法でバイアスを掛けて分配
    if (initGame) return dealWithSubjectiveInfo(dst, dice);

    array<Cards, N> tmp = detCards;
    int tmpNDeal[N];
    for (int r = 0; r < N; r++) tmpNDeal[r] = NDeal[r];
    Cards fromCards = dealCards;
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
    turnCount = field.turnCount();

    // 交換ありの場合には配列のインデックスは階級を基本とする
    // 交換がない場合にはプレーヤー番号で代用する
    for (int p = 0; p < N; p++) {
        int cl = initGame ? p : field.classOf(p);

        infoClassPlayer[cl] = p;
        infoClass[p] = cl;

        int org = field.getNCards(p) + field.getUsedCards(p).count();
        int own = field.getNCards(p);

        if (inChange && cl == getChangePartnerClass(field.classOf(playerNum))) {
            // 交換中なら自分の交換相手のカードの枚数は引いておく
            int nch = N_CHANGE_CARDS(cl);
            org -= nch;
            own -= nch;
        }

        NOwn[cl] = NDeal[cl] = own;
        NOrg[cl] = org;
        NDet[cl] = org - own;
        usedCards[cl] = field.getUsedCards(p);
        detCards[cl] += usedCards[cl]; // 既に使用されたカードは確定
    }

    myNum = playerNum;
    myCards = field.getCards(myNum);
    // 交換無し時の階級の代用込みでの設定
    myClass = infoClass[myNum];
    firstTurnClass = inChange ? -1 : infoClass[field.firstTurn()];

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

    // 各メソッドの使用可、不可を設定
    if (okForRejection()) { // 採択棄却法使用OK
        if (!field.isInitGame() && myClass < MIDDLE) setWeightInWA();
    } else failed = true;
}

void RandomDealer::checkDeal(Cards *const dst, bool sbj) const {
    for (int p = 0; p < N; p++) DERR << dst[p] << endl;
    for (int r = 0; r < N; r++) {
        ASSERT(countCards(dst[infoClassPlayer[r]]) == NOwn[r],
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
            if (NDet[FUGO] >= 7 || NDet[HINMIN] >= 7) return false;
            break;
        case FUGO:
            if (NDet[DAIFUGO] >= 7 || NDet[DAIHINMIN] >= 7) return false;
            break;
        case HEIMIN:
            if (NDet[DAIFUGO] >= 8 || NDet[FUGO] >= 8
                || NDet[HINMIN] >= 8 || NDet[DAIHINMIN] >= 8) return false;
            break;
        case HINMIN:
            if (NDet[DAIFUGO] >= 9 || NDet[FUGO] >= 9 || NDet[DAIHINMIN] >= 9) return false;
            break;
        case DAIHINMIN:
            if (NDet[DAIFUGO] >= 9 || NDet[FUGO] >= 9 || NDet[HINMIN] >= 9) return false;
            break;
        default: UNREACHABLE; break;
    }
    return true;
}

// 採択棄却法のためのカード交換モデル
Cards RandomDealer::change(const int p, const Cards cards, const int qty,
                           const EngineSharedData& shared, EngineThreadTools *const ptools) const {
    Cards cand[N_MAX_CHANGES];
    int NCands = genChange(cand, cards, qty);
    Field field;
    // 交換方策によって交換してみる
    int index = changeWithPolicy(cand, NCands, cards, qty, field, shared.baseChangePolicy, ptools->dice);
    return cand[index];
}

double changeProb(const int p, const Cards cards, const int qty,
                  const EngineSharedData& shared, EngineThreadTools *const ptools,
                                const Cards cc) {
    Cards cand[N_MAX_CHANGES];
    int NCands = genChange(cand, cards, qty);
    Field field;
    // callbackに合う交換確率を求める
    double score[N_MAX_CHANGES];
    changePolicyScore(score, cand, NCands, cards, qty, field, shared.baseChangePolicy, 0);
    int idx = 0;
    for (idx = 0; idx < NCands; idx++) if (cand[idx] == cc) break;
    SoftmaxSelector<double> selector(score, NCands, 1.0);
    return selector.prob(idx);
}

double RandomDealer::dealWithChangeRejection(Cards *const dst,
                                             const SharedData& shared,
                                             ThreadTools *const ptools) {
    // 採択棄却法のカード交換パート
    auto& dice = ptools->dice;
    if (initGame) {
        dealWithSubjectiveInfo(dst, dice);
        return 0;
    }

    bool success = false;
    int trials = 0;
    double logLikelihood = 0;
    const double probFrac = 1e-2;

    BitCards R[N] = {0};
    for (int t = 0; t < MAX_REJECTION; t++) {
        double lls = 0;

        int ptClass = getChangePartnerClass(myClass);
        if (myClass < MIDDLE) {
            // 1. Walker's Alias methodで献上下界を決めて交換相手に分配
            R[ptClass] = detCards[ptClass];
            if (NDeal[ptClass]) {
                Cards tmpDist = selectInWA(dice.random());
                R[ptClass] += pickNBits64(tmpDist, NDeal[ptClass], tmpDist.count() - NDeal[ptClass], dice);
            }
        } else {
            // 1. 交換相手のカードを決め打って期待した交換が選ばれるか調べる
            int ptClass = getChangePartnerClass(myClass);
            R[ptClass] = detCards[ptClass] + recvCards;
            BitCards remained = CARDS_NULL;
            int numDealOthers = NdealCards - NDeal[ptClass];
            int numChangeMine = N_CHANGE_CARDS(myClass);
            if (NDeal[ptClass]) {
                dist2_64(&remained, &R[ptClass], dealCards, numDealOthers, NDeal[ptClass], dice);
                // 交換相手の交換尤度を計算
                double prob = changeProb(infoClassPlayer[ptClass], R[ptClass], numChangeMine, shared, ptools, recvCards);
                //lls += log(prob * (1 - probFrac) + probFrac / (ptClass == DAIFUGO ? 78 : 11));
                if (dice.random() >= prob) continue;
                R[ptClass] -= recvCards;
            }
        }

        // 2. 残りカードを平民と自分が関与した以外の交換系にそれぞれ分ける
        BitCards rem = dealCards;
        if (myClass != HEIMIN) {
            rem -= R[ptClass] - detCards[ptClass];
            int otherRich = DAIFUGO + FUGO - min(myClass, ptClass);
            int otherPoor = getChangePartnerClass(otherRich);
            int numDealOtherPair = NDeal[otherRich] + NDeal[otherPoor];
            BitCards otherPair = CARDS_NULL;
            R[HEIMIN] = detCards[HEIMIN];
            if (NDeal[HEIMIN]) {
                dist2_64(&otherPair, &R[HEIMIN], rem, numDealOtherPair, NDeal[HEIMIN], dice);
                rem = otherPair;
            } else {
                otherPair = rem;
            }
        }

        // 3. 自分が関与した以外の交換系の配布
        // 残り札を自分が関わっていない交換の系で分配
        BitCards remained[2] = {0};
        if (myClass == HEIMIN) {
            dist2_64(&remained[0], &remained[1], rem, NDeal[0] + NDeal[4], NDeal[1] + NDeal[3], dice);
        } else {
            // 平民で無いときは自分が関わっていない系に全振り
            remained[1 - min(myClass, ptClass)] = rem;
        }

        bool ok = true;
        for (int cl = FUGO; cl >= 0; cl--) {
            int rich = cl, poor = getChangePartnerClass(cl);
            if (rich == myClass || poor == myClass) continue;
            int numChange = N_CHANGE_CARDS(rich);
            if (!dist2Rest_64(numChange, &R[rich], &R[poor], remained[cl],
                              NOrg[rich], NOrg[poor], detCards[rich], detCards[poor], dice)) {
                ok = false; break;
            }
            Cards cc = change(infoClassPlayer[rich], R[rich], numChange, shared, ptools);
            if (!Cards(R[poor] + cc).holds(detCards[poor])
                || !Cards(R[rich] - cc).holds(detCards[rich])) {
                ok = false; break;
            }
            R[rich] -= cc;
            R[poor] += cc;
        }
        if (!ok) continue;

        logLikelihood = lls;
        success = true;
        break;
    }

    if (success) {
        for (int cl = 0; cl < N; cl++) {
            dst[infoClassPlayer[cl]] = R[cl] - usedCards[cl];
        }
        dst[myNum] = myCards;
        checkDeal(dst);
    } else { // 失敗の場合は逆関数法に変更
        if (++failures >= 2) failed = true; 
        dealWithBias(dst, dice);
        return 0;
    }
    return logLikelihood;
}

double RandomDealer::changeLikelihood(const Cards *const dst, const SharedData& shared, ThreadTools *const ptools) {
    // 手札の交換尤度を返す
    if (initGame) return 0;

    // モデルを使って逆演算で尤度を近似
    double likelihoodProduct = 1;
    for (int rich = 0; rich < MIDDLE; rich++) {
        double likelihood = 0;
        int poor = getChangePartnerClass(rich);
        int richID = infoClassPlayer[rich];
        int poorID = infoClassPlayer[poor];
        int changeQty = N_CHANGE_CARDS(rich);
        // 交換時に呼ばれたときはまだ上位から下位に渡されていないことに注意
        Cards richAfterChange = dst[richID] + usedCards[rich];
        Cards poorAfterChange = dst[poorID] + usedCards[poor];
        if (myClass == rich) {
            Cards richAfterPresent = richAfterChange, poorAfterPresent = poorAfterChange;
            // 自分がこの交換上位のとき献上が起こるパターン数のみ調べる
            if (!inChange) {
                richAfterPresent += sentCards;
                poorAfterPresent -= sentCards;
            }
            Cards higher = pickHigher(richAfterPresent, poorAfterPresent);
            double pattern = dCombination(higher.count(), changeQty);
            likelihood += pattern;
        } else if (myClass == poor) {
            Cards richAfterPresent = richAfterChange;
            if (!inChange) richAfterPresent += recvCards;
            double prob = changeProb(richID, richAfterPresent, changeQty, shared, ptools, recvCards);
            likelihood += prob;
        } else {
            // カード交換前の手札をすべて生成
            Cards thrown[N_MAX_CHANGES];
            const int numThrowns = genChange(thrown, dst[poorID] + usedCards[poor], changeQty);
            for (int i = 0; i < numThrowns; i++) {
                // 上位の献上後のカード
                Cards richAfterPresent = richAfterChange + thrown[i];
                Cards poorAfterPresent = poorAfterChange - thrown[i];
                //この交換の元になったパターン数を調べる
                Cards higher = pickHigher(richAfterPresent, poorAfterPresent);
                double pattern = dCombination(higher.count(), changeQty);
                if (pattern > 0) {
                    // この交換が発生した確率を調べる
                    double prob = changeProb(richID, richAfterPresent, changeQty, shared, ptools, thrown[i]);
                    likelihood += prob * pattern;
                }
            }
        }
        likelihoodProduct *= likelihood;
    }
    return log(likelihoodProduct);
}

double RandomDealer::onePlayLikelihood(const Field& field, Move move, unsigned time,
                                       const SharedData& shared, ThreadTools *const ptools) const {
    MoveInfo *const mv = ptools->buf;
    const int turn = field.turn();
    const Cards myCards = field.getCards(turn);
    const Board b = field.board;
    const int NMoves = genMove(mv, myCards, b);
    assert(myCards.holds(move.cards()));
    if (NMoves <= 1) return 1;

    // 場の情報をまとめる
    for (int i = 0; i < NMoves; i++) {
        bool mate = checkHandMate(0, mv + NMoves, mv[i], field.hand[turn],
                                  field.opsHand[turn], b, field.fieldInfo);
        if (mate) mv[i].setMPMate();
    }

    // プレー尤度計算
    int moveIndex = searchMove(mv, NMoves, [move](const MoveInfo& tmp)->bool{
        return tmp == move;
    });

    double prob = 0;
    if (moveIndex >= 0) {
        std::array<double, N_MAX_MOVES> score;
        playPolicyScore(score.data(), mv, NMoves, field, shared.basePlayPolicy, 0);
        // Mateの手のスコアを設定
        double maxScore = *std::max_element(score.begin(), score.begin() + NMoves);
        for (int i = 0; i < NMoves; i++) {
            if (mv[i].isMate()) score[i] = maxScore + 4;
        }
        SoftmaxSelector<double> selector(score.data(), NMoves, Settings::estimationTemperaturePlay);
        prob = selector.prob(moveIndex);
    }
    // 確率をソフトにする
    const double probFrac = 5e-3;
    return prob * (1 - probFrac) + probFrac / NMoves;
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
    const int NMyDC = countCards(myDealtCards);

    // 相手の献上後の所持カードで判明しているもの
    const Cards partnerDealtCards = maskCards(detCards[getChangePartnerClass(myClass)], myDealtCards);
    // 相手の献上後の所持カードの上界より高い札のみ献上でもらっている可能性がある
    const Cards partnerDealtMask = anyCards(partnerDealtCards) ? pickHigher(pickHigh(partnerDealtCards, 1)) : CARDS_ALL;

    Cards tmp = pickLow(myDealtCards, NMyDC - N_CHANGE_CARDS(myClass) + 1) & partnerDealtMask;
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
        dealCardsUnderInWA[probs.size()] = dealCards & pickLower(IntCardToCards(ic));
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

void RandomDealer::prepareSubjectiveInfo() {
    // 自分以外の未使用カード
    dealCards = remCards - myCards;

    // 自分
    detCards[myClass] |= myCards;
    NDeal[myClass] = 0;
    NDet[myClass] = NOrg[myClass];

    // 初手がすでに済んでいる場合、初手プレーヤーにD3
    if (!inChange
        && turnCount > 0
        && dealCards.contains(INTCARD_D3)) {
        detCards[firstTurnClass] |= CARDS_D3;
        dealCards -= CARDS_D3;
        NDeal[firstTurnClass] -= 1;
        NDet[firstTurnClass] += 1;
    }
    if (!initGame && !inChange) {
        if (myClass < MIDDLE) { // 自分が上位のとき
            // 交換であげたカードのうちまだ確定扱いでないもの
            if (!inChange) { // 交換時はまだ不明
                Cards sCards = dealCards & sentCards;
                if (sCards) {
                    int nc = countCards(sCards);
                    int myChangePartnerClass = getChangePartnerClass(myClass);
                    detCards[myChangePartnerClass] |= sCards;
                    dealCards -= sCards;
                    NDeal[myChangePartnerClass] -= nc;
                    NDet[myChangePartnerClass] += nc;
                }
            }
        }
    }

    // 結局配る枚数
    NdealCards = countCards(dealCards);
    ASSERT(NdealCards == accumulate(NDeal.begin(), NDeal.begin() + N, 0),
           cerr << NdealCards << " " << NDeal << " " << accumulate(NDeal.begin(), NDeal.begin() + N, 0) << endl;);

    if (!inChange && turnCount > 0) {
        // 他人が使用したカード枚数から採択棄却の際の候補数を設定
        unsigned NOppUsedCards = Cards(CARDS_ALL - remCards - usedCards[myClass]).count();
        // 1 -> ... -> max -> ... -> 1 と台形に変化
        int line1 = (BUCKET_MAX - 1) * NOppUsedCards / 4 + 1;
        int line2 = BUCKET_MAX;
        //int line3 = (BUCKET_MAX - 1) * NdealCards / 16 + 1;
        //buckets = NOppUsedCards > 0 ? min(min(line1, line2), line3) : 1;
        buckets = NOppUsedCards > 0 ? min(line1, line2) : 1;
        // 全てのカードが明らかになっていないプレーヤーは着手を検討する必要があるのでフラグを立てる
        playFlag.reset();
        for (int p = 0; p < N; p++) {
            if (p != myNum && NDeal[infoClass[p]] > 0) playFlag.set(p);
        }
    } else {
        buckets = 1;
    }
    for (int p = 0; p < N; p++) {
        DERR << "cl:" << p << " p:" << (int)infoClassPlayer[p] << " Org:" << (int)NOrg[p] << " Own:" << (int)NOwn[p]
        << " Det:" << (int)NDet[p] << " Deal:" << (int)NDeal[p] << " " << detCards[p] << endl;
    }
}