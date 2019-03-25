#pragma once

#include "engineSettings.h"
#include "estimation.hpp"
#include "simulation.hpp"

// マルチスレッディングのときはスレッド、
// シングルの時は関数として呼ぶ

namespace UECda {
    namespace Fuji {
        
        template <class root_t, class field_t, class sharedData_t, class threadTools_t>
        void MonteCarloThread
        (const int threadId, root_t *const proot,
         const field_t *const pfield,
         sharedData_t *const pshared,
         threadTools_t *const ptools) {
            
            constexpr uint32_t MINNEC_N_TRIALS = 4; // 全体での最小限のトライ数。UCB-Rootにしたので実質不要になった
            
            // プレー用
            using galaxy_t = typename threadTools_t::galaxy_t;
            using world_t = typename galaxy_t::world_t;
            
            auto& dice = ptools->dice;
            auto& gal = ptools->gal;
            
            Clock clock;
            
            const int myPlayerNum = proot->myPlayerNum;
            const int candidates = proot->candidates; // 候補数
            auto& child = proot->child;
            
            int threadNTrials[256]; // 当スレッドでのトライ数(候補ごと)
            int threadNTrialsSum = 0; // 当スレッドでのトライ数(合計)
            
            for (int c = 0; c < candidates; ++c)threadNTrials[c] = 0;
            
            int threadMaxNTrials = 0; // 当スレッドで現時点で最大のトライ数
            
            int threadNWorlds = 0; // 当スレッドが作成し使用している世界の数
            const int threadMaxNWorlds = gal.size();//( gal->size() / N_THREADS ); // 当スレッドに与えられている世界作成スペースの数
            
            assert(threadMaxNWorlds > 0);
            
            // 世界創世者
            // 連続作成のためここに置いておく
            RandomDealer<N_PLAYERS> estimator;
            estimator.set(*pfield, myPlayerNum);

            Field pf = *pfield;
            pf.attractedPlayers.set(myPlayerNum);
            pf.dice = &ptools->dice;
            pf.mv = ptools->buf;
            
            if (proot->rivalPlayerNum >= 0) {
                pf.attractedPlayers.set(proot->rivalPlayerNum);
            }
            
            uint64_t simuTime = 0ULL; // プレイアウトと雑多な処理にかかった時間
            uint64_t estTime = 0ULL; // 局面推定にかかった時間
            
            // 諸々の準備が終わったので時間計測開始
            clock.start();
            
            while(!proot->exitFlag) { // 最大で最高回数までプレイアウトを繰り返す
                
                world_t *pWorld = nullptr;
                
                //サンプル着手決定
                int tryingIndex = -1;
                if (candidates == 2) {
                    // 2つの時は同数(分布サイズ単位)に割り振る
                    if (child[0].size() == child[1].size())
                        tryingIndex = dice.rand() % 2;
                    else
                        tryingIndex = child[0].size() < child[1].size()
                        ? 0 : 1;
                } else {
                    // UCB-root アルゴリズムに変更
                    double bestScore = -DBL_MAX;
                    const double allSize = proot->monteCarloAllScore.size();
                    for (int c = 0; c < candidates; ++c) {
                        double tmpScore;
                        double size = child[c].size();
                        if (child[c].simulations < MINNEC_N_TRIALS) {
                            // 最低プレイアウト数をこなしていないものは、大きな値にする
                            // ただし最低回数のもののうちどれかがランダムに選ばれるようにする
                            tmpScore = (double)((1U << 16) - (child[c].simulations << 8) + (dice.rand() % (1U << 6)));
                        } else {
                            ASSERT(size, cerr << child[c].toString() << endl;);
                            tmpScore = child[c].mean() + 0.7 * sqrt(sqrt(allSize) / size); // ucbr値
                        }
                        if (tmpScore > bestScore) {
                            bestScore = tmpScore;
                            tryingIndex = c;
                        }
                    }
                }
                ASSERT(0 <= tryingIndex && tryingIndex < candidates,
                       cerr << tryingIndex << " in " << candidates << endl;);
                
                //DERR << "SAMPLE MOVE..." << ps->getMoveById(tryingIndex) << endl;
                
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
                        //pWorld = gal->access(threadMaxNWorlds * threadId + pastNTrials);
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
                            estimator.create(pWorld, Settings::monteCarloDealType,
                                             *pfield, *pshared, ptools);
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
                        //pWorld=gal->pickRand( threadMaxNWorlds*threadId, threadNWorlds, &dice );
                        pWorld = gal.pickRand(0, threadNWorlds, &dice);
                        if (pWorld == nullptr) goto THREAD_EXIT; // どうしようもないのでスレッド強制終了
                    } else goto THREAD_EXIT; // どうしようもないのでスレッド強制終了
                }
                
                // 世界確定
                DOUT << "SAMPLE WORLD..." << (pWorld - gal.world) << endl;
                
                //PlayoutScore score;
                
                // ここでプレイアウト実行
                // alphaカットはしない
                Field f;
                if (proot->isChange) {
                    copyField(pf, &f);
                    setWorld(pf, *pWorld, &f);
                    startChangeSimulation(&f, myPlayerNum, child[tryingIndex].changeCards, pshared, ptools);
                } else {
                    copyField(pf, &f);
                    setWorld(pf, *pWorld, &f);
                    startRootSimulation(&f, child[tryingIndex].move, pshared, ptools);
                }
                //CERR << "TRIAL : " << i << " " << moves.getMoveById(tryingIndex) << " : " << r << endl;
                
                proot->feedSimulationResult(tryingIndex, f, pshared); // 結果をセット(排他制御は関数内で)
                if (proot->exitFlag) goto THREAD_EXIT;
                
                simuTime += clock.restart();
                
#ifndef FIXED_N_PLAYOUTS
                // 終了判定
                if (threadId == 0
                    && threadNTrialsSum % max(4, 32 / N_THREADS) == 0
                    && proot->allSimulations > candidates * MINNEC_N_TRIALS) {
                    
                    // Regretによる打ち切り判定
                    struct Dist { double mean,sem,reg; };
                    double tmpClock = simuTime;
                    double line = -1600.0 * double(2 * tmpClock * VALUE_PER_CLOCK) / proot->rewardGap;
                    
                    // regret check
                    Dist d[N_MAX_MOVES + 64];
                    for (int m = 0; m < candidates; m++) {
                        d[m].reg = 0.0;
                        d[m].mean = child[m].mean();
                        
                        ASSERT(child[m].size(), cerr << child[m].toString() << endl;);
                        
                        d[m].sem = sqrt(child[m].mean_var()); // 推定平均値の分散
                        //cerr << d[m].sem << endl;
                    }
                    for (int t = 0; t < 1600; t++) {
                        double tmpBest = -1.0;
                        double tmpScore[256];
                        for (int m = 0; m < candidates; m++) {
                            const Dist& tmpD = d[m];
                            NormalDistribution<double> norm(tmpD.mean, tmpD.sem);
                            double tmpDBL = norm.rand(&dice);
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
                            goto THREAD_EXIT;
                        }
                    }
                }
#endif // FIXED_N_PLAYOUTS
            }
        THREAD_EXIT:;//終了
        }
    }
}