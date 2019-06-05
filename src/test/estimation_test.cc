
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


uint64_t worldKey(const Field& f) {
    uint64_t key = 0;
    for (int p = 0; p < N_PLAYERS; p++) {
        key |= fill_bits<uint64_t, N_PLAYERS>(1ULL << p) & f.hand[p].key;
    }
    return key;
}

void testEstimationRate(const MatchRecord& match, DealType type) {
    shared.initMatch(-1);
    for (int i = 0; i < 16; i++) {
        tools[i].dice.srand(1 + i);
    }
    mt19937 dice(0);

     // カード一致率
    long long cnt = 0;
    long long same = 0, all = 0;
    long long time = 0;

    long long perfect = 0; // 完全一致率
    double total_entropy = 0, total_cross_entropy = 0;

    double entropy = 0, cross_entropy = 0;
    long long ecnt = 0;

    for (const auto& game : match.games) {
        shared.initGame();
        int tc = dice() % game.plays.size();
        Field field;
        iterateGameLogAfterChange
        (field, game,
        [](const Field& field)->void{}, // first callback
        [&](const Field& field, Move pl, uint32_t tm)->int{ // play callback
            if (field.numPlayersAlive() <= 2) return 0;
            //cerr << field.toDebugString() << endl;
            // この手の前までの情報から手札推定を行う
            RandomDealer estimator(field, field.turn());
            // 一致度計測
            for (int j = 0; j < 2; j++) {
                cl.start();
                World world = estimator.create(type, game, shared, &tools[0]);
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
                constexpr int N = 256;
                World worlds[N];
                map<uint64_t, double> worldKeyMap;
                for (int i = 0; i < N; i++) {
                    worlds[i] = estimator.create(type, game, shared, &tools[0]);
                    worldKeyMap[worlds[i].key]++;
                }
                // 全体の情報量をまとめる
                double total_e = 0, total_ce = 0;
                for (auto& val : worldKeyMap) {
                    double prob = val.second / N;
                    total_e += -prob * log2(prob);
                    if (val.first == worldKey(field)) total_ce += -log2(prob);
                }
                total_entropy += total_e;
                total_cross_entropy += total_ce;
                // カードごとの配置確率をまとめる
                int own[64][N_PLAYERS] = {0};
                for (int i = 0; i < N; i++) {
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
                        if (p == field.turn()) continue;
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
        MatchRecord match(rf);
        testEstimationRate(match, DealType::RANDOM);
        testEstimationRate(match, DealType::SBJINFO);
        testEstimationRate(match, DealType::BIAS);
        testEstimationRate(match, DealType::REJECTION);
    }
    
    return 0;
}