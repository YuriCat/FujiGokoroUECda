#include "../settings.h"
#include "estimation.hpp"
#include "simulation.hpp"
#include "monteCarlo.hpp"

using namespace std;

namespace Settings {
    const double valuePerClock = 5.0 / (THINKING_LEVEL * THINKING_LEVEL) / pow(10.0, 10);
    // 時間の価値(1秒あたり),3191は以前のPCのクロック周波数(/microsec)なので意味は無い
    const double valuePerSec = valuePerClock * 3191 * pow(10.0, 6);
}

int selectBanditAction(const RootInfo& root, Dice& dice) {
    // バンディット手法により次に試す行動を選ぶ
    int actions = root.candidates;
    const auto& a = root.child;
    if (actions == 2) {
        // 2つの時は同数(分布サイズ単位)に割り振る
        if (a[0].size() == a[1].size()) return dice() % 2;
        else return a[0].size() < a[1].size() ? 0 : 1;
    } else {
        // UCB-root アルゴリズム
        int index = 0;
        double bestScore = -DBL_MAX;
        double sqAllSize = sqrt(root.monteCarloAllScore.size());
        for (int i = 0; i < actions; i++) {
            double score;
            if (a[i].simulations < 4) {
                // 最低プレイアウト数をこなしていないものは、大きな値にする
                // ただし最低回数のもののうちどれかがランダムに選ばれるようにする
                score = (double)((1U << 16) - (a[i].simulations << 8) + (dice() % (1U << 6)));
            } else {
                score = a[i].mean() + 0.7 * sqrt(sqAllSize / a[i].size()); // ucbr値
            }
            if (score > bestScore) {
                bestScore = score;
                index = i;
            }
        }
        return index;
    }
}

bool finishCheck(const RootInfo& root, double simuTime, Dice& dice) {
    // Regretによる打ち切り判定

    const int candidates = root.candidates; // 候補数
    auto& child = root.child;
    double rewardScale = root.rewardGap;

    struct Dist { double mean, sem, reg; };
    double regretThreshold = 1600.0 * double(2 * simuTime * Settings::valuePerSec) / rewardScale;

    // regret check
    Dist d[N_MAX_MOVES];
    for (int i = 0; i < candidates; i++) {
        d[i] = {child[i].mean(), sqrt(child[i].mean_var()), 0};
    }
    for (int t = 0; t < 1600; t++) {
        double bestValue = -100;
        double value[N_MAX_MOVES];
        for (int i = 0; i < candidates; i++) {
            const Dist& td = d[i];
            std::normal_distribution<double> nd(td.mean, td.sem);
            value[i] = nd(dice);
            bestValue = max(bestValue, value[i]);
        }
        for (int i = 0; i < candidates; i++) {
            d[i].reg += bestValue - value[i];
        }
    }
    for (int i = 0; i < candidates; i++) {
        if (d[i].reg < regretThreshold) return true;
    }
    return false;
}

void MonteCarloThread(const int threadId, const int numThreads,
                      RootInfo *const proot, const Field *const pfield,
                      SharedData *const pshared, ThreadTools *const ptools) {
    const int myPlayerNum = proot->myPlayerNum;
    auto& dice = ptools->dice;

    int numSimulations[N_MAX_MOVES] = {0};
    int numSimulationsSum = 0;

    int numWorlds = 0; // 作成した世界の数
    std::array<World, 128> worlds;

    // 世界生成のためのクラスを初期化
    const auto& record = pshared->record.latestGame();
    RandomDealer estimator(*pfield, myPlayerNum);

    Field pf = *pfield;
    pf.myPlayerNum = -1; // 客観視点に変更
    pf.addAttractedPlayer(myPlayerNum);
    if (proot->rivalPlayerNum >= 0) {
        pf.addAttractedPlayer(proot->rivalPlayerNum);
    }

    uint64_t simuTime = 0ULL; // プレイアウトと雑多な処理にかかった時間
    uint64_t estTime = 0ULL; // 局面推定にかかった時間

    // 諸々の準備が終わったので時間計測開始
    ClockMicS clock(0);

    while (!proot->exitFlag) { // 最大で最高回数までプレイアウトを繰り返す

        int world = 0;
        int action = selectBanditAction(*proot, dice);

        if (numSimulations[action] < numWorlds) {
            // まだ全ての世界でこの着手を検討していない
            world = numSimulations[action];
        } else if (numThreads * numWorlds + threadId < (int)worlds.size()) {
            // 新しい世界を作成
            simuTime += clock.restart();
            worlds[numWorlds] = estimator.create(DealType::REJECTION, record, *pshared, ptools);
            estTime += clock.restart();
            world = numWorlds++;
        } else {
            // 順番に選択
            world = numSimulations[action] % numWorlds;
        }

        numSimulations[action]++;
        numSimulationsSum++;

        // シミュレーション実行
        Field f = pf;
        setWorld(worlds[world], &f);
        if (proot->isChange) {
            startChangeSimulation(f, myPlayerNum, proot->child[action].changeCards, pshared, ptools);
        } else {
            startPlaySimulation(f, proot->child[action].move, pshared, ptools);
        }

        proot->feedSimulationResult(action, f, pshared); // 結果をセット(排他制御は関数内で)
        simuTime += clock.restart();

        // 終了判定
        if (Settings::fixedSimulationCount < 0
            && threadId == 0
            && numSimulationsSum % max(4, 32 / numThreads) == 0
            && proot->allSimulations > proot->candidates * 4) {
            if (proot->exitFlag) break;
            if (finishCheck(*proot, simuTime * 1e-6, dice)) {
                proot->exitFlag = true;
                break;
            }
        }
    }
}