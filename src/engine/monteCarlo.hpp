#pragma once

#include "engineSettings.h"
#include "data.hpp"
#include "estimation.hpp"
#include "simulation.hpp"

// マルチスレッディングのときはスレッド、
// シングルの時は関数として呼ぶ

template <class dice_t>
static int selectBandit(const RootInfo& root, dice_t& dice) {
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

static void MonteCarloThread
(const int threadId, RootInfo *const proot,
 const Field *const pfield, SharedData *const pshared,
 ThreadTools *const ptools) {
    
    auto& dice = ptools->dice;
    auto& gal = ptools->gal;
    
    const int myPlayerNum = proot->myPlayerNum;
    const int candidates = proot->candidates; // 候補数
    auto& child = proot->child;
    
    int threadNTrials[256] = {0};
    int threadNTrialsSum = 0; // 当スレッドでのトライ数(合計)
    
    int threadMaxNTrials = 0; // 当スレッドで現時点で最大のトライ数
    
    int threadNWorlds = 0; // 当スレッドが作成し使用している世界の数
    const int threadMaxNWorlds = gal.size(); // 当スレッドに与えられている世界作成スペースの数

    // 世界生成のためのクラスを初期化
    RandomDealer<EngineGameRecord> estimator(pshared->record.latestGame(), *pfield, myPlayerNum);

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
        
        ImaginaryWorld *pWorld = nullptr;
        int tryingIndex = selectBandit(*proot, dice);          
        
        const int pastNTrials = threadNTrials[tryingIndex]++; // 選ばれたもののこれまでのトライアル数
        threadNTrialsSum++;
        
        if (threadNWorlds >= threadMaxNWorlds) {
            // このとき、世界作成は既に終わっている
            if (pastNTrials < threadMaxNWorlds) {
                // まだ全ての世界でこの着手を検討していないので順番通りの世界で検討
                pWorld = gal.access(pastNTrials);
                if (!pWorld->isActive()) {
                    // 何らかの事情で世界作成スケジュールがずれている
                    // 仕方が無いので既にある世界からランダムに選ぶ
                    pWorld = nullptr;
                }
            } else {
                // 全ての世界からランダムに選ぶ
            }
        } else {
            // 世界作成が終わっていない
            if (pastNTrials < threadNWorlds) {
                // まだ全ての世界でこの着手を検討していないので順番通りの世界で検討
                pWorld = gal.access(pastNTrials);
                
                if (!pWorld -> isActive()) {
                    // 何らかの事情で世界作成スケジュールがずれている
                    // 仕方が無いので既にある世界からランダムに選ぶ
                    pWorld = nullptr;
                }
            } else {
                // 新しい世界を作成し、そこにプレイアウトを割り振る
                pWorld = gal.searchSpace(0 , threadMaxNWorlds);
                if (pWorld != nullptr) {
                    // 世界作成スペースが見つかった
                    simuTime += clock.restart();
                    // 世界作成
                    estimator.create(pWorld, Settings::monteCarloDealType, *pshared, ptools);
                    estTime += clock.restart();
                    
                    if (gal.regist(pWorld) != 0) { // 登録失敗
                        // 仕方が無いので既にある世界からランダムに選ぶ
                        pWorld = nullptr;
                    } else threadNWorlds++; // 当スレッドの作成世界数up
                }
            }
        }
        
        if (threadNTrials[tryingIndex] > threadMaxNTrials) {
            threadMaxNTrials = threadNTrials[tryingIndex];
        }
        
        // この時点で世界が決まっていない場合はランダムに選ぶ
        if (pWorld == nullptr) {
            if (threadNWorlds > 0) {
                pWorld = gal.pickRand(0, threadNWorlds, dice);
                if (pWorld == nullptr) return; // どうしようもないのでスレッド強制終了
            } else return; // どうしようもないのでスレッド強制終了
        }

        // シミュレーション実行
        Field f;
        copyField(pf, &f);
        setWorld(*pWorld, &f);
        if (proot->isChange) {
            startChangeSimulation(f, myPlayerNum, child[tryingIndex].changeCards, pshared, ptools);
        } else {
            startPlaySimulation(f, child[tryingIndex].move, pshared, ptools);
        }
        
        proot->feedSimulationResult(tryingIndex, f, pshared); // 結果をセット(排他制御は関数内で)
        if (proot->exitFlag) return;
        
        simuTime += clock.restart();
        
#ifndef FIXED_N_PLAYOUTS
        // 終了判定
        if (threadId == 0
            && threadNTrialsSum % max(4, 32 / N_THREADS) == 0
            && proot->allSimulations > candidates * 4) {
            
            // Regretによる打ち切り判定
            struct Dist { double mean, sem, reg; };
            double tmpClock = simuTime / pow(10, 6);
            double line = -1600.0 * double(2 * tmpClock * VALUE_PER_SEC) / proot->rewardGap;
            
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
                if (d[m].reg > line) {
                    proot->exitFlag = 1;
                    return;
                }
            }
        }
#endif // FIXED_N_PLAYOUTS
    }
}