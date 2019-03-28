#pragma once

// 相手手札を推定して配布

#include "engineSettings.h"
#include "../core/field.hpp"
#include "../core/record.hpp"

// Walker's Alias Method はこちらを参考に
// http://qiita.com/ozwk/items/6d62a0717bdc8eac8184

namespace UECda {
    namespace Fuji {
        
        namespace Deal {
            
            constexpr uint64_t CPTable_afterChange[INTCARD_MAX + 1][5] =
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
        template <int N_REST, class dice64_t>
        int dist2Rest_64(uint64_t *const rest, uint64_t *const goal0, uint64_t *const goal1,
                         const uint64_t arg, int N0, int N1,
                         const uint64_t rest0, const uint64_t rest1, dice64_t *const pdice) {
            
            // 0が交換上手側、1が下手側
            // カードのビットが高い順、低い順のどちらに並んでいるかが重要
            
            uint64_t tmp0 = 0ULL, tmp1 = 0ULL;
            uint64_t all = arg | rest0 | rest1;
            
            DERR << "all(" << countBits64(all) << ")" << OutCards(all) << endl;
            DERR << "goal (" << N0 << "," << N1 << ")" << endl;
            DERR << "div(" << countBits64(arg) << ")" << OutCards(arg) << endl;
            DERR << "rest0(" << countBits64(rest0) << ")"<< OutCards(rest0) << endl;
            DERR << "rest1(" << countBits64(rest1) << ")"<< OutCards(rest1) << endl;
            
            // まず確定ビットを探す
            if (rest0) {
                uint64_t low = highestNBits(all, N1 + N_REST);
                uint64_t set0 = rest0 & ~low;
                if (set0) {
                    int NSet0 = countBits64(set0);
                    tmp0 |= set0; all -= set0; N0 -= NSet0;
                }
            }
            
            assert((int)countBits64(all) == N0 + N1);
            dist2_64(&tmp0, &tmp1, all, N0, N1, pdice);
            // 献上
            uint64_t highNRest = highestNBits(tmp1, N_REST);
            tmp0 |= highNRest; tmp1 -= highNRest;
            
            if (!holdsBits(tmp0, rest0) || (!holdsBits(tmp1, rest1 & ~highNRest))) return 0;
            
            *goal0 |=tmp0;
            *goal1 |= tmp1;
            *rest = tmp0 & rest1;
            
            return 1;
        }
        
        template <int N>
        class RandomDealer {
            // ランダムに手札配置を作る
            // Nは全員の人数
            // 基本的に連続分配を想定しているので、異なるプレーヤーが互いに分配するような状況ではインスタンスを複数作るべき
            // 他のメソッドに比べ、異なるルールにも比較的対応している部分もあるが、その分メモリ効率が悪い
        public:
            RandomDealer() {}
            ~RandomDealer() {}
            
            template <class field_t, class world_t, class sharedData_t, class threadTools_t>
            int create(world_t *const dst, DealType type, const field_t& field,
                       const sharedData_t& shared, threadTools_t *const ptools) {
                Cards c[N_PLAYERS];
                // レベル指定からカード分配
                switch (type) {
                    case DealType::RANDOM: // 残りカードを完全ランダム分配
                        dealAllRand(c, &ptools->dice); break;
                    case DealType::SBJINFO: // 交換等は考慮するが残りは完全ランダム
                        dealWithAbsSbjInfo(c, &ptools->dice); break;
                    case DealType::BIAS: // 逆関数法でバイアスを掛けて配る
                        dealWithBias(c, &ptools->dice); break;
                    case DealType::REJECTION: // 採択棄却法で良さそうな配置のみ返す
                        dealWithRejection(c, shared, ptools); break;
                    default: UNREACHABLE; break;
                }
                dst->set(field, c);
                return 0;
            }
            
            template <class dice64_t>
            void dealAllRand(Cards *const dst, dice64_t *const pdice) const {
                // 完全ランダム分配
                // 自分の分だけは実際のものにする
                ana.start();
                DERR << "START DEAL-RAND" << endl << OutCards(distCards) << endl;
                BitCards tmp[N] = {0};
                tmp[myClass] = myCards;
                Cards dCards = maskCards(remCards, myCards);
                BitArray32<4, N> tmpNOwn = NOwn;
                tmpNOwn.assign(myClass, 0);
                ASSERT(tmpNOwn.sum() == countCards(dCards),
                       cerr << "tmpNOwn = " << tmpNOwn << endl;
                       cerr << "distributedCards = " << OutCards(dCards) << "(" << countCards(dCards) << ")" << endl;);
                dist64<N>(tmp, dCards, tmpNOwn, pdice);
                for (int r = 0; r < N; r++) {
                    dst[infoClassPlayer[r]] = andCards(remCards, tmp[r]);
                }
                checkDeal(dst);
                DERR << "END DEAL-RAND" << endl;
                ana.end(0);
            }
            
            template <class dice64_t>
            void dealWithAbsSbjInfo(Cards *const dst, dice64_t *const pdice) const {
                // 主観情報のうち完全な（と定義した）情報のみ扱い、それ以外は完全ランダムとする
                ana.start();
                DERR << "START DEAL-ABSSBJ" << endl << OutCards(distCards) << endl;
                BitCards tmp[N] = {0};
                dist64<N>(tmp, distCards, NDeal, pdice);
                for (int r = 0; r < N; r++) {
                    dst[infoClassPlayer[r]] = remCards & (detCards[r] | tmp[r]);
                }
                checkDeal(dst);
                DERR << "END DEAL-ABSSBJ" << endl;
                ana.end(1);
            }
            
            template <class dice64_t>
            void dealWithBias(Cards *const dst, dice64_t *const pdice) const {
                //　逆関数法でバイアスを掛けて分配
                if (flag.test(0)) return dealWithAbsSbjInfo(dst, pdice);

                ana.start();
                DERR << "START DEAL-BIAS" << endl << OutCards(distCards) << endl;
                
                std::array<Cards, N> tmp = detCards;
                int tmpNDeal[N];
                for (int r = 0; r < N; r++) tmpNDeal[r] = NDeal[r];
                
                for (Cards fromCards = distCards; anyCards(fromCards);) {
                    IntCard ic = popIntCardHigh(&fromCards);
                    
                    uint64_t weightSum[N];
                    weightSum[0] = tmpNDeal[0] * Deal::CPTable_afterChange[ic][0];
                    for (int r = 1; r < N; r++) {
                        weightSum[r] = weightSum[r - 1] + tmpNDeal[r] * Deal::CPTable_afterChange[ic][r];
                    }
                    assert(weightSum[N - 1] > 0);
                    uint32_t ran = pdice->rand() % weightSum[N - 1];
                    int r;
                    for (r = 0; r < N; r++) {
                        if (weightSum[r] > ran) break;
                    }
                    tmp[r].insert(ic);
                    tmpNDeal[r]--;
                }
                
                for (int r = 0; r < N; r++) {
                    dst[infoClassPlayer[r]] = remCards & tmp[r];
                }
                checkDeal(dst);
                DERR << "EAND DEAL-BIAS" << endl << distCards << endl;
                ana.end(2);
            }
            
            template <class sharedData_t, class threadTools_t>
            void dealWithRejection(Cards *const dst, const sharedData_t& shared,
                                   threadTools_t *const ptools) {
                // 採択棄却法メイン
                // 設定されたレートの分、カード配置を作成し、手札親和度最大のものを選ぶ
                assert(HARate >= 1 && HARate < 64);
                
                ana.start();
                if (HARate <= 1) {
                    // 交換効果のみ検討
                    if (dealWithRejection_ChangePart(dst, shared, ptools) != 0) {
                        // 失敗報告。ただし、カードの分配自体は出来ているので問題ない。
                        ana.addFailure(3);
                        // 失敗の回数が一定値を超えた場合には逆関数法に移行
                        if (++failures > 1) flag.set(1);
                    }
                } else {
                    // 進行得点を調べる
                    
                    Cards cand[HARATE_MAX][N]; // カード配置候補
                    int bestCand = 0;
                    double bestLHS = -9999;
                    double candLHS[HARATE_MAX];
                    
                    DERR << "START DEAL-REJEC" << endl << OutCards(distCards) << endl;
                    
                    for (uint32_t t = 0; t < HARate; t++) {
                        if (dealWithRejection_ChangePart(cand[t], shared, ptools) != 0) {
                            // 失敗報告。ただし、カードの分配自体は出来ているので問題ない
                            ana.addFailure(3);
                            ++failures;
                            if (failures > 1) {
                                // 失敗の回数が一定値を超えたので、以降は逆関数法に移行
                                flag.set(1);
                            }
                        }
                        // 配ってみた手札の尤度を計算する
                        double lhs = calcPlayLikelihood(cand[t], shared, ptools);
                        if (lhs > bestLHS) {
                            bestLHS = lhs;
                            bestCand = t;
                        }
                        candLHS[t] = lhs;
                    }
                    
                    //ランダムに選んでどうか
                    SoftmaxSelector selector(candLHS, HARate, 0.3);
                    bestCand = selector.run_all(&ptools->dice);
                    
                    for (int p = 0; p < N; p++) dst[p] = cand[bestCand][p];
                    for (int p = 0; p < N; p++) {
                        DERR << "r:" << infoClass[p] << " ";
                        DERR << OutCards(dst[p]) << endl;
                    }
                    DERR << bestLHS << endl;
                }
                ana.end(3);
                for (int r = 0; r < N; r++) {
                    assert(countCards(dst[infoClassPlayer[r]]) == NOwn[r]);
                }
            }
            
            void init() {
                failures = 0;
                
                detCards.fill(CARDS_NULL);
                NOwn.clear();
                NOrg.clear();
                NDet.clear();
                NDeal.clear();
                infoClassPlayer.clear();
                infoClass.clear();
                
                flag.reset();
            }
            
            template <class field_t>
            void set(const field_t& field, int playerNum) {
                init();
                
                // 連続分配のために情報をセット(主観情報から)
                myNum = playerNum;
                
                myCards = field.getCards(myNum);
                myDealtCards = field.getDealtCards(myNum);
                remCards = field.getRemCards();
                
                phase = field.phase;
                
                if (phase.isInitGame()) {
                    myClass = myNum; // このときだけ、ランクの情報がプレーヤー番号そのまま
                    flag.set(0);
                    
                    for (int p = 0; p < N; p++) {
                        infoClassPlayer.assign(p, p);
                        infoClass.assign(p, p);
                        
                        uint32_t org = field.getNCards(p) + countCards(field.getUsedCards(p));
                        uint32_t own = field.getNCards(p);
                        
                        // 交換が無いので枚数調整無し
                        NOwn.assign(p, own);
                        NOrg.assign(p, org);
                        NDet.assign(p, org - own);
                        detCards[p] |= field.getUsedCards(p); // 既に使用されたカードは確定
                    }
                    firstTurnPlayerClass = field.getFirstTurnPlayer();
                } else {
                    myClass = field.getPlayerClass(myNum);
                    
                    for (int p = 0; p < N; p++) {
                        int r = field.getPlayerClass(p);
                        
                        infoClassPlayer.assign(r, p);
                        infoClass.assign(p, r);
                        
                        uint32_t org = field.getNCards(p) + countCards(field.getUsedCards(p));
                        uint32_t own = field.getNCards(p);
                        
                        // 交換中は枚数調整
                        if (phase.isInChange()) {
                            if (p == myNum) {
                                // 自分の枚数を増やす
                                uint32_t nch = N_CHANGE_CARDS(r);
                                //org += nch;
                                //own += nch;
                            } else if (r == getChangePartnerClass(myClass)) { // 自分の交換相手のカードの枚数は引いておく
                                uint32_t nch = N_CHANGE_CARDS(r);
                                org -= nch;
                                own -= nch;
                            }
                        }
                        
                        NOwn.assign(r, own);
                        NOrg.assign(r, org);
                        NDet.assign(r, org - own);
                        detCards[r] |= field.getUsedCards(p); // 既に使用されたカードは確定
                    }
                    // 交換中でなく、カード交換に関与した場合
                    if (!phase.isInChange() && myClass != MIDDLE) {
                        sentCards = field.getSentCards(myNum);
                        recvCards = field.getRecvCards(myNum);
                        
                        assert((int)countCards(sentCards) == N_CHANGE_CARDS(myClass));
                        if (myClass > MIDDLE) assert((int)countCards(recvCards) == N_CHANGE_CARDS(myClass));
                    }
                    firstTurnPlayerClass = field.getPlayerClass(field.getFirstTurnPlayer());
                }
                
                NDeal = NOwn;
                
                // サブ情報
                turnNum = field.getTurnNum();
                
                // すでに分かっている情報から確実な部分を分配
                dealPart_AbsSbjInfo();
                
                // 各メソッドの使用可、不可を設定
                if (checkCompWithRejection()) { // 採択棄却法使用OK
                    if (!field.isInitGame() && myClass < MIDDLE) setWeightInWA();
                } else {
                    flag.set(1);
                }
            }
            
        private:
            
            // 引数は交換ありの場合は階級、交換無しの場合はプレーヤー番号と同じ
            std::array<Cards, N> detCards; // 現時点で所持が特定されている、またはすでに使用したカード
            BitArray32<4, N> NOwn; // 持っているカード数
            BitArray32<4, N> NDet; // 特定＋使用カード数
            BitArray32<4, N> NDeal; // 配布カード数
            BitArray32<4, N> NOrg; // 初期配布カード数
            BitArray32<4, N> infoClassPlayer;
            BitArray32<4, N> infoClass;
            uint32_t NDistCards;
            Cards remCards; // まだ使用されていないカード
            Cards distCards; // まだ特定されていないカード
            
            int myNum, myClass;
            int myChangePartner, firstTurnPlayerClass;
            
            Cards myCards;
            Cards myDealtCards; // 配布時
            Cards recvCards, sentCards;
            
            GamePhase phase;
            
            BitSet32 flag;
            uint32_t failures;
            
            // 採択棄却法時の棄却回数限度
            static constexpr int MAX_REJECTION = 800;
            
            // 自分が上位のときに献上された最小の札の確率
            int candidatesInWA;
            double thresholdInWA[16];
            Cards distCardsUnderInWA[16];
            Cards distCardsOverInWA[16];
            
            // 進行得点関連
            int turnNum;
            static constexpr uint32_t HARATE_MAX = 32;
            uint32_t HARate;
            
            // 着手について検討の必要があるプレーヤーフラグ
            BitSet32 playFlag;
            
            static AtomicAnalyzer<5, 1, 0> ana;
            
            // inner function
            void checkDeal(Cards *const dst) const {
                for (int p = 0; p < N; p++) DERR << OutCards(dst[p]) << endl;
                for (int r = 0; r < N; r++) {
                    ASSERT(countCards(dst[infoClassPlayer[r]]) == NOwn[r],
                           cerr << "class = " << r << " NOwn = "<< NOwn[r];
                           cerr << " cards = " << OutCards(dst[infoClassPlayer[r]]) << endl;);
                }
            }
            
            int checkCompWithRejection() const {
                // 採択棄却法使用可能性
                // 数値は経験的に決定している
                int comp = 1;
                if (!flag.test(0)) {
                    switch (myClass) {
                        case DAIFUGO:
                            if (NDet[FUGO] >= 9 || NDet[HINMIN] >= 9) comp = 0;
                            break;
                        case FUGO:
                            if (NDet[DAIFUGO] >= 9 || NDet[DAIHINMIN] >= 9) comp = 0;
                            break;
                        case HEIMIN:
                            if (NDet[DAIFUGO] >= 7 || NDet[FUGO] >= 7 || NDet[HINMIN] >= 8 || NDet[DAIHINMIN] >= 8) comp = 0;
                            break;
                        case HINMIN:
                            if (NDet[DAIFUGO] >= 5 || NDet[FUGO] >= 3 || NDet[DAIHINMIN] >= 5) comp = 0;
                            break;
                        case DAIHINMIN:
                            if (NDet[DAIFUGO] >= 4 || NDet[FUGO] >= 5 || NDet[HINMIN] >= 5) comp = 0;
                            break;
                        default: UNREACHABLE; break;
                    }
                }
                return comp;
            }
            
            template <class sharedData_t, class threadTools_t>
            int dealWithRejection_ChangePart(Cards *const dst,
                                             const sharedData_t& shared,
                                             threadTools_t *const ptools) const {
                // 採択棄却法のカード交換パート
                assert(NDistCards == NDeal.sum());
                auto *const pdice = &ptools->dice;
                
                if (flag.test(0)) {
                    // 初期化ゲームではとりあえずランダム
                    BitCards tmp[N] = {0};
                    dist64<N>(tmp, distCards, NDeal, pdice);
                    for (auto p = 0; p < N; p++) dst[p] = remCards & (detCards[p] | tmp[p]);
                    return 0;
                }
                if (flag.test(1)) {
                    // 計算量が多くなるので逆関数法にする
                    dealWithBias(dst, pdice);
                    return 0;
                }

                bool success = false;
                int trials = 0;
                /*Cards R[N] = {0};

                if (myClass < MIDDLE) {
                    // 富豪以上の場合
                    Cards 

                } else if (myClass > MIDDLE) {
                    // 貧民以下の場合

                } else {
                    // 平民の場合
                }*/

                switch (myClass) {
                    case DAIFUGO:
                    {
                        BitCards R1_R3, R1, R2, R3, R4, R1_R3Rest;

                        while (trials++ <= MAX_REJECTION) {
                            // セットされたウェイトに従って大貧民に分配
                            R4 = detCards[DAIHINMIN];
                            if (NDeal[DAIHINMIN]) {
                                // Walker's Alias methodで献上下界を決めてランダム分配
                                Cards tmpDist = selectInWA(pdice->drand());
                                ASSERT(countCards(tmpDist) >= NDeal[DAIHINMIN],
                                        cerr << OutCards(tmpDist) << " -> " << NDeal[DAIHINMIN] << endl;);
                                R4 |= pickNBits64(tmpDist, NDeal[DAIHINMIN], countCards(tmpDist) - NDeal[DAIHINMIN], pdice);
                            }
                            
                            assert(countCards(R4 & remCards) == NOwn[DAIHINMIN]);
                            
                            // カードを富豪-貧民系と平民に分ける
                            assert(countCards(distCards & ~R4) == NDeal[FUGO] + NDeal[HINMIN] + NDeal[HEIMIN]);
                            
                            R2 = detCards[HEIMIN];
                            if (NDeal[HEIMIN]) {
                                R1_R3 = CARDS_NULL;
                                dist2_64(&R1_R3, &R2, distCards & ~R4, NDeal[1] + NDeal[3], NDeal[2], pdice);
                            } else {
                                R1_R3 = distCards & ~R4;
                            }
                            
                            assert(countCards(R1_R3 | detCards[1] | detCards[3]) == NOrg[1] + NOrg[3]);
                            
                            // 富豪-貧民系
                            R1 = CARDS_NULL;
                            R3 = CARDS_NULL;
                            if (!dist2Rest_64<1>(&R1_R3Rest, &R1, &R3, R1_R3, NOrg[1], NOrg[3], detCards[1], detCards[3], pdice)) continue;
                            Cards c = change<1>(infoClassPlayer[FUGO], R1, shared, ptools);
                            if (!holdsCards(c, R1_R3Rest) || (c & detCards[FUGO])) continue;
                            R1 -= c;
                            R3 |= c;
                            success = true;
                            break;
                        }
                        if (success) {
                            dst[myNum] = myCards;
                            dst[infoClassPlayer[FUGO     ]] = R1 & remCards;
                            dst[infoClassPlayer[HINMIN   ]] = R3 & remCards;
                            dst[infoClassPlayer[HEIMIN   ]] = R2 & remCards;
                            dst[infoClassPlayer[DAIHINMIN]] = R4 & remCards;
                            checkDeal(dst);
                        }
                    }
                    break;
                    case FUGO:
                    {
                        BitCards R0_R4, R0, R2, R3, R4, R0_R4Rest;

                        while (trials++ <= MAX_REJECTION) {
                            // セットされたウェイトに従って貧民に分配
                            R3 = detCards[HINMIN];
                            if (NDeal[HINMIN]) {
                                // Walker's Alias methodで献上下界を決めてランダム分配
                                Cards tmpDist = selectInWA(pdice->drand());
                                ASSERT(countCards(tmpDist) >= NDeal[HINMIN],
                                        cerr << OutCards(tmpDist) << " -> " << NDeal[HINMIN] << endl;);
                                R3 |= pickNBits64(tmpDist, NDeal[HINMIN], countCards(tmpDist) - NDeal[HINMIN], pdice);
                            }
                            assert(countCards(R3 & remCards) == NOwn[HINMIN]);
                            
                            // カードを大富豪-大貧民系と平民に分ける
                            assert(countCards(distCards & ~R3) == NDeal[DAIFUGO] + NDeal[DAIHINMIN] + NDeal[HEIMIN]);
                            
                            R2 = detCards[HEIMIN];
                            if (NDeal[HEIMIN]) {
                                R0_R4 = CARDS_NULL;
                                dist2_64(&R0_R4, &R2, distCards & ~R3, NDeal[DAIFUGO] + NDeal[DAIHINMIN], NDeal[HEIMIN], pdice);
                            } else {
                                R0_R4 = distCards & ~R3;
                            }
                            
                            assert(countCards(R0_R4 | detCards[DAIFUGO] | detCards[DAIHINMIN]) == NOrg[DAIFUGO] + NOrg[DAIHINMIN]);
                            
                            // 大富豪-大貧民系
                            R0 = CARDS_NULL; R4 = CARDS_NULL;
                            if (!dist2Rest_64<2>(&R0_R4Rest, &R0, &R4, R0_R4, NOrg[0], NOrg[4], detCards[0], detCards[4], pdice)) continue;
                            Cards c = change<2>(infoClassPlayer[DAIFUGO], R0, shared, ptools);
                            if (!holdsCards(c, R0_R4Rest) || (c & detCards[DAIFUGO])) continue;
                            R0 -= c;
                            R4 |= c;
                            success = true;
                            break;
                        }
                        if (success) {
                            dst[myNum] = myCards;
                            dst[infoClassPlayer[DAIFUGO  ]] = R0 & remCards;
                            dst[infoClassPlayer[HINMIN   ]] = R3 & remCards;
                            dst[infoClassPlayer[HEIMIN   ]] = R2 & remCards;
                            dst[infoClassPlayer[DAIHINMIN]] = R4 & remCards;
                            checkDeal(dst);
                        }
                    }
                    break;
                    case HEIMIN:
                    {
                        BitCards R0_R4, R1_R3, R0, R1, R3, R4, R0_R4Rest, R1_R3Rest;

                        while (trials++ <= MAX_REJECTION) {
                            // カードを大富豪-大貧民系と富豪-貧民系に分ける
                            R0_R4 = CARDS_NULL; R1_R3 = CARDS_NULL;
                            dist2_64(&R0_R4, &R1_R3, distCards, NDeal[0] + NDeal[4], NDeal[1] + NDeal[3], pdice);
                            
                            // 富豪-貧民系
                            R1 = CARDS_NULL;R3 = CARDS_NULL;
                            if (!dist2Rest_64<1>(&R1_R3Rest, &R1, &R3, R1_R3, NOrg[1], NOrg[3], detCards[1], detCards[3], pdice)) continue;
                            Cards c = change<1>(infoClassPlayer[FUGO], R1, shared, ptools);
                            if (!holdsCards(c, R1_R3Rest) || (c & detCards[FUGO])) continue;
                            R1 -= c;
                            R3 |= c;
                            
                            assert(countCards(R1 & remCards) == NOwn[FUGO  ]);
                            assert(countCards(R3 & remCards) == NOwn[HINMIN]);
                            
                            // 大富豪-大貧民系
                            R0 = CARDS_NULL; R4 = CARDS_NULL;
                            if (!dist2Rest_64<2>(&R0_R4Rest, &R0, &R4, R0_R4, NOrg[0], NOrg[4], detCards[0], detCards[4], pdice)) continue;
                            c = change<2>(infoClassPlayer[DAIFUGO], R0, shared, ptools);
                            if (!holdsCards(c, R0_R4Rest) || (c & detCards[DAIFUGO])) continue;
                            R0 -= c;
                            R4 |= c;
                            
                            assert(countCards(R0 & remCards) == NOwn[DAIFUGO  ]);
                            assert(countCards(R4 & remCards) == NOwn[DAIHINMIN]);
                            
                            success = true;
                            break;
                        }
                        if (success) {
                            dst[myNum] = myCards;
                            dst[infoClassPlayer[DAIFUGO  ]] = R0 & remCards;
                            dst[infoClassPlayer[FUGO     ]] = R1 & remCards;
                            dst[infoClassPlayer[HINMIN   ]] = R3 & remCards;
                            dst[infoClassPlayer[DAIHINMIN]] = R4 & remCards;
                            checkDeal(dst);
                        }
                    }
                    break;
                    case HINMIN:
                    {
                        BitCards R0_R2_R4, R0_R4, R0, R1, R2, R4, R0_R4Rest;

                        while (trials++ <= MAX_REJECTION) {
                            // カードを大富豪-大貧民系と富豪と平民に分ける
                            R1 = detCards[FUGO] | recvCards;
                            if (NDeal[FUGO]) {
                                R0_R2_R4 = CARDS_NULL;
                                dist2_64(&R0_R2_R4, &R1, distCards, NDeal[0] + NDeal[2] + NDeal[4], NDeal[1], pdice);
                                // 富豪の交換が実際に沿うか検証
                                Cards c = change<1>(infoClassPlayer[FUGO], R1, shared, ptools);
                                if (!holdsCards(detCards[3], c)) continue; // 矛盾
                                R1 -= c;
                            } else {
                                R0_R2_R4 = distCards;
                            }
                            
                            assert(countCards(R1 & remCards) == NOwn[FUGO]);
                            
                            R2 = detCards[HEIMIN];
                            if (NDeal[HEIMIN]) {
                                R0_R4 = CARDS_NULL;
                                dist2_64(&R0_R4, &R2, R0_R2_R4, NDeal[DAIFUGO] + NDeal[DAIHINMIN], NDeal[HEIMIN], pdice);
                            } else {
                                R0_R4 = R0_R2_R4;
                            }
                            
                            // 大富豪-大貧民系
                            R0 = CARDS_NULL; R4 = CARDS_NULL;
                            if (!dist2Rest_64<2>(&R0_R4Rest, &R0, &R4, R0_R4, NOrg[0], NOrg[4], detCards[0], detCards[4], pdice)) continue;
                            Cards c = change<2>(infoClassPlayer[DAIFUGO], R0, shared, ptools);
                            if (!holdsCards(c, R0_R4Rest) || (c & detCards[DAIFUGO])) continue; // 矛盾
                            R0 -= c;
                            R4 |= c;
                            success = true;
                            break;
                        }
                        
                        if (success) {
                            dst[myNum] = myCards;
                            dst[infoClassPlayer[DAIFUGO  ]] = R0 & remCards;
                            dst[infoClassPlayer[FUGO     ]] = R1 & remCards;
                            dst[infoClassPlayer[HEIMIN   ]] = R2 & remCards;
                            dst[infoClassPlayer[DAIHINMIN]] = R4 & remCards;
                            checkDeal(dst);
                        }
                    }
                    break;
                    case DAIHINMIN:
                    {
                        BitCards R1_R2_R3, R1_R3, R0, R1, R2, R3, R1_R3Rest;
                        
                        while (trials++ <= MAX_REJECTION) {
                            // カードを富豪-貧民系と大富豪と平民に分ける
                            R0 = detCards[DAIFUGO] | recvCards;
                            if (NDeal[DAIFUGO]) {
                                R1_R2_R3 = CARDS_NULL;
                                dist2_64(&R1_R2_R3, &R0, distCards, NDeal[1] + NDeal[2] + NDeal[3], NDeal[0], pdice);
                                // 大富豪の交換が実際に沿うか検証
                                Cards c = change<2>(infoClassPlayer[DAIFUGO], R0, shared, ptools);
                                if (!holdsCards(detCards[DAIHINMIN], c)) continue; // 矛盾
                                R0 -= c;
                            } else {
                                R1_R2_R3 = distCards;
                            }
                            
                            assert(countCards(R0 & remCards) == NOwn[DAIFUGO]);
                            
                            R2 = detCards[HEIMIN];
                            if (NDeal[HEIMIN]) {
                                R1_R3 = CARDS_NULL;
                                dist2_64(&R1_R3, &R2, R1_R2_R3, NDeal[FUGO] + NDeal[HINMIN], NDeal[HEIMIN], pdice);
                            } else {
                                R1_R3 = R1_R2_R3;
                            }
                            
                            // 富豪-貧民系
                            R1 = CARDS_NULL; R3 = CARDS_NULL;
                            if (!dist2Rest_64<1>(&R1_R3Rest, &R1, &R3, R1_R3, NOrg[1], NOrg[3], detCards[1], detCards[3], pdice)) continue;
                            Cards c = change<1>(infoClassPlayer[FUGO], R1, shared, ptools);
                            if (!holdsCards(c, R1_R3Rest) || (c & detCards[FUGO])) continue; // 矛盾
                            R1 -= c;
                            R3 |= c;
                            success = true;
                            break;
                        }
                        
                        if (success) {
                            dst[myNum] = myCards;
                            dst[infoClassPlayer[DAIFUGO]] = R0 & remCards;
                            dst[infoClassPlayer[FUGO   ]] = R1 & remCards;
                            dst[infoClassPlayer[HEIMIN ]] = R2 & remCards;
                            dst[infoClassPlayer[HINMIN ]] = R3 & remCards;
                            checkDeal(dst);
                        }
                    }
                    break;
                    default: UNREACHABLE; break; // 階級がおかしい
                }
                if (success) {
                    /*for (int c = 0; c < N; c++) {
                        dst[infoClassPlayer[c]] = R[c] & remCards;
                    }
                    dst[myNum] = myCards;
                    checkDeal(dst);*/
                } else {
                    DERR << "DEAL_REJEC FAILED..." << endl;
                    // 失敗の場合は逆関数法に変更
                    dealWithBias(dst, pdice);
                    return -1;
                }
                return 0;
            }
            
            /*template<class sharedData_t, class threadTools_t>
             int dealWithLikelifood(Cards *const dst, const sharedData_t& shared,
             threadTools_t *const ptools) {
             // 尤度計算
             // 主観的情報で明らかな矛盾のない配り方
             dealWithAbsSbjInfo(dst, &ptools->dice);
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
            template <int C_QTY, class sharedData_t, class threadTools_t>
            Cards change(const int p, const Cards cards,
                         const sharedData_t& shared,
                         threadTools_t *const ptools) const {
                Cards cand[(C_QTY == 1) ? 12 : 78];
                int NCands = genChange(cand, cards, C_QTY);
                Field field;
                // 交換方策によって交換してみる
                int index = changeWithPolicy(cand, NCands, cards, C_QTY, field, shared.baseChangePolicy, &ptools->dice);
                return cand[index];
            }
            
            Cards selectInWA(double urand) const {
                double v = urand * candidatesInWA;
                int k = (int)v;
                double u = 1 + k - v;
                if (u < thresholdInWA[k]) return distCardsUnderInWA[k];
                else return distCardsOverInWA[k];
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
                    //cerr << "presented lower bound candidate = " << OutCards(tmp) << endl;
                    int index = 0;
                    while (tmp) {
                        const IntCard ic = popIntCardLow(&tmp);
                        // ic が献上によって得られたカードの下界だった場合のパターン数を計算
                        const Cards c = IntCardToCards(ic);
                        const Cards lowerDist = pickLower(c) & distCards;
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
                        distCardsUnderInWA[index] = distCards & pickLower(IntCardToCards(ic));
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
                        distCardsOverInWA[l] = distCardsUnderInWA[g];
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
            
            void dealPart_AbsSbjInfo() {
                // 自分以外の未使用カード
                distCards = maskCards(remCards, myCards);
                
                // 自分
                detCards[myClass] |= myCards;
                NDeal.assign(myClass, 0);
                NDet.assign_part(myClass, NOrg);
                
                // 初手がすでに済んでいる場合、初手プレーヤーにD3
                if (!phase.isInChange()
                    && turnNum > 0
                    && distCards.contains(INTCARD_D3)) {
                    detCards[firstTurnPlayerClass] |= CARDS_D3;
                    distCards -= CARDS_D3;
                    NDeal.minus(firstTurnPlayerClass, 1);
                    NDet.plus(firstTurnPlayerClass, 1);
                }
                if (!flag.test(0) && !phase.isInChange()) {
                    if (myClass < MIDDLE) { // 自分が上位のとき
                        // 交換であげたカードのうちまだ確定扱いでないもの
                        if (!phase.isInChange()) { // 交換時はまだ不明
                            Cards sCards = distCards & sentCards;
                            if (sCards) {
                                int nc = countCards(sCards);
                                int myChangePartnerClass = getChangePartnerClass(myClass);
                                detCards[myChangePartnerClass] |= sCards;
                                distCards -= sCards;
                                NDeal.minus(myChangePartnerClass, nc);
                                NDet.plus(myChangePartnerClass, nc);
                            }
                        }
                    }
                }
                
                // 交換で自分のもらったカードの枚数番目までの高ランクカードは渡さない(ヒューリスティック信念)
                
                // 結局配る枚数
                NDistCards = countCards(distCards);
                ASSERT(NDistCards == NDeal.sum(), cerr << NDistCards << " " << NDeal << " " << NDeal.sum() << endl;);
                
                // 初手がすでに済んでいる段階では、自分と、自分以外で全てのカードが明らかになっていないプレーヤーは着手を検討する必要がある
                if (!phase.isInChange() && turnNum > 0) {
                    // HA設定
                    uint32_t NOppUsedCards = countCards(maskCards(CARDS_ALL, addCards(remCards, detCards[infoClass[myNum]]))); // 他人が使用したカード枚数
                    // 1 -> ... -> max -> ... -> 1 と台形に変化
                    HARate = (NOppUsedCards > 0) ? min({ HARATE_MAX, (HARATE_MAX - 1) * NOppUsedCards / 4 + 1, (HARATE_MAX - 1) * NDistCards / 16 + 1 }) : 1;
                    
                    playFlag.reset();
                    for (int p = 0; p < N; p++) {
                        if (p != myNum && NDeal[infoClass[p]] > 0) playFlag.set(p);
                    }
                } else {
                    HARate = 1;
                }
                for (int p = 0; p < N; p++) {
                    DERR << "r:" << p << " p:" << infoClassPlayer[p] << " Org:" << NOrg[p] << " Own:" << NOwn[p]
                    << " Det:" << NDet[p] << " Deal:" << NDeal[p] << " " << OutCards(detCards[p]) << endl;
                }
            }
            
            template <class sharedData_t, class threadTools_t>
            double calcPlayLikelihood(Cards *const c, const sharedData_t& shared, threadTools_t *const ptools) const {
                
                // 想定した手札配置から、試合進行がどの程度それっぽいか考える
                // 計算時間解析は、各プレーについて検討
                
                // 時間解析するかどうか
                const int by_time = shared.estimating_by_time;
                
                // 対数尤度
                double playLH = 0;
                
                BitSet32 pwFlag;
                std::array<Cards, N> orgCards;
                BitSet32 tmpPlayFlag = playFlag;
                MoveInfo *const mv = ptools->buf;
                
                pwFlag.reset();
                for (int p = 0; p < N; p++) orgCards[p] = c[p] | detCards[infoClass[p]];
                
                Field field;
                const auto& gLog = shared.matchLog.latestGame();
                iterateGameLogInGame
                (field, gLog, gLog.plays(), orgCards,
                 // after change callback
                 [](const auto& field)->void{},
                 // play callback
                 [&pwFlag, &playLH, &orgCards, mv, tmpPlayFlag, by_time, &shared]
                 (const auto& field, const auto& chosenMove, uint32_t usedTime)->int{
                     const uint32_t tp = field.getTurnPlayer();
                     
                     const Cards usedCards = chosenMove.cards();
                     const Board bd = field.getBoard();
                     const Cards myCards = field.getCards(tp);
                     const Hand& myHand = field.getHand(tp);
                     const Hand& opsHand = field.getOpsHand(tp);
                     
                     if (!holdsCards(myCards, usedCards)) return -1; // 終了(エラー?)
                     if (tmpPlayFlag.test(tp)) {
                         // カードが全確定しているプレーヤー(主に自分と、既に上がったプレーヤー)については考慮しない
                         
                         // 場の情報をまとめる
                         const int NMoves = genMove(mv, myHand, bd);
                         assert(NMoves > 0);
                         
                         if (NMoves > 1) {
                             for (int m = 0; m < NMoves; m++) {
                                 bool mate = checkHandMate(0, mv + NMoves, mv[m], myHand, opsHand, bd, field.fieldInfo);
                                 if (mate) mv[m].setMPMate();
                             }
                         }
                         
                         // フェーズ(空場0、通常場1、パス支配場2)
                         const int ph = bd.isNF() ? 0 : (field.fieldInfo.isPassDom()? 2 : 1);
                         // プレー尤度計算
                         if (NMoves > 1) {
                             // search move
                             int chosenIdx = searchMove(mv, NMoves, [chosenMove](const auto& tmp)->bool{
                                 return tmp.meldPart() == chosenMove.meldPart();
                             });
                             
                             if (chosenIdx == -1) { // 自分の合法手生成では生成されない手が出された
                                 playLH += log(1 / (double)(NMoves + 1));
                             } else {
                                 std::array<double, N_MAX_MOVES> score;
                                 calcPlayPolicyScoreSlow<0>(score.data(), mv, NMoves, field, shared.basePlayPolicy);
                                 // Mateの手のスコアを設定
                                 double maxScore = *std::max_element(score.begin(), score.begin() + NMoves);
                                 for (int m = 0; m < NMoves; m++) {
                                     if (mv[m].isMate())score[m] = maxScore + 4;
                                 }
                                 SoftmaxSelector selector(score.data(), NMoves, Settings::simulationTemperaturePlay);
                                 selector.to_prob();
                                 if (selector.sum != 0) {
                                     playLH += log(max(selector.prob(chosenIdx), 1 / 256.0));
                                 } else { // 等確率とする
                                     playLH += log(1 / (double)NMoves);
                                 }
                             }
                         }
                     }
                     return 0;
                });
                return playLH;
            }
        };
        
        template<>AtomicAnalyzer<5, 1, 0> RandomDealer<N_PLAYERS>::ana("RandomDealer");
    }
}
