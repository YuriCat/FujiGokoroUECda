
// 相手手札推定のテスト

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../core/dominance.hpp"
#include "../engine/data.hpp"
#include "../engine/estimation.hpp"
#include "test.h"

using namespace std;

static SharedData shared;
static ThreadTools tools;
static Clock cl;


uint64_t worldKey(const Field& f) {
    uint64_t key = 0;
    for (int p = 0; p < N_PLAYERS; p++) {
        key |= fill_bits<uint64_t, N_PLAYERS>(1ULL << p) & CardsToHashKey(f.getCards(p));
    }
    return key;
}

int cardIndex(IntCard ic) {
    return ic == INTCARD_JOKER ? (N_CARDS - 1) : (ic - INTCARD_MIN);
}

void testEstimationRate(const MatchRecord& match, DealType type, PlayerModel *pmodel = nullptr) {
    shared.initMatch(-1);
    if (pmodel != nullptr) shared.playerModel = *pmodel;
    tools.dice.seed(1);
    mt19937 dice(0);

    long long time = 0;
    long long cnt = 0;
    long long epcnt = 0;

    // 正解との一致
    long long perfect = 0;
    double same_cnt = 0, same_ratio = 0;

    // 生成された配置内での一致
    double same_cnt_gen = 0, same_ratio_gen = 0;
    long long perfect_gen = 0;

    // 札分布
    array<long long, N_CARDS> distribution[N_PLAYERS] = {0};
    array<long long, N_CARDS> realDistribution[N_PLAYERS] = {0};

    // 手札の性質
    long long handCount = 0;
    long long eachRankDist[2][5] = {0};
    long long plainSeq[2] = {0}, jokerSeq[2] = {0};

    // 支配
    long long dominanceMatrix[2][2] = {0};

    for (const auto& game : match.games) {
        shared.initGame();
        int tc = dice() % game.plays.size();
        Field field;
        for (Move move : PlayRoller(field, game)) {
            if (field.numPlayersAlive() <= 2) continue;
            //cerr << field.toDebugString() << endl;
            // この手の前までの情報から手札推定を行う
            RandomDealer estimator(field, field.turn());
            World worlds[2];
            // 一致度計測
            for (int j = 0; j < 2; j++) {
                cl.start();
                worlds[j] = estimator.create(type, game, shared, &tools);
                time += cl.stop();
                cnt++;
                int tsame = 0, tall = 0;
                for (int p = 0; p < N_PLAYERS; p++) {
                    if (p == field.turn() || !field.isAlive(p)) continue;
                    Cards sameCards = field.getCards(p) & worlds[j].cards[p];
                    tsame += sameCards.count();
                    tall += field.numCardsOf(p);
                }
                if (tsame == tall) perfect++;
                same_cnt += tsame;
                same_ratio += double(tsame) / tall;
            }
            // 生成された配置内の類似度
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < i; j++) {
                    int tsame_gen = 0, tall_gen = 0;
                    for (int p = 0; p < N_PLAYERS; p++) {
                        if (p == field.turn() || !field.isAlive(p)) continue;
                        Cards sameCards = worlds[i].cards[p] & worlds[j].cards[p];
                        tsame_gen += sameCards.count();
                        tall_gen += field.numCardsOf(p);
                    }
                    epcnt++;
                    if (tsame_gen == tall_gen) perfect_gen++;
                    same_cnt_gen += tsame_gen;
                    same_ratio_gen += double(tsame_gen) / tall_gen;
                }
            }
            // 階級ごとの統計
            if (!field.isInitGame()) {
                for (int i = 0; i < 2; i++) {
                    for (int p = 0; p < N_PLAYERS; p++) {
                        if (p == field.turn() || !field.isAlive(p)) continue;
                        Cards dealt = worlds[i].cards[p];
                        for (IntCard ic : dealt) distribution[field.classOf(p)][cardIndex(ic)]++;
                        Cards real = field.getCards(p);
                        for (IntCard ic : real) realDistribution[field.classOf(p)][cardIndex(ic)]++;
                    }
                }
            }
            // 手札の性質の統計
            for (int i = 0; i < 2; i++) {
                for (int p = 0; p < N_PLAYERS; p++) {
                    if (p == field.turn() || !field.isAlive(p)) continue;
                    handCount++;
                    Cards dealt = worlds[i].cards[p];
                    Cards real = field.getCards(p);
                    for (int r = RANK_3; r <= RANK_2; r++) eachRankDist[0][popcnt(dealt[r])]++;
                    for (int r = RANK_3; r <= RANK_2; r++) eachRankDist[1][popcnt(real[r])]++;
                    if (canMakeSeq(dealt.plain(), 0, 3)) plainSeq[0]++;
                    if (canMakeSeq(dealt.plain(), dealt.joker(), 3)) jokerSeq[0]++;
                    if (canMakeSeq(real.plain(), 0, 3)) plainSeq[1]++;
                    if (canMakeSeq(real.plain(), real.joker(), 3)) jokerSeq[1]++;
                }
            }
            // 局面の統計
            for (int i = 0; i < 2; i++) {
                // 支配性
                bool dominant = true, realDominant = true;
                for (int p = 0; p < N_PLAYERS; p++) {
                    if (p == field.turn() || !field.isAwake(p)) continue;
                    if (!dominatesCards(move, worlds[i].cards[p], field.board)) dominant = false;
                    if (!dominatesCards(move, field.getCards(p), field.board)) realDominant = false;
                }
                dominanceMatrix[realDominant][dominant]++;
            }
        }
    }
    cerr << "type " << type << ": ";
    cerr << perfect << " / " << cnt << " (" << double(perfect) / cnt << ") ";
    cerr << "same " << same_cnt / cnt << " (" << same_ratio / cnt << ") ";
    cerr << "perf-gen " << double(perfect_gen) / epcnt;
    cerr << " same-gen " << same_cnt_gen / epcnt << " (" << same_ratio_gen / epcnt << ") ";
    cerr << "in " << time / cnt << " clock" << endl;

    for (int c = 0; c < N_PLAYERS; c++) {
        auto sum = accumulate(distribution[c].begin(), distribution[c].end(), 0LL);
        cerr << "D";
        for (int i = 0; i < N_CARDS; i++) {
            cerr << " " << std::setw(2) << int(1000 * distribution[c][i] / sum);
        }
        cerr << endl;
        sum = accumulate(realDistribution[c].begin(), realDistribution[c].end(), 0LL);
        cerr << "R";
        for (int i = 0; i < N_CARDS; i++) {
            cerr << " " << std::setw(2) << int(1000 * realDistribution[c][i] / sum);
        }
        cerr << endl;
    }

    cerr << "rank qty D ";
    for (int q = 1; q <= 4; q++) cerr << eachRankDist[0][q] / double(handCount) << " ";
    cerr << "R ";
    for (int q = 1; q <= 4; q++) cerr << eachRankDist[1][q] / double(handCount) << " ";
    cerr << "seq ratio";
    cerr << " D " << plainSeq[0] / double(handCount) << " " << jokerSeq[0] / double(handCount);
    cerr << " R " << plainSeq[1] / double(handCount) << " " << jokerSeq[1] / double(handCount);
    cerr << endl;

    cerr << "dominance =" << endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            cerr << dominanceMatrix[i][j] << " ";
        }
        cerr << endl;
    }
}

bool EstimationTest(const vector<string>& recordFiles, PlayerModel *pmodel) {
    shared.baseChangePolicy.bin(DIRECTORY_PARAMS_IN + "change_policy.bin");
    shared.basePlayPolicy.bin(DIRECTORY_PARAMS_IN + "play_policy.bin");
    loadEstimationParams(DIRECTORY_PARAMS_IN + "est_score.bin");

    for (string rf : recordFiles) {
        MatchRecord match(rf);
        testEstimationRate(match, DealType::RANDOM);
        testEstimationRate(match, DealType::SBJINFO);
        testEstimationRate(match, DealType::BIAS);
        testEstimationRate(match, DealType::NEW_BIAS);
        testEstimationRate(match, DealType::REJECTION);
        if (pmodel->trained) testEstimationRate(match, DealType::REJECTION, pmodel);
    }

    return 0;
}