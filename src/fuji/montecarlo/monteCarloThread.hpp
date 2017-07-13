/*
 monteCarloThread.hpp
 Katsuki Ohto
 */

#ifndef UECDA_MONTECARLO_THREAD_HPP_
#define UECDA_MONTECARLO_THREAD_HPP_

#include "../../settings.h"

#include "monteCarlo.h"
#include "../estimation/worldCreator.hpp"

#include "playouter.hpp"

// マルチスレッディングのときはスレッド、
// シングルの時は関数として呼ぶ

namespace UECda{
    namespace Fuji{
        
        template<class field_t, class root_t, class sharedData_t, class threadTools_t>
        void MonteCarloThread
        (const int threadId, root_t *const proot,
         const field_t *const pfield,
         sharedData_t *const pshared,
         threadTools_t *const ptools){
            
            // プレー用
            using dice64_t = ThreadTools::dice64_t;
            using galaxy_t = ThreadTools::galaxy_t;
            using world_t = galaxy_t::world_t;
            
            auto& dice = ptools->dice;
            auto& gal = ptools->gal;
            
            Clock clock;
            
            const int myPlayerNum = pfield->getMyPlayerNum();
            
            const int NPlayers = pfield->getNAlivePlayers();
            
            const int bestClass = N_PLAYERS - NPlayers;
            constexpr int worstClass = N_PLAYERS - 1;
            
            int bestReward;
            int worstReward;
            int rewardGap;
            
            const int NChilds = proot->NChilds; // 候補数
            
            int threadNTrials[256]; // 当スレッドでのトライ数(候補ごと)
            int threadNTrialsSum = 0; // 当スレッドでのトライ数(合計)
            
            for(int c = 0; c < NChilds; ++c){
                threadNTrials[c] = 0;
            }
            
            int threadMaxNTrials = 0; // 当スレッドで現時点で最大のトライ数
            
            int threadNWorlds = 0; // 当スレッドが作成し使用している世界の数
            const int threadMaxNWorlds = gal.size();//( gal->size() / N_THREADS ); // 当スレッドに与えられている世界作成スペースの数
            
            assert(threadMaxNWorlds > 0);
            
            //const int threadMinNecNTrials=min( 1, MonteCarlo::MINNEC_N_TRIALS / N_THREADS ); // 当スレッドで1つの候補に対して最低割り振るべきトライ数
            
            // 世界創世者
            // 連続作成のためここに置いておく
            WorldCreator<dice64_t> wc;
            
            wc.prepare(*pfield);
            
            Playouter po; // プレイアウタ
            
            PlayouterField pf;
            setSubjectiveField(*pfield, &pf);
            pf.attractedPlayers.set(myPlayerNum);
            pf.dice = &ptools->dice;
            pf.mv = ptools->buf;
            
            //CERR << pfield->phase << endl;
            //CERR << pf.phase << endl;
            
            if(proot->rivalPlayerNum >= 0){
                pf.attractedPlayers.set(proot->rivalPlayerNum);
            }
            
            uint64_t poTime = 0ULL; // プレイアウトと雑多な処理にかかった時間
            uint64_t estTime = 0ULL; // 局面推定にかかった時間
            
            // 報酬設定
            {
                bestReward = pshared->game_reward[bestClass];
                worstReward = pshared->game_reward[worstClass];
                rewardGap = bestReward - worstReward;
                
                DERR << "bestClass : " << bestClass << " worstClass : " << worstClass << endl;
                DERR << "bestRew : " << bestReward << " worstRew : " << worstReward << " gap : " << rewardGap << endl;
            }
            
            // 諸々の準備が終わったので時間計測開始
            clock.start();
            
            while(!proot->ex){ // 最大で最高回数までプレイアウトを繰り返す
                
                world_t *pWorld = nullptr;
                
                //サンプル着手決定
                int tryId = proot->chooseBestUCB(&dice);
                ASSERT(0 <= tryId && tryId < proot->NChilds, cerr << tryId << " in " << proot->NChilds << endl;);
                
                //DERR << "SAMPLE MOVE..." << ps->getMoveById(tryId) << endl;
                
                const int pastNTrials = threadNTrials[tryId]++; // 選ばれたもののこれまでのトライアル数
                threadNTrialsSum++;
                
                //cerr<<threadNTrialsSum<<",";
                
                if(threadNWorlds >= threadMaxNWorlds){
                    // このとき、世界作成は既に終わっている
                    if(pastNTrials < threadMaxNWorlds){
                        // まだ全ての世界でこの着手を検討していないので順番通りの世界で検討
                        pWorld = gal.access(pastNTrials);
                        if(!pWorld->isActive()){
                            // 何らかの事情で世界作成スケジュールがずれている
                            // 仕方が無いので既にある世界からランダムに選ぶ
                            pWorld = nullptr;
                        }
                    }else{
                        // 全ての世界からランダムに選ぶ
                    }
                }else{
                    // 世界作成が終わっていない
                    if(pastNTrials < threadNWorlds){
                        // まだ全ての世界でこの着手を検討していないので順番通りの世界で検討
                        //pWorld = gal->access(threadMaxNWorlds * threadId + pastNTrials);
                        pWorld = gal.access(pastNTrials);
                        
                        if(!pWorld -> isActive()){
                            // 何らかの事情で世界作成スケジュールがずれている
                            // 仕方が無いので既にある世界からランダムに選ぶ
                            pWorld = nullptr;
                        }
                    }else{
                        // 新しい世界を作成し、そこにプレイアウトを割り振る
                        //pWorld = gal->searchSpace(threadMaxNWorlds * threadId, threadMaxNWorlds);
                        
                        pWorld = gal.searchSpace(0 , threadMaxNWorlds);
                        
                        if(pWorld != nullptr){
                            // 世界作成スペースが見つかった
                            
                            poTime += clock.restart();
                            
                            // 世界作成
                            if(Settings::monteCarloDealType == DealType::REJECTION){
                                wc.create<DealType::REJECTION>(pWorld, *pfield, *pshared, ptools);
                            }else if(Settings::monteCarloDealType == DealType::BIAS){
                                wc.create<DealType::BIAS>(pWorld, *pfield, *pshared, ptools);
                            }else if(Settings::monteCarloDealType == DealType::SBJINFO){
                                wc.create<DealType::SBJINFO>(pWorld, *pfield, *pshared, ptools);
                            }else{
                                wc.create<DealType::RANDOM>(pWorld, *pfield, *pshared, ptools);
                            }
                            
                            
                            estTime += clock.restart();
                            
                            if(gal.regist(pWorld) != 0){ // 登録失敗
                                // 仕方が無いので既にある世界からランダムに選ぶ
                                pWorld = nullptr;
                            }else{
                                ++threadNWorlds; // 当スレッドの作成世界数up
                            }
                        }
                    }
                }
                
                if(threadNTrials[tryId] > threadMaxNTrials){
                    threadMaxNTrials = threadNTrials[tryId];
                }
                
                // この時点で世界が決まっていない場合はランダムに選ぶ
                if(pWorld == nullptr){
                    if(threadNWorlds > 0){
                        //pWorld=gal->pickRand( threadMaxNWorlds*threadId, threadNWorlds, &dice );
                        pWorld = gal.pickRand(0, threadNWorlds, &dice);
                        if(pWorld == nullptr){
                            goto THREAD_EXIT; // どうしようもないのでスレッド強制終了
                        }
                    }else{
                        goto THREAD_EXIT; // どうしようもないのでスレッド強制終了
                    }
                }
                
                // 世界確定
                DOUT << "SAMPLE WORLD..." << (pWorld - gal.world) << endl;
                
                //PlayoutScore score;
                
                // ここでプレイアウト実行
                // alphaカットはしない
                PlayouterField f;
                if(proot->isChange){
                    copyField(pf, &f);
                    setWorld(pf, *pWorld, &f);
                    ASSERT(examCards(proot->child[tryId].c), cerr << OutCards(proot->child[tryId].c) << endl;);
                    po.startChange(&f, myPlayerNum, proot->child[tryId].c, pshared, ptools);
                }else{
                    //po.startRoot(&score,root->child[tryId],*pWorld,*field);
                    copyField(pf, &f);
                    //CERR << f.phase << endl;
                    setWorld(pf, *pWorld, &f);
                    //CERR << f.phase << endl;
                    po.startRoot(&f, proot->child[tryId].mi, pshared, ptools);
                }
                //int r = std::rand() % 5;
                
                //CERR << "TRIAL : " << i << " " << moves.getMoveById(tryId) << " : " << r << endl;
                
                proot->feedResult(tryId, f, pshared); // 結果をセット(排他制御は関数内で)
                if(proot->ex){
                    goto THREAD_EXIT;
                }
                
                poTime += clock.restart();
  
#ifndef FIXED_N_PLAYOUTS
                if(threadId == 0
                   && threadNTrialsSum % max(4, 32 / N_THREADS) == 0
                   //root->trials % 32 == 0
                   && proot->trials > NChilds * MonteCarlo::MINNEC_N_TRIALS
                   ){
                    
                    //cerr<<"cut ";
                    
                    // Regretによる打ち切り判定
                    struct Dist{ double mean,sem,reg; };
                    // time
                    const double tmpClock = (double)poTime;
                    
                    const double line = -1600.0 * ((double)(2 * tmpClock * VALUE_PER_CLOCK)) / (double)rewardGap;
                    
                    // regret check
                    Dist d[256];
                    for(int m = 0; m < NChilds; ++m){
                        d[m].reg = 0.0;
                        d[m].mean = proot->child[m].mean();
                        
                        ASSERT(proot->child[m].size(), cerr << proot->child[m].toString() << endl;);
                        
                        d[m].sem = sqrt(proot->child[m].mean_var()); // 推定平均値の分散
                        //cerr << d[m].sem << endl;
                    }
                    for(int t = 0; t < 1600; ++t){
                        double tmpBest = -1.0;
                        double tmpScore[256];
                        for(int m = 0; m < NChilds; ++m){
                            const Dist& tmpD = d[m];
                            NormalDistribution<double> norm(tmpD.mean, tmpD.sem);
                            double tmpDBL = norm.rand(&dice);
                            tmpScore[m] = tmpDBL;
                            if(tmpDBL > tmpBest){
                                tmpBest = tmpDBL;
                            }
                        }
                        for(int m = 0; m < NChilds; ++m){
                            d[m].reg += (tmpScore[m] - tmpBest);
                        }
                    }
                    
                    for(int m = 0; m < NChilds; ++m){
                        if(d[m].reg > line){
                            /*cerr<<"trials = " << proot->trials << " childs = "<<NChilds<<" line = "<<line<<endl;
                             for(int mm=0;mm<NChilds;mm++){
                             cerr<<d[mm].reg<<endl;
                             }
                             getchar();*/
                            proot->ex = 1;
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
#endif // UECDA_MONTECARLO_THREAD_HPP_

