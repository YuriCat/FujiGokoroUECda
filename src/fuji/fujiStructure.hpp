/*
 fujiStructure.hpp
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_FUJISTRUCTURE_HPP_
#define UECDA_FUJI_FUJISTRUCTURE_HPP_

// 思考用の構造体
#include "montecarlo/playout.h"

#include "../structure/field/clientField.hpp"
#include "estimation/galaxy.hpp"
#include "model/playerModel.hpp"

#include "policy/changePolicy.hpp"
#include "policy/playPolicy.hpp"

namespace UECda{
    namespace Fuji{

        // Field以外のデータ構造
        // Fieldは基本盤面情報+盤面を進めたり戻したりするときに値が変化するもの
        // それ以外の重目のデータ構造は SharedData
        // スレッドごとのデータは ThreadTools
        
        struct ThreadTools{
            // 各スレッドの持ち物
            using dice64_t = XorShift64;
            using move_t = MoveInfo;
            
#ifndef POLICY_ONLY
            // MCしないなら世界生成なし
            using galaxy_t = Galaxy<ImaginaryWorld>;
            
            // 世界生成プール
            galaxy_t gal;
#endif
            // サイコロ
            dice64_t dice;
            
            // 着手生成バッファ
            static constexpr int BUFFER_LENGTH = 8192;
            
            // スレッド番号
            int threadIndex;
            
            move_t buf[BUFFER_LENGTH];
            
            void init(int index){
                memset(buf, 0, sizeof(buf));
                threadIndex = index;
#ifndef POLICY_ONLY
                gal.clear();
#endif
            }
            void close(){}
        };
        
        struct SharedData{
            // 全体で共通のデータ
            
            // 計算量解析
            int modeling_time;
            int estimating_by_time;
            
            volatile double exp_wr; // 今ターンの期待報酬(0~1)
            
            uint32_t game_reward[N_PLAYERS];
            
            // 基本方策
            ChangePolicy<policy_value_t> baseChangePolicy;
            PlayPolicy<policy_value_t> basePlayPolicy;
            
            MinMatchLog<MinClientGameLog<MinClientPlayLog<N_PLAYERS>>> matchLog;
            MinClientGameLog<MinClientPlayLog<N_PLAYERS>> gameLog;
            
#if defined(RL_POLICY)
            // 方策学習
            ChangePolicyLearner<policy_value_t> changeLearner;
            PlayPolicyLearner<policy_value_t> playLearner;
#endif
            
#ifndef POLICY_ONLY
            using galaxy_t = ThreadTools::galaxy_t;
            GalaxyAnalyzer<galaxy_t, N_THREADS> ga;
            MyTimeAnalyzer timeAnalyzer;
            
            // 推定用方策
            ChangePolicy<policy_value_t> estimationChangePolicy;
            PlayPolicy<policy_value_t> estimationPlayPolicy;
            
            // 相手方策モデリング
            PlayerModelSpace playerModelSpace;
#endif
            void initMatch(){
                
                // 計算量解析初期化
                modeling_time = 0;
                estimating_by_time = 0;
                
#if defined(RL_POLICY)
                changeLearner.setClassifier(&baseChangePolicy);
                playLearner.setClassifier(&basePlayPolicy);
#endif
            }
            void setMyPlayerNum(int myPlayerNum){
#ifndef POLICY_ONLY
                playerModelSpace.init(myPlayerNum);
#endif
            }
            void initGame(){
                
            }
            template<class field_t>
            void closeGame(const field_t& field){
#if defined(POLICY_ONLY) && defined(RL_POLICY)
                // reinforcement learning
                playLearner.feedReward(((N_PLAYERS - 1) / 2.0 - field.getMyNewClass()) / (N_PLAYERS - 1));
                playLearner.updateParams();
#endif
            }
            void closeMatch(){
#ifndef POLICY_ONLY
                playerModelSpace.closeMatch();
#endif
            }
        };
        
        // 多人数不完全情報ゲームのため色々なデータ構造が必要になるのをここでまとめる
        // POLICY_ONLY フラグにてデータが減るので注意
        
        struct FujiField : public ClientField{
            
            //MonteCarloLog MCLog;
            
            uint32_t last_exp_reward; // 前ターンの期待報酬
            
            std::bitset<16> rare_play_flag; // 序盤で珍しいプレーを行ってバグチェック&見せプレー
            
            double changeEntropySum, playEntropySum;
            uint64_t fuzzyChangeTimes, fuzzyPlayTimes;
            
            // 手札配置パターン数(他人の手札は全くわからないと仮定)
            double worldPatterns;
            double lastWorldPatterns;
            
            double calcWorldPatterns()const{
                int n[N_PLAYERS - 1];
                int cnt = 0;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(p != (int)getMyPlayerNum()){
                        n[cnt] = getNCards(p);
                        ++cnt;
                    }
                }
                
                assert(cnt == N_PLAYERS - 1);
                return dCombination(n, N_PLAYERS - 1);
            }
            
            void initWorldPatterns(){
                worldPatterns = calcWorldPatterns();
                lastWorldPatterns = worldPatterns;
            }
            
            void procWorldPatterns(int qty){
                lastWorldPatterns = worldPatterns;
                if(qty > 0){
                    worldPatterns = calcWorldPatterns();
                    CERR << worldPatterns << endl;
                }
            }
            
            double getWPCmp()const{
                assert(worldPatterns > 0.0);
                return lastWorldPatterns / worldPatterns;
            }
            
            void initMatch(){
                ClientField::initMatch();
                
                rare_play_flag.reset();
                
                changeEntropySum = playEntropySum = 0;
                fuzzyChangeTimes = fuzzyPlayTimes = 0;
            }

            void initGame(){
                ClientField::initGame();
            }
            
            void closeGame(){
                ClientField::closeGame();
            }
            
            void closeMatch(){
                ClientField::closeMatch();
#ifdef MONITOR
                cerr << "mean entropy of change = " << (changeEntropySum / fuzzyChangeTimes) << endl;
                cerr << "mean entropy of play   = " << (playEntropySum / fuzzyPlayTimes) << endl;
#endif
            }
            
            FujiField(){}
            ~FujiField(){}
        };
    }
}

#endif // UECDA_FUJI_FUJISTRUCTURE_HPP_
