
// 相手手札推定のテスト

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/data.hpp"
#include "../engine/estimation.hpp"
#include "test.h"

using namespace std;

static SharedData shared;
static ThreadTools tools[16];
static Clock cl;

void testEstimationRate(const MatchRecord& mrecord, DealType type) {
    Field field;

    shared.initMatch();
    for (int i = 0; i < 16; i++) {
        tools[i].init(i);
        tools[i].dice.srand(1 + i);
    }
    mt19937 dice(0);

     // カード一致率
    long long cnt = 0;
    long long same = 0, all = 0;
    long long time = 0;

    long long perfect = 0; // 完全一致率

    double entropy = 0;
    double cross_entropy = 0;
    long long ecnt = 0;

    for (int i = 0; i < mrecord.games(); i++) {
        shared.initGame();
        const auto& grecord = mrecord.game(i);
        int tc = dice() % grecord.plays();
        iterateGameLogAfterChange
        (field, grecord,
        [](const Field& field)->void{}, // first callback
        [&](const Field& field, Move pl, uint32_t tm)->int{ // play callback
            if (field.getNAlivePlayers() <= 2) return 0;
            //cerr << field.toDebugString() << endl;
            // この手の前までの情報から手札推定を行う
            RandomDealer estimator(field, field.turn());
            // 一致度計測
            ImaginaryWorld world;
            for (int j = 0; j < 2; j++) {
                cl.start();
                estimator.create(&world, type, grecord, shared, &tools[0]);
                time += cl.stop();
                cnt++;
                int tsame = 0, tall = 0;
                for (int p = 0; p < N_PLAYERS; p++) {
                    if (p == field.turn() || !field.isAlive(p)) continue;
                    Cards sameCards = field.hand[p].cards & world.cards[p];
                    tsame += sameCards.count();
                    tall += field.hand[p].qty;
                }
                if (tsame == tall) perfect++;
                same += tsame; all += tall;
            }
            // 多様性計測
            if (field.turnCount() == tc) {
                ecnt++;
                ImaginaryWorld worlds[256];
                for (int i = 0; i < 256; i++) {
                    estimator.create(&worlds[i], type, grecord, shared, &tools[0]);
                }
                // 局面の多様性を計算
                int own[64][N_PLAYERS] = {0};
                for (int i = 0; i < 256; i++) {
                    for (int ic = 0; ic < 64; ic++) {
                        for (int p = 0; p < N_PLAYERS; p++) {
                            if (worlds[i].cards[p].contains(IntCard(ic))) {
                                own[ic][p]++;
                            }
                        }
                    }
                }
                // 各カードごとの情報量を計算
                double e = 0, ce = 0;
                for (int ic = 0; ic < 64; ic++) {
                    int sum = 0;
                    for (int p = 0; p < N_PLAYERS; p++) sum += own[ic][p];
                    for (int p = 0; p < N_PLAYERS; p++) {
                        double prob = (own[ic][p] + 1.0 / N_PLAYERS) / double(sum + 1);
                        e += -prob * log2(prob);
                        if (field.getCards(p).contains(IntCard(ic))) ce += -log2(prob);
                    }
                }
                entropy += e;
                cross_entropy += ce;
            }
            return 0;
        },
        [](const Field& field)->void{}); // last callback
    }
    cerr << "type " << type << ": ";
    cerr << perfect << " / " << cnt << " (" << double(perfect) / cnt << ") ";
    cerr << same << " / " << all << " (" << double(same) / all << ") ";
    cerr << "entropy " << entropy / ecnt << " centropy " << cross_entropy / ecnt;
    cerr << " in " << time / cnt << " clock" << endl;
}

bool EstimationTest(const vector<string>& recordFiles) {

    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");

    for (string rf : recordFiles) {
        MatchRecord mrecord(rf);
        testEstimationRate(mrecord, DealType::RANDOM);
        testEstimationRate(mrecord, DealType::SBJINFO);
        testEstimationRate(mrecord, DealType::BIAS);
        testEstimationRate(mrecord, DealType::REJECTION);
    }
    
    return 0;
}