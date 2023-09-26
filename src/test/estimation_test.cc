
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

long long factorial(int n) { return n > 1 ? n * factorial(n - 1) : 1; }

int patternCount(const Field& field) {
    if (field.opsHand[field.turn()].qty > 10) return 10000;
    long long f = factorial(field.opsHand[field.turn()].qty);
    for (int p = 0; p < N_PLAYERS; p++) if (p != field.turn()) f /= factorial(field.hand[p].qty);
    return f;
}

void testEstimationRate(const MatchRecord& match, DealType type) {
    shared.initMatch(-1);
    for (int i = 0; i < 16; i++) {
        tools[i].dice.srand(1 + i);
    }
    mt19937 dice(0);

    long long time = 0;
    long long cnt = 0;
    long long ecnt = 0;
    long long epcnt = 0;

    // 正解との一致
    long long perfect = 0;
    double same_cnt = 0, same_ratio = 0;

    // 生成された配置内での一致
    double same_cnt_gen = 0, same_ratio_gen = 0;
    long long perfect_gen = 0;

    // 情報量
    double total_entropy = 0, total_cross_entropy = 0;
    double entropy = 0, cross_entropy = 0;

    for (const auto& game : match.games) {
        shared.initGame();
        int tc = dice() % game.plays.size();
        Field field;
        for (Move move : PlayRoller(field, game)) {
            if (field.numPlayersAlive() <= 2) continue;
            //cerr << field.toDebugString() << endl;
            // この手の前までの情報から手札推定を行う
            RandomDealer estimator(field, field.turn());
            // 最終局面チェック
            if (type == DealType::REJECTION && patternCount(field) <= 12) {
                constexpr int N = 64;
                World worlds[N];
                map<uint64_t, double> likelihoodMap, countMap;
                for (int i = 0; i < N; i++) {
                   worlds[i] = estimator.create(type, game, shared, &tools[0]);
                   countMap[worlds[i].key]++;
                   likelihoodMap[worlds[i].key] += worlds[i].likelihood;
                }
                cerr << patternCount(field) << endl;
                for (auto v : likelihoodMap) {
                    cerr << v.first << " " << countMap[v.first] << " " << v.second
                    << " (" << v.second / countMap[v.first] << ") "
                    <<  (v.first == worldKey(field) ? " <-" : "") << endl;
                }
                //getchar();
            }
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
                same_cnt += tsame;
                same_ratio += double(tsame) / tall;
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
                // 生成された配置内の類似度
                for (int i = 0; i < N; i++) {
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
        }
    }
    cerr << "type " << type << ": ";
    cerr << perfect << " / " << cnt << " (" << double(perfect) / cnt << ") ";
    cerr << "same " << same_cnt / cnt << " (" << same_ratio / cnt << ") ";
    cerr << "perf-gen " << double(perfect_gen) / epcnt;
    cerr << " same-gen " << same_cnt_gen / epcnt << " (" << same_ratio_gen / epcnt << ") ";
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