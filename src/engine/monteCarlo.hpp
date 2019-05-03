#pragma once

#include "engineSettings.h"
#include "data.hpp"
#include "estimation.hpp"
#include "simulation.hpp"

// マルチスレッディングのときはスレッド、
// シングルの時は関数として呼ぶ

template <class dice_t>
static int selectBanditAction(const RootInfo& root, dice_t& dice) {
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

template <class dice_t>
static bool finishCheck(const RootInfo& root, double simuTime, dice_t& dice) {
    // Regretによる打ち切り判定

    const int candidates = root.candidates; // 候補数
    auto& child = root.child;
    double rewardScale = root.rewardGap;

    struct Dist { double mean, sem, reg; };
    double line = -1600.0 * double(2 * simuTime * VALUE_PER_SEC) / rewardScale;
    
    // regret check
    Dist d[N_MAX_MOVES + 64];
    for (int m = 0; m < candidates; m++) {
        d[m].reg = 0.0;
        d[m].mean = child[m].mean();
        d[m].sem = sqrt(child[m].mean_var()); // 推定平均値の分散
    }
    for (int t = 0; t < 1600; t++) {
        double tmpBest = -1.0;
        double tmpScore[256];
        for (int m = 0; m < candidates; m++) {
            const Dist& tmpD = d[m];
            std::normal_distribution<double> nd(tmpD.mean, tmpD.sem);
            double tmpDBL = nd(dice);
            tmpScore[m] = tmpDBL;
            if (tmpDBL > tmpBest) tmpBest = tmpDBL;
        }
        for (int m = 0; m < candidates; m++) {
            d[m].reg += tmpScore[m] - tmpBest;
        }
    }
    for (int m = 0; m < candidates; m++) {
        if (d[m].reg > line) return true;
    }
    return false;
}

static void MonteCarloThread
(const int threadId, RootInfo *const proot,
 const Field *const pfield, SharedData *const pshared,
 ThreadTools *const ptools) {
    const int myPlayerNum = proot->myPlayerNum;
    auto& dice = ptools->dice;

    int numSimulations[256] = {0};
    int numSimulationsSum = 0;

    int numWorlds = 0; // 作成した世界の数
    std::array<ImaginaryWorld, 128> worlds;

    // 世界生成のためのクラスを初期化
    const auto& record = pshared->record.latestGame();
    RandomDealer<EngineGameRecord> estimator(record, *pfield, myPlayerNum);

    Field pf = *pfield;
    pf.myPlayerNum = -1; // 客観視点に変更
    pf.addAttractedPlayer(myPlayerNum);
    pf.setMoveBuffer(ptools->buf);
    if (proot->rivalPlayerNum >= 0) {
        pf.attractedPlayers.set(proot->rivalPlayerNum);
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
        } else if (numWorlds < (int)worlds.size()) {
            // 新しい世界を作成
            simuTime += clock.restart();
            estimator.create(&worlds[numWorlds], Settings::monteCarloDealType, *pshared, ptools);
            estTime += clock.restart();
            world = numWorlds++;
        } else {
            // ランダム選択
            world = dice() % numWorlds;
        }

        numSimulations[action]++;
        numSimulationsSum++;

        // シミュレーション実行
        Field f;
        copyField(pf, &f);
        setWorld(worlds[world], &f);
        if (proot->isChange) {
            startChangeSimulation(f, myPlayerNum, proot->child[action].changeCards, pshared, ptools);
        } else {
            startPlaySimulation(f, proot->child[action].move, pshared, ptools);
        }
        
        proot->feedSimulationResult(action, f, pshared); // 結果をセット(排他制御は関数内で)
        if (proot->exitFlag) return;
        
        simuTime += clock.restart();
        
#ifndef FIXED_N_PLAYOUTS
        // 終了判定
        if (threadId == 0
            && numSimulationsSum % max(4, 32 / N_THREADS) == 0
            && proot->allSimulations > proot->candidates * 4) {
            if (finishCheck(*proot, double(simuTime) / pow(10, 6), dice)) {
                proot->exitFlag = 1;
                return;
            }
        }
#endif // FIXED_N_PLAYOUTS
    }
}