
// 相手手札推定のテスト

#include "../UECda.h"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/engineSettings.h"
#include "../engine/linearPolicy.hpp"
#include "../engine/engineStructure.hpp"
#include "../engine/estimation.hpp"

using namespace UECda;
using namespace std;

EngineSharedData shared;
EngineThreadTools tools;
Clock cl;

void testEstimationRate(const MatchRecord& mrecord, DealType type) {
    Field field;

    shared.initMatch();
    tools.init(0);
    tools.dice.srand(1);
    mt19937 dice(0);

    long long cnt = 0;
    long long same = 0, all = 0;
    long long time = 0;

    double entropy = 0;
    long long ecnt = 0;

    for (int i = 0; i < mrecord.games(); i++) {
        shared.initGame();
        const auto& grecord = mrecord.game(i);
        int tc = dice() % grecord.plays();
        iterateGameLogAfterChange
        (field, grecord,
        [](const auto& field)->void{}, // first callback
        [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
            if (field.getNAlivePlayers() <= 2) return 0;
            //cerr << field.toDebugString() << endl;
            // この手の前までの情報から手札推定を行う
            RandomDealer<decltype(grecord)> estimator(grecord, field, field.turn());
            // 一致度計測
            ImaginaryWorld world;
            for (int j = 0; j < 8; j++) {
                cl.start();
                estimator.create(&world, type, shared, &tools);
                time += cl.stop();
                cnt++;
                for (int p = 0; p < N_PLAYERS; p++) {
                    if (p == field.turn() || !field.isAlive(p)) continue;
                    Cards sameCards = field.hand[p].cards & world.cards[p];
                    same += sameCards.count();
                    all += field.hand[p].qty;
                }
            }
            // 多様性計測
            if (field.turnCount() == tc) {
                ecnt++;
                ImaginaryWorld worlds[256];
                for (int i = 0; i < 256; i++) {
                    estimator.create(&worlds[i], type, shared, &tools);
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
                double e = 0;
                for (int ic = 0; ic < 64; ic++) {
                    int sum = 0;
                    for (int p = 0; p < N_PLAYERS; p++) sum += own[ic][p];
                    for (int p = 0; p < N_PLAYERS; p++) {
                        if (own[ic][p] > 0) {
                            double prob = own[ic][p] / double(sum);
                            if (own[ic][p] > 0) e += -prob * log2(prob);
                        }
                    }
                }
                entropy += e;
            }
            return 0;
        },
        [](const auto& field)->void{}); // last callback
    }
    cerr << "type " << type << ": " << same << " / " << all << " (" << double(same) / all << ") ";
    cerr << "entropy " << entropy / ecnt << " in " << time / cnt << " clock" << endl;
}

int main(int argc, char* argv[]) {
    vector<string> logFileNames;
    
    for (int c = 1; c < argc; c++) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = string(argv[c + 1]);
        } else if (!strcmp(argv[c], "-l")) { // log path
            logFileNames.push_back(string(argv[c + 1]));
        }
    }

    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");

    for (const string& log : logFileNames) {
        MatchRecord mrecord(log);
        testEstimationRate(mrecord, DealType::RANDOM);
        testEstimationRate(mrecord, DealType::SBJINFO);
        testEstimationRate(mrecord, DealType::BIAS);
        testEstimationRate(mrecord, DealType::REJECTION);
    }
    
    return 0;
}
