#pragma once

// 相手手札を推定して配布

#include "engineSettings.h"
#include "../core/field.hpp"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "mate.hpp"

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
template <class dice64_t>
bool dist2Rest_64(int numRest,
                  uint64_t *const rest, uint64_t *const goal0, uint64_t *const goal1,
                  const uint64_t arg, int N0, int N1,
                  const uint64_t rest0, const uint64_t rest1, dice64_t& dice) {   
    // 0が交換上手側、1が下手側
    uint64_t tmp0 = 0ULL, tmp1 = 0ULL;
    uint64_t all = arg | rest0 | rest1;

    // まず確定ビットを探す
    if (rest0) {
        uint64_t low = highestNBits(all, N1 + numRest);
        uint64_t set0 = rest0 & ~low;
        if (set0) {
            int NSet0 = popcnt64(set0);
            tmp0 |= set0; all -= set0; N0 -= NSet0;
        }
    }
    assert((int)popcnt64(all) == N0 + N1);
    dist2_64(&tmp0, &tmp1, all, N0, N1, dice);
    // 献上
    uint64_t highNRest = highestNBits(tmp1, numRest);
    tmp0 |= highNRest; tmp1 -= highNRest;

    if (!holdsBits(tmp0, rest0)
        || !holdsBits(tmp1, rest1 & ~highNRest)) return false;

    *goal0 = tmp0;
    *goal1 = tmp1;
    *rest = tmp0 & rest1;

    return true;
}

template <class gameRecord_t>
class RandomDealer {
    // ランダムに手札配置を作る
    // 基本的に連続分配を想定しているので、異なるプレーヤーが互いに分配するような状況ではインスタンスを複数作るべき
    static constexpr int N = N_PLAYERS;
public:
    RandomDealer(const gameRecord_t& gr, const Field& f, int turn):
    record(gr) {
        turnCount = f.turnCount();
        set(f, turn);
    }
    
    int create(ImaginaryWorld *const dst, DealType type,
               const EngineSharedData& shared, EngineThreadTools *const ptools) {
        Cards c[N_PLAYERS];
        // レベル指定からカード分配
        switch (type) {
            case DealType::RANDOM: // 残りカードを完全ランダム分配
                dealAllRand(c, ptools->dice); break;
            case DealType::SBJINFO: // 交換等は考慮するが残りは完全ランダム
                dealWithSubjectiveInfo(c, ptools->dice); break;
            case DealType::BIAS: // 逆関数法でバイアスを掛けて配る
                dealWithBias(c, ptools->dice); break;
            case DealType::REJECTION: // 採択棄却法で良さそうな配置のみ返す
                dealWithRejection(c, shared, ptools); break;
            default: UNREACHABLE; break;
        }
        dst->set(turnCount, c);
        return 0;
    }
    
    template <class dice64_t>
    void dealAllRand(Cards *const dst, dice64_t& dice) const {
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

    template <class dice64_t>
    void dealWithSubjectiveInfo(Cards *const dst, dice64_t&dice) const {
        // 主観情報のうち完全な（と定義した）情報のみ扱い、それ以外は完全ランダムとする
        BitCards tmp[N] = {0};
        dist64<N>(tmp, dealCards, NDeal.data(), dice);
        for (int cl = 0; cl < N; cl++) {
            dst[infoClassPlayer[cl]] = detCards[cl] + tmp[cl] - usedCards[cl];
        }
        checkDeal(dst);
    }

    template <class dice64_t>
    void dealWithBias(Cards *const dst, dice64_t& dice) const {
        //　逆関数法でバイアスを掛けて分配
        if (initGame) return dealWithSubjectiveInfo(dst, dice);

        std::array<Cards, N> tmp = detCards;
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
    
    void dealWithRejection(Cards *const dst,
                           const SharedData& shared,
                           ThreadTools *const ptools) {
        // 採択棄却法メイン
        Cards deal[BUCKET_MAX][N]; // カード配置候補
        int bestDeal = 0;
        for (int i = 0; i < buckets; i++) {
            if (failed) {
                dealWithBias(deal[i], ptools->dice);
            } else {
                bool ok = dealWithChangeRejection(deal[i], shared, ptools);
                // 失敗の回数が一定値を超えた以降は逆関数法に移行
                if (!ok && ++failures > 1) failed = true;
            }
        }
        // 役提出の尤度を計算してし使用する手札配置を決定
        if (buckets > 1) {
            double lhs[BUCKET_MAX];
            for (int i = 0; i < buckets; i++) {
                lhs[i] = calcPlayLikelihood(deal[i], record, shared, ptools);
            }
            SoftmaxSelector<double> selector(lhs, buckets, 0.3);
            bestDeal = selector.select(ptools->dice.random());
        }
        for (int p = 0; p < N; p++) dst[p] = deal[bestDeal][p];
        checkDeal(dst);
    }

    void set(const Field& field, int playerNum) {
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
        firstTurnClass = infoClass[field.firstTurn()];

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
    
private:
    // 棋譜
    const gameRecord_t& record;
    int turnCount;
    
    // 引数は交換ありの場合は階級、交換無しの場合はプレーヤー番号と同じ
    std::array<int8_t, N> NOwn;  // 持っているカード数
    std::array<int8_t, N> NDet;  // 特定＋使用カード数
    std::array<int8_t, N> NDeal; // 配布カード数
    std::array<int8_t, N> NOrg;  // 初期配布カード数
    std::array<int8_t, N> infoClassPlayer;
    std::array<int8_t, N> infoClass;
    unsigned NdealCards;
    Cards remCards;  // まだ使用されていないカード
    Cards dealCards; // まだ特定されていないカード
    std::array<Cards, N> usedCards; // 使用済みカード
    std::array<Cards, N> detCards; // 現時点で所持が特定されている、またはすでに使用したカード

    int myNum, myClass;
    int myChangePartner, firstTurnClass;

    Cards myCards;
    Cards myDealtCards; // 配布時
    Cards recvCards, sentCards;
    
    bool inChange;
    bool initGame;
    bool failed;
    unsigned failures;
    
    // 採択棄却法時の棄却回数限度
    static constexpr int MAX_REJECTION = 800;
    
    // 自分が上位のときに献上された最小の札の確率
    int candidatesInWA;
    double thresholdInWA[16];
    Cards dealCardsUnderInWA[16];
    Cards dealCardsOverInWA[16];
    
    // 進行得点関連
    static constexpr int BUCKET_MAX = 32;
    int buckets;
    
    // 着手について検討の必要があるプレーヤーフラグ
    std::bitset<N_PLAYERS> playFlag;
    
    // inner function
    void checkDeal(Cards *const dst, bool sbj = true) const {
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
    
    bool okForRejection() const {
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
            default: UNREACHABLE; break;
        }
        return true;
    }

    bool dealWithChangeRejection(Cards *const dst,
                                 const EngineSharedData& shared,
                                 EngineThreadTools *const ptools) const {
        // 採択棄却法のカード交換パート
        auto& dice = ptools->dice;
        if (initGame) {
            dealWithSubjectiveInfo(dst, dice);
            return true;
        }

        bool success = false;
        int trials = 0;

        BitCards R[N] = {0};
        if (myClass != HEIMIN) {
            // 交換に関与しているとき

            while (trials++ <= MAX_REJECTION) {
                int ptClass = getChangePartnerClass(myClass);
                if (myClass < MIDDLE) {
                    // 1. Walker's Alias methodで献上下界を決めて交換相手に分配
                    R[ptClass] = detCards[ptClass];
                    if (NDeal[ptClass]) {
                        Cards tmpDist = selectInWA(dice.random());
                        R[ptClass] += pickNBits64(tmpDist, NDeal[ptClass], countCards(tmpDist) - NDeal[ptClass], dice);
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
                        // 交換相手の交換が実際に沿うか検証
                        Cards cc = change(infoClassPlayer[ptClass], R[ptClass], numChangeMine, shared, ptools);
                        if (!holdsCards(detCards[myClass], cc)) continue; // 矛盾
                        R[ptClass] -= cc;
                    }
                }

                // 2. 残りカードを平民と自分が関与した以外の交換系にそれぞれ分ける
                BitCards remained = dealCards & ~R[ptClass];
                int otherRich = DAIFUGO + FUGO - min(myClass, ptClass);
                int otherPoor = getChangePartnerClass(otherRich);
                int numDealOtherPair = NDeal[otherRich] + NDeal[otherPoor];
                BitCards otherPair = CARDS_NULL;
                R[HEIMIN] = detCards[HEIMIN];
                if (NDeal[HEIMIN]) {
                    dist2_64(&otherPair, &R[HEIMIN], remained, numDealOtherPair, NDeal[HEIMIN], dice);
                } else {
                    otherPair = remained;
                }

                // 3. 自分が関与した以外の交換系の配布
                int numChangeOtherPair = N_CHANGE_CARDS(otherRich);
                BitCards restricted;
                if (!dist2Rest_64(numChangeOtherPair, &restricted,
                                  &R[otherRich], &R[otherPoor], otherPair,
                                  NOrg[otherRich], NOrg[otherPoor],
                                  detCards[otherRich], detCards[otherPoor], dice)) continue;
                Cards cc = change(infoClassPlayer[otherRich], R[otherRich], numChangeOtherPair, shared, ptools);
                // 交換と既知の情報との矛盾をチェック
                if (!holdsCards(cc, restricted) || (cc & detCards[otherRich])) continue;
                R[otherRich] -= cc;
                R[otherPoor] += cc;
                success = true;
                break;
            }
        } else {
            // 交換無関与のとき
            BitCards R0_R4, R1_R3, R0_R4Rest, R1_R3Rest;
            while (trials++ <= MAX_REJECTION) {

                // 1. カードを大富豪-大貧民系と富豪-貧民系に分ける
                BitCards remained[2] = {0};
                dist2_64(&remained[0], &remained[1], dealCards, NDeal[0] + NDeal[4], NDeal[1] + NDeal[3], dice);

                // 2. それぞれの系で交換を起こしてみて矛盾が無いか調べる
                bool ok = true;
                for (int cl = FUGO; cl >= 0; cl--) {
                    int rich = cl, poor = getChangePartnerClass(cl);
                    int numChange = N_CHANGE_CARDS(rich);
                    BitCards restricted;
                    if (!dist2Rest_64(numChange, &restricted, &R[rich], &R[poor], remained[cl],
                                      NOrg[rich], NOrg[poor], detCards[rich], detCards[poor], dice)) {
                        ok = false; break;
                    }
                    Cards cc = change(infoClassPlayer[rich], R[rich], numChange, shared, ptools);
                    if (!holdsCards(cc, restricted) || (cc & detCards[rich])) {
                        ok = false; break;
                    }
                    R[rich] -= cc;
                    R[poor] += cc;
                }
                if (!ok) continue;

                success = true;
                break;
            }
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
            dealWithBias(dst, dice);
            return false;
        }
        return true;
    }
    
    /*template <class sharedData_t, class threadTools_t>
        int dealWithLikelifood(Cards *const dst, const sharedData_t& shared,
        threadTools_t *const ptools) {
        // 尤度計算
        // 主観的情報で明らかな矛盾のない配り方
        dealWithSubjectiveInfo(dst, ptools->dice);
        Field field;
        
        double logLHSum = 0, logLHOK = 0;
        // カード交換前の手札をすべて生成
        Cards thrownCards[N_MAX_CHANGES];
        const int throwns = genChange(thrownCards, dst[infoClassPlayer[DAIHINMIN]] | usedCards[DAIHINMIN], N_CHANGE_CARDS(DAIFUGO));
        for (int i = 0; i < throwns; i++) {
        // 交換前の大富豪の手札
        Cards dealtCards = dst[infoClassPlayer[DAIFUGO]] | usedCards[DAIFUGO] | thrownCards[i];
        Cards changeCards[N_MAX_CHANGES];
        double score[N_MAX_CHANGES + 1];
        const int changes = genChange(changeCards, dealtCards, N_CHANGE_CARDS(DAIFUGO));
        calcChangePolicyScoreSlow<>(score, changes, )
        
        }
        
        return 0;
    }*/
    
    // 採択棄却法のためのカード交換モデル
    Cards change(const int p, const Cards cards, const int qty,
                 const EngineSharedData& shared, EngineThreadTools *const ptools) const {
        Cards cand[78];
        int NCands = genChange(cand, cards, qty);
        Field field;
        // 交換方策によって交換してみる
        int index = changeWithPolicy(cand, NCands, cards, qty, field, shared.baseChangePolicy, ptools->dice);
        return cand[index];
    }
    
    Cards selectInWA(double urand) const {
        double v = urand * candidatesInWA;
        int k = (int)v;
        double u = 1 + k - v;
        if (u < thresholdInWA[k]) return dealCardsUnderInWA[k];
        else return dealCardsOverInWA[k];
    }
    
    void setWeightInWA() {
        std::vector<double> tmpProb;
        const int T = NDeal[getChangePartnerClass(myClass)]; // 交換相手の配布枚数
        const int NMyDC = countCards(myDealtCards);

        // 相手の献上後の所持カードで判明しているもの
        const Cards partnerDealtCards = maskCards(detCards[getChangePartnerClass(myClass)], myDealtCards);
        // 相手の献上後の所持カードの上界より高い札のみ献上でもらっている可能性がある
        const Cards partnerDealtMask = anyCards(partnerDealtCards) ? pickHigher(pickHigh(partnerDealtCards, 1)) : CARDS_ALL;

        if (T > 0) {
            Cards tmp = pickLow(myDealtCards, NMyDC - N_CHANGE_CARDS(myClass) + 1) & partnerDealtMask;
            int index = 0;
            while (tmp) {
                const IntCard ic = tmp.popLowest();
                // ic が献上によって得られたカードの下界だった場合のパターン数を計算
                const Cards c = IntCardToCards(ic);
                const Cards lowerDist = pickLower(c) & dealCards;
                const int lowerDists = countCards(lowerDist);
                double combinations;
                if (lowerDists < T) { // 配布不可能
                    continue;
                } else {
                    combinations = dCombination(lowerDists, T);
                    if (N_CHANGE_CARDS(myClass) > 1) {
                        // 下界が確定したときの他の献上札のパターン数をかける
                        combinations *= dCombination(countCards(pickHigher(c) & myDealtCards), N_CHANGE_CARDS(myClass) - 1);
                    }
                }
                tmpProb.push_back(combinations);
                dealCardsUnderInWA[index] = dealCards & pickLower(IntCardToCards(ic));
                index += 1;
            }
            ASSERT(index > 0,);
            // WA mathod 用の配列に入れる
            double sum = 0;
            for (int i = 0; i < index; i++) sum += tmpProb[i];
            for (int i = 0; i < index; i++) tmpProb[i] *= index / sum;

            std::queue<int> small, large;
            for (int i = 0; i < index; i++) {
                if (tmpProb[i] < 1) small.push(i);
                else large.push(i);
            }
            while (small.size() > 0 && large.size() > 0) {
                int l = small.front();
                int g = large.front();
                small.pop();
                large.pop();
                
                thresholdInWA[l] = tmpProb[l];
                dealCardsOverInWA[l] = dealCardsUnderInWA[g];
                tmpProb[g] += -1.0 + tmpProb[l];
                if (tmpProb[g] < 1) small.push(g);
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
            candidatesInWA = index;
        }
    }

    void prepareSubjectiveInfo() {
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
        ASSERT(NdealCards == std::accumulate(NDeal.begin(), NDeal.begin() + N, 0),
               cerr << NdealCards << " " << NDeal << " " << std::accumulate(NDeal.begin(), NDeal.begin() + N, 0) << endl;);

        if (!inChange && turnCount > 0) {
            // 他人が使用したカード枚数から採択棄却の際の候補数を設定
            unsigned NOppUsedCards = Cards(CARDS_ALL - remCards - usedCards[myClass]).count();
            // 1 -> ... -> max -> ... -> 1 と台形に変化
            int line1 = (BUCKET_MAX - 1) * NOppUsedCards / 4 + 1;
            int line2 = BUCKET_MAX;
            int line3 = (BUCKET_MAX - 1) * NdealCards / 16 + 1;
            buckets = NOppUsedCards > 0 ? min(min(line1, line2), line3) : 1;
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

    double calcPlayLikelihood(Cards *const c, const gameRecord_t& gLog,
                              const EngineSharedData& shared, EngineThreadTools *const ptools) const {
        // 想定した手札配置から、試合進行がどの程度それっぽいか考える

        double playLH = 0; // 対数尤度
        std::array<Cards, N> orgCards;
        MoveInfo *const mv = ptools->buf;
        for (int p = 0; p < N; p++) orgCards[p] = c[p] | detCards[infoClass[p]];
        Field field;
        iterateGameLogInGame
        (field, gLog, gLog.plays(), orgCards,
        // after change callback
        [](const auto& field)->void{},
        // play callback
        [this, &playLH, &orgCards, mv, &shared]
        (const auto& field, const Move chosen, uint32_t usedTime)->int{
            const int tp = field.turn();
            const Board b = field.board;
            const Cards myCards = field.getCards(tp);

            if (field.turnCount() >= turnCount) return -1; // 決めたところまで読み終えた(オフラインでの判定用)
            if (!holdsCards(myCards, chosen.cards())) return -1; // 終了(エラー)
            // カードが全確定しているプレーヤー(主に自分と、既に上がったプレーヤー)については考慮しない
            if (!playFlag.test(tp)) return 0;
            const int NMoves = genMove(mv, myCards, b);
            if (NMoves <= 1) return 0;

            // 場の情報をまとめる
            for (int i = 0; i < NMoves; i++) {
                bool mate = checkHandMate(0, mv + NMoves, mv[i], field.hand[tp],
                                          field.opsHand[tp], b, field.fieldInfo);
                if (mate) mv[i].setMPMate();
            }

            // フェーズ(空場0、通常場1、パス支配場2)
            const int ph = b.isNull() ? 0 : (field.fieldInfo.isPassDom()? 2 : 1);
            // プレー尤度計算
            int chosenIdx = searchMove(mv, NMoves, [chosen](const auto& tmp)->bool{
                return tmp == chosen;
            });

            if (chosenIdx == -1) { // 自分の合法手生成では生成されない手が出された
                playLH += log(0.1 / (double)(NMoves + 1));
            } else {
                std::array<double, N_MAX_MOVES> score;
                calcPlayPolicyScoreSlow<0>(score.data(), mv, NMoves, field, shared.basePlayPolicy);
                // Mateの手のスコアを設定
                double maxScore = *std::max_element(score.begin(), score.begin() + NMoves);
                for (int i = 0; i < NMoves; i++) {
                    if (mv[i].isMate()) score[i] = maxScore + 4;
                }
                SoftmaxSelector<double> selector(score.data(), NMoves, Settings::simulationTemperaturePlay);
                playLH += log(max(selector.prob(chosenIdx), 1 / 256.0));
            }
            return 0;
        });
        return playLH;
    }
};