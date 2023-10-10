
// 相手手札推定のテスト

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
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
        key |= fill_bits<uint64_t, N_PLAYERS>(1ULL << p) & f.hand[p].key;
    }
    return key;
}

void testEstimationRate(const MatchRecord& match, DealType type, PlayerModel *pmodel = nullptr) {
    shared.initMatch(-1);
    if (pmodel != nullptr) shared.playerModel = *pmodel;
    tools.dice.srand(1);
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
                    Cards sameCards = field.hand[p].cards & worlds[j].cards[p];
                    tsame += sameCards.count();
                    tall += field.hand[p].qty;
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
                        tall_gen += field.hand[p].qty;
                    }
                    epcnt++;
                    if (tsame_gen == tall_gen) perfect_gen++;
                    same_cnt_gen += tsame_gen;
                    same_ratio_gen += double(tsame_gen) / tall_gen;
                }
            }
        }
    }
    cerr << "type " << type << ": ";
    cerr << perfect << " / " << cnt << " (" << double(perfect) / cnt << ") ";
    cerr << "same " << same_cnt / cnt << " (" << same_ratio / cnt << ") ";
    cerr << "perf-gen " << double(perfect_gen) / epcnt;
    cerr << " same-gen " << same_cnt_gen / epcnt << " (" << same_ratio_gen / epcnt << ") ";
    cerr << "in " << time / cnt << " clock" << endl;
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
        testEstimationRate(match, DealType::REJECTION, pmodel);
    }

    return 0;
}