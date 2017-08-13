/*
 monteCarloPlayer.hpp
 Katsuki Ohto
 */

#ifndef UECDA_MONTECARLOPLAYER_HPP_
#define UECDA_MONTECARLOPLAYER_HPP_

#include "monteCarloThread.hpp"

// モンテカルロ法
namespace UECda{
    namespace Fuji{
        class MonteCarloPlayer{
            // モンテカルロ法による着手 & 交換決定機関
        private:
            int mode;
            static AtomicAnalyzer<2, 1, 0> ana;
            
        public:
            
            MonteCarloPlayer(){}
            
            template<class playSpace_t, class field_t, class sharedData_t, class threadTools_t>
            Move playSub(playSpace_t& moves, const int NCands, field_t& field,
                         const FieldAddInfo& fieldInfo,
                         sharedData_t *const pshared, threadTools_t threadTools[]);
            
            template<class field_t, class sharedData_t, class threadTools_t>
            int changeSub(const Cards *const, const int NCands, field_t& field,
                          sharedData_t *const pshared, threadTools_t threadTools[]);
            
            template<class playSpace_t, class field_t, class sharedData_t, class threadTools_t>
            Move play(playSpace_t& moves, const int NCands, field_t& field,
                      const FieldAddInfo& fieldInfo,
                      sharedData_t *const pshared, threadTools_t threadTools[]){
                // 着手を返す
                Move ret;
                mode = 1;
                ana.start();
                ret = playSub(moves, NCands, field, fieldInfo, pshared, threadTools);
                ana.end(mode);
                return ret;
            }
            
            template<class field_t, class sharedData_t, class threadTools_t>
            int change(const Cards *const cand, const int NCands, field_t& field,
                       sharedData_t *const pshared, threadTools_t threadTools[]){
                // インデックスを返す
                int ret;
                mode = 0;
                ana.start();
                ret = changeSub(cand, NCands, field, pshared, threadTools);
                ana.end(mode);
                return ret;
            }
        };
        
        AtomicAnalyzer<2, 1, 0> MonteCarloPlayer::ana("MonteCarloPlayer");
        
        template<class field_t, class sharedData_t, class threadTools_t>
        int MonteCarloPlayer::changeSub(const Cards *const cand, const int NCands, field_t& field,
                                        sharedData_t *const pshared, threadTools_t threadTools[]){
            
            MonteCarloRootNode root;
            
            int NActiveCands = NCands;
            
            // プレイアウト最高回数
            unsigned int PLAYOUT_MAX = std::min(10000, (int)(pow((double)NActiveCands, 0.8) * 700));
            
            //ルートノード設定
            root.setChange(cand, NActiveCands, field, PLAYOUT_MAX, *pshared);
            
            pshared->exp_wr = 0;
            
#ifdef MULTI_THREADING
            // detach threads
            std::vector<std::thread> thr;
            for(int ith = 0; ith < Settings::NChangeThreads; ++ith){
                thr.emplace_back(std::thread(&MonteCarloThread<field_t, MonteCarloRootNode, sharedData_t, threadTools_t>, ith, &root,
                                             &field, pshared, &threadTools[ith]));
            }
            for(auto& th : thr){
                th.join();
            }
#else
            // call function
            MonteCarloThread<field_t, MonteCarloRootNode, sharedData_t, threadTools_t>(0, &root, &field,
                                                                        pshared, &threadTools[0]);
            
#endif // MULTI_THREADING
            
            //全スレッドが終了
            
#ifdef MONITOR
            root.printResult(256);
#endif
            int best = root.searchBestIndex();
            
#ifdef LOGGING
#endif
            field.last_exp_reward = root.getExpReward(best);
            
            return best;
        }
        
        
        
        template<class playSpace_t, class field_t, class sharedData_t, class threadTools_t>
        Move MonteCarloPlayer::playSub(playSpace_t& ps, const int NCands, field_t& field,
                                       const FieldAddInfo& fieldInfo,
                                       sharedData_t *const pshared, threadTools_t threadTools[]){
            
            int NActiveCands = NCands;
            
#ifdef PRUNE_ROOT_PLAY_BY_HEURISTICS
            // まずはヒューリスティック機関に枝刈りを行ってもらう
            NActiveCands = Heuristics::pruneRoot(&ps, NCands, field);
            if(NActiveCands == 1){
                // 候補が1つになった
                return ps.mv[0].mv();
            }
#endif
            MonteCarloRootNode root;
            
            // プレイアウト最高回数
            unsigned int PLAYOUT_MAX = std::min(5000, (int)(pow((double)NActiveCands, 0.8) * 700));
            
            // ルートノード設定
            root.setPlay(ps, NActiveCands, field, fieldInfo, PLAYOUT_MAX, *pshared);
            
            pshared->exp_wr = 0; // 最高報酬0
            
#ifdef MULTI_THREADING
            //detach threads
            std::vector<std::thread> thr;
            for(int ith = 0; ith < Settings::NPlayThreads; ++ith){
                thr.emplace_back(std::thread(&MonteCarloThread<field_t, MonteCarloRootNode, sharedData_t, threadTools_t>, ith, &root,
                                             &field, pshared, &threadTools[ith]));
            }
            for(auto& th : thr){
                th.join();
            }
#else
            // call function
            MonteCarloThread<field_t, MonteCarloRootNode, sharedData_t, threadTools_t>(0, &root, &field,
                                                                        pshared, &threadTools[0]);
            
#endif // MULTI_THREADING
            
            // 全スレッドが終了
            
#ifdef MONITOR
            root.printResult(256);
#endif
            
            int bestId = root.searchBestIndex();
            
            assert(bestId != -1);
            
            field.last_exp_reward = root.getExpReward(bestId);
 
            return root.child[bestId].mi.mv();
        }
    }
}

#endif // UECDA_MONTECARLOPLAYER_HPP_

